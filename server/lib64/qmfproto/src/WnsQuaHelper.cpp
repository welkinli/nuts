/*
 * WnsQuaHelper.cpp
 *
 *  Created on: 2012-11-14
 *      Author: himzheng
 */

#include "WnsQuaHelper.h"

namespace WNS
{

    WnsQuaHelper::WnsQuaHelper()
    {
    }

    WnsQuaHelper::~WnsQuaHelper()
    {
    }

    int WnsQuaHelper::ParseQua(std::string strQua, WNS::WnsQua &stQua)
    {
        if (strQua.empty())
        {
            return qua_parse_err_empty;
        }
        stQua.Str = strQua;
        if (strQua[0] == 'I' && strQua.size() >= 10)//IPHQZ3.3.0/100330&NA/000000&IPH&NA&iPhone&V2
        {
            stQua.Buzi = "QZ";
            stQua.Plat = "IPH";
            stQua.MainVer = strQua.substr(5, 5);
        }
        else if (strQua[0] == 'V')//V1_AND_QZ_3.4.0_1267_RDM_B
        {
            std::vector<std::string> split;
            SplitString(strQua, "_", split);
            if (split.size() < 7)
            {
                return qua_parse_err_elligal;
            }
            stQua.QVer     = split[0];
            stQua.Plat     = split[1];
            stQua.Buzi     = split[2];
            stQua.MainVer  = split[3];
            stQua.BuildVer = split[4];
            stQua.Chan     = split[5];
        } else
        {
            return qua_parse_err_nonrecongnition;
        }
        return 0;
    }

    void WnsQuaHelper::SplitString(std::string &str, std::string pattern, std::vector<std::string> &result)
    {
        std::string::size_type pos;
        str += pattern;
        size_t size = str.size();
        for (size_t i = 0; i < size; i++)
        {
            pos = str.find(pattern, i);
            if (pos < size)
            {
                std::string s = str.substr(i, pos - i);
                std::string n;
                for (size_t j = 0; j < s.length(); j++)
                {
                    if (s[j] != ' ' && s[j] != '\t') n.push_back(s[j]);
                }
                if (n.size() > 0) result.push_back(n);
                i = pos + pattern.size() - 1;
            }
        }
    }

}
