#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include "RemoteCmds.h"
#include "AisleManage.h"
#include "MyPublicFunction.h"
#include "MyServerSocket.h"
#include "uart_api.h"
#include "AsyncCmds.h"
#include "xProtocol.h"

//----------------------Define macro for-------------------//


//---------------------------end---------------------------//


//--------------------Define variable for------------------//

/*
Description			: for g_RemoteData->m_PData.
Default value		: /
The scope of value	: /.
First used			: /.
*/
static unsigned char g_Data[100] = {0};

/*
Description			: server maybe send data that send to slave,
					: we need save.
Default value		: /.
The scope of value	: /.
First used			: /.
*/
MyCustMadeJson g_RemoteData = {0};

/*
Description			: if we set 1 sync server time, otherwise 0.
Default value		: /.
The scope of value	: /.
First used			: /.
*/
char g_IsSyncServerTime = 0;

//---------------------------end--------------------------//


//------------Declaration static function for--------------//

static int 		GetFWInfo(int FWType);
static void 	SameHandle(unsigned char HandleType, MyCustMadeJson CMDInfo);
static int 		RemoteCMD_NewFWNotice(int fd, MyCustMadeJson CMDInfo);
static int 		RemoteCMD_ConfigSlave(int fd, MyCustMadeJson CMDInfo);
static int 		RemoteCMD_SearchStatus(int fd, MyCustMadeJson CMDInfo);
static int 		RemoteCMD_RestartSlave(int fd, MyCustMadeJson CMDInfo);
static int 		RemoteCMD_SetSlaveAddrTab(int fd, MyCustMadeJson CMDInfo);
static int 		RemoteCMD_GetSlaveData(int fd, MyCustMadeJson CMDInfo);
static int 		RemoteCMD_SyncServerTime(int fd, MyCustMadeJson CMDInfo);

//---------------------------end---------------------------//

/***********************************************************************
**Function Name	: RemoteCMD_Init
**Description	: /.
**Parameters	: /.
**Return		: 0
***********************************************************************/
int RemoteCMD_Init()
{
	g_RemoteData.m_Type = INVAILD_CUST_MADE_JSON;	
	memset(g_RemoteData.m_Addr, 0xff, 10);
	g_RemoteData.m_PData = g_Data;
	memset(g_RemoteData.m_PData, 0, 100);	
	g_RemoteData.m_DataLen = 0;

	L_DEBUG("%s:remote cmd init ok!\n",__FUNCTION__);
	
	return 0;
}

/***********************************************************************
**Function Name	: RemoteCMD_SyncServerTime
**Description	: syn server time.
**Parameter		: FWType - in.
**Return		: none.
***********************************************************************/
static int 	RemoteCMD_SyncServerTime(int fd, MyCustMadeJson CMDInfo)
{
	struct tm set_tm;
	struct timeval tv;
	time_t set_time;
	int year = 0;

	year = (int)CMDInfo.m_PData[5];
	year <<= 8;
	year |= (int)CMDInfo.m_PData[6];
	
	set_tm.tm_year = year - 1900;
	set_tm.tm_mon = (int)CMDInfo.m_PData[3] - 1;
	set_tm.tm_mday = (int)CMDInfo.m_PData[4];
	set_tm.tm_hour = (int)CMDInfo.m_PData[0];
	set_tm.tm_min = (int)CMDInfo.m_PData[1];
	set_tm.tm_sec = (int)CMDInfo.m_PData[2];
	
	set_time = mktime(&set_tm);
	
	tv.tv_sec = set_time;
	tv.tv_usec = 0;
	
	if (settimeofday(&tv, (struct timezone *)0))
	{
		printf("%s:sync server time failed!\n", __FUNCTION__);
		return 0;
	}

	g_IsSyncServerTime = 1;

	return 0;
}

/***********************************************************************
**Function Name	: GetFWInfo
**Description	: get fw information from string.
**Parameter		: FWType - in.
**Return		: none.
***********************************************************************/
static int GetFWInfo(int FWType)
{
	int fw_size 			= 0;
	int sections 			= 0;
	int last_section_size 	= 0;
	FILE *fp 				= NULL;
	
	fp = fopen(FW_0_VER, "r");
	
	if (NULL == fp)
	{
		printf("%s:open %s failed\n", __FUNCTION__, FW_0_VER);
		return -1;
	}
	
	fscanf(fp, "%s\n", g_FWInfo.m_Version);
	fscanf(fp, "%d\n", &fw_size);
	
	fclose(fp);
	
	if (0 == fw_size)
	{
		printf("%s:read version failed!\n", __FUNCTION__);
		return -2;
	}
	
	sections = fw_size / (AVG_SECTION_FW_SIZE);

	last_section_size = fw_size % (AVG_SECTION_FW_SIZE);
	
	g_FWInfo.m_SectionSum = (last_section_size) ? ((sections) + 1) : sections;	//-- N + 1 --//
	g_FWInfo.m_LastSectionSize = last_section_size;
	
	L_DEBUG("sections = %d\n", g_FWInfo.m_SectionSum);
		
	return 0;
	
}

/***********************************************************************
**Function Name	: SameHandle
**Description	: we will do the same handle what we will get cmd information,cmd data, cmd type. 
				: but the function is not same.
**Parameter		: HandleType - in.
				: CMDInfo - in.
**Return		: none.
***********************************************************************/
static void SameHandle(unsigned char HandleType, MyCustMadeJson CMDInfo)
{	
	TIME start;
	int cmd_level = REMOTE_CMD_FLAG;

	if (REMOTE_CMD_NEW_FW_NOTICE == g_RemoteData.m_Type)
	{
		return;	//-- we can`t end fw update --//
	}

	if (REMOTE_CMD_CONFIG_MID_TIME == CMDInfo.m_Type && REMOTE_CMD_CONFIG_MID_TIME == g_RemoteData.m_Type)
	{
		return; //-- we can`t add a cmd added and the cmd is running --//
	}

	GET_SYS_CURRENT_TIME(start);

	while (INVAILD_CUST_MADE_JSON != g_RemoteData.m_Type)				//-- we just handle a remote cmd at the same time --//
	{ 
		if (IS_TIMEOUT(start, (12000)))
		{
			RemoteCMD_Init(); //-- the target slave is only one --//
			break;
		}

		Delay_ms(5);
	}

	if (REMOTE_CMD_CONFIG_MID_TIME == CMDInfo.m_Type)
	{
		cmd_level = INNER_CMD_FLAG;
	}

	g_RemoteData.m_Type = CMDInfo.m_Type;								//-- cmd type --//
	
	memcpy(g_RemoteData.m_Addr, CMDInfo.m_PData, 2);					//-- slave id --//
									
	memcpy(g_RemoteData.m_PData, CMDInfo.m_PData, CMDInfo.m_DataLen);	//-- cmd data --//

	g_RemoteData.m_DataLen = CMDInfo.m_DataLen;							//-- data len --//
		
	AddAsyncCmd(HandleType, cmd_level);
}

/***********************************************************************
**Function Name	: RemoteCMD_NewFWNotice
**Description	: notice middleware to update slave.
**Parameters	: fd - in.
				: CMDInfo - in.
**Return		: 0
***********************************************************************/
static int RemoteCMD_NewFWNotice(int fd, MyCustMadeJson CMDInfo)
{
	int res = 0;
	MyCustMadeJson json = {0};

	if (0 == CMDInfo.m_PData[2])	//-- slave fw --//
	{
		res = GetFWInfo(CMDInfo.m_Type);
	
		if (0 == res)
		{
			SameHandle('F', CMDInfo);
		}	
	}

	return 0;
}

/***********************************************************************
**Function Name	: RemoteCMD_ConfigSlave
**Description	: configure slave.
**Parameters	: fd - in.
				: CMDInfo - in.
**Return		: 0
***********************************************************************/
static int RemoteCMD_ConfigSlave(int fd, MyCustMadeJson CMDInfo)
{
	
	SameHandle('P', CMDInfo);
	
	return 0;
}

/***********************************************************************
**Function Name	: RemoteCMD_ConfigSlave
**Description	: search slave status.
**Parameters	: fd - in.
				: CMDInfo - in.
**Return		: 0
***********************************************************************/
static int RemoteCMD_SearchStatus(int fd, MyCustMadeJson CMDInfo)
{

	SameHandle('S', CMDInfo);
	
	return 0;
}

/***********************************************************************
**Function Name	: RemoteCMD_RestartSlave
**Description	: restart slave.
**Parameters	: fd - in.
				: CMDInfo - in.
**Return		: 0
***********************************************************************/
static int RemoteCMD_RestartSlave(int fd, MyCustMadeJson CMDInfo)
{

	
	SameHandle('R', CMDInfo);
	
	return 0;
}

/***********************************************************************
**Function Name	: RemoteCMD_GetSlaveData
**Description	: get slave data.
**Parameters	: fd - in.
				: CMDInfo - in.
**Return		: 0
***********************************************************************/
static int 	RemoteCMD_GetSlaveData(int fd, MyCustMadeJson CMDInfo)
{
	if (17 == CMDInfo.m_Type)
	{
		AddAsyncCmd('A', INNER_CMD_FLAG);
		AddAsyncCmd('S', INNER_CMD_FLAG);
		AddAsyncCmd('c', INNER_CMD_FLAG);
		
		return 0;
	}
	
	SameHandle('G', CMDInfo);
	
	return 0;
}

/***********************************************************************
**Function Name	: RemoteCMD_Pro
**Description	: process remote cmd.
**Parameters	: fd - in.
				: CMDInfo - in.
**Return		: 0
***********************************************************************/
int RemoteCMD_Pro(int fd, MyCustMadeJson CMDInfo)
{
	if (REMOTE_CMD_NEW_FW_NOTICE == CMDInfo.m_Type)
	{
		L_DEBUG("UPDATE CMD\n");
		RemoteCMD_NewFWNotice(fd, CMDInfo);	
		
		return 0;	
	}
	else if (REMOTE_CMD_CONFIG_MID_TIME == CMDInfo.m_Type && 0 < fd)
	{
		L_DEBUG("SYNC server time ok\n");
		CMDInfo.m_Type = CONF_TIME_DATA_TYPE;
		RemoteCMD_SyncServerTime(fd, CMDInfo);

		return 0;
	}
	
	if (3 >= CMDInfo.m_DataLen) //-- request data handle --//
	{
		if (REMOTE_CMD_SEARCH_SLAVE_STATUS == CMDInfo.m_Type)
		{
			L_DEBUG("SEARCH SLAVES STATUS CMD\n");
			RemoteCMD_SearchStatus(fd, CMDInfo);
			
			return 0;			
		}
		
		RemoteCMD_GetSlaveData(fd, CMDInfo);
	}
	else if (4 <= CMDInfo.m_DataLen)	//-- request modify data handle --//
	{
		L_DEBUG("CONFIG SLAVES CMD\n");
		RemoteCMD_ConfigSlave(fd, CMDInfo);		
	}
		
	return 0;
}





