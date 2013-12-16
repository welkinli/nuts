/*
 * process.cpp
 *
 *  Created on: 2013-12-15
 */

#include "plugin.h"
/*
Bet 下注
Call 跟注
Fold 弃牌
Check 让牌 / 观让
raise 加注
Re-raise 再加注
All-in 全押 / 全进

这里分客户端命令和服务器命令
 */
static enum{
	register=1,
	further_register=2,
	fast_login=3,
	pwd_login=4,
	imei_pwd_login=5,
	enter_section=6,
	start_game=7,//准备好
	leave_game=8,
	follow=9,

};
extern redisContext *g_redis_context;
int cmd_process(plugin_task *tsp)
{
	uint8_t *data_start = eservice_buf_start(tsp->input_ebp);
	plugin_header_t *plugin_head = tsp->plugin_head;
	switch(plugin_head->Cmd)
	{
	case register: //login

		break;
	case 2://enter
		break;
	case 3:
		break;
	default:
		g_err("cmd error ,cmd:%d,uin:%lu",plugin_head->Cmd,plugin_head->Uin);
		return -1;
	}

}



