/* clkhandler.c - clkhandler */

#include <xinu.h>

/*------------------------------------------------------------------------
 * clkhandler - high level clock interrupt handler
 *------------------------------------------------------------------------
 */
void	clkhandler(void)
{
	/* Decrement the ms counter, and see if a second has passed */

	if((++count1000) >= 1000) {

		/* One second has passed, so increment seconds count */

		clktime++;

		/* Reset the local ms counter for the next second */

		count1000 = 0;
	}

	/* Handle sleeping processes if any exist */

	if(!isempty(sleepq)) {

		/* Decrement the delay for the first process on the	*/
		/*   sleep queue, and awaken if the count reaches zero	*/

		if((--queuetab[firstid(sleepq)].qkey) <= 0) {
			wakeup();
		}
	}

	/* 进程时间计数器 */
	struct	procent	*prptr;
	prptr = &proctab[getpid()];
	prptr->prtime++;

	/* 递减可用时间片 */
	prptr->prremain--;
	if((--prptr->prremain) <= 0){
		preempt = QUANTUM;
		// resched();
		return;
	}

	/* Decrement the preemption counter, and reschedule when the */
	/*   remaining time reaches zero			     */

	if((--preempt) <= 0) {
		preempt = QUANTUM;
		// resched();
	}
}
