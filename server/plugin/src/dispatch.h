/*
 * dispatch.h
 *
 *  Created on: 2013-12-8
 *      Author: welkinli
 */

#ifndef DISPATCH_H_
#define DISPATCH_H_
#include <arpa/inet.h>

enum{
	REDIS_SVR=1,
	NORMAL_SVR=2,
};

typedef struct{
	char ip[16];
	unsigned int port;
	char type;
	void init(){
		memset(ip,0,sizeof(ip));
		port =0;
	}
	void setip(const char *s)
	{
		strncpy(ip,s,sizeof(ip));
	}
	boolean is_redis_svr()
	{
		return (type==REDIS_SVR);
	}
}dispatch_svr_t;

typedef	union {
	struct _ip_port {
		uint32_t ip;
		uint32_t port;
	} ip_port;

	uint64_t key64;
} dispatch_svr_key_t;


int plugin_connect_to_dispatch_and_send_data(eservice_unit_t *eup,
		wns_acc_task_struct *tsp, struct eservice_buf_t *send_ebp);

#endif /* DISPATCH_H_ */
