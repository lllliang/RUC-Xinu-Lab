/* initialize.c - nulluser, sysinit */

/* Handle system initialization and become the null process */

#include <xinu.h>
#include <string.h>

extern	void	start(void);	/* Start of Xinu code			*/
extern	void	*_end;		/* End of Xinu code			*/

/* Function prototypes */

extern	void main(void);	/* Main is the first process created	*/
static	void sysinit(); 	/* Internal system initialization	*/
extern	void meminit(void);	/* Initializes the free memory list	*/
local	process startup(void);	/* Process to finish startup tasks	*/

/* Declarations of major kernel variables */

struct	procent	proctab[NPROC];	/* Process table			*/
struct	sentry	semtab[NSEM];	/* Semaphore table			*/
struct	memblk	memlist;	/* List of free memory blocks		*/

/* Active system status */

int	prcount;		/* Total number of live processes	*/
pid32	currpid;		/* ID of currently executing process	*/

extern struct TSS_ TSS;

/* Control sequence to reset the console colors and cusor positiion	*/

#define	CONSOLE_RESET	" \033[0m\033[2J\033[;H"

/*------------------------------------------------------------------------
 * nulluser - initialize the system and become the null process
 *
 * Note: execution begins here after the C run-time environment has been
 * established.  Interrupts are initially DISABLED, and must eventually
 * be enabled explicitly.  The code turns itself into the null process
 * after initialization.  Because it must always remain ready to execute,
 * the null process cannot execute code that might cause it to be
 * suspended, wait for a semaphore, put to sleep, or exit.  In
 * particular, the code must not perform I/O except for polled versions
 * such as kprintf.
 *------------------------------------------------------------------------
 */

void	nulluser()
{	
	SearchPages();

	vminit();
	/* Initialize the system */
	sysinit();	
	/* Enable interrupts */
	enable();

	ltss(0x7 << 3);
	TSS.ss0 = (0x3 << 3);

	/* Create a process to finish startup and start main */
	resume(create((void *)startup, INITSTK, INITPRIO,
					"Startup process", 0, NULL));

	/* Become the Null process (i.e., guarantee that the CPU has	*/
	/*  something to run when no other process is ready to execute)	*/

	while (TRUE) {

		/* Halt until there is an external interrupt */

		asm volatile ("hlt");
	}

}


/*------------------------------------------------------------------------
 *
 * startup  -  Finish startup takss that cannot be run from the Null
 *		  process and then create and resumethe main process
 *
 *------------------------------------------------------------------------
 */
local process	startup(void)
{
	/* Create a process to execute function main() */

	syscall_resume(syscall_create((void *)main, INITSTK, INITPRIO,
					"Main process", 0, NULL));

	/* Startup process exits at this point */

	return OK;
}


/*------------------------------------------------------------------------
 *
 * sysinit  -  Initialize all Xinu data structures and devices
 *
 *------------------------------------------------------------------------
 */
static	void	sysinit()
{
	int32	i;
	struct	procent	*prptr;		/* Ptr to process table entry	*/
	struct	sentry	*semptr;	/* Ptr to semaphore table entry	*/

	/* Platform Specific Initialization */

	platinit();

	/* Reset the console */

	kprintf(CONSOLE_RESET);
	kprintf("\n%s\n\n", VERSION);

	/* Initialize the interrupt vectors */

	initevec();

	/* Initialize system variables */

	/* Count the Null process as the first process in the system */

	prcount = 1;

	/* Scheduling is not currently blocked */

	Defer.ndefers = 0;

	/* Initialize process table entries free */

	for (i = 0; i < NPROC; i++) {
		prptr = &proctab[i];
		prptr->prstate = PR_FREE;
		prptr->prname[0] = NULLCH;
		prptr->prstkbase = NULL;
		prptr->prprio = 0;
	}

	/* Initialize the Null process entry */	

	prptr = &proctab[NULLPROC];
	prptr->prstate = PR_CURR;
	prptr->prprio = 0;
	strncpy(prptr->prname, "prnull", 7);
	prptr->phy_page_dir = KERNEL_END - PAGE_SIZE * 10;
	prptr->prstkbase = allocstk(NULLSTK, prptr->phy_page_dir);
	prptr->prstklen = NULLSTK;
	prptr->prstkptr = (char*)prptr->prstkbase - prptr->prstklen; // 初始化esp
	prptr->free_list_ptr = NULL;
	prptr->max_heap = KERNEL_END;

	currpid = NULLPROC;
	
	/* Initialize semaphores */

	for (i = 0; i < NSEM; i++) {
		semptr = &semtab[i];
		semptr->sstate = S_FREE;
		semptr->scount = 0;
		semptr->squeue = newqueue();
	}

	/* Initialize buffer pools */

	bufinit();

	/* Create a ready list for processes */

	readylist = newqueue();

	/* Initialize the real time clock */

	clkinit();

	sndreadylist = newqueue();

	for (i = 0; i < NDEVS; i++) {
		init(i);
	}
	return;
}

int32	stop(char *s)
{
	kprintf("%s\n", s);
	kprintf("looping... press reset\n");
	while(1)
		/* Empty */;
}

int32	delay(int n)
{
	DELAY(n);
	return OK;
}

void lcr3(uint32 pgdir){
	asm volatile(
		"mov %0, %%cr3\n\t"
		:
		:"r"(pgdir)
		:
	);
}

void vminit(){
	// 从32MB处获取10个页
	// 第0个页用于页目录

	Page_t *pages = (Page_t*)(KERNEL_END - PAGE_SIZE * 12);
	memset(pages, 0, PAGE_SIZE);
	// 1 ~ 8个页用于页表
	for(int i = 0; i < 8; i++){
		// KERNEL_END - PAGE_SIZE*(i+1), 向页目录中存入第i个页表
		// 初始化页目录
		pages[0].entries[i] = (uint32)((char *)pages + ((i + 1) * PAGE_SIZE)) | PG_P | PG_RW_W | PG_US_U;
		for(int j = 0; j < 1024; j++){
			// 映射页表
			pages[i + 1].entries[j] = (((i << 10) + j) << 12) | PG_P | PG_RW_W | PG_US_U;
		}
	}
	// 映射记录内核栈页
	pages[0].entries[1021] = (uint32)((char*)pages + (9 * PAGE_SIZE)) | PG_P | PG_RW_W | PG_US_U;
	memset((char*)pages + 9 * PAGE_SIZE, 0, PAGE_SIZE);
	pages[9].entries[1023] = 0x6000 | PG_P | PG_RW_W | PG_US_U; // for temp
	// 1M 
	
	lcr3((uint32)pages);
	asm(
		"movl %cr0, %eax\n\t"
		"orl $0x80000000, %eax\n\t"
		"movl %eax, %cr0\n\t"
	);
	
}
