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



    restore(mask);
}

void InitializeFreeList(){
    uint32 *tmp_ptr = (uint32*)heapsbrk(PAGE_SIZE);
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
}