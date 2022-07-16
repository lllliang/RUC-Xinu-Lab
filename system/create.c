/* create.c - create, newpid */

#include <xinu.h>

local	int newpid();


/*------------------------------------------------------------------------
 *  create  -  Create a process to start running a function on x86
 *------------------------------------------------------------------------
 */
pid32	create(
	  void		*funcaddr,	/* Address of the function	*/
	  uint32	ssize,		/* Stack size in bytes		*/
	  pri16		priority,	/* Process priority > 0		*/
	  char		*name,		/* Name (for debugging)		*/
	  uint32	nargs,		/* Number of args that follow	*/
	  ...
	)
{
	uint32		savsp, *pushsp, savsp2;
	intmask 	mask;    	/* Interrupt mask		*/
	pid32		pid;		/* Stores new process id	*/
	struct	procent	*prptr;		/* Pointer to proc. table entry */
	int32		i;
	uint32		*a;		/* Points to list of args	*/
	uint32		argv_a; //记录args的值
	uint32		*saddr;		/* Stack address		*/
	uint32		*usaddr;

	mask = disable();
	if (ssize < MINSTK)
		ssize = MINSTK;
	ssize = (uint32) roundmb(ssize);
	if((pid = newpid()) == SYSERR || (priority < 1)){
		restore(mask);
		return SYSERR;
	}

	prcount++;
	prptr = &proctab[pid];
	prptr->phy_page_dir = (uint32)AllocPage();
	// prptr->phy_page_dir = (uint32)Alloc_Page_Dir();
	// if((saddr = (uint32*)alloc_kstk(ssize, prptr->phy_page_dir)) == (uint32 *)SYSERR){
	// 	restore(mask);
	// 	return SYSERR;
	// }

	if((saddr = (uint32*)alloc_kstk(ssize, prptr->phy_page_dir)) == (uint32 *)SYSERR || 
		(usaddr = (uint32*)alloc_ustk(ssize, prptr->phy_page_dir)) == (uint32 *)SYSERR){
		restore(mask);
		return SYSERR;
	}

	/* Initialize process table entry for new process */
	prptr->prstate = PR_SUSP;	/* Initial state is suspended	*/
	prptr->prprio = priority;
	prptr->prstkbase = (char *)saddr;
	prptr->prstklen = ssize;
	prptr->prname[PNMLEN-1] = NULLCH;
	for (i=0 ; i<PNMLEN-1 && (prptr->prname[i]=name[i])!=NULLCH; i++)
		;
	prptr->prsem = -1;
	prptr->prparent = (pid32)getpid();
	prptr->prhasmsg = FALSE;

	/* Set up stdin, stdout, and stderr descriptors for the shell	*/
	prptr->prdesc[0] = CONSOLE;
	prptr->prdesc[1] = CONSOLE;
	prptr->prdesc[2] = CONSOLE;

	prptr->free_list_ptr = NULL;
	prptr->max_heap = KERNEL_END;

	/* 初始化程序运行时间 */
	prptr->prtime = 0;
	
	prptr->prremain = init_remain_time(priority);

	/* Initialize stack as if the process was called		*/
	uint32 *saddr_ = saddr; //0xfffffffc
	uint32 *usaddr_ = usaddr; //0xffbffffc
	saddr = (uint32 *)0x1ffeffc; //这里采用临时映射
	// saddr = (uint32 *)0x1fffffc; //这里采用临时映射
	usaddr = (uint32 *)0x1ffdffc;
	*saddr = STACKMAGIC;
	*usaddr = STACKMAGIC;

	savsp = (uint32)saddr_;

	/* Push arguments */
	a = (uint32 *)(&nargs + 1);	/* Start of args		*/
	a += nargs -1;			/* Last argument		*/
	for ( ; nargs > 0 ; nargs--){	/* Machine dependent; copy args	*/
		argv_a = *a--;
		*--saddr 	= argv_a;
		--saddr_;
		*--usaddr 	= argv_a;
		--usaddr_;
		//*--saddr = *a--;
	}	/* onto created process's stack	*/
	*--saddr 	= (long)INITRET;	/* Push on return address	*/
	--saddr_;
	*--usaddr 	= (long)INITRET;
	--usaddr_;

	prptr->esp0 = (uint32)saddr_;
	/* The following entries on the stack must match what ctxsw	*/
	/*   expects a saved process state to contain: ret address,	*/
	/*   ebp, interrupt mask, flags, registers, and an old SP	*/

	// savsp2 = (uint32)usaddr;
	savsp2 = (uint32)usaddr_;
	/*//////////////////////////////////////////////////////////////*/
	/*                      构建返回到用户态的中断栈                 */
	*--saddr = 0x33; 			// init ss	0011 0011
	--saddr_;
	*--saddr = savsp2;			// save esp for User INITRET
	--saddr_;
	*--saddr = 0x00000202;		// init EFLAGS	 0010 0000 0010  212     		0010 0001 0010 
	--saddr_;
	*--saddr = 0x23;			// init cs	0010 0011							0010 0000 0000 
	--saddr_;
	*--saddr = (long)funcaddr;	// init eip
	--saddr_;
	// *--saddr = (long)0x101c79;
	/*//////////////////////////////////////////////////////////////*/

	*--saddr = 0x10;	//ds
	--saddr_;
	*--saddr = 0x10;	//es
	--saddr_;
	*--saddr = 0x10;	//fs
	--saddr_;
	*--saddr = 0x10;	//gs
	--saddr_;

	*--saddr = (long)ret_k2u;
	--saddr_;

	// *--saddr = (long)funcaddr;	/* Make the stack look like it's*/
					/*   half-way through a call to	*/
					/*   ctxsw that "returns" to the*/
					/*   new process		*/
	// *--saddr = savsp;		/* This will be register ebp	*/
	*--saddr = (uint32)prptr->prstkbase;
	--saddr_;
					/*   for process exit		*/

	savsp = (uint32)saddr_;

	// savsp = (uint32) saddr;		/* Start of frame for ctxsw	*/
	*--saddr = 0x00000200;		/* New process runs with	*/
	--saddr_;
					/*   interrupts enabled		*/

	/* Basically, the following emulates an x86 "pushal" instruction*/

	*--saddr = 0;			/* %eax */
	--saddr_;
	*--saddr = 0;			/* %ecx */
	--saddr_;
	*--saddr = 0;			/* %edx */
	--saddr_;
	*--saddr = 0;			/* %ebx */
	--saddr_;
	*--saddr = 0;			/* %esp; value filled in below	*/
	--saddr_;
	pushsp = saddr;			/* Remember this location	*/
	*--saddr = savsp;		/* %ebp (while finishing ctxsw)	*/
	--saddr_;
	*--saddr = 0;			/* %esi */
	--saddr_;
	*--saddr = 0;			/* %edi */
	--saddr_;
	*pushsp = (unsigned long) (prptr->prstkptr = (char *)saddr_);
	restore(mask);
	return pid;
}

/*------------------------------------------------------------------------
 *  newpid  -  Obtain a new (free) process ID
 *------------------------------------------------------------------------
 */
local	pid32	newpid(void)
{
	uint32	i;			/* Iterate through all processes*/
	static	pid32 nextpid = 1;	/* Position in table to try or	*/
					/*   one beyond end of table	*/

	/* Check all NPROC slots */

	for (i = 0; i < NPROC; i++) {
		nextpid %= NPROC;	/* Wrap around to beginning */
		if (proctab[nextpid].prstate == PR_FREE) {
			return nextpid++;
		} else {
			nextpid++;
		}
	}kprintf("newpid error\n");
	return (pid32) SYSERR;
}
