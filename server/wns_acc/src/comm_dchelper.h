#ifndef _COMM_DCHELPER__
#define _COMM_DCHELPER__

#include <string>
#include <vector>
#include "dcapi_cpp.h"
#include "tool.h"
#include "sac_agent.h"
namespace SAC_QUERTY
{
	class WnsSacObj
	{
	public:
		WnsSacObj() : appid(0),userip(0), hostip(0), serverip(0),uin(0){}
		int appid;
		unsigned int userip;	
		unsigned int hostip;
		unsigned int serverip;

		std::string userip_a;
		std::string hostip_a;
		std::string serverip_a;
		std::string qua;
		std::string deviceinfo;
		std::string releaseversion;
		std::string servicecmd;
		std::string build;

		unsigned int uin;

		void setUserIp(unsigned int v)
		{
			userip = ntohl(v);
			userip_a = TOOL::Int2Str(v);
		}
		void setServerIp(unsigned int v) 
		{ 
			serverip = v; 
			serverip_a = TOOL::Int2Str(htonl(v));
		}
		void setHostIp(unsigned int v) 
		{
			hostip = v; 
			hostip_a = TOOL::Int2Str(htonl(v));
		}

		void setQua(std::string &v) 
		{ 
			// new: V1_IPH_SQ_X.X.X_XXX_市场_渠道
			// 3.3版本无线规则: IPHQZ3.3.0.4 / ADQZ_B5_24
			//
			// releaseversion = IPH_SQ_3.4.0
			// build = 1000
			if (v.length() > 2 && v[0] == 'V' && v[1] == '1')
			{
				// V1_IPH_SQ_3.4.0_1000_RDM_
				// : 0   1  2     3    4
				const char *token[5];
				const char *pch;
				token[0] = pch = strchr(v.c_str(), '_');

				int count = 1;
				while (count < 5 && pch != NULL)
				{
					token[count] = pch = strchr(pch + 1, '_');
					++count;
				}

				if (pch != NULL)
				{
					releaseversion.assign(token[0] + 1, token[3] - token[0] - 1);
					if (pch[1] == 'R' && pch[2] == 'D' && pch[3] == 'M')
					{
						releaseversion.append("_RDM");
					}

					build.assign(token[3] + 1, token[4] - token[3] - 1);
				}
				else
				{
					releaseversion = "unknown";
				}
			}
			else
			{
				releaseversion = v.substr(0, v.find('/'));
			}

			qua = v;
		}
	};

	

	static int sac_query(const WnsSacObj sacObj)
	{
		ST_BeatInfo beat_info;
		ST_QueryCond query_cond;
		char error_msg_buf[256];

		query_cond.uiQueryDimensionId = DIMENSION_ALL;
		//appid uin userip
		query_cond.uiAppid = sacObj.appid;
		query_cond.uiOpUin = sacObj.uin;
		query_cond.uiIpAddr = sacObj.userip;
		memset(query_cond.szQua, 0 , sizeof(query_cond.szQua));
		memset(query_cond.szCmd, 0 , sizeof(query_cond.szCmd));

		for(unsigned int i = 0; i < sacObj.qua.length() && i< sizeof(query_cond.szQua) ;++i){
			query_cond.szQua[i] = sacObj.qua[i];
		}
 		for(unsigned int i = 0; i < sacObj.servicecmd.length() && i< sizeof(query_cond.szCmd) ;++i){
 			query_cond.szCmd[i] = sacObj.servicecmd[i];
 		}
 		int ret = sac_query_beatinfo(&beat_info, error_msg_buf, sizeof(error_msg_buf), &query_cond);
		if(beat_info.uiCtrlType == CTRLTYPE_LIMIT_TIME && ret > 0){
			return CTRLTYPE_LIMIT_TIME;
		}
		return 0;
	}
}



namespace comm
{

class WnsDcObj
{
 public:
  WnsDcObj() :
      proversion(0), 
      protocol(0),
      userip(0), hostip(0), serverip(0), cgiip(0),
      appid(0),
      cmd(0), subcmd(0),
      resulttype(0), resultcode(0),
      timecost(0), reqsize(0), rspsize(0),
      uin(0),seq(0), authtype(0) ,clogflag(0) {}

  void setProVersion(int v) { proversion = v; }
  void setProtocol(int v) { protocol = v; }
  void setAppid(int v) { appid = v; }
  void setSeq(unsigned int v) { seq = v; }
  
  void setCmd(int _cmd, int _subcmd) 
  { 
      cmd = _cmd; 
      subcmd = _subcmd; 
  }

  void setResult(int wnscode, int bizcode) 
  { 
      resulttype = (wnscode == 0 || wnscode == 1050 || wnscode == 1051) ? 0 : 1; 
	  
      resultcode = wnscode;
  }

  void setTimecost(int v) { timecost = v; }
  void setReqsize(int v) { reqsize = v; }
  void setRspsize(int v) { rspsize = v; }
  void setErrmsg(std::string &v) { errmsg = v; }
  void setUin(unsigned int v) { uin = v; }

  void setDevice(std::string &info)
  {
      deviceinfo = info;
  }

  void setServiceCmd(std::string &cmd)
  {
      servicecmd = cmd;
  }

  void setQua(std::string &v) 
  { 
      // new: V1_IPH_SQ_X.X.X_XXX_市场_渠道
      // 3.3版本无线规则: IPHQZ3.3.0.4 / ADQZ_B5_24
      //
      // releaseversion = IPH_SQ_3.4.0
      // build = 1000
      if (v.length() > 2 && v[0] == 'V' && v[1] == '1')
      {
          // V1_IPH_SQ_3.4.0_1000_RDM_
          // : 0   1  2     3    4
          const char *token[5];
          const char *pch;
          token[0] = pch = strchr(v.c_str(), '_');

          int count = 1;
          while (count < 5 && pch != NULL)
          {
              token[count] = pch = strchr(pch + 1, '_');
              ++count;
          }

          if (pch != NULL)
          {
              releaseversion.assign(token[0] + 1, token[3] - token[0] - 1);
              if (pch[1] == 'R' && pch[2] == 'D' && pch[3] == 'M')
              {
                  releaseversion.append("_RDM");
              }

              build.assign(token[3] + 1, token[4] - token[3] - 1);
          }
          else
          {
              releaseversion = "unknown";
          }
      }
      else
      {
          releaseversion = v.substr(0, v.find('/'));
      }

      qua = v;
  }

  void setUserIp(unsigned int v)
  {
      userip = v;
      userip_a = TOOL::Int2Str(htonl(v));
  }
  void setServerIp(unsigned int v) 
  { 
      serverip = v; 
      serverip_a = TOOL::Int2Str(htonl(v));
  }
  void setHostIp(unsigned int v) 
  {
      hostip_a = TOOL::Int2Str(htonl(v));
      hostip = v; 
  }
  void setCgiIp(unsigned int v) 
  { 
      cgiip_a = TOOL::Int2Str(htonl(v));
      cgiip = v; 
  }

  /*
  void setServerIp(std::string v) 
  { 
      serverip_a = v; 
      serverip = TOOL::Str2Int((char *)v.c_str());
  }
  void setHostIp(std::string v) 
  {
      hostip = TOOL::Str2Int((char *)v.c_str());
      hostip_a = v; 
  }
  void setCgiIp(std::string v) 
  { 
      cgiip = TOOL::Str2Int((char *)v.c_str());
      cgiip_a = v; 
  }
  */
  void setAuthInfo(int type, std::string key) 
  { 
      authtype = type;
      authkey = key;
  }

  void SetClogFlag()
  {
      clogflag = 1;
  }
  int proversion;
  int protocol;

  unsigned int userip;
  std::string userip_a;
  unsigned int hostip;
  std::string hostip_a;
  unsigned int serverip;
  std::string serverip_a;
  unsigned int cgiip;
  std::string cgiip_a;

  int appid;
  int cmd;
  int subcmd;

  int resulttype;
  int resultcode;

  int timecost;
  int reqsize;
  int rspsize;
  
  std::string errmsg;

  unsigned int uin;
  unsigned int seq;
  std::string deviceid;
  std::string qua;
  std::string deviceinfo;
  std::string releaseversion;
  std::string servicecmd;
  std::string build;

  int authtype;
  std::string authkey;
  int clogflag;
};

class CDcHelper
{
    enum
    {
        type_downstream = 1000116,
        type_upstream = 1000097,
    } TypeId;

#define MODULE_STR "qmf_access"
#define APPNAME_UPSTREAM "cdp1000097"
#define APPNAME_DOWNSTREAM "cdp1000116"
#define APPNAME_SACSTRAM "devicefreqctrl"

    public:
     CDcHelper();
     ~CDcHelper();

     int init();
     static CDcHelper *Instance();

     int saveUpstream(WnsDcObj &obj)
     {
         return save(m_upstream_clogger, type_upstream, obj);
     }

     int saveDownstream(WnsDcObj &obj)
     {
         return save(m_downstream_clogger, type_downstream, obj);
     }

    private:
     DataCollector::CLogger m_upstream_clogger;
     DataCollector::CLogger m_downstream_clogger;
	 DataCollector::CLogger m_sac_clogger;
     static CDcHelper *m_instance;

     int save(DataCollector::CLogger &, int, WnsDcObj &);
};

}
#endif

