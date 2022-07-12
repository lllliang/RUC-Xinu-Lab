#include <xinu.h>

extern void _fdoprnt(char *, va_list, int (*)(did32, char), int);

// Syscall list to dispatch in kernel space

const void *syscalls[] = {
	NULL,
	&create,		// 1
	&resume,		// 2
	&recvclr,		// 3
	&receive,		// 4
	&sleepms,		// 5
	&sleep,			// 6
	&fprintf,		// 7
	&printf,		// 8
	&fscanf,		// 9
	&read,			// 10
	&open,			// 11
	&control,		// 12
	&kill,			// 13
	&getpid,		// 14
	&ltss, 			// 15
	NULL,
};

uint32 do_syscall(const uint32 id, const uint32 args_count, ...){
	__asm("pushl %ebx");
	__asm("movl 0x8(%ebp), %eax"); 	// syscall 	id
	__asm("movl 0xc(%ebp), %ebx");	// args_count
	__asm("movl	%ebp, %ecx");	
	__asm("addl $0x10, %ecx"); // 传递参数地址
	__asm("int $40");
	__asm("popl %ebx");
}

uint32 syscall_handler(const uint32 id, const uint32 args_count, const uint32* args){
	__asm("movl 0x10(%ebp), %ecx");		// 获取args基地址
	for(int32 i = args_count - 1; i >= 0; i--){
		// 需要进行逆向的参数压栈，符合传参规则
		asm(
			"movl -0x4(%ebp), %ebx\n\t"    	// 获取i
			"movl %ecx, %edx\n\t"	// &args
			"movl $0x4, %eax\n\t"	// 4
			"imul %eax, %ebx\n\t"			// eax = index*4
			"addl %ebx, %edx\n\t"	// &args[i]
			"pushl (%edx)\n\t"		// 将第i个参数压入栈中
		);
	}
	// call function here

	__asm("nop");

	if(id == 1) __asm("call create");
	else if(id == 2) __asm("call resume");
	else if(id == 3) __asm("call recvclr");
	else if(id == 4) __asm("call receive");
	else if(id == 5) __asm("call sleepms");
	else if(id == 6) __asm("call sleep");
	else if(id == 7) __asm("call fprintf");
	else if(id == 8) __asm("call printf");
	else if(id == 9) __asm("call fscanf");
	else if(id == 10) __asm("call read");
	else if(id == 11) __asm("call open");
	else if(id == 12) __asm("call control");
	else if(id == 13) __asm("call kill");
	else if(id == 14) __asm("call getpid");
	else if(id == 15) __asm("call ltss");
	else if(id == 16) __asm("call addargs");

	
}




// uint32 do_syscall_(const uint32 id,const uint32 args_count, ...) {

// 	// paramList: %ecx %edx %ebx %esi %edi
// 	// ps: callee-saved %ebx, %esi, %edi
// 	switch(id){
// 		case 1: // create
// 			__asm("pushl	%ebx");
// 			__asm("pushl	%esi");
// 			__asm("pushl	%edi");

// 			__asm("movl 0x8(%ebp), 	%eax"); // syscall 	id
// 			__asm("movl 0x10(%ebp),	%ecx"); // void* 	funcaddr
// 			__asm("movl 0x14(%ebp), %edx"); // uint32	ssize
// 			__asm("movl 0x18(%ebp), %ebx"); // pri16	priority
// 			__asm("movl 0x1c(%ebp), %esi"); // char*	name
// 			__asm("movl 0x20(%ebp), %edi"); // uint32	nargs

// 			__asm("int $40");

// 			__asm("popl		%ebx");
// 			__asm("popl		%esi");
// 			__asm("popl		%edi");
// 			break;
// 		case 2: // resume
// 			__asm("movl 0x8(%ebp), 	%eax");
// 			__asm("movl 0x10(%ebp), %ecx"); // pid32	pid

// 			__asm("int $40");
// 			break;
// 		case 3: // recvclr
// 			__asm("movl 0x8(%ebp),	%eax");

// 			__asm("int $40");
// 			break;
// 		case 4: // receive
// 			__asm("movl 0x8(%ebp),	%eax");

// 			__asm("int $40");
// 			break;
// 		case 5: // sleepms
// 			__asm("movl 0x8(%ebp), 	%eax");
// 			__asm("movl 0x10(%ebp), %ecx"); // int32	delay

// 			__asm("int $40");
// 			break;
// 		case 6: // sleep
// 			__asm("movl 0x8(%ebp), 	%eax");
// 			__asm("movl 0x10(%ebp), %ecx"); // int32	delay

// 			__asm("int $40");
// 			break;
// 		case 7: // fprintf
// 			__asm("pushl	%ebx");

// 			__asm("movl 0x8(%ebp), 	%eax"); // syscall 	id
// 			__asm("movl 0x10(%ebp),	%ecx"); // int		dev
// 			__asm("movl 0x14(%ebp), %edx"); // char*	fmt
// 			__asm("movl 0x18(%ebp), %ebx"); // void*	args_ptr

// 			__asm("int $40");

// 			__asm("popl		%ebx");
// 			break;
// 		case 8: // printf
// 			__asm("movl 0x8(%ebp), 	%eax"); // syscall 	id
// 			__asm("movl 0x10(%ebp),	%ecx"); // char*	fmt
// 			__asm("movl 0x14(%ebp), %edx"); // void*	args_ptr

// 			__asm("int $40");
// 			break;
// 		case 9: // fscanf
// 			__asm("pushl	%ebx");

// 			__asm("movl 0x8(%ebp), 	%eax"); // syscall 	id
// 			__asm("movl 0x10(%ebp),	%ecx"); // int		dev
// 			__asm("movl 0x14(%ebp), %edx"); // char*	fmt
// 			__asm("movl 0x18(%ebp), %ebx"); // int		args

// 			__asm("int $40");

// 			__asm("popl		%ebx");
// 			break;
// 		case 10: // read
// 			__asm("pushl	%ebx");

// 			__asm("movl 0x8(%ebp), 	%eax"); // syscall 	id
// 			__asm("movl 0x10(%ebp),	%ecx"); // did32 	descrp
// 			__asm("movl 0x14(%ebp), %edx"); // char*	buffer
// 			__asm("movl 0x18(%ebp), %ebx"); // uint32	count

// 			__asm("int $40");

// 			__asm("popl		%ebx");
// 			break;
// 		case 11: // open
// 			__asm("pushl	%ebx");

// 			__asm("movl 0x8(%ebp), 	%eax"); // syscall 	id
// 			__asm("movl 0x10(%ebp),	%ecx"); // did32 	descrp
// 			__asm("movl 0x14(%ebp), %edx"); // char*	name
// 			__asm("movl 0x18(%ebp), %ebx"); // char*	mode

// 			__asm("int $40");

// 			__asm("popl		%ebx");
// 			break;
// 		case 12: // control
// 			__asm("pushl	%ebx");
// 			__asm("pushl	%esi");

// 			__asm("movl 0x8(%ebp), 	%eax"); // syscall 	id
// 			__asm("movl 0x10(%ebp),	%ecx"); // did32 	descrp
// 			__asm("movl 0x14(%ebp), %edx"); // int32	func
// 			__asm("movl 0x18(%ebp), %ebx"); // int32	arg1
// 			__asm("movl 0x1c(%ebp), %esi"); // int32	arg2

// 			__asm("int $40");

// 			__asm("popl		%ebx");
// 			__asm("popl		%esi");
// 			break;
// 		case 13: // kill
// 			__asm("movl 0x8(%ebp), 	%eax");
// 			__asm("movl 0x10(%ebp), %ecx"); // pid32	pid

// 			__asm("int $40");
// 			break;
// 		case 14: // getpid
// 			__asm("movl 0x8(%ebp),	%eax");

// 			__asm("int $40");
// 			break;
// 		case 15: // ltss
// 			__asm("movl 0x8(%ebp), 	%eax");
// 			__asm("movl 0x10(%ebp), %ecx"); // pid32	pid

// 			__asm("int $40");
// 			break;
// 		case 16: // resched
// 			__asm("movl 0x8(%ebp),	%eax");

// 			__asm("int $40");
// 			break;
// 	}
// 	__asm("addl $0x4, %esp");
// 	__asm("ret");
// }


// uint32 syscall_handler_(const uint32 id, ...){
// 	uint32 return_value;

// 	uint32 *args_array = &id + 1;

// 	// 	/* paser param */
// 	// create
// 	void 	*funcaddr;
// 	uint32	ssize;
// 	pri16	priority;
// 	char	*name;
// 	uint32	nargs;

// 	pid32	pid;
// 	int32	delay;
// 	int 	dev;
// 	char	*fmt;
// 	did32	descrp;
// 	char	*buffer;
// 	uint32	count;

// 	// control
// 	char	*mode;
// 	int32	func;
// 	int32	arg1;
// 	int32	arg2;
// 	// printf
// 	int 	putc(did32, char);
// 	// fscanf
// 	int 	buf;
// 	int 	args;

// 	switch(id){
// 	case 1 : /* create */
// 		funcaddr 	= args_array[0];
// 		ssize		= args_array[1];
// 		priority	= args_array[2];
// 		name 		= args_array[3];
// 		nargs		= args_array[4];

// 		return_value = create(funcaddr, ssize, priority, name, nargs);
// 		break;
	
// 	case 2 : /* resume */
// 		pid		= args_array[0];
// 		return_value = resume(pid);
// 		break;
	
// 	case 3 : /* recvclr */
// 		return_value = recvclr();
// 		break;
	
// 	case 4 : /* receive */
// 		return_value = receive();
// 		break;
	
// 	case 5 : /* sleepms */
// 		delay 	= args_array[0];
// 		return_value = sleepms(delay);
// 		break;
	
// 	case 6 : /* sleep */
// 		delay 	= args_array[0];
// 		return_value = sleep(delay);
// 		break;

// 	case 7 : /* fprintf */
// 		dev		= args_array[0];
// 		fmt		= args_array[1];
// 		_fdoprnt((char*)fmt, args_array[2], putc, dev);
// 		return_value = 0;
// 		break;
	
// 	case 8 : /* printf */
// 		fmt		= args_array[0];
// 		_fdoprnt((char*)fmt, args_array[1], putc, stdout);
// 		return_value = 0;
// 		break;

// 	case 9 : /* fscanf */
// 		dev 	= args_array[0];
// 		fmt		= args_array[1];
// 		args	= args_array[2];
// 		return_value = fscanf(dev, fmt, args);
// 		break;

// 	case 10 : /* read */
// 		descrp	= args_array[0];
// 		buffer	= args_array[1];
// 		count	= args_array[2];
// 		return_value = read(descrp, buffer, count);
// 		break;
	
// 	case 11 : /* open */
// 		descrp	= args_array[0];
// 		name	= args_array[1];
// 		mode	= args_array[2];
// 		return_value = open(descrp, name, mode);
// 		break;

// 	case 12 : /* control */
// 		descrp	= args_array[0];
// 		func	= args_array[1];
// 		arg1	= args_array[2];
// 		arg2	= args_array[3];
// 		return_value = control(descrp, func, arg1, arg2);
// 		break;

// 	case 13 : /* kill */
// 		pid		= args_array[0];
// 		return_value = kill(pid);
// 		break;
		
// 	case 14 : /* getpid */
// 		return_value = getpid();
// 		break;

// 	case 15 : /* ltss */
// 		pid = args_array[0];
// 		ltss(pid);
// 		break;
	
// 	case 16 : /* resched */ 
// 		resched();
// 		break;
// 	}
// 	return return_value;
// }

/*

longest 5 params

&create		=> (* (pid32(*)		(void*, uint32, pri16, char*, unit32, ...)) syscalls[1])
&resume		=> (* (pri16(*)		(pid32)) 									syscalls[2])
&recvclr	=> (* (umsg32(*)	(void)) 									syscalls[3])
&receive	=> (* (umsg32(*)	(void)) 									syscalls[4])
&sleepms	=> (* (syscall(*)	(int32)) 									syscalls[5])
&sleep		=> (* (syscall(*)	(int32)) 									syscalls[6])
&fprintf	=> (* (int(*)		(int, char*, ...)) 							syscalls[7])
&printf		=> (* (int(*)		(const char*, ...)) 						syscalls[8])
&fscanf		=> (* (int(*)		(int, char*, args)) 						syscalls[9])
&read		=> (* (syscall(*)	(did32, char*, uint32)) 					syscalls[10])
&open		=> (* (syscall(*)	(did32, char*, char*)) 						syscalls[11])
&control	=> (* (syscall(*)	(did32, int32, int32, int32)) 				syscalls[12])
&kill		=> (* (syscall(*)	(pid32)) 									syscalls[13])
&getpid		=> (* (pid32(*)		(void)) 									syscalls[14])


*/



