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
static void		IncSlave(MyCustMadeJson CMDInfo);
static void		DecSlave(MyCustMadeJson CMDInfo);
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
**Function Name	: IncSlave
**Description	: increase slave addresses to slave address table. 
**Parameter		: CMDInfo - in.
**Return		: none.
***********************************************************************/
static void	IncSlave(MyCustMadeJson CMDInfo)
{
	FILE *fp = NULL;
	int i = 0;
	int pos = 2;
	int slave_sum = 0;
	int sum = 0;
	int aisle = 0;
	int res = 0;
	unsigned char address[SLAVE_ADDR_LEN] = {0};
	unsigned char slaves_addr_path[38] = {0};
	
	i = CMDInfo.m_PData[CMDInfo.m_DataLen - 1];
	aisle = g_UartFDS[i];
	
	slave_sum = GetSlaveSumOnAisle(aisle);
	
	if (MAX_SLAVE_SUM == slave_sum)
	{
		printf("%s:no space for new slave reg!\n",__FUNCTION__);
		return;
	}

	sprintf(slaves_addr_path, "%s%.2d", SLAVES_ADDR_CONF, i);

	fp = fopen(slaves_addr_path, "a");
	
	if (NULL == fp)
	{
		printf("%s:open %s failed!\n", __FUNCTION__, slaves_addr_path);
		return;
	}
	
	sum = CMDInfo.m_PData[1];
			
	while (sum--)
	{
		for (i = 0; i < slave_sum; ++i)
		{
			res = GetSlaveAddrByPos(i, aisle);
			
			address[0] = (unsigned char)(res >> 8);
			address[1] = (unsigned char)res;
			
			if (0 == memcmp(address ,&CMDInfo.m_PData[pos], SLAVE_ADDR_LEN))	//-- new slave was registered --//
			{
				printf("%s:slave(%.5d) was registered!\n",__FUNCTION__,res);								
				break;
			}
		}
	
		if (i == slave_sum) //-- new slave was not registered --//
		{
			
			i = (int)(CMDInfo.m_PData[pos] << 8);
			i |= (int)CMDInfo.m_PData[pos + 1];
					
			fprintf(fp, "%.5d\n", i);		
		}
		
		pos += 2;
	}	
				
	fclose(fp);
}

/***********************************************************************
**Function Name	: DecSlave
**Description	: decrease slave addresses from slave address table. 
**Parameter		: CMDInfo - in.
**Return		: none.
***********************************************************************/
static void	DecSlave(MyCustMadeJson CMDInfo)
{
	FILE *fp = NULL;
	int slave_sum = 0;
	int pos = 2;
	int sum = 0;
	int i = 0;
	int aisle = 0;
	int res = 0;
	unsigned char address[SLAVE_ADDR_LEN] = {0};
	unsigned char slaves_addr_path[38] = {0};
	
	i = CMDInfo.m_PData[CMDInfo.m_DataLen - 1];
	aisle = g_UartFDS[i];
	
	slave_sum = GetSlaveSumOnAisle(aisle);
	
	if (0 == slave_sum)
	{
		printf("%s:no slave address\n", __FUNCTION__);
		return;
	}
	
	sprintf(slaves_addr_path, "%s%.2d", SLAVES_ADDR_CONF, i);
	
	fp = fopen(slaves_addr_path, "w");
	
	if (NULL == fp)
	{
		printf("%s:open %s failed!\n", __FUNCTION__, slaves_addr_path);
		return;
	}
	
	for (i = 0; i < slave_sum; ++i)
	{
		sum = CMDInfo.m_PData[1];
		pos = 2;
		
		while (sum--)
		{	
			res = GetSlaveAddrByPos(i, aisle);
			
			address[0] = (unsigned char)(res >> 8);
			address[1] = (unsigned char)res;
			
			if (0 != memcmp(address ,&CMDInfo.m_PData[pos], SLAVE_ADDR_LEN))
			{
				fprintf(fp, "%.5d\n", res);
			}
			
			pos += 2;
		}
	}
	
	fclose(fp);
}

/***********************************************************************
**Function Name	: RemoteCMD_NewFWNotice
**Description	: notice middleware to update slave.
**Parameters	: fd - in.
				: CMDInfo - in.
**Return		: 0
***********************************************************************/
static RemoteCMD_NewFWNotice(int fd, MyCustMadeJson CMDInfo)
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
static RemoteCMD_ConfigSlave(int fd, MyCustMadeJson CMDInfo)
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
static RemoteCMD_SearchStatus(int fd, MyCustMadeJson CMDInfo)
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
static RemoteCMD_RestartSlave(int fd, MyCustMadeJson CMDInfo)
{

	SameHandle('R', CMDInfo);
	
	return 0;
}

/***********************************************************************
**Function Name	: RemoteCMD_SetSlaveAddrTab
**Description	: set slave address table.
**Parameters	: fd - in.
				: CMDInfo - in.
**Return		: 0
***********************************************************************/
static RemoteCMD_SetSlaveAddrTab(int fd, MyCustMadeJson CMDInfo)
{
	
	while (INVAILD_CUST_MADE_JSON != g_RemoteData.m_Type)				//-- we just handle a remote cmd at the same time --//
	{
		Delay_ms(5);
	}
		
	if (0 == CMDInfo.m_PData[0])
	{
		DecSlave(CMDInfo);
	}
	else if (1 == CMDInfo.m_PData[0])
	{
		IncSlave(CMDInfo);
	}
	
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
	else if (REMOTE_CMD_SET_SLAVE_ADDR_TAB == CMDInfo.m_Type)
	{
		L_DEBUG("SET SALVES ADDR TAB\n");
		RemoteCMD_SetSlaveAddrTab(fd, CMDInfo);		
		
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





