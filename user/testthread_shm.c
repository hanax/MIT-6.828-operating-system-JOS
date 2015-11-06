#include <inc/lib.h>

void mythread(void * arg) {
	
	int k, o = 0;
	for (k = 0; k != 1000000; ++k) o ++;
	//cprintf("Thread %d is running \n", *(int *)arg); // Wrong
	cprintf("Thread %d is running \n", (int)arg); // Correct

}

void
umain(int argc, char **argv)
{
	cprintf("==================================\n");

	int i;	
	uint32_t t[5];

	for (i = 0; i != 5; ++i)
		//pthread_create(&t[i], mythread, &i); // Wrong
		pthread_create(&t[i], mythread, (void *)i); // Correct

	for (i = 0; i != 5; ++i)
		pthread_join(t[i]);

	cprintf("==================================\n");
}
