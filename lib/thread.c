#include <inc/string.h>
#include <inc/spinlock.h>
#include <inc/lib.h>
#include <inc/x86.h>

extern char end[];
static char *nxt_stack = end;
int pthread_init_stack(envid_t child, void *arg, uintptr_t *init_esp);

int
pthread_create(uint32_t * t_id, void (*fun)(void *), void *arg) 
{
	struct Trapframe childp_tf;

	envid_t childp = sys_exothread();
	if (childp < 0) {
		panic("pthread_create: %e\n", childp);
	}

	int r;
	childp_tf = envs[ENVX(childp)].env_tf;
	childp_tf.tf_eip = (uint32_t)fun;
	if ((r = pthread_init_stack(childp, arg, &childp_tf.tf_esp)) < 0)
		return r;

	if ((r = sys_env_set_trapframe(childp, &childp_tf)) < 0) {
		cprintf("pthread_create: %e\n", r);
		return r;
	}
	if ((r = sys_env_set_status(childp, ENV_RUNNABLE)) < 0) {
		cprintf("pthread_create: %e\n", r);
		return r;
	}

	*t_id = childp;

	return 0;
}

int
pthread_init_stack(envid_t child, void *arg, uintptr_t *init_esp)
{
	nxt_stack = ROUNDUP(nxt_stack, PGSIZE);

	int r;
	if ((r = sys_page_alloc(0, nxt_stack, PTE_W | PTE_U | PTE_P)) < 0)
		return r;
	nxt_stack += PGSIZE;

	uint32_t * stack_addr = (uint32_t *)nxt_stack;

	stack_addr[-2] = (uint32_t)exit;
	stack_addr[-1] = (uint32_t)arg;

	*init_esp = (uint32_t) (stack_addr - 2);

	return 0;
}

int 
pthread_join(envid_t id) 
{
	int r;
	while (1) {
		if ((r = sys_join(id)) != 0)
			break;
		sys_yield();
	}
	return r;
}

int
pthread_mutex_init(pthread_mutex_t * mutex, pthread_mutexattr_t * attr)
{
	mutex->locked = 0;
	return 0;
}

int
pthread_mutex_lock(pthread_mutex_t * mutex)
{
	while (xchg(&mutex->locked, 1) == 1) ;
	return 0;
}

int
pthread_mutex_unlock(pthread_mutex_t * mutex)
{
	xchg(&mutex->locked, 0);
	return 0;
}