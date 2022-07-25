/*  main.c  - main */

#include <xinu.h>
#include "../include/filesystem.h"

process	main(void)
{
	char	*elf;
	uint32	elf_offset;

	elf = getmem(8192);
	memset(elf, 0, 8192);

	read_file("../hello.elf", elf, 8192);

	elf_offset = get_elf_entrypoint(elf);
	printf("The offset is:%d\n\n",elf_offset);
	resume(create(elf + elf_offset, INITSTK, INITPRIO, "hello", 1, (uint32)printf));

	freemem(elf, 8192);
	
	return OK;
}
