#include	<stdio.h>
#include	<pthread.h>

static int global_value = 0;

__thread int thread_value = 0;

void *
test_tls_func(void *arg)
{
	printf("I am %d, tls: %p\n", pthread_self(), &thread_value);
	global_value++;
	thread_value++;
	return NULL;
}

int
main(int arg, char **argv)
{
	int i, tnum = 0;;
	pthread_t tid;
	
	if (argv[1]) {
		tnum = atoi(argv[1]);
	}
	else {
		tnum = 2;
	}

	for (i = 0; i < tnum; i++) {
		pthread_create(&tid, NULL,  test_tls_func, NULL);
	}

	printf("glabal value: %d, thread value: %d\n", global_value, thread_value);
	sleep(1);
	printf("glabal value: %d, thread value: %d\n", global_value, thread_value);
	
	return 0;
}

