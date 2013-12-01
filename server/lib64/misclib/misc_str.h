#ifndef _MISC_STR_H_
#define _MISC_STR_H_

#define MISC_STR_STRING_LIST_ERROR_INVALID_POINTER 	(MISC_STR_ERROR_BASE-1)
#define MISC_STR_STRING_LIST_ERROR_FAIL_TO_ALLOC_MEM	(MISC_STR_ERROR_BASE-2)

typedef struct _MISC_STR_STRING_LIST_ITEM
{
	char *sItem;
	struct _MISC_STR_STRING_LIST_ITEM *pstNext;
} MISC_STR_STRING_LIST_ITEM;

typedef struct _MISC_STR_STRING_LIST
{
	MISC_STR_STRING_LIST_ITEM *pstItem;
	MISC_STR_STRING_LIST_ITEM *pstItemTail;
	MISC_STR_STRING_LIST_ITEM *pstItemCur;
} MISC_STR_STRING_LIST;

MISC_STR_STRING_LIST *Misc_Str_StringListCreate(void);
int Misc_Str_StringListAdd(MISC_STR_STRING_LIST *pstStringList, char *sItem);
void Misc_Str_StringListFree(MISC_STR_STRING_LIST *pstStringList);
char *Misc_Str_StringListGetFirst(MISC_STR_STRING_LIST *pstStringList);
char *Misc_Str_StringListGetNext(MISC_STR_STRING_LIST *pstStringList);

char *Misc_Str_Trim(char *s);
char *Misc_Str_StrUppercase(char * s);
char *Misc_Str_StrLowercase(char * s);
int Misc_Str_StrCmp(char *s1, char *s2);
int Misc_Str_StrNCCmp(char *s1, char *s2);
char *Misc_Str_StrReplaceChar(char *pStrSrc, char cOld, char cNew);
char *Misc_Str_StrCat(char *s, const char *sFormat, ...);

char *Misc_Str_Strcpy( char *sOBJ, char *sSrc);
char *Misc_Str_Strncpy( char *sOBJ, char *sSrc, int iMaxChar);

char* Misc_Str_Quote(char* dest, char* src);
char* Misc_Str_QuoteWild(char* dest, char* src);

char* Misc_Str_CutHZString(char* sSrc);

#endif
