#include <inc/lib.h>
#include <inc/x86.h>

void mythread(void * arg) {

	cprintf("Thread: Hello there! \n");

}

void
umain(int argc, char **argv)
{
	cprintf("==================================\n");

	uint32_t t;
	uint64_t time_a, time_b;

	time_a = read_tsc();
	pthread_create(&t, mythread, NULL);
	time_b = read_tsc();

	pthread_join(t);
	cprintf("Create thread time: %d ms\n", (time_b - time_a)/1);

	cprintf("----------------------------------\n");

	time_a = read_tsc();
	int r = fork();
	int k, o = 0;

	if(r == 0) {
		cprintf("Child process: Hello there! \n");
	} else {
		time_b = read_tsc();
		cprintf("Create process time: %d ms\n", (time_b - time_a)/1);
		for (k = 0; k != 10000000; ++k) o ++;
		cprintf("==================================\n");
	}

}
