/*
@note:
	读取配置文件, #后面的内容为注释,	支持[section]章节
@eg:
	#define CONFIG_NOTIFY_TEST (*CSingleton<CBitMapConfigFile>::instance())("notify", "test").c_str())
@config file:
	[notify] #notify section
	test=123
*/

#ifndef _FILE_CONFIG_H_
#define _FILE_CONFIG_H_

#include <iostream>
#include <fstream>
#include <map>
#include <string>
#include <stdlib.h>
#include <stdio.h>

#define INI_GLOBAL_SECTION      "__global__"

using namespace std;

class CFileConfig
{
public:
	CFileConfig(bool parse_raw_data = false);
	CFileConfig(const char* szFileName, bool parse_raw_data = false);
	virtual ~CFileConfig();

public:
	string& operator[](const char* szName);
	string& operator()(const char* szSection, const char* szName);

	int ParseFile(const char* szConfigFile);
    map<string, map<string, string> > &GetRawData();
    map<string, string> GetMap();
	
private:
	static int StrimString(char* szLine);
	int ParseFile();

private:
	ifstream m_ConfigFile;
	map<string, string> m_ConfigMap;
    map<string, map<string, string> > m_raw_data;
    bool m_parse_raw_data;
};

#endif

