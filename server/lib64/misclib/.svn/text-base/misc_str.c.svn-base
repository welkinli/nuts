#include	<sys/stat.h>
#include	<string.h>
#include	<stdio.h>
#include	<fcntl.h>
#include	<stdlib.h>
#include	<ctype.h>
#include	<stdarg.h>

#include	"misc_com.h"
#include	"misc_str.h"

MISC_STR_STRING_LIST *Misc_Str_StringListCreate(void)
{
	MISC_STR_STRING_LIST *pstStringList;
	
	pstStringList = (MISC_STR_STRING_LIST *)malloc(sizeof(MISC_STR_STRING_LIST));
	memset(pstStringList, 0, sizeof(MISC_STR_STRING_LIST));
	return pstStringList;
}

int Misc_Str_StringListAdd(MISC_STR_STRING_LIST *pstStringList, char *sItem)
{
	MISC_STR_STRING_LIST_ITEM *pstStringListItem;
	
	if ((pstStringList == NULL) || (sItem == NULL))
	{
		return MISC_STR_STRING_LIST_ERROR_INVALID_POINTER;
	}
	
	pstStringListItem = (MISC_STR_STRING_LIST_ITEM *)malloc(sizeof(MISC_STR_STRING_LIST_ITEM));
	if (pstStringListItem == NULL)
	{
		return MISC_STR_STRING_LIST_ERROR_FAIL_TO_ALLOC_MEM;
	}
	
	pstStringListItem->sItem = (char *)malloc(strlen(sItem)+1);
	if (pstStringListItem->sItem == NULL)
	{
		free(pstStringListItem);
		return MISC_STR_STRING_LIST_ERROR_FAIL_TO_ALLOC_MEM;
	}
	strncpy(pstStringListItem->sItem, sItem, 1024);
	
	if (pstStringList->pstItem == NULL)
	{
		pstStringList->pstItem = pstStringListItem;
	}
	if (pstStringList->pstItemTail != NULL)
	{
		pstStringList->pstItemTail->pstNext = pstStringListItem;
	}
	pstStringList->pstItemTail = pstStringListItem;
	pstStringListItem->pstNext = NULL;
	
	return 0;
}

void Misc_Str_StringListFree(MISC_STR_STRING_LIST *pstStringList)
{
	MISC_STR_STRING_LIST_ITEM *pstStringListItem;
	
	if (pstStringList == NULL)
	{
		return;
	}
	
	pstStringListItem = pstStringList->pstItem;
	while (pstStringListItem != NULL)
	{
		pstStringList->pstItem = pstStringList->pstItem->pstNext;
		free(pstStringListItem->sItem);
		free(pstStringListItem);
		pstStringListItem = pstStringList->pstItem;
	}
	free(pstStringList);
	return;
}

char *Misc_Str_StringListGetFirst(MISC_STR_STRING_LIST *pstStringList)
{
	if (pstStringList == NULL)
	{
		return NULL;
	}
	
	pstStringList->pstItemCur = pstStringList->pstItem;
	if (pstStringList->pstItemCur == NULL)
	{
		return NULL;
	}
	
	return pstStringList->pstItemCur->sItem;
}
	
char *Misc_Str_StringListGetNext(MISC_STR_STRING_LIST *pstStringList)
{
	if ((pstStringList == NULL) || (pstStringList->pstItemCur == NULL))
	{
		return NULL;
	}
	
	pstStringList->pstItemCur = pstStringList->pstItemCur->pstNext;
	if (pstStringList->pstItemCur == NULL)
	{
		return NULL;
	}
	
	return pstStringList->pstItemCur->sItem;
}

////////////////////////////////////////////////////////////////////

char *Misc_Str_Trim(char *s)
{
	char *pb;
	char *pe;
	char *ps;
	char *pd;

	if (strcmp(s, "") == 0) 
		return s;
	
	pb = s;
		 
	while (((*pb == ' ') || (*pb == '\t') || (*pb == '\n') || (*pb == '\r')) && (*pb != 0))
	{
		pb ++;
	}
	
	pe = s;
	while (*pe != 0)
	{
		pe ++;
	}
	pe --;
	while ((pe >= s) && ((*pe == ' ') || (*pe == '\t') || (*pe == '\n') || (*pe == '\r')))
	{
		pe --;
	}
	
	ps = pb;
	pd = s;
	while (ps <= pe)
	{
		*pd = *ps;
		ps ++;
		pd ++;
	}
	*pd = 0;
	
	return s;
}

char *Misc_Str_StrUppercase(char * s)
{
	char *p;
	
	p = s;
	while (*p != 0)
	{
		*p = toupper(*p);
		p++;
	}
		
	return s;
}

char *Misc_Str_StrLowercase(char * s)
{
	char *p;
	
	p = s;
	while (*p != 0)
	{
		*p = tolower(*p);
		p++;
	}
		
	return s;
}


int Misc_Str_StrCmp(char *s1, char *s2)
{
	char *p1;
	char *p2;
	
	if (s1==NULL) {
		if (s2==NULL)  return 0;
		if (*s2==0) 	return 0;
		return -1;
	}
	
	if (s2==NULL) {
		if (*s1==0) return 0;
		return 1;
	}
	
	
	p1 = s1;
	p2 = s2;
	
	while(1)
	{
		if (*p1 > *p2)
			return 1;
			
		if (*p1 < *p2)
			return -1;
			
		if (*p1 == 0)
			return 0;
			
		p1 ++;
		p2 ++;
	}
}

int Misc_Str_StrNCCmp(char *s1, char *s2)
{
	char *p1;
	char *p2;
	char c1, c2;

	if (s1==NULL) {
		if (s2==NULL)  return 0;
		if (*s2==0) 	return 0;
		return -1;
	}
	
	if (s2==NULL) {
		if (*s1==0) return 0;
		return 1;
	}
	
	p1 = s1;
	p2 = s2;
	
	while(1)
	{
		c1 = toupper(*p1);
		c2 = toupper(*p2);
		
		if (c1 > c2)
			return 1;
			
		if (c1 < c2)
			return -1;
			
		if (c1 == 0)
			return 0;
			
		p1 ++;
		p2 ++;
	}
}

char *Misc_Str_StrReplaceChar(char *pStrSrc, char cOld, char cNew)
{
	char *pChar;

	pChar = pStrSrc;
	while (*pChar)
	{
		if (*pChar == cOld)
		{
			*pChar = cNew;
		}
		pChar ++;
	}
	return pStrSrc;
}

char *Misc_Str_StrCat(char *s, const char *sFormat, ...)
{
	va_list ap;

	va_start(ap, sFormat);
	(void) vsnprintf(s+strlen(s), 1024, sFormat, ap);
	va_end(ap);
	return (s);
}


char *Misc_Str_Strcpy( char *sOBJ, char *sSRC)
{
	if (sSRC==NULL) {
		*sOBJ = 0;
		return sOBJ;
	}
	return strncpy( sOBJ, sSRC, 1024);
}


char *Misc_Str_Strncpy( char *sOBJ, char *sSRC, int iMaxChar)
{
	if (sSRC==NULL) {
		*sOBJ = 0;
		return sOBJ;
	}
	return strncpy( sOBJ, sSRC, iMaxChar );
}


char* Misc_Str_Quote(char* dest, char* src)
{
  register int i, j;
  //int n;

	i = j = 0;
	while (src[i]) {
		switch((unsigned char)src[i]) {
			case '\n':
				dest[j++] = '\\';
				dest[j++] = 'n';
				break;
			case '\t':
				dest[j++] = '\\';
				dest[j++] = 't';
				break;
			case '\r':
				dest[j++] = '\\';
				dest[j++] = 'r';
				break;
			case '\b':
				dest[j++] = '\\';
				dest[j++] = 'b';
				break;
			case '\'':
				dest[j++] = '\\';
				dest[j++] = '\'';
				break;
			case '\"':
				dest[j++] = '\\';
				dest[j++] = '\"';
				break;
			case '\\':
				dest[j++] = '\\';
				dest[j++] = '\\';
				break;
			default:
				dest[j++] = src[i];
		}
		i++;
	}
	dest[j]=0;
	return dest;
}

char* Misc_Str_QuoteWild(char* dest, char* src)
{
  register int i, j;
  //int n;

	i = j = 0;
	while (src[i]) {
		switch((unsigned char)src[i]) {
			case '\n':
				dest[j++] = '\\';
				dest[j++] = 'n';
				break;
			case '\t':
				dest[j++] = '\\';
				dest[j++] = 't';
				break;
			case '\r':
				dest[j++] = '\\';
				dest[j++] = 'r';
				break;
			case '\b':
				dest[j++] = '\\';
				dest[j++] = 'b';
				break;
			case '\'':
				dest[j++] = '\\';
				dest[j++] = '\'';
				break;
			case '\"':
				dest[j++] = '\\';
				dest[j++] = '\"';
				break;
			case '\\':
				dest[j++] = '\\';
				dest[j++] = '\\';
				break;
			case '%':
				dest[j++] = '\\';
				dest[j++] = '%';
				break;
			case '_':
				dest[j++] = '\\';
				dest[j++] = '_';
				break;
			default:
				dest[j++] = src[i];
		}
		i++;
	}
	dest[j]=0;
	return dest;
}

char* Misc_Str_CutHZString(char* sSrc)
{
	int iLength;
	int i = 1;
	unsigned char* pcSrc = (unsigned char*)sSrc;
	iLength = strlen(sSrc);
	
	while (*pcSrc)
	{
    		if (*pcSrc > 127 && i<iLength)
    			{
    				pcSrc+=2; i+=2;
    			}
    		else if (*pcSrc <= 127)
    			{
    				pcSrc++; i++;
    			}
    		else
    			{
    				*pcSrc = 0;
    			}
    	}
	return sSrc;
}

