


/*-------------------------
| 根据syscall进行特权级区分  |
|      0~3GB  user          |
|      3GB+ kernel          |
---------------------------*/


// size: 4 bytes
// typedef struct PageDirEntry
// {
//     unsigned present        : 1;
//     unsigned writable       : 1; 
//     unsigned user           : 1;
//     unsigned WT             : 1;
//     unsigned cacheDisabled  : 1;
//     unsigned accessed       : 1;
//     unsigned dirty          : 1;
//     unsigned reserved       : 2;
//     unsigned AVL            : 3;
//     unsigned PPN            : 20;
// }PageDirEntry_t;

typedef uint32 PageTableEntry_t;

// 页表
typedef struct Page{
    PageTableEntry_t entries[1024];
}Page_t;


/* va 32 bit */
// +--------10------+-------10-------+---------12----------+
// | Page Directory |   Page Table   | Offset within Page  |
// |      Index     |      Index     |                     |
// +----------------+----------------+---------------------+
//  \--- PDX(va) --/ \--- PTX(va) --/

//8MB = 2^23
#define KERNEL_END 0x2000000



#define PG_US_S (0 << 2)    // kernel
#define PG_US_U (1 << 2)    // User
#define PG_RW_R (0 << 1)    // 
#define PG_RW_W (1 << 1)    // writable
#define PG_P    1           // present


#define PAGE_SIZE 4096 // 4k
// 1024项，每项1024张表，共4M
#define PAGE_DIR_NUM 1024
// 取end后第二个4k作为PageDir
// 一张表1024项，每项管理4K空间，一张表管理4MB空间
#define PAGE_TABLE_NUM 1024

#define PDXSHIFT 22
#define PTXSHIFT 12
#define PAGE_INDEX_MASK 0x3ff
#define PAGE_OFFSET_MASK 0xfff

// 将byte转化为Page数，并且向上进位
#define Byte2Page(bytes)    (((bytes + PAGE_SIZE-1) & (~(PAGE_SIZE - 1))) / PAGE_SIZE)
#define ROUNDTO 8
#define ROUND(size)     (((size) + (ROUNDTO - 1)) & (~(ROUNDTO - 1)))
#define SIZE_T_SIZE     (ROUND(sizeof(uint32)))

#define WRITE_ENTRY(index, entry)   ((Page_t *)0x1fff000)->entries[index] = entry
#define GET_ENTRY(index)            ((Page_t *)0x1fff000)->entries[index]

#define TEMP_ADDR   0x1fff000
#define KER_ADDR    0x1ffe000
#define USER_ADDR   0x1ffd000

// 获取页目录index
#define PDX(va) (((uint32)(va) >> PDXSHIFT) & PAGE_INDEX_MASK)
// 获取页表index
#define PTX(va) (((uint32)(va) >> PTXSHIFT) & PAGE_INDEX_MASK)
// 获取offset
#define P_OFFSET(va) ((uint32)(va) & PAGE_OFFSET_MASK)


extern uint32 free_page_num;
extern uint32 *Page_Dir;
extern uint32 total_page_num;
extern uint32 max_phy_addr; // 最大物理地址

char*   allocstk(uint32, uint32);
char*   alloc_kstk(uint32, uint32);
char*   alloc_ustk(uint32, uint32);
void    AccessTemp(uint32);
void    AccessKernel(uint32);
void    AccessUser(uint32);
Page_t* AllocPage();
void    FreePage(Page_t*);
char*   allocmem(uint32);
syscall deallocmem(char*, uint32);
void    InitializeFreeList();
uint32  heapsbrk(uint32);
void    SetAllocTag(uint32 *, uint32);
void    SetFreeTag(uint32 *, uint32);
uint32* ScanFreeBlock(uint32);
void    DeleteFreeBlock(uint32 *);
void    AssignFreeBlock(uint32 *, uint32 *, uint32);
void    InsertFreeBlock(uint32 *);
void    FreeHeapPage(uint32);
