
struct TSS_
{	//4*(1+7+10+6+2) = 104 bytes
	uint32 backlink;   //4
	uint32 esp0, ss0, esp1, ss1, esp2, ss2, cr3;  //4*7
	uint32 eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi; //4r*10
	uint32 es, cs, ss, ds, fs, gs; //4*6
	uint32 ldtr; //4*2
    uint16 trap, iomap;
};

extern struct TSS_ TSS; 
