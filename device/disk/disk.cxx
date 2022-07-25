#include <xinu.h>

extern "C"
{

struct diskblock disktable[DISKTABLE_NUM];

char lba_read(uint32 addr){
    static uint16 cache[CACHE_SIZE];
    static uint32 last = -1;

    uint32 tmp, i;
    uint32 now = addr >> 9;
    if(now != last){
        // 0x01F6
        // 获取lba addr 的起始4bit
        tmp = (now >> 24) & MASK_BIT4;
        outb(0x01F6, tmp | 0xE0);

        // 0x01F5
        // 处理16-23bit
        tmp = (now >> 16) & MASK_BIT8;
        outb(0x01F5, tmp);

        // 0x01F4
        tmp = (now >> 8) & MASK_BIT8;
        outb(0x01F4, tmp);

        // 0x01F3
        tmp = (now >> 0) & MASK_BIT8;
        outb(0x01F3, tmp);

        // 进行其他的一些register
        outb(0x01F2, 1);
        while(inb(0x01F7) & (1 << 7));
        outb(0x01F7, 0x20);

        while(TRUE){
            tmp = inb(0x01F7);
            if(tmp & 8) break;
        }

        insw(0x1F0, (int32)cache, CACHE_SIZE);
        last = now;
    }
    return *((char*)cache + (addr & 0x1FF));
}

devcall	diskread(
	  struct dentry	*devptr,	/* Entry in device switch table	*/
	  char	*buff,			/* Buffer of characters		*/
	  int32	count 			/* Count of character to read	*/
	)
{
    for(int32 i = 0; i < count; i++) buff[i] = diskgetc(devptr);
    return (devcall)OK;
}

devcall	diskgetc(
	  struct dentry	*devptr		/* Entry in device switch table	*/
    )
{
    struct diskblock *diskptr;
    diskptr = &disktable[devptr->dvminor];
    char a = lba_read(diskptr->lba_addr);
    diskptr->lba_addr++;
    return (devcall)a;
}

devcall	diskseek (
	  struct dentry *devptr,	/* Entry in device switch table */
	  uint32	offset		/* Byte position in the file	*/
	)
{
    struct diskblock *diskptr;
    diskptr = &disktable[devptr->dvminor];
    diskptr->lba_addr = offset;
    return (devcall)OK;
}

}
