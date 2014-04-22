// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;
	uint32_t err = utf->utf_err;
	int r;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).

	// LAB 4: Your code here.
	addr = ROUNDDOWN(addr , PGSIZE);
	if ((err & FEC_WR) == 0)
		panic("pgfault: (err & FEC_WR) == 0\n");

	if ((uvpd[PDX(addr)] & PTE_P) == 0 
		|| (uvpt[(uint32_t)addr / PGSIZE] & PTE_COW) == 0)
		panic("pgfault: page not copy-on-write\n");

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.
	//   No need to explicitly delete the old page's mapping.

	// LAB 4: Your code here.
	r = sys_page_alloc(0, (void *)PFTEMP, PTE_W | PTE_U | PTE_P);
	if (r < 0) 
		panic("pgfault: sys_page_alloc %e\n", r);

	memcpy((void *)PFTEMP, addr, PGSIZE);

	r = sys_page_map(0, PFTEMP, 0, addr, PTE_W | PTE_U | PTE_P);
	if (r < 0) 
		panic("pgfault: sys_page_map %e\n", r);
	//panic("pgfault not implemented");
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, unsigned pn)
{
	int r;
	// LAB 4: Your code here.
	void *va = (void *)(pn * PGSIZE);
	if ((uvpt[pn] & PTE_W) || (uvpt[pn] & PTE_COW)) {
		r = sys_page_map(0, va, envid, va, PTE_P | PTE_U | PTE_COW);
		if (r < 0) panic("duppage: sys_page_map %e\n", r);
		r = sys_page_map(0, va, 0, va, PTE_P | PTE_U | PTE_COW);
		if (r < 0) panic("duppage: sys_page_map %e\n", r);
	} else {
		r = sys_page_map(0, va, envid, va, PTE_P | PTE_U);
		if (r < 0) panic("duppage: sys_page_map %e\n", r);
	}
	//panic("duppage not implemented");
	return 0;
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use uvpd, uvpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
	static int cnt_p = 111111;
	// LAB 4: Your code here.
	set_pgfault_handler(pgfault);
	envid_t envid = sys_exofork();
	if (envid < 0) panic("fork: sys_exofork %e\n", envid);
	else if (envid == 0) {
		thisenv = &envs[ENVX(sys_getenvid())];
	} else {
		extern void _pgfault_upcall(void);
		size_t i;
		for (i = 0; i < UTOP; i += PGSIZE) 
			if ( (i < UXSTACKTOP - PGSIZE || i >= UXSTACKTOP)
				&& (uvpd[PDX(i)] & PTE_P) 
				&& (uvpt[i / PGSIZE] & PTE_P) 
				&& (uvpt[i / PGSIZE] & PTE_U) )
				duppage(envid, i / PGSIZE);

		int r = sys_page_alloc(envid, 
								(void *)(UXSTACKTOP - PGSIZE), 
								PTE_P | PTE_U | PTE_W);
		if (r < 0) panic("fork: sys_page_alloc %e\n", r);

		r = sys_env_set_pgfault_upcall(envid, _pgfault_upcall);
		if (r < 0) panic("fork: sys_env_set_pgfault_upcall %e\n", r);

		r = sys_env_set_status(envid, ENV_RUNNABLE);
		if (r < 0) panic("fork: sys_env_set_status %e\n", r);

		/*
		uint32_t priority = cnt_p --; 
		if (cnt_p < 0) cnt_p = 111111;
		//cprintf("ppPPP: %d\n", priority);
		r = sys_env_set_priority(envid, priority);
		if (r < 0) panic("fork: sys_env_set_priority %e\n", r);
		*/
	}

	return envid;
	//panic("fork not implemented");
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
