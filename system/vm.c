#include <xinu.h>


void FlushTlb(void *page){
    asm volatile(
        "invlpg (%0)\n\t"
        :
        :"r"(page)
        :"memory"
    );
}

void print_Dir(uint32 pgdir, uint32 pgi,uint32 index){
    AccessTemp(pgdir);
    uint32 pgt = GET_ENTRY(pgi);
    kprintf("DIR[%d]%x\n", pgi, pgt);
    AccessTemp(pgt);
    uint32 e = GET_ENTRY(index);
    kprintf("DIR[%d, %d]%x\n\n", pgi, index, e);
}


// 使用32MB的最后三个页用作临时映射
void AccessTemp(uint32 page_addr){
    intmask mask = disable();
    ((Page_t*)(KERNEL_END - 4 * PAGE_SIZE))->entries[1023] = page_addr | PG_P | PG_RW_W | PG_US_U;
    FlushTlb((Page_t*)TEMP_ADDR);
    restore(mask);
}
void AccessKernel(uint32 page_addr){
    intmask mask = disable();
    ((Page_t*)(KERNEL_END - 4 * PAGE_SIZE))->entries[1022] = page_addr | PG_P | PG_RW_W | PG_US_U;
    FlushTlb((Page_t*)KER_ADDR);
    restore(mask);
}
void AccessUser(uint32 page_addr){
    intmask mask = disable();
    ((Page_t*)(KERNEL_END - 4 * PAGE_SIZE))->entries[1021] = page_addr | PG_P | PG_RW_W | PG_US_U;
    FlushTlb((Page_t*)TEMP_ADDR);
    restore(mask);
}

// 分配栈空间
char *allocstk(uint32 nbytes, uint32 pgdir){
    intmask mask = disable();
    uint32 page_num = Byte2Page(nbytes);
    
    AccessTemp(pgdir);
    for(int i = 0; i < 8; i++){
        WRITE_ENTRY(i, (KERNEL_END - (11 - i) * PAGE_SIZE) | PG_P | PG_RW_W | PG_US_U);
    }

    uint32 stack_pt = (uint32)AllocPage();
    AccessTemp(pgdir);
    WRITE_ENTRY(1023, stack_pt | PG_P | PG_RW_W | PG_US_U);
    AccessTemp(stack_pt);
    memset((void*)TEMP_ADDR, 0, PAGE_SIZE);

    uint32 stack_page, stack_page_base;
    for(int i = 0; i < page_num; i++){
        stack_page = (uint32)AllocPage();
        if(i == 0) stack_page_base = stack_page;
        AccessTemp(stack_pt);
        WRITE_ENTRY(1023 - i, stack_page | PG_P | PG_RW_W | PG_US_U);
    }
    AccessTemp(stack_page_base);
    restore(mask);
    return (char *)0xfffffffc;
}

// 分配栈空间
char *alloc_kstk(uint32 nbytes, uint32 pgdir){
    intmask mask = disable();
    uint32 page_num = Byte2Page(nbytes);
    
    AccessTemp(pgdir);
    for(int i = 0; i < 8; i++){
        WRITE_ENTRY(i, (KERNEL_END - (11 - i) * PAGE_SIZE) | PG_P | PG_RW_W | PG_US_U);
    }
    uint32 stack_pt = (uint32)AllocPage();
    AccessTemp(pgdir);
    WRITE_ENTRY(1023, stack_pt | PG_P | PG_RW_W | PG_US_S);
    AccessTemp(stack_pt);
    memset((void*)TEMP_ADDR, 0, PAGE_SIZE);

    uint32 stack_page, stack_page_base;
    for(int i = 0; i < page_num; i++){
        stack_page = (uint32)AllocPage();
        if(i == 0) stack_page_base = stack_page;
        AccessTemp(stack_pt);
        // kprintf("[%d](WritePage) write page's address: 0x%x to 0x%x \n", i, (uint32)stack_page, (uint32)stack_pt);
        WRITE_ENTRY(1023 - i, stack_page | PG_P | PG_RW_W | PG_US_S);
    }
    AccessKernel(stack_page_base);

    restore(mask);
    return (char *)0xfffffffc; // 4GB - 4
}

char *alloc_ustk(uint32 nbytes, uint32 pgdir){
    intmask mask = disable();
    uint32 page_num = Byte2Page(nbytes);

    uint32 stack_pt = (uint32)AllocPage();
    AccessTemp(pgdir);
    WRITE_ENTRY(1022, stack_pt | PG_P | PG_RW_W | PG_US_U);
    AccessTemp(stack_pt);
    // memset((void*)TEMP_ADDR, 0, PAGE_SIZE);

    uint32 stack_page, stack_page_base;
    for(int i = 0; i < page_num; i++){
        stack_page = (uint32)AllocPage();
        if(i == 0) stack_page_base = stack_page;
        // kprintf("[I](WritePage) write page's address: 0x%x to 0x%x \n", (uint32)stack_page, (uint32)stack_pt);
        AccessTemp(stack_pt);
        WRITE_ENTRY(1023 - i, stack_page | PG_P | PG_RW_W | PG_US_U);
    }
    AccessUser(stack_page_base);
    restore(mask);
    return (char *)0xffbffffc; // 4GB - 4MB - 8
}


// 用于分配一页4kb的页表
Page_t* AllocPage(){
    intmask mask = disable();
    uint32 phy_addr;
    for(int i = total_page_num; i > 0; i--){
        phy_addr = *(Page_Dir - i);
        // 寻找可用页
        if((phy_addr & 1) == 0){
            *(Page_Dir - i) = phy_addr | 1;
            // kprintf("[I](GetOnePage) get page's physical address: 0x%x\n", (uint32)phy_addr);
            AccessTemp(phy_addr);
            // memset((void*)TEMP_ADDR, 0, PAGE_SIZE);
            restore(mask);
            return (void*)phy_addr;
        }
    }
    restore(mask);
    panic("No valid page\n");
    return NULL;
}

void FreePage(Page_t *addr){
    if((uint32)addr < KERNEL_END){
        kprintf("Fail to free kernel page %x\n", (uint32)addr);
        return;
    }
    if((uint32)addr & 0xfff){
        kprintf("Make sure that %x`s last 12bit is zeros\n", (uint32)addr);
        return;
    }
    uint32 index = (max_phy_addr - (uint32)addr) / PAGE_SIZE;
    uint32 *Pool = Page_Dir - total_page_num;
    Pool[index] &= ~1; //将末位置0
}

char* allocmem(uint32 nbytes){
    intmask mask = disable();
    if(proctab[currpid].free_list_ptr == NULL) InitializeFreeList();
    uint32 new_size = ROUND(nbytes + SIZE_T_SIZE);

    uint32 *current = ScanFreeBlock(new_size);
    if(current != 0){
        restore(mask);
        return (char*)current;
    }

    uint32 lastByte = proctab[currpid].max_heap - 4;
    uint32 *ptr     = (uint32*)lastByte;
    if(!(*ptr & 1) && ((uint32)ptr > KERNEL_END)){
        uint32 free_size    = *ptr & ~3;
        uint32 need_size    = new_size - free_size;
        uint32 pages_needed = Byte2Page(need_size);
        uint32 total_size   = pages_needed * PAGE_SIZE;
        uint32 extra_size   = total_size - need_size;
        current = ptr - free_size / 4 + 2;
        heapsbrk(total_size);
        if(extra_size < 16){
            DeleteFreeBlock(current);
            SetAllocTag(current, free_size + total_size);
        }
        else{
            AssignFreeBlock(current, (uint32*)(proctab[currpid].max_heap - extra_size + 4), extra_size);
            SetAllocTag(current, new_size);
        }
        restore(mask);
        return (char*)current;
    }

    uint32 pages_needed = Byte2Page(new_size);
    uint32 total_size   = pages_needed * PAGE_SIZE;
    uint32 extra_size   = total_size - new_size;
    // uint32 *ptr;

    ptr = (uint32 *)heapsbrk(total_size);
    if(extra_size < 16) SetAllocTag(ptr + 1, total_size);
    else{
        SetAllocTag(ptr + 1, new_size);
        SetFreeTag((uint32*)(proctab[currpid].max_heap - extra_size + 4), extra_size);
        InsertFreeBlock((uint32*)(proctab[currpid].max_heap - extra_size + 4));
    }
    if(ptr == (void*)(-1)){
        restore(mask);
        return NULL;
    }
    restore(mask);
    return (char*)(ptr + 1);
}

syscall deallocmem(char* addr, uint32 nbytes){
    intmask mask = disable();
    if((uint32)addr < KERNEL_END){
        kprintf("Fail to free kernel memory\n");
        return SYSERR;
    }

    uint32 *curr = (uint32*)addr;
    uint32  free_size = *(curr - 1) & ~3;
    bool8   flag;
    if(!(*(curr - 2) & 1) && (curr - 2) > (uint32*)KERNEL_END){
        uint32 prev_size = *(curr - 2) & ~3;
        curr -= prev_size / 4;
        free_size += prev_size;
        flag = TRUE;
    }
    if(!(*(curr + free_size / 4 - 1) & 1) && (curr + free_size / 4 - 1) <= (uint32*)((uint32)proctab[currpid].max_heap - 15)){
        uint32 next_size = *(curr + free_size / 4 - 1) & ~3;
        DeleteFreeBlock(curr + free_size / 4);
        free_size += next_size;
    }

    if(!flag) InsertFreeBlock(curr);

    uint32 index = (uint32)curr;
    if(index - 4 + free_size == proctab[currpid].max_heap){
        while(free_size >= PAGE_SIZE){
            FreeHeapPage(proctab[currpid].max_heap -= PAGE_SIZE);
            free_size -= PAGE_SIZE;
        }
    }
    if(free_size == 0) DeleteFreeBlock(curr);
    else SetFreeTag(curr, free_size);

    restore(mask);
    return 0;
}

void FreeHeapPage(uint32 addr){
    uint32 pgdir = proctab[currpid].phy_page_dir;
    uint32 dir_top = PDX(addr);
    uint32 tbl_top = PTX(addr);
    AccessTemp(pgdir);
    uint32 table = GET_ENTRY(dir_top) & ~0x3ff;
    AccessTemp(table);
    uint32 page = GET_ENTRY(tbl_top) & ~0x3ff;
    FreePage((Page_t*)page);
    if(tbl_top == 0) FreePage((Page_t*)table);
}

void InsertFreeBlock(uint32 *ptr){
    uint32 *first = (uint32*)proctab[currpid].free_list_ptr;
    if(first == NULL){
        *first = (uint32)ptr;
        proctab[currpid].free_list_ptr = (uint32)ptr;
        *ptr = (uint32)ptr;
        *(ptr + 1) = (uint32)ptr;
        return;
    }

    uint32 *next = (uint32*)*first;
    uint32 *prev_block = (uint32*)*(next + 1);
    *(next + 1) = (uint32)ptr;
    *prev_block = (uint32)ptr;
    *ptr = (uint32)next;
    *(ptr + 1) = (uint32)prev_block;
}

void InitializeFreeList(){
    uint32 *tmp_ptr = (uint32*)heapsbrk(PAGE_SIZE);
    tmp_ptr += 1;
    SetFreeTag(tmp_ptr, PAGE_SIZE);
    *tmp_ptr = (uint32)tmp_ptr;         // set next
    *(tmp_ptr + 1) = (uint32)tmp_ptr;   // set prev
    proctab[currpid].free_list_ptr = (uint32)tmp_ptr;
}

uint32 heapsbrk(uint32 nbytes){
    uint32 pgdir        = proctab[currpid].phy_page_dir;
    uint32 page_num     = Byte2Page(nbytes);
    uint32 maxheap      = proctab[currpid].max_heap;
    uint32 table_top    = PTX(maxheap);
    uint32 dir_top      = PDX(maxheap);
    uint32 new_maxheap  = maxheap + page_num + PAGE_SIZE;
    uint32 tmp_page, tmp_table;
    while(page_num > 0){
        tmp_page = (uint32)AllocPage();
        if(table_top == 0){
            tmp_table = (uint32)AllocPage();
            AccessTemp(pgdir);
            WRITE_ENTRY(dir_top, tmp_table | PG_P | PG_RW_W | PG_US_U);
            FlushTlb((void*)TEMP_ADDR);
        }
        else{
            AccessTemp(pgdir);
            tmp_table = GET_ENTRY(dir_top) & ~0xfff;
        }
        if(table_top == 1023) dir_top++;
        AccessTemp(tmp_table);
        WRITE_ENTRY(table_top++, tmp_page | PG_P | PG_RW_W | PG_US_U);
        FlushTlb((void*)TEMP_ADDR);
        table_top %= 1024;
        page_num--;
    }
    proctab[currpid].max_heap = new_maxheap;
    return maxheap;
}

void SetAllocTag(uint32 *ptr, uint32 size){
    *(ptr - 1) = size | 1;
    *(ptr + size / 4 - 2) = size | 1;
}

void SetFreeTag(uint32 *ptr, uint32 size){
    *(ptr - 1) = size;
    *(ptr + size / 4 - 2) = size;
}

uint32* ScanFreeBlock(uint32 newsize){
    uint32* first   = (uint32*)proctab[currpid].free_list_ptr;
    uint32* curr    = (uint32*)first;
    if(!curr) return 0;
    uint32* block = NULL;
    uint32  min_diff = ~0;
    bool8   flag = 0;
    while(TRUE){
        uint32 free_size = *(curr - 1);
        if((newsize <= free_size - 16) && (min_diff > free_size - newsize)){
            flag = 1;
            min_diff = free_size - newsize;
            block = curr;
        }
        else if(newsize <= free_size && newsize > free_size - 16){
            SetAllocTag(curr, free_size);
            DeleteFreeBlock(curr);
            return curr;
        }
        curr = (uint32*)*curr;
        if(*first == (uint32)curr) break;
    }
    if(!flag) return NULL;
    uint32 free_size = *(block - 1);
    uint32 left = free_size - newsize;
    SetAllocTag(block, newsize);
    AssignFreeBlock(block, block + newsize / 4, left);
    return block;

}

void DeleteFreeBlock(uint32 *ptr){
    uint32 *next    = (uint32*)*ptr;
    uint32 *first   = (uint32*)proctab[currpid].free_list_ptr;
    if(next == ptr){
        *first = 0;
        proctab[currpid].free_list_ptr = NULL;
        return;
    }
    uint32 *prev = (uint32*)*(ptr + 1);
    *(next + 1) = (uint32)prev;
    *prev = (uint32)next;
    if(*first == (uint32)ptr){
        *first = (uint32)next;
        proctab[currpid].free_list_ptr = *first;
    }
}

void AssignFreeBlock(uint32 *old, uint32 *new, uint32 newsize){
    uint32 *first = (uint32*)proctab[currpid].free_list_ptr;
    SetFreeTag(new, newsize);
    FlushTlb((void*)old);
    if(*old == (uint32)old){
        *first = (uint32)new;
        proctab[currpid].free_list_ptr = *first;
        *(new + 1) = (uint32)new;
        *new = (uint32)new;
        return;
    }
    uint32 *prev = (uint32 *)*(old + 1);
    uint32 *next = (uint32 *)*old;
    *prev = (uint32)new;
    *(next + 1) = (uint32)new;
    *new = (uint32)next;
    *(new + 1) = (uint32)prev;
}