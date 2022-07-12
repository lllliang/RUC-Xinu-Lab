/* create.c - create, newpid */

#include <xinu.h>

local	int newpid();
extern struct TSS_ TSS;


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
	struct TSS_		*tss;	

	mask = disable();
	if (ssize < MINSTK)
		ssize = MINSTK;
	ssize = (uint32) roundmb(ssize);
	if ( (priority < 1) || ((pid=newpid()) == SYSERR) ||
	     ((saddr = (uint32 *)getstk(ssize)) == (uint32 *)SYSERR) || //创建内核栈
		 ((usaddr = (uint32 *)getstk(ssize)) == (uint32 *)SYSERR)) { //创建用户栈
		restore(mask);
		return SYSERR;
	}

	prcount++;
	prptr = &proctab[pid];

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

	/* 初始化程序运行时间 */
	prptr->prtime = 0;
	
	prptr->prremain = init_remain_time(priority);

	/* Initialize stack as if the process was called		*/

	*saddr = STACKMAGIC;
	*usaddr = STACKMAGIC;

	savsp = (uint32)saddr;

	/* Push arguments */
	a = (uint32 *)(&nargs + 1);	/* Start of args		*/
	a += nargs -1;			/* Last argument		*/
	for ( ; nargs > 0 ; nargs--){	/* Machine dependent; copy args	*/
		argv_a = *a--;
		*--saddr 	= argv_a;
		*--usaddr 	= argv_a;
		//*--saddr = *a--;
	}	/* onto created process's stack	*/
	*--saddr 	= (long)INITRET;	/* Push on return address	*/
	*--usaddr 	= (long)INITRET;

	prptr->esp0 = (long)saddr;
	
	// 修改特权级对应参数
	// tss->ss0 	= (0x3 << 3);	// 内核栈段选择符
	// tss->esp0 	= (long)saddr;	// 内核栈地址

	//init tss
	// tss->esp0		= (long)saddr;
	// tss->ss0		= (0x2) << 3; //内核ss  gdt第三位

	
	/* The following entries on the stack must match what ctxsw	*/
	/*   expects a saved process state to contain: ret address,	*/
	/*   ebp, interrupt mask, flags, registers, and an old SP	*/

	savsp2 = (uint32)usaddr;
	/*//////////////////////////////////////////////////////////////*/
	/*                      构建返回到用户态的中断栈                 */
	*--saddr = 0x33; 			// init ss	0011 0011
	*--saddr = savsp2;			// save esp for User INITRET
	*--saddr = 0x00000202;		// init EFLAGS	 0010 0000 0010  212     		0010 0001 0010 
	*--saddr = 0x23;			// init cs	0010 0011							0010 0000 0000 
	*--saddr = (long)funcaddr;	// init eip
	// *--saddr = (long)0x101c79;
	/*//////////////////////////////////////////////////////////////*/

	*--saddr = 0x10;	//ds
	*--saddr = 0x10;	//es
	*--saddr = 0x10;	//fs
	*--saddr = 0x10;	//gs

	*--saddr = (long)ret_k2u;

	// *--saddr = (long)funcaddr;	/* Make the stack look like it's*/
					/*   half-way through a call to	*/
					/*   ctxsw that "returns" to the*/
					/*   new process		*/
	*--saddr = savsp;		/* This will be register ebp	*/
					/*   for process exit		*/
	savsp = (uint32) saddr;		/* Start of frame for ctxsw	*/
	*--saddr = 0x00000200;		/* New process runs with	*/
					/*   interrupts enabled		*/

	/* Basically, the following emulates an x86 "pushal" instruction*/

	*--saddr = 0;			/* %eax */
	*--saddr = 0;			/* %ecx */
	*--saddr = 0;			/* %edx */
	*--saddr = 0;			/* %ebx */
	*--saddr = 0;			/* %esp; value filled in below	*/
	pushsp = saddr;			/* Remember this location	*/
	*--saddr = savsp;		/* %ebp (while finishing ctxsw)	*/
	*--saddr = 0;			/* %esi */
	*--saddr = 0;			/* %edi */
	*pushsp = (unsigned long) (prptr->prstkptr = (char *)saddr);
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
