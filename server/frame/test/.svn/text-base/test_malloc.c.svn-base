#ifndef G_LOG_DOMAIN
#define G_LOG_DOMAIN    "TEST_MALLOC"
#endif

#ifdef	__cplusplus
extern "C" {
#endif
#include	<glib.h>
#ifdef	__cplusplus
}
#endif

#include	"test_malloc.h"
#include	<malloc.h>

#define	MAX_ALLOC_NUM	500
#define	MAX_ALLOC_LOOP	10000

uint32_t g_log_level = 3;
CRollLog *g_albert_logp = NULL;

void *alloc_array[MAX_ALLOC_LOOP][MAX_ALLOC_NUM];

int
main(int argc, char **argv)
{
	void *ptr1 = NULL;
	int i = 0, j = 0;
	int size = 0;
	struct timeval  start, end;
	uint32_t micro_s = 0;
	int rand_n;
	uint64_t alloc_num = 0, free_num = 0, total_data = 0;
	
	srand(time(NULL));
	
	if (argv[1])
		size = atoi(argv[1]) * 1024;
	else
		size = 80 * 1024;

	mallopt(M_MMAP_MAX, 0); // 禁止malloc调用mmap分配内存
	mallopt(M_TRIM_THRESHOLD, -1); // 禁止内存紧缩

	gettimeofday(&start, NULL);
	for (j = 0; j < MAX_ALLOC_LOOP; j++) {
		rand_n = rand() % MAX_ALLOC_NUM;
		
		for (i = rand_n; i < MAX_ALLOC_NUM; i++) {
			if (size < 128 * 1024) {
				alloc_array[j][i] = malloc(size + rand_n);
				total_data += (size + rand_n);
			}
			else {
				alloc_array[j][i] = malloc(size + (2048 * rand_n));
				total_data += (size + (2048 * rand_n));
			}
			if (!(alloc_array[j][i])) {
				g_warn("malloc() failed: %s", strerror(errno));
				exit(1);
			}

			alloc_num++;
		}

		for (i = 0; i < rand_n; i++) {
			if (size < 128 * 1024) {
				alloc_array[j][i] = malloc(size + rand_n);
				total_data += (size + rand_n);
			}
			else {
				alloc_array[j][i] = malloc(size + (2048 * rand_n));
				total_data += (size + (2048 * rand_n));
			}
			if (!(alloc_array[j][i])) {
				g_warn("malloc() failed: %s", strerror(errno));
				exit(1);
			}

			alloc_num++;
		}

		for (i = 0; i < MAX_ALLOC_NUM; i++) {
			if (alloc_array[j][i]) {
				free(alloc_array[j][i]);
				alloc_array[j][i] = NULL;

				free_num++;
			}
		}
	}
	gettimeofday(&end, NULL);

	micro_s = time_duration(&end, &start);
	g_dbg("duration micro: %u, alloc num: %qu, free num: %qu, total data: %quM", micro_s, alloc_num, free_num, total_data / 1024 / 1024);

	return 0;
}

