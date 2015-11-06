#include <inc/lib.h>

pthread_mutex_t l[5], mutex;
char name[][10] = {"XYQ", "PROF. CXQ", "TA. YPF", "TA. LHR", "TA. WCK"};

void mythread(void * arg) {
	int k, o = 0;
	int id = ((int)arg);
	int id2 = (id + 1) % 5;
	char *_name = name[id];
	// Here's a solution to deadlock
	//if (id > id2) { k = id; id = id2; id2 = k; }

	pthread_mutex_lock(&l[id]);

	// If yield() or loop a long time here, the program must deadlock because we use round-robin scheduler
	//sys_yield();
	for (k = 0; k != 10000000; ++k) o ++;

	pthread_mutex_lock(&mutex);
	cprintf("%s pick up left fork! \n", _name);
	pthread_mutex_unlock(&mutex);

	pthread_mutex_lock(&l[id2]);

	pthread_mutex_lock(&mutex);
	cprintf("%s pick up right fork! \n", _name);
	pthread_mutex_unlock(&mutex);

	pthread_mutex_lock(&mutex);
	cprintf("%s start eating! Yumyum \n", _name);
	pthread_mutex_unlock(&mutex);
	
	pthread_mutex_unlock(&l[id]);

	pthread_mutex_lock(&mutex);
	cprintf("%s put down left fork! \n", _name);
	pthread_mutex_unlock(&mutex);

	pthread_mutex_unlock(&l[id2]);

	pthread_mutex_lock(&mutex);
	cprintf("%s put down right fork! \n", _name);
	pthread_mutex_unlock(&mutex);

}

void
umain(int argc, char **argv)
{
	cprintf("==================================\n");

	int i;
	for (i = 0; i != 5; ++i)
		pthread_mutex_init(&l[i], NULL);
	pthread_mutex_init(&mutex, NULL);
	
	uint32_t t[5];
	for (i = 0; i != 5; ++i)
		pthread_create(&t[i], mythread, (void *)i);

	for (i = 0; i != 5; ++i)
		pthread_join(t[i]);

	cprintf("----------------------------------\n");
	cprintf("Everyone has finished eating! hahahahaha\n");
	cprintf("==================================\n");
}
