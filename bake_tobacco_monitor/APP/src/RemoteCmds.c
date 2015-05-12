#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
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

	while (INVAILD_CUST_MADE_JSON != g_RemoteData.m_Type)				//-- we just handle a remote cmd at the same time --//
	{
		Delay_ms(5);
	}
	
	g_RemoteData.m_Type = CMDInfo.m_Type;								//-- cmd type --//
	
	memcpy(g_RemoteData.m_Addr, CMDInfo.m_PData, 2);					//-- slave id --//
									
	memcpy(g_RemoteData.m_PData, CMDInfo.m_PData, CMDInfo.m_DataLen);	//-- cmd data --//

	g_RemoteData.m_DataLen = CMDInfo.m_DataLen;							//-- data len --//
		
	AddAsyncCmd(HandleType, REMOTE_CMD_FLAG);
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
	//--
	if (REMOTE_CMD_NEW_FW_NOTICE == CMDInfo.m_Type)
	{
		L_DEBUG("UPDATE CMD\n");
		RemoteCMD_NewFWNotice(fd, CMDInfo);	
		
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





