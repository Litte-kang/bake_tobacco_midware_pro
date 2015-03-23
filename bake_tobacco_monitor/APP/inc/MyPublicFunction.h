#ifndef _MY_PUBLIC_FUNCTION_H_
#define _MY_PUBLIC_FUNCTION_H_

#include <sys/time.h>

#ifndef NULL
#define NULL (void*)0
#endif

//--------------------------------------MACRO---------------------------------------//

#define TIME					struct timeval
#define GET_SYS_CURRENT_TIME(n)	gettimeofday(&n, NULL)
#define IS_TIMEOUT(n,m)			IsTimeout((int)&n, m)

#if 1
#define L_DEBUG	printf
#else
#define L_DEBUG
#endif

#define MAX_SLAVE_SUM		64	//-- the max number of connecting slaves a aisle --//
#define SLAVE_ADDR_LEN		2	//-- the length of slave address --//

#define MID_ID_PATH			"./conf/mid_id"

#define FW_0_VER			"./fws/fw_0/0.version"
#define FW_1_VER			"./fws/fw_1/1.version"

//-----------------------------------MACRO END-------------------------------------//

//-------------------------------------------NEW TYPE------------------------------------//


//-----------------------------------------NEW TYPE END-----------------------------------//

//---------------------------------------DECLARATION VARIAVLE--------------------------------------------//

/*
Description			: middleware machine id
Default value		: /
The scope of value	: /
First used			: AppInit()
*/
extern unsigned char g_MyLocalID[11];

//-------------------------------------DECLARATION VARIABLE END-------------------------------------------//

//--------------------------------------------------DECLARATION FUNCTION----------------------------------------//

extern int 				IsTimeout(int StartTime, unsigned int threshold);
extern void 			Delay_ms(unsigned int xms);
extern unsigned char* 	MyStrStr(unsigned char *pSrc, unsigned int SrcLen, const unsigned char *pDst, unsigned int DstLen, unsigned int *len);
extern int 				CreateCRC16CheckCode_1(unsigned char *pData, unsigned int len);
extern int 				BackupAsciiData(const char *pFileName, unsigned char *pData);

//-----------------------------------------------DECLARATION FUNCTION END--------------------------------------------//

#endif
