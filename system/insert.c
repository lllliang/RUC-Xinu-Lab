/* insert.c - insert */

#include <xinu.h>

/*------------------------------------------------------------------------
 *  insert  -  Insert a process into a queue in descending key order
 *------------------------------------------------------------------------
 */



uint16 getqueue_size(
	qid16	q
){
	qid16	curr, tail;
	uint16	size = 1;
	tail = queuetail(q);
	if((curr = firstid(q)) == tail) return 0;
	while(queuetab[curr].qnext != tail){
		size++;
		curr = queuetab[curr].qnext;
	}
	return size;
}



status insert2ready(
	pid32	pid, 
	int32	key, 
	uint16 	remain
){
	// if(pid == 0) return OK;
	if(remain > 0) 
		return insert(pid, readylist, key);
	else if(remain == 0) 
		return insert(pid, sndreadylist, key);
	else 
		return SYSERR;
}



status	insert(
	  pid32		pid,		/* ID of process to insert	*/
	  qid16		q,		/* ID of queue to use		*/
	  int32		key		/* Key for the inserted process	*/
	)
{
	qid16	tail, prev;

	if((int32)(q) < NPROC || (int32)(q) >=305 || isbadpid(pid)){
		return SYSERR;
	}
	// if (isbadqid(q) || isbadpid(pid)) {
	// 	return SYSERR;
	// }

	tail = queuetail(q);
	prev = queuetab[tail].qprev;

	queuetab[pid].qnext = tail;
	queuetab[pid].qprev = prev;
	queuetab[pid].qkey = key;
	queuetab[prev].qnext = pid;
	queuetab[tail].qprev = pid;
	return OK;
}




// status	insert(
// 	  pid32		pid,		/* ID of process to insert	*/
// 	  qid16		q,		/* ID of queue to use		*/
// 	  int32		key		/* Key for the inserted process	*/
// 	)
// {
// 	qid16	curr;			/* Runs through items in a queue*/
// 	qid16	prev;			/* Holds previous node index	*/

// 	if (isbadqid(q) || isbadpid(pid)) {
// 		return SYSERR;
// 	}

// 	curr = firstid(q);
// 	while (queuetab[curr].qkey >= key) {
// 		curr = queuetab[curr].qnext;
// 	}

// 	/* Insert process between curr node and previous node */

// 	prev = queuetab[curr].qprev;	/* Get index of previous node	*/
// 	queuetab[pid].qnext = curr;
// 	queuetab[pid].qprev = prev;
// 	queuetab[pid].qkey = key;
// 	queuetab[prev].qnext = pid;
// 	queuetab[curr].qprev = pid;
// 	return OK;
// }
