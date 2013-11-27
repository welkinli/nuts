#ifndef G_LOG_DOMAIN
#define G_LOG_DOMAIN    "TEST_BUDDY"
#endif

#ifdef	__cplusplus
extern "C" {
#endif
#include	<glib.h>
#ifdef	__cplusplus
}
#endif

#include	"test_buddy.h"

#define	MAX_ALLOC_NUM	500
#define	MAX_ALLOC_LOOP	10000

dmm_element_block_t alloc_array[MAX_ALLOC_LOOP][MAX_ALLOC_NUM];

uint32_t g_log_level = 3;
CRollLog *g_albert_logp = NULL;

int
main(int argc, char **argv)
{
	int res = -255;
	static dmm_manager_attr_t attr;
	struct timeval  start, end;
	uint32_t micro_s = 0;
	int i = 0, j = 0;
	int rand_n;
	uint32_t size;
	uint64_t alloc_num = 0, free_num = 0, total_data = 0, total_space = 0;
	
	srand(time(NULL));

	if (argv[1])
		size = atoi(argv[1]) * 1024;
	else
		size = 80 * 1024;
	
	/* init buddy */
	memset(&attr, 0, sizeof(attr));
	attr.minbuddysize = 128 * 1024;
	attr.page_size = 4096;
	attr.data_alloc_per_page = 1024;			/* 8M */
	attr.buddy_list_level = 6;

	res = dmm_manager_init("./buddy", &attr, 1);
	if (res < 0) {
		g_warn("dmm_manager_init() failed: %d", res);
		return res;
	}

	gettimeofday(&start, NULL);
	for (j = 0; j < MAX_ALLOC_LOOP; j++) {
		rand_n = rand() % MAX_ALLOC_NUM;
		
		for (i = rand_n; i < MAX_ALLOC_NUM; i++) {
			res = dmm_manager_alloc(&(alloc_array[j][i]), size + (2048 * rand_n));
			if (res < 0) {
				g_warn("dmm_manager_alloc() failed: %d", res);
				exit(1);
			}

			alloc_num++;

			total_data += (size + (2048 * rand_n));
			total_space += alloc_array[j][i].length;
		}

		for (i = 0; i < rand_n; i++) {
			res = dmm_manager_alloc(&(alloc_array[j][i]), size + (2048 * rand_n));
			if (res < 0) {
				g_warn("dmm_manager_alloc() failed: %d", res);
				exit(1);
			}

			alloc_num++;

			total_data += (size + (2048 * rand_n));
			total_space += alloc_array[j][i].length;
		}

		for (i = 0; i < MAX_ALLOC_NUM; i++) {
			dmm_manager_free(&(alloc_array[j][i]));

			free_num++;
		}
	}
	gettimeofday(&end, NULL);
	
	micro_s = time_duration(&end, &start);	
	g_dbg("duration micro: %u, alloc num: %qu, free num: %qu, total data: %quM, total space: %quM", micro_s, alloc_num, free_num, total_data / 1024 / 1024, total_space / 1024 / 1024);

	return 0;
}


