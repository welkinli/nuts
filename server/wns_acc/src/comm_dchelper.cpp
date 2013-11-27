#include <sstream>
#include <sys/time.h>

#include "comm_dchelper.h"
#include "tool.h"
#include "common_albert.h"
#include "albert_log.h"

using namespace comm;

int g_sac_dcagent_flag = 0;
extern int g_log_level;
extern CRollLog *g_albert_logp;

CDcHelper *CDcHelper::m_instance = NULL;

CDcHelper *CDcHelper::Instance()
{
	if (NULL == m_instance)
	{
		m_instance = new CDcHelper();
		m_instance->init();
	}

	return m_instance;
}

CDcHelper::CDcHelper() 
{
}

CDcHelper::~CDcHelper()
{
}

int CDcHelper::init()
{
    std::string upstream(APPNAME_UPSTREAM);
    std::string downstream(APPNAME_DOWNSTREAM);
	std::string sacstrream(APPNAME_SACSTRAM);
	m_upstream_clogger.set_socktype(2); // udp no-block
	m_downstream_clogger.set_socktype(2); 
	m_sac_clogger.set_socktype(2); 

	m_upstream_clogger.init(upstream);
    m_downstream_clogger.init(downstream);
	m_sac_clogger.init(sacstrream);

	return 0;
}

int CDcHelper::save(DataCollector::CLogger &clogger, int logtype, WnsDcObj &obj)
{
    std::string deviceinfo_enc;
	int ret;
    clogger.encode(obj.deviceinfo, deviceinfo_enc);

    struct timeval tm_;
    std::string tm_str;
    gettimeofday(&tm_, NULL);
    unsigned long long tm_stamp = static_cast<unsigned long long>(tm_.tv_sec)*1000 + tm_.tv_usec/1000; //ms 

    TOOL::Time2str(tm_, tm_str);

    char buf[1024] = {0};
	if(g_sac_dcagent_flag)
	{
		//report to safe center
		snprintf(buf, sizeof(buf),
			"version=2&" //版本号
			"time=%llu&"
			"appid=10178&"
			"opuin=%u&"
			"svrip=%u&"
			"userip=%u&"
			"ret=%d&"
			"qua=%s&"
			"login_type=%d&"
			"cmd=%s&"
			"device=2",
			tm_stamp,
			obj.uin,
			obj.serverip,
			obj.userip,
			obj.resultcode,
			obj.qua.c_str(),
			obj.authtype,
			obj.servicecmd.c_str()
			);
		std::string safe_str(buf);
		ret = m_sac_clogger.write_baselog(DataCollector::LT_NORMAL, safe_str, true);
	}
	//report to static


	 snprintf(buf, sizeof(buf),
	             "time=%llu&stime=%s&"
	             "typeid=%d&"
	             "version=0&logappid=7002&module=%s&plat=5&"
	             "userip=%u&"
	             "userip_a=%s&"
	             "locip=%u&"
	             "locip_a=%s&"
	             "svrip=%u&"
	             "svrip_a=%s&"
	             "cgiip=%u&"
	             "cgiip_a=%s&"
	             "qzacmd=%d&qzasubcmd=%d&"
	             "result=%d&resultcode=%d&"
	             "tmcost=%d&"
	             "reqsize=%d&rspsize=%d&"
	             "touin=%u&opuin=%u&"
	             "appid=%d&"
	             "qua=%s&deviceinfo=%s&releaseversion=%s&build=%s&"
	             "auth_type=%d&detail=%s&commandname=%s&seq=%d",
	             tm_stamp, tm_str.c_str(),
	             logtype,
	             MODULE_STR,
	             obj.userip,
	             obj.userip_a.c_str(),
	             obj.hostip,
	             obj.hostip_a.c_str(),  // host ip
	             obj.serverip,
	             obj.serverip_a.c_str(), // srvip
	             obj.cgiip,
	             obj.cgiip_a.c_str(), // acc ip
	             obj.cmd, obj.subcmd,
	             obj.resulttype, obj.resultcode,
	             obj.timecost, // timecost
	             obj.reqsize, obj.rspsize,
	             obj.uin, obj.uin,
	             obj.appid,
	             obj.qua.c_str(),
	             deviceinfo_enc.c_str(),// deviceinfo
	             obj.releaseversion.c_str(),
	             obj.build.c_str(),
	             obj.authtype,
	             "", // empty detail
	             obj.servicecmd.c_str(),
	             obj.seq);

    std::string log_str(buf);
	g_dbg("###str=%s", buf);

    if(obj.clogflag == 1)
    {
        ret = clogger.write_baselog(DataCollector::LT_NORMAL, log_str, true);
    }
    else
    {
        ret = clogger.write_baselog(DataCollector::LT_NORMAL, log_str, false);
    }


    return 0;
}

