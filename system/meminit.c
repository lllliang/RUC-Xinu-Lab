/* meminit.c - memory bounds and free list init */

#include <xinu.h>

/* Memory bounds */

void	*minheap;		/* Start of heap			*/
void	*maxheap;		/* Highest valid heap address		*/

/* Memory map structures */

uint32	bootsign = 1;		/* Boot signature of the boot loader	*/

struct	mbootinfo *bootinfo = (struct mbootinfo *)1;
				/* Base address of the multiboot info	*/
				/*  provided by GRUB, initialized just	*/
				/*  to guarantee it is in the DATA	*/
				/*  segment and not the BSS		*/


// struct TSS_ TSS[NPROC]; 


struct TSS_ TSS;



/* Segment table structures */

/* Segment Descriptor */

struct __attribute__ ((__packed__)) sd {
	unsigned short	sd_lolimit;
	unsigned short	sd_lobase;
	unsigned char	sd_midbase;
	unsigned char   sd_access;
	unsigned char	sd_hilim_fl;
	unsigned char	sd_hibase;
};

// #define	NGD			107	/* Number of global descriptor entries	*/
#define NGD		8
#define FLAGS_GRANULARITY	0x80
#define FLAGS_SIZE		0x40
#define	FLAGS_SETTINGS		(FLAGS_GRANULARITY | FLAGS_SIZE)

struct sd gdt_copy[NGD] = {
/*   sd_lolimit  sd_lobase   sd_midbase  sd_access   sd_hilim_fl sd_hibase */
/* 0th entry NULL */
{            0,          0,           0,         0,            0,        0, },
/* 1st, Kernel Code Segment */				//1001 1010		//1100 1111
{       0xffff,          0,           0,      0x9a,         0xcf,        0, },
/* 2nd, Kernel Data Segment */
{       0xffff,          0,           0,      0x92,         0xcf,        0, },
/* 3rd, Kernel Stack Segment */				//1001 0010
{       0xffff,          0,           0,      0x92,         0xcf,        0, },
/* 4th, User Code Segment */				//1111 1010		//1100 0000
{		0xffff,			 0,			  0,	  0xfa,			0xc0,		 0, },
/* 5th  User Data Segment */
{		0xffff,			 0,			  0,	  0xf2, 		0xc0, 		 0, },
/* 6th  User Stack Segment */
{		0xffff,			 0,			  0,	  0xf2, 		0xc0, 		 0, },
// /* 7th	Tss */
{		0x67,	 		 0,			  0, 	  0x89,			0x00,		 0, }
};
extern	struct	sd gdt[];	/* Global segment table			*/


uint32 free_page_num = 0;
uint32 *Page_Dir = (uint32 *)(KERNEL_END - 12 * PAGE_SIZE);
uint32 total_page_num = 0;
uint32 max_phy_addr; // 最大物理地址

// 搜寻空闲block 并存入page
void SearchPages(){
	struct	memblk	*memptr;	/* Ptr to memory block		*/
	struct	mbmregion	*mmap_addr;	/* Ptr to mmap entries		*/
	struct	mbmregion	*mmap_addrend;	/* Ptr to end of mmap region	*/
	struct	memblk	*next_memptr;	/* Ptr to next memory block	*/
	uint32	next_block_length;	/* Size of next memory block	*/

	mmap_addr = (struct mbmregion*)NULL;
	mmap_addrend = (struct mbmregion*)NULL;

	/* Initialize the free list */
	memptr = &memlist;
	memptr->mnext = (struct memblk *)NULL;
	memptr->mlength = 0;

	/* Initialize the memory counters */
	/*    Heap starts at the end of Xinu image */
	minheap = (void*)KERNEL_END;
	maxheap = minheap;

	/* Check if Xinu was loaded using the multiboot specification	*/
	/*   and a memory map was included				*/
	if(bootsign != MULTIBOOT_SIGNATURE) {
		panic("could not find multiboot signature");
	}
	if(!(bootinfo->flags & MULTIBOOT_BOOTINFO_MMAP)) {
		panic("no mmap found in boot info");
	}

	/* Get base address of mmap region (passed by GRUB) */
	mmap_addr = (struct mbmregion*)bootinfo->mmap_addr;

	/* Calculate address that follows the mmap block */
	mmap_addrend = (struct mbmregion*)((uint8*)mmap_addr + bootinfo->mmap_length);

	/* Read mmap blocks and initialize the Xinu free memory list	*/
	while(mmap_addr < mmap_addrend) {

		/* If block is not usable, skip to next block */
		if(mmap_addr->type != MULTIBOOT_MMAP_TYPE_USABLE) {
			mmap_addr = (struct mbmregion*)((uint8*)mmap_addr + mmap_addr->size + 4);
			continue;
		}

		if((uint32)maxheap < ((uint32)mmap_addr->base_addr + (uint32)mmap_addr->length)) {
			maxheap = (void*)((uint32)mmap_addr->base_addr + (uint32)mmap_addr->length);
		}

		/* Ignore memory blocks within the Xinu image */
		if((mmap_addr->base_addr + mmap_addr->length) < ((uint32)minheap)) {
			mmap_addr = (struct mbmregion*)((uint8*)mmap_addr + mmap_addr->size + 4);
			continue;
		}

		/* The block is usable, so add it to Xinu's memory list */

		/* This block straddles the end of the Xinu image */
		if((mmap_addr->base_addr <= (uint32)minheap) &&
		  ((mmap_addr->base_addr + mmap_addr->length) >
		  (uint32)minheap)) {

			/* This is the first free block, base address is the minheap */
			next_memptr = (struct memblk *)roundmb(minheap);

			/* Subtract Xinu image from length of block */
			next_block_length = (uint32)truncmb(mmap_addr->base_addr + mmap_addr->length - (uint32)minheap);
			uint32 temp = (uint32)next_memptr;
			for(int i = 0; i < next_block_length / PAGE_SIZE; i++){
				free_page_num++;
				*(Page_Dir - free_page_num) = temp;
				temp += 4096;
			}
		} 
		else {

			/* Handle a free memory block other than the first one */
			next_memptr = (struct memblk *)roundmb(mmap_addr->base_addr);

			/* Initialize the length of the block */
			next_block_length = (uint32)truncmb(mmap_addr->length);
			uint32 temp = (uint32)next_memptr;
			for(int i = 0; i < next_block_length / PAGE_SIZE; i++){
				free_page_num++;
				*(Page_Dir - free_page_num) = temp;
				temp += 4096;
			}
		}

		/* Add then new block to the free list */
		memptr->mnext = next_memptr;
		memptr = memptr->mnext;
		memptr->mlength = next_block_length;
		memlist.mlength += next_block_length;

		/* Move to the next mmap block */
		mmap_addr = (struct mbmregion*)((uint8*)mmap_addr + mmap_addr->size + 4);
	}

	/* End of all mmap blocks, and so end of Xinu free list */
	if(memptr) {
		memptr->mnext = (struct memblk *)NULL;
	}
	total_page_num = free_page_num;
	max_phy_addr = *(Page_Dir - free_page_num);
}




/*------------------------------------------------------------------------
 * meminit - initialize memory bounds and the free memory list
 *------------------------------------------------------------------------
 */
void	meminit(void) {

	struct	memblk	*memptr;	/* Ptr to memory block		*/
	struct	mbmregion	*mmap_addr;	/* Ptr to mmap entries		*/
	struct	mbmregion	*mmap_addrend;	/* Ptr to end of mmap region	*/
	struct	memblk	*next_memptr;	/* Ptr to next memory block	*/
	uint32	next_block_length;	/* Size of next memory block	*/

	mmap_addr = (struct mbmregion*)NULL;
	mmap_addrend = (struct mbmregion*)NULL;

	/* Initialize the free list */
	memptr = &memlist;
	memptr->mnext = (struct memblk *)NULL;
	memptr->mlength = 0;

	/* Initialize the memory counters */
	/*    Heap starts at the end of Xinu image */
	minheap = (void*)&end;
	maxheap = minheap;

	/* Check if Xinu was loaded using the multiboot specification	*/
	/*   and a memory map was included				*/
	if(bootsign != MULTIBOOT_SIGNATURE) {
		panic("could not find multiboot signature");
	}
	if(!(bootinfo->flags & MULTIBOOT_BOOTINFO_MMAP)) {
		panic("no mmap found in boot info");
	}

	/* Get base address of mmap region (passed by GRUB) */
	mmap_addr = (struct mbmregion*)bootinfo->mmap_addr;

	/* Calculate address that follows the mmap block */
	mmap_addrend = (struct mbmregion*)((uint8*)mmap_addr + bootinfo->mmap_length);

	/* Read mmap blocks and initialize the Xinu free memory list	*/
	while(mmap_addr < mmap_addrend) {

		/* If block is not usable, skip to next block */
		if(mmap_addr->type != MULTIBOOT_MMAP_TYPE_USABLE) {
			mmap_addr = (struct mbmregion*)((uint8*)mmap_addr + mmap_addr->size + 4);
			continue;
		}

		if((uint32)maxheap < ((uint32)mmap_addr->base_addr + (uint32)mmap_addr->length)) {
			maxheap = (void*)((uint32)mmap_addr->base_addr + (uint32)mmap_addr->length);
		}

		/* Ignore memory blocks within the Xinu image */
		if((mmap_addr->base_addr + mmap_addr->length) < ((uint32)minheap)) {
			mmap_addr = (struct mbmregion*)((uint8*)mmap_addr + mmap_addr->size + 4);
			continue;
		}

		/* The block is usable, so add it to Xinu's memory list */

		/* This block straddles the end of the Xinu image */
		if((mmap_addr->base_addr <= (uint32)minheap) &&
		  ((mmap_addr->base_addr + mmap_addr->length) >
		  (uint32)minheap)) {

			/* This is the first free block, base address is the minheap */
			next_memptr = (struct memblk *)roundmb(minheap);

			/* Subtract Xinu image from length of block */
			next_block_length = (uint32)truncmb(mmap_addr->base_addr + mmap_addr->length - (uint32)minheap);
		} else {

			/* Handle a free memory block other than the first one */
			next_memptr = (struct memblk *)roundmb(mmap_addr->base_addr);

			/* Initialize the length of the block */
			next_block_length = (uint32)truncmb(mmap_addr->length);
		}

		/* Add then new block to the free list */
		memptr->mnext = next_memptr;
		memptr = memptr->mnext;
		memptr->mlength = next_block_length;
		memlist.mlength += next_block_length;

		/* Move to the next mmap block */
		mmap_addr = (struct mbmregion*)((uint8*)mmap_addr + mmap_addr->size + 4);
	}

	/* End of all mmap blocks, and so end of Xinu free list */
	if(memptr) {
		memptr->mnext = (struct memblk *)NULL;
	}
}


/*------------------------------------------------------------------------
 * setsegs  -  Initialize the global segment table
 *------------------------------------------------------------------------
 */
void	setsegs()
{
	extern int	etext;
	struct sd	*psd;
	uint32		np, ds_end;
	uint32		base;	// tss base address

	ds_end = 0xffffffff/PAGE_SIZE; /* End page number of Data segment */
	np = ((int)&etext - 0 + PAGE_SIZE-1) / PAGE_SIZE;	/* Number of code pages */
	
	
	psd = &gdt_copy[1];	/* Kernel code segment: identity map from address
				   0 to etext */
	psd->sd_lolimit = np;
	psd->sd_hilim_fl = FLAGS_SETTINGS | ((np >> 16) & 0xff);

	psd = &gdt_copy[2];	/* Kernel data segment */
	psd->sd_lolimit = ds_end;
	psd->sd_hilim_fl = FLAGS_SETTINGS | ((ds_end >> 16) & 0xff);

	psd = &gdt_copy[3];	/* Kernel stack segment */
	psd->sd_lolimit = ds_end;
	psd->sd_hilim_fl = FLAGS_SETTINGS | ((ds_end >> 16) & 0xff);

	// init tss
	psd = &gdt_copy[7];
	base = (long)(&TSS);
	psd->sd_lobase 	= base & 0xffff;
	psd->sd_midbase = (base >> 16) & 0xff;
	psd->sd_hibase	= (base >> 24) & 0xff; 

	// init tss in gdt
	// for(int i = 0; i < 100; i++){
	// 	psd = &gdt_copy[i+7];
	// 	base = (long)(&TSS[i]);
	// 	psd->sd_lobase 		=  base & 0xffff;
	// 	psd->sd_midbase 	= (base >> 16) & 0xff;
	// 	psd->sd_hibase		= (base >> 24) & 0xff; 
	// 	psd->sd_lolimit		= 0x67;
	// 	psd->sd_hilim_fl	= 0x00;	//G B 0 AVL hilimit 
	// 	psd->sd_access		= 0x89;
	// }

	memcpy(gdt, gdt_copy, sizeof(gdt_copy));
}
