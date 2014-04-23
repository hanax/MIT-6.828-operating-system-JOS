// program to cause a breakpoint trap

#include <inc/lib.h>

void
umain(int argc, char **argv)
{
	asm volatile("int $3");

	cprintf("Hey jude\n");
	cprintf("Don't make it bad\n");
	cprintf("Take a sad song and make it better\n");
}

