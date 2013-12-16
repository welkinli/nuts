/*
 *  comm.h
 *
 *  Created on: 2013-12-8
 *      Author: welkinli
 */

#ifndef COMM_H_
#define COMM_H_

static void HexToStr(const char* lpszInStr, int nInLen, char* lpszHex)
{
	int i, j = 0;
	char l_byTmp;
	for (i = 0; i < nInLen; i++)
	{
		l_byTmp = (lpszInStr[i] & 0xf0) >> 4;
		if (l_byTmp < 10)
			lpszHex[j++] = l_byTmp + '0';
		else
			lpszHex[j++] = l_byTmp - 10 + 'A';
		l_byTmp = lpszInStr[i] & 0x0f;
		if (l_byTmp < 10)
			lpszHex[j++] = l_byTmp + '0';
		else
			lpszHex[j++] = l_byTmp - 10 + 'A';
	}
	lpszHex[j] = 0;

	return;
}




#endif /* COMM_H_ */
