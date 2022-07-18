#include <xinu.h>

void kbdhandler(void) 
{
	int ch;
	
	/* complete (or modify) the code s.t. it works */
	while ((ch = kbdgetc((void*)NULL)) >= 0) 
	{
		switch(ch)
		{
		case -1:
			break;
		case 0:
			break;
		default:
			vgaputc((void*)NULL, ch);
			break;
		}
	}

}
