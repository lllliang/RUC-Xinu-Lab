#define DISKTABLE_NUM 1
#define CACHE_SIZE 256

#define MASK_BIT4 0xF
#define MASK_BIT8 0xFF


struct	diskblock {
	uint32	lba_addr;
};

extern struct diskblock disktable[];
