/*
 * WnsQuaHelper.h
 *
 *  Created on: 2012-11-14
 *      Author: himzheng
 */

#ifndef WNSQUAHELPER_H_
#define WNSQUAHELPER_H_
#include "wns_singleton.h"
#include <string>
#include <vector>

namespace WNS
{
    struct WnsQua
    {
            std::string QVer;//Qua版本号
            std::string Plat;//平台标识 IPH/AND/IPD/APD/WIN
            std::string Buzi;//业务标识 QZ/PY
            std::string MainVer;//业务主版本号
            std::string BuildVer;//业务编译版本号
            std::string Chan;//渠道号

            std::string Str;//原始字符串
            ///是否苹果操作系统
            bool isIOS() {
                return isIphone() || isIpad();
            }
            bool isANDOS()
            {
                return isAndroid() || isApad();
            }
            bool isIphone() {
                return Plat == "IPH";
            }
            bool isIpad() {
                return Plat == "IPD";
            }
            bool isAndroid() {
                return Plat == "AND";
            }
            bool isApad() {
                return Plat == "APD";
            }
            bool isWin(){
                return Plat == "WIN";
            }

            bool isQzone() {
                return Buzi == "QZ";
            }
            bool isPengyou() {
                return Buzi == "PY";
            }
            bool isQuaRight() {
                return Plat.size() > 0 && MainVer.size() > 0;
            }
        };

    enum QUA_HELPER_ERRCODE
    {
        qua_parse_err_empty = -99,
        qua_parse_err_elligal = -98,
        qua_parse_err_nonrecongnition = -97,
        qua_check_err_appid_invalid = -96,
    };

    class WnsQuaHelper : public WNS::CSingleton<WnsQuaHelper>
    {
        public:
            WnsQuaHelper();
            virtual ~WnsQuaHelper();
            /**
             * 解析QUA
             * @param strQua
             * @param stQua
             * @return
             */
            int ParseQua(std::string strQua, WNS::WnsQua &stQua);

        protected:
            void SplitString(std::string &str, std::string pattern, std::vector<std::string> &result);

    };

}

#endif /* WNSQUAHELPER_H_ */
