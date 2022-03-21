/*  main.c  - main */

#include <xinu.h>


void sndA(void){
	while(1) {
		putc(CONSOLE, 'A');
	}
}
void sndB(void){
	while(1) {
		putc(CONSOLE, 'B');
	}
}

//经过测试 main函数优先级为20

process main(void){
	resume(create(sndA, 1024, 20, "pr1", 0)); //优先级降序
	resume(create(sndB, 1024, 20, "pr2", 0));
	// printf("main function complete...\n");
	return OK;
}

// process	main(void)
// {

// 	/* Run the Xinu shell */

// 	recvclr();          //删除所有等待被接收的消息，但不阻塞进程
// 	resume(create(shell, 8192, 50, "shell", 1, CONSOLE));
	

// 	/* Wait for shell to exit and recreate it */

// 	while (TRUE) {
// 		receive();   //调用receive的进程将被阻塞，知道某个信息到达
// 		sleepms(200);
// 		kprintf("\n\nMain process recreating shell\n\n");
// 		resume(create(shell, 4096, 20, "shell", 1, CONSOLE));
// 	}
// 	return OK;
    
// }
