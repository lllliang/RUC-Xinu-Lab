/* resched.c - resched, resched_cntl */

#include <xinu.h>

struct	defer	Defer;


void reset_remain_time(uint16 q2size){
	pid32	pid;
	struct procent *proc;
	
	for(uint16 i = 0; i < q2size; i++){
		pid = dequeue(sndreadylist);
		proc = &proctab[pid];
		proc->prremain = retime(proc->prprio);
		insert(pid, readylist, proc->prprio);
	}
}

pid32 get_pid(void){
	qid16 curr;
	curr = dequeue(readylist);
	if(!curr){  /* 如果是0号进程*/
		insert(0, readylist, 0);
		return dequeue(readylist);
	}
	return curr;
}


pid32 mux_pid(
	uint16 q1size, 
	uint16 q2size)
{
	//下列操作集合成一个函数叭
	if(q1size == 1 && q2size > 0){
		reset_remain_time(q2size); /* 还没实现 */
		/* choose a currpid */
		return get_pid();
	}
	else if (q1size == 1 && q2size == 0){
		return dequeue(readylist); /* 此时只剩下0号进程 */
	}
	/* 剩下0号进程与其他进程 */
	return get_pid();
}



/*------------------------------------------------------------------------
 *  resched  -  Reschedule processor to highest priority eligible process
 *------------------------------------------------------------------------
 */
void	resched(void)		/* Assumes interrupts are disabled	*/
{
	struct procent *ptold;	/* Ptr to table entry for old process	*/
	struct procent *ptnew;	/* Ptr to table entry for new process	*/
	uint16 q1size, q2size;


	/* If rescheduling is deferred, record attempt and return */

	if (Defer.ndefers > 0) {
		Defer.attempt = TRUE;
		return;
	}

	/* Point to process table entry for the current (old) process */

	ptold = &proctab[currpid];

	if (ptold->prstate == PR_CURR) {  /* Process remains eligible */
		// if (ptold->prprio > firstkey(readylist)) {
		// 	return;   //如果当前进程的优先级大于readylist
		// }
		/* Old process will no longer remain current */
		ptold->prstate = PR_READY;
		// insert(currpid, readylist, ptold->prprio);
		insert2ready(currpid, ptold->prprio, ptold->prremain);
	}

	/* Force context switch to highest priority ready process */
	q1size = getqueue_size(readylist);
	q2size = getqueue_size(sndreadylist);
	
	currpid = mux_pid(q1size, q2size);

	// currpid = dequeue(readylist);  //先修改currpid再进行ctxsw
	ptnew = &proctab[currpid];
	ptnew->prstate = PR_CURR;
	preempt = QUANTUM;		/* Reset time slice for process	*/
	ctxsw(&ptold->prstkptr, &ptnew->prstkptr);

	/* Old process returns here when resumed */

	return;
}




/*------------------------------------------------------------------------
 *  resched_cntl  -  Control whether rescheduling is deferred or allowed
 *------------------------------------------------------------------------
 */
status	resched_cntl(		/* Assumes interrupts are disabled	*/
	  int32	defer		/* Either DEFER_START or DEFER_STOP	*/
	)
{
	switch (defer) {

	    case DEFER_START:	/* Handle a deferral request */

		if (Defer.ndefers++ == 0) {
			Defer.attempt = FALSE;
		}
		return OK;

	    case DEFER_STOP:	/* Handle end of deferral */
		if (Defer.ndefers <= 0) {
			return SYSERR;
		}
		if ( (--Defer.ndefers == 0) && Defer.attempt ) {
			resched();
		}
		return OK;

	    default:
		return SYSERR;
	}
}
