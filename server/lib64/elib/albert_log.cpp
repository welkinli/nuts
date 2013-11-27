
#include "albert_log.h"
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <time.h>
#include <sys/mman.h>
#include <iostream>
#include <fstream>
#include <sys/time.h>


log_open_fail::log_open_fail(const string & s):runtime_error(s) {}

//CRollLog* CRollLog::_instance = NULL;
//static CRollLog*ins = NULL;

//std::map<int, CRollLog*> CRollLog::_instance_map;

static const char *LEVEL_NAME[] = {"", "", "CRIT", "ERROR", "NOTICE", "DEBUG", "TRACE"};
static int FILE_OPEN_FLAGS = O_CREAT|O_APPEND|O_LARGEFILE;
static mode_t FILE_OPEN_MODE = S_IRWXU | S_IRGRP | S_IROTH;

CRollLog::CRollLog()
{
	_setlevel = DEBUG_PRI;
	_buf_count = 0;
	_pid = getpid();

	_flags[0] = true;
	_flags[1] = false;
	_flags[2] = false;
	_flags[3] = true;

	_logfd = -1;
	_log_buffer = (char*)MAP_FAILED;
	time_format();
}

CRollLog::~CRollLog()
{
	close(_logfd);
}

#if 0
CRollLog* CRollLog::instance()
{
#if 0
	int thread_id = pthread_self();
	std::map<int, CRollLog*>::iterator itr = _instance_map.find(thread_id);
	if ( itr != _instance_map.end() ) {
		return itr->second;
	}
#else
	return ins;
#endif

}

CRollLog* CRollLog::instance_and_new()
{
#if 0
	int thread_id = pthread_self();
	std::map<int, CRollLog*>::iterator itr = _instance_map.find(thread_id);
	if ( itr != _instance_map.end() ) {
		return itr->second;
	}
	else {
		CRollLog* instance = new CRollLog();
		_instance_map.insert(make_pair(thread_id, instance));
		return instance;
	}
#else
	ins = new CRollLog();
	return ins;

#endif

}
#endif

void CRollLog::init(const string & sPreFix, const string& module, int micro_acc, size_t maxsize, size_t maxnum)
	throw(log_open_fail)
{
	_module = module;
	_filename = sPreFix;
	_max_log_size = maxsize;
	_max_log_num = maxnum;
	_buf_count = 0;
	_pid = getpid();
	
	_flags[F_Time] = true;
	_flags[F_Module] = false;
	_flags[F_PID] = false;
	_flags[F_DEBUGTIP] = true;

	_micro_acc = micro_acc;
	
//	_lock = false;
	// prealloc inner buffer :
	if (_log_buffer == MAP_FAILED) {
		_log_buffer = (char*)mmap (0, DEFAULT_LINE_MAX_LEN, PROT_WRITE | PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		if (_log_buffer == MAP_FAILED) {
			throw log_open_fail(string("mmap log buffer fail in") + _filename);
		}
	}
	// open file:
	_logfd = open((_filename+".log").c_str(), O_WRONLY|O_CREAT|O_APPEND, 0644 );
	if ( _logfd < 0 )
		throw log_open_fail(string("can not open file:")+_filename+".log");
}


inline string CRollLog::cur_time()
{
	time_t tNow = time(0);
	struct tm curr;
	localtime_r(&tNow, &curr);
	char sTmp[1024];
	strftime(sTmp,sizeof(sTmp),_time_format.c_str(),&curr);
	return string(sTmp);
}

void CRollLog::write_log(int level, const char* fmt, ...)
{
	if ( not check_level(level) )
		return;

	// gen prefix :
	int offset(0);
	if(_flags[F_Time]){
		if (_micro_acc != 0) {
			struct timeval tv;
			struct tm curr;
			
			gettimeofday(&tv, NULL);
			localtime_r(&(tv.tv_sec), &curr);
			offset += strftime(_log_buffer, 256,  _time_format.c_str(), &curr);
			offset += snprintf(_log_buffer + offset, DEFAULT_LINE_MAX_LEN - offset - 1, "[%ld] ", tv.tv_usec);
		}
		else {
			time_t tNow = time(0);
			struct tm curr;
			localtime_r(&tNow, &curr);
			offset += strftime(_log_buffer, 256,  _time_format.c_str(), &curr);
	//		_log_buffer[offset++] = ' ';
		}
	}
	if(_flags[F_Module]) {
		offset += snprintf(_log_buffer+offset, DEFAULT_LINE_MAX_LEN - offset - 1, "[%s]", _module.c_str());
	}
	if(_flags[F_PID]) {
		offset += snprintf(_log_buffer+offset, DEFAULT_LINE_MAX_LEN - offset - 1, "[%u]", _pid);
	}
	if(_flags[F_DEBUGTIP]) {
		offset += snprintf(_log_buffer+offset, DEFAULT_LINE_MAX_LEN - offset - 1, "[%s]", LEVEL_NAME[level]);
	}
	_log_buffer[offset++] = ' ';
	
	va_list ap;
	va_start(ap, fmt);
	offset += vsnprintf(_log_buffer + offset , DEFAULT_LINE_MAX_LEN - 1024, fmt, ap);
	_log_buffer[offset] = '\n';			/* add by albertyang */
	offset++;
	va_end(ap);

	write(_logfd, _log_buffer, offset);
}

void CRollLog::write_log_module(const char *log_domain, int level, const char* fmt, ...)
{
	if ( not check_level(level) )
		return;

	// gen prefix :
	int offset(0);
	if(_flags[F_Time]){
		if (_micro_acc != 0) {
			struct timeval tv;
			struct tm curr;
			
			gettimeofday(&tv, NULL);
			localtime_r(&(tv.tv_sec), &curr);
			offset += strftime(_log_buffer, 256,  _time_format.c_str(), &curr);
			offset += snprintf(_log_buffer + offset, DEFAULT_LINE_MAX_LEN - offset - 1, "[%ld] ", tv.tv_usec);
		}
		else {
			time_t tNow = time(0);
			struct tm curr;
			localtime_r(&tNow, &curr);
			offset += strftime(_log_buffer, 256,  _time_format.c_str(), &curr);
	//		_log_buffer[offset++] = ' ';
		}
	}

	if (log_domain)
		offset += snprintf(_log_buffer+offset, DEFAULT_LINE_MAX_LEN - offset - 1, "[%s]", log_domain);
	
	_log_buffer[offset++] = ' ';
	
	va_list ap;
	va_start(ap, fmt);
	offset += vsnprintf(_log_buffer + offset , DEFAULT_LINE_MAX_LEN - 1024, fmt, ap);
	_log_buffer[offset] = '\n';			/* add by albertyang */
	offset++;
	va_end(ap);

	write(_logfd, _log_buffer, offset);
}

bool CRollLog::check_level(int level)
{
	if ( (level > _setlevel) || _filename.empty() ) {
		fprintf(stderr, "check_level, level %d:%d, filename: %s\n", level, _setlevel, _filename.c_str());
		return false;	
	}

	_buf_count++;
	if(_buf_count % FLUSH_COUNT == 0) {
		_buf_count = 0;

		// close and roll file
		int length = lseek(_logfd, 0, SEEK_END);
		if(length > (int)_max_log_size) {
			// 2005-06-06, 多线程(进程)时会写乱,需要重新open之后判断
			close(_logfd);
			_logfd = open((_filename+".log").c_str(), O_WRONLY|O_CREAT|O_APPEND, 0644 );
			if ( _logfd < 0 ) return false;

			length = lseek(_logfd, 0, SEEK_END);
			if(length > (int)_max_log_size) {

				close(_logfd);  // 关闭当前文件
				// remove the last one
				string from,to;
				char sTmp[32] = {0};
				snprintf(sTmp, sizeof(sTmp) - 1, "%d",_max_log_num-1);
				from = _filename + sTmp +".log";
				if (access(from.c_str(), F_OK) == 0) {
					if (remove(from.c_str()) < 0) {
						fprintf(stderr, "remove from %s fail!!!\n", from.c_str());
						// return _level<=_setlevel;
					}
				}
				// rename the others
				for (int i = _max_log_num-2; i >= 0; i--) {
					snprintf(sTmp, sizeof(sTmp) - 1, "%d",i);
					if (i == 0) from = _filename+".log";
					else from = _filename+sTmp+".log";
					snprintf(sTmp, sizeof(sTmp) - 1, "%d",i+1);
					to = _filename+sTmp+".log";
					if (access(from.c_str(), F_OK) == 0) {
						if (rename(from.c_str(), to.c_str()) < 0) {
							fprintf(stderr, "rename from %s to %s fail!!!\n", from.c_str(), to.c_str());
						}
					}
				}
				_logfd = open((_filename+".log").c_str(), O_WRONLY|O_CREAT|O_APPEND, 0644 );
				if ( _logfd < 0 ) return false;
			}
		}
		////////////////
	}	
	return true;
}

///////////////////////////////////////////////////////////////////////////////


