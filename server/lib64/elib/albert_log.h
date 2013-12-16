#ifndef __BASE_LOG_H__
#define __BASE_LOG_H__
 
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string>
#include <map>
#include <fstream>
#include <stdexcept>

#ifndef	G_LOG_DOMAIN
#define	G_LOG_DOMAIN	"DOMAIN"
#endif

using namespace std;

class CRollLog; 
typedef CRollLog& (*__Roll_Func)(CRollLog&);

//#define TheLogger			(*(CRollLog::instance()))
//#define LOG_INIT			TheLogger.init
//#define LOG_SET_LEVEL		TheLogger.set_level
//#define LOG_SET_TITLE		TheLogger.set_titleflag

#define DETAIL(lev, fmt, args...)  g_albert_logp->write_log(lev, fmt, ##args)
#define DEBUG_LOG(fmt, args...)	   DETAIL(CRollLog::DEBUG_PRI, fmt, ##args)
#define NOTI_LOG(fmt, args...)	   DETAIL(CRollLog::NOTICE_PRI, fmt, ##args)
#define ERROR_LOG(fmt, args...)	   DETAIL(CRollLog::ERROR_PRI, fmt, ##args)
#define ABORT_LOG(fmt, args...)	   do {DETAIL(CRollLog::ERROR_PRI, fmt, ##args);abort();} while (0)

//#define GDETAIL(lev, fmt, args...)  do {if (!g_albert_logp) {g_albert_logp = CRollLog::instance();} if (!g_albert_logp) {fprintf(stderr, fmt"\n", ##args);} else {g_albert_logp->write_log_module(G_LOG_DOMAIN, lev, fmt, ##args);}} while (0)
#define GDETAIL(lev, fmt, args...)  do {if (!g_albert_logp) {fprintf(stderr, fmt"\n", ##args);} else {g_albert_logp->write_log_module(G_LOG_DOMAIN, lev, "[%-10s][%-4d][%-10s]"fmt, __FILE__, __LINE__, __FUNCTION__,##args);}} while (0)
#define GDEBUG_LOG(fmt, args...)	   GDETAIL(CRollLog::DEBUG_PRI, fmt, ##args)
#define GNOTI_LOG(fmt, args...)	   GDETAIL(CRollLog::NOTICE_PRI, fmt, ##args)
#define GERROR_LOG(fmt, args...)	   GDETAIL(CRollLog::ERROR_PRI, fmt, ##args)
#define GABORT_LOG(fmt, args...)	   do {GDETAIL(CRollLog::ERROR_PRI, fmt, ##args);abort();} while (0)

const int DEFAULT_FILE_MAX_SIZE = 200*(1<<20); // 200MB
const int DEFAULT_FILE_MAX_NUM = 100;
const int DEFAULT_LINE_MAX_LEN = (1 * 1024 * 1024);

struct log_open_fail: public runtime_error{ log_open_fail(const string& s);};

/**
 * 滚动日志类
 */
class CRollLog
{
public:
	CRollLog();
	/**
	 * no implementation
	 */
	CRollLog(const CRollLog&);
	~CRollLog();

public:
	/**
	 * log级别
	 */
	enum Log_Level{
		NO_LOG = 1,  
		CRIT_LOG = 2,
		ERROR_PRI = 3,  
		NOTICE_PRI = 4, 
		DEBUG_PRI = 5,
		TRACE_PRI = 6 
	};
	/**
	 * 填充字段定义:时间/模块名/pid/debug信息提示
	 */
	enum title_flag {
		F_Time = 0,
		F_Module = 1,
		F_PID = 2,
		F_DEBUGTIP = 3
	};

	//static std::map<int, CRollLog*> _instance_map;
	//static CRollLog* instance();
	//static CRollLog* instance_and_new();
public:
	/**
	 * 滚动日志初始化
	 * @throw log_open_fail when file cann't been opened
	 * @param sPreFix 滚动日志的前缀,会自动补上.log
	 * @param module 模块名,调用者模块名,可以记入log文件中
	 * @param maxsize 一个日志文件的最大值
	 * @param maxnum 滚动日志的数量,旧的文件自动更新为sPreFix(i).log
	 */
	void init(const string& sPreFix, const string& module, 
			int micro_acc = 0,
			size_t maxsize=DEFAULT_FILE_MAX_SIZE,size_t maxnum=DEFAULT_FILE_MAX_NUM)
		throw(log_open_fail);

	/**
	 * 设置时间格式
	 * @param format 时间格式,参考linux下date命令
	 */
	void time_format(const string& format="[%Y-%m-%d %H:%M:%S]") {_time_format = format;}

	/**
	 * 设置日志级别
	 * @param Log_Level 日志级别
	 */
	void set_level(int l){_setlevel = (Log_Level)l;}

	/**
	 * 设置行首自动填充的字段
	 * @param title_flag title字段,set之后该字段会自动添加
	 * @see #clear_titleflag
	 */
	void set_titleflag(int f) { if (f>0 && f < 4) { _flags[f] = true;} }
	/**
	 * 清除行首自动填充的字段
	 * @param title_flag title字段,clear之后该字段不会自动添加
	 * @see #set_titleflag
	 */
	void clear_titleflag(title_flag f) { if (f>0 && f<4) {_flags[f] = false;} }
	
	void write_log(int level, const char* fmt, ...);
	void write_log_module(const char *log_domain, int level, const char* fmt, ...);

protected:
	bool check_level(int level); 
	string cur_time();

public:
	static const unsigned FLUSH_COUNT = 1024;

	int _logfd;
	Log_Level _setlevel;
	unsigned _buf_count;
	unsigned _max_log_size;
	unsigned _max_log_num;

	string _module;
	string _filename;
	string _time_format;
	unsigned _pid;

	bool _flags[4];
	char* _log_buffer;
	int _micro_acc;
};

#endif //


