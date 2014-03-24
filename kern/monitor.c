// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/x86.h>

#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/kdebug.h>
#include <kern/trap.h>
#include <kern/pmap.h>

#define CMDBUF_SIZE	80	// enough for one VGA text line


struct Command {
	const char *name;
	const char *desc;
	// return -1 to force monitor to exit
	int (*func)(int argc, char** argv, struct Trapframe* tf);
};

static struct Command commands[] = {
	{ "help", "Display this list of commands", mon_help },
	{ "kerninfo", "Display information about the kernel", mon_kerninfo },
	{ "backtrace", "Display information about the stack", mon_backtrace },
	{ "nocolorful", "Go back to black&white world", mon_nocolor},
	{ "comeoncolorful", "Let's go colorful", mon_color},
	{ "showmappings", "Display physical page mappings and permission bits", mon_showmappings},
	{ "changepermission", "Change the permissions of mapping", mon_changepermission},
	{ "dump", "Dump the contents of a range of memory given either a virtual or physical address range", mon_dump},
	{ "seegdt", "GDT", mon_seegdt},
};
#define NCOMMANDS (sizeof(commands)/sizeof(commands[0]))

/***** Implementations of basic kernel monitor commands *****/

int
mon_help(int argc, char **argv, struct Trapframe *tf)
{
	int i;

	for (i = 0; i < NCOMMANDS; i++)
		cprintf("%s - %s\n", commands[i].name, commands[i].desc);
	return 0;
}

int
mon_kerninfo(int argc, char **argv, struct Trapframe *tf)
{
	extern char _start[], entry[], etext[], edata[], end[];

	cprintf("Special kernel symbols:\n");
	cprintf("  _start                  %08x (phys)\n", _start);
	cprintf("  entry  %08x (virt)  %08x (phys)\n", entry, entry - KERNBASE);
	cprintf("  etext  %08x (virt)  %08x (phys)\n", etext, etext - KERNBASE);
	cprintf("  edata  %08x (virt)  %08x (phys)\n", edata, edata - KERNBASE);
	cprintf("  end    %08x (virt)  %08x (phys)\n", end, end - KERNBASE);
	cprintf("Kernel executable memory footprint: %dKB\n",
		ROUNDUP(end - entry, 1024) / 1024);
	return 0;
}

int
mon_backtrace(int argc, char **argv, struct Trapframe *tf)
{
	// Your code here.
	struct Eipdebuginfo info;
	uint32_t* ebp = (uint32_t*) read_ebp();	
	uint32_t eip = ebp[1];
	cprintf("Stack  backtrace:\n");
	int i;
	while (ebp != 0){
		cprintf("  ebp %08x  eip %08x  args", ebp, eip);
		for (i = 0; i < 5; ++ i)
			cprintf(" %08x", ebp[2 + i]);
		cprintf("\n");
		
		debuginfo_eip((uintptr_t)eip, &info);
		cprintf("         %s:%d: %.*s+%u\n", 
			info.eip_file, info.eip_line, 
			info.eip_fn_namelen, info.eip_fn_name, 
			(unsigned int)(eip - info.eip_fn_addr));

		ebp = (uint32_t* )ebp[0];
		eip = ebp[1];
	}
	return 0;
}

int
mon_nocolor(int argc, char **argv, struct Trapframe *tf)
{
	is_nocolor = 1;
	return 0;
}

int
mon_color(int argc, char **argv, struct Trapframe *tf)
{
	is_nocolor = 0;
	return 0;
}

int
mon_showmappings(int argc, char **argv, struct Trapframe *tf)
{
	if(argc != 3) {
		cprintf("Should be:\n");
		cprintf("\tshowmappings [start_address] [end_address]\n");
	}else {
		uintptr_t p_start, p_end, p_i;
		p_start = ROUNDDOWN(strtol(argv[1], NULL, 0), PGSIZE);
		p_end = ROUNDUP(strtol(argv[2], NULL, 0), PGSIZE);

		for(p_i = p_start; p_i <= p_end; p_i += PGSIZE) {
			cprintf("Virtual Address: 0x%08x \n", p_i);
			pte_t *pte = pgdir_walk(kern_pgdir, (void *)p_i, 0);
			
			if(pte != NULL && ((*pte) & PTE_P)) {
				cprintf("\t Physical Address: 0x%08x \n", PTE_ADDR(*pte));
				
				cprintf("\t Permission Bits: ");
				uint32_t pte_u, pte_w;
				pte_u = (*pte) & PTE_U, pte_w = (*pte) & PTE_W;
				cprintf("Kernel ");
				if(pte_w) cprintf("RW");
				else cprintf("R-");
				cprintf("; User ");
				if(!pte_u) cprintf("--");
				else if(pte_w) cprintf("RW");
				else cprintf("R-");
				cprintf(";\n");
			}else {
				cprintf("\t Physical Address: N/A \n");
				cprintf("\t Permission Bits: N/A \n");
			}
		}
	}
	return 0;
}

int
mon_changepermission(int argc, char **argv, struct Trapframe *tf)
{
	if(argc != 3) {
		cprintf("Should be:\n");
		cprintf("\tchangepermission [virtual_address] [U|S][R|W][P|-]\n");
	}else {
		uintptr_t va = strtol(argv[1], NULL, 0);
		cprintf("Virtual Address: 0x%08x \n", va);

		pte_t *pte = pgdir_walk(kern_pgdir, (void *)va, 0);
		if(pte != NULL && ((*pte) & PTE_P)) {
			uint32_t pte_u, pte_w, pte_p;
			pte_u = (*pte) & PTE_U, pte_w = (*pte) & PTE_W;
			cprintf("\t");
			if(pte_u) cprintf("U"); else cprintf("S");
			if(pte_w) cprintf("W"); else cprintf("R");
			
			cprintf("P -> %s\n", argv[2]);
			*pte &= ~PTE_U, *pte &= ~PTE_W, *pte &= ~PTE_P;
			if(argv[2][0] == 'U') *pte |= PTE_U;
			if(argv[2][1] == 'W') *pte |= PTE_W;
			if(argv[2][2] == 'P') *pte |= PTE_P;
		}else {
			cprintf("\t No physical pages mapped at: 0x%08x \n", va);
			return 0;
		}
	}
	return 0;
}

int
mon_dump(int argc, char **argv, struct Trapframe *tf)
{
	if(argc != 4) {
		cprintf("Should be:\n");
		cprintf("\tdump [V|P] [start_address] [end_address]\n");
	}else {
		if(argv[1][0] == 'V') {
			uintptr_t p_start, p_end, p_i;
			p_start = ROUNDDOWN(strtol(argv[2], NULL, 0), 4);
			p_end = ROUNDDOWN(strtol(argv[3], NULL, 0), 4);

			for(p_i = p_start; p_i < p_end; p_i += 4) {
				pte_t *pte = pgdir_walk(kern_pgdir, (void *)p_i, 0);
			
				if(pte != NULL && ((*pte) & PTE_P)) {
					cprintf("\t0x%08x", *((uint32_t* )p_i));
				}else {
					cprintf("\tN/A");
				}
				if((p_i & 0xf) == 0xc) cprintf("\n");
			}
			cprintf("\n");
		}else if(argv[1][0] == 'P') {
			// Under construction
		}else {
			cprintf("Should be:\n");
			cprintf("\tdump [V|P] [start_address] [end_address]\n");
		}
	}
	return 0;
}

int
mon_seegdt(int argc, char **argv, struct Trapframe *tf)
{
	char gdt[6];
	unsigned long gdtaddr;
	unsigned short gdtlimit;
	asm("sgdt %0":"=m"(gdt));
	gdtaddr = *(unsigned long *)(&gdt[2]);
	gdtlimit = *(unsigned short *)(&gdt[0]);
	cprintf("GDT Base: %08x\n", gdtaddr);
	cprintf("GDT Limit: %04x\n", gdtlimit);
	return 0;
}

/***** Kernel monitor command interpreter *****/

#define WHITESPACE "\t\r\n "
#define MAXARGS 16

static int
runcmd(char *buf, struct Trapframe *tf)
{
	int argc;
	char *argv[MAXARGS];
	int i;

	// Parse the command buffer into whitespace-separated arguments
	argc = 0;
	argv[argc] = 0;
	while (1) {
		// gobble whitespace
		while (*buf && strchr(WHITESPACE, *buf))
			*buf++ = 0;
		if (*buf == 0)
			break;

		// save and scan past next arg
		if (argc == MAXARGS-1) {
			cprintf("Too many arguments (max %d)\n", MAXARGS);
			return 0;
		}
		argv[argc++] = buf;
		while (*buf && !strchr(WHITESPACE, *buf))
			buf++;
	}
	argv[argc] = 0;

	// Lookup and invoke the command
	if (argc == 0)
		return 0;
	for (i = 0; i < NCOMMANDS; i++) {
		if (strcmp(argv[0], commands[i].name) == 0)
			return commands[i].func(argc, argv, tf);
	}
	cprintf("Unknown command '%s'\n", argv[0]);
	return 0;
}

void
monitor(struct Trapframe *tf)
{
	char *buf;

	cprintf("Welcome to the JOS kernel monitor!\n");
	cprintf("Type 'help' for a list of commands.\n");

	if (tf != NULL)
		print_trapframe(tf);

	while (1) {
		buf = readline("K> ");
		if (buf != NULL)
			if (runcmd(buf, tf) < 0)
				break;
	}
}
