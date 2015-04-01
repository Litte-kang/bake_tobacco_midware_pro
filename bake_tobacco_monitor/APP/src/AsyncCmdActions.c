#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "AsyncCmdActions.h"
#include "MyPublicFunction.h"
#include "AisleManage.h"
#include "MyClientSocket.h"
#include "MyCustMadeJson.h"
#include "RemoteCmds.h"
#include "AsyncCmds.h"
#include "xProtocol.h"

//----------------------Define macro for-------------------//

#define COUNTER(N,M) \
{\
	if (N > M)\
	{\
		counter++;\
	}\
}\

//---------------------------end---------------------------//


//--------------------Define variable for------------------//

/*
Description			: save tmp log.
Default value		: \
The scope of value	: \
First used			: \
*/
unsigned char g_TmpLog[80] = {0};

//---------------------------end--------------------------//


//------------Declaration static function for--------------//

static void 	SendFWUpdateNotice(int aisle, int id, int IsSingle);
static void 	SendConfigData(int aisle, int id, int IsSingle);
static void 	SendCommonReqInfo(int aisle, int id, int IsSingle, unsigned char type);
static int 		UploadAckToSer(int type, const char *pFileName, unsigned char *pData, unsigned int len);

//---------------------------end---------------------------//

/***********************************************************************
**Function Name	: UploadAckToSer
**Description	: upload ack to server.
**Parameter		: type - in.
				: pFileName - backup file name.
				: pData - data.
				: len - the length of data.
**Return		: 0 - ok, ohter - failed.
***********************************************************************/
static int UploadAckToSer(int type, const char *pFileName, unsigned char *pData, unsigned int len)
{
	MyCustMadeJson json = {0};
	unsigned char json_str[100] = {0};
	int res = 0;
	
	json.m_Type = type;
	memcpy(json.m_Addr, g_MyLocalID, 10);
	json.m_PData = pData;
	json.m_DataLen = len;
	
	memset(json_str, 0, 100);
	res = CustomMadeJson(json, json_str);
	
	res = SendDataToServer(json_str, res);
	if (0 != res)
	{
		BackupAsciiData(pFileName, json_str);
	}
		
	return res;
}

/***********************************************************************
**Function Name	: SendCommonReqInfo
**Description	: curve,status,alert,restart slave req are common,so we can use the same way.
**Parameter		: aisle - in.
				: id - slave id.
				: IsSingle - 0:notice all slaves,1:notice one.
				: type - in.
**Return		: 0 - ok, ohter - failed.
***********************************************************************/
static void SendCommonReqInfo(int aisle, int id, int IsSingle, unsigned char type)
{
	unsigned int res = 0;
	unsigned int slave_sum = 0;
	unsigned int position = 0;
	unsigned int send_again_counter = 0;
	unsigned int timeout = 0;
	unsigned int counter = 0;
	unsigned char address[SLAVE_ADDR_LEN] = {0};
	TIME start;
	
	switch (type)
	{
		case ALERT_DATA_TYPE:		//-- rec bytes less 30 --//
		case STATUS_DATA_TYPE:
		case RESTART_SLAVE_DATA_TYPE:
			timeout = 2;
			break;
		case CURVE_DATA_TYPE:		//-- rec bytes more 30 --//
			timeout = 4;
			break;
		default:
			return;
	}
	
	//--- get slaver sum and start position on aisle ---//
	if (1 == IsSingle)
	{
		if (!GetSlavePositionOnTab(id, &position, aisle))
		{
			slave_sum = position + 1;
		}
		else
		{
			position = slave_sum;
		}
	}
	else
	{
		slave_sum = GetSlaveSumOnAisle(aisle);
		position = GetCurSlavePositionOnTab(aisle);	
	}

	while (position < slave_sum)
	{	
	
		SetCurSlavePositionOnTab(aisle, position);
		
		res = GetSlaveAddrByPos(position, aisle);
					
		L_DEBUG("send a req(%d) to %.5d by %d aisle\n", type, res, aisle);
		
		address[0] = (unsigned char)(res >> 8);
		address[1] = (unsigned char)res;
			
		SendCommunicationRequest(aisle, address, type);
				
		GET_SYS_CURRENT_TIME(start);
		
		do
		{	
			Delay_ms(5);
			
			if (NULL_DATA_FLAG == GetAisleFlag(aisle))
			{

				res = IS_TIMEOUT(start, (timeout * 1000 + 5));	//-- we will send notice again slave, if not receive ack in (timeout)s --//
				if (0 != res)
				{
					printf("%s:receive %.5d req(%d) ack timeout!\n", __FUNCTION__,((int)(address[0] << 8) | address[1]), type);
					sprintf(g_TmpLog, "receive %.5d req(%d) ack timeout!</br>",((int)(address[0] << 8) | address[1]), type);
					SaveTmpData(g_TmpLog);
					SendCommunicationRequest(aisle, address, type);
					GET_SYS_CURRENT_TIME(start);
					send_again_counter++;
				}

			}
			else
			{
				
				while (!(PRO_DATA_OK_FLAG & GetAisleFlag(aisle)))	//-- wait --//
				{
					Delay_ms(5);
				}
				
				break;	//-- receive ack --//
			}
					
		}while(4 > send_again_counter);  //-- we will notice next slave, if we have sent 4 times --//
				
		position++;
		COUNTER(4, send_again_counter);
		send_again_counter = 0;	
		SetAisleFlag(aisle, NULL_DATA_FLAG);
	}

	SetCurSlavePositionOnTab(aisle, 0);
	SetAisleFlag(aisle, NULL_DATA_FLAG);
	
	L_DEBUG("%d slaves req data successful by %d aisle!\n", counter, aisle);	
}

/***********************************************************************
**Function Name	: SearchSlaverAlertInfo
**Description	: search slaver`s alert information.
**Parameter		: aisle - in.
				: id - slave id.
				: IsSingle - 0:search all slaves,1:search one.
**Return		: none.
***********************************************************************/
static void SendConfigData(int aisle, int id, int IsSingle)
{
	unsigned int res = 0;
	unsigned int slave_sum = 0;
	unsigned int position = 0;
	unsigned int send_again_counter = 0;
	unsigned char data[1 + SLAVE_ADDR_LEN] = {0};
	unsigned char address[SLAVE_ADDR_LEN] = {0};
	unsigned int counter = 0;
	TIME start;
	
	//--- get slave sum and start position on aisle ---//	
	if (1 == IsSingle)
	{
		if (!GetSlavePositionOnTab(id, &position, aisle))
		{
			slave_sum = position + 1;
		}
		else
		{
			position = slave_sum;
		}
	}
	else
	{
		slave_sum = GetSlaveSumOnAisle(aisle);
		position = GetCurSlavePositionOnTab(aisle);		
	}

	while (position < slave_sum)
	{	
	
		SetCurSlavePositionOnTab(aisle, position);	//-- set the current slave address table position --//	
		
		res = GetSlaveAddrByPos(position, aisle);
			
		L_DEBUG("send conf data to %.5d by %d aisle\n",res, aisle);
		
		address[0] = (unsigned char)(res >> 8);
		address[1] = (unsigned char)res;
		
		memcpy(data, address, SLAVE_ADDR_LEN);		
		SendConfigration(aisle, address, g_RemoteData.m_Type, &g_RemoteData.m_PData[2], (g_RemoteData.m_DataLen - 2));
				
		GET_SYS_CURRENT_TIME(start);
		
		do
		{
			Delay_ms(5);	
			
			if (NULL_DATA_FLAG == GetAisleFlag(aisle))
			{
				res = IS_TIMEOUT(start, (4 * 1000 + 5));	//-- we will send request again slave, if not receive ack in 5s --//
				if (0 != res)
				{
					printf("%s:receive %.5d conf ack timeout!\n", __FUNCTION__, ((int)(address[0] << 8) | address[1]));
					sprintf(g_TmpLog, "receive %.5d conf ack timeout!</br>",((int)(address[0] << 8) | address[1]));
					SaveTmpData(g_TmpLog);
					SendConfigration(aisle, address, g_RemoteData.m_Type, &g_RemoteData.m_PData[2], (g_RemoteData.m_DataLen - 2));
					
					GET_SYS_CURRENT_TIME(start);
					send_again_counter++;
					
					data[2] = 0;	//-- conf failed --//
				}
			}
			else
			{
				data[2] = 1;	//-- conf ok --//
				
				while (!(PRO_DATA_OK_FLAG & GetAisleFlag(aisle)))	//-- wait --//
				{
					Delay_ms(5);
				}
				
				break;	//-- receive ack --//
			}
				
		}while(4 > send_again_counter);  //-- we will notice next slave, if we have sent 3 times --//
		
		//--- tell server what we modify ok ---//
		res = UploadAckToSer(g_RemoteData.m_Type, CONF_DATA_ACK_BACKUP, data, (1 + SLAVE_ADDR_LEN));
		//--- end ---//	
		
		SetAisleFlag(aisle, NULL_DATA_FLAG);
		position++;
		COUNTER(4, send_again_counter);
		send_again_counter = 0;
	}

	SetCurSlavePositionOnTab(aisle, 0);
	SetAisleFlag(aisle, NULL_DATA_FLAG);
	
	L_DEBUG("%d slaves configure successful by %d aisle!\n", counter, aisle);
}

/***********************************************************************
**Function Name	: SendFWUpdateNotice
**Description	: send a fw update notic to slaver.
**Parameter		: aisle - in.
				: id - slave id.
				: IsSingle - 0:search all slaves,1:search one.
**Return		: none.
***********************************************************************/
static void SendFWUpdateNotice(int aisle, int id, int IsSingle)
{
	unsigned int slave_sum = 0;
	unsigned int position = 0;
	unsigned char data[2 + SLAVE_ADDR_LEN] = {0};
	unsigned char notice[9 + SLAVE_ADDR_LEN] = {0};
	unsigned char send_again_notice[5 + SLAVE_ADDR_LEN] = {0};
	unsigned char address[SLAVE_ADDR_LEN] = {0};
	int res = 0;
	int fw_count = 0;
	int send_again_counter = 0;
	unsigned int counter = 0;
	TIME start;
	TIME start1;
	
	data[SLAVE_ADDR_LEN] = 0; //-- fw type --//
	
	//--- get slaver sum and start position on aisle ---//	
	if (1 == IsSingle)
	{
		if (!GetSlavePositionOnTab(id, &position, aisle))
		{
			slave_sum = position + 1;
		}
		else
		{
			position = slave_sum;
		}
	}
	else
	{
		slave_sum = GetSlaveSumOnAisle(aisle);
		position = GetCurSlavePositionOnTab(aisle);	
	}
	//--- end of get slaver sum and start position on aisle ---// 
	
	//--- fill send_again_notice ---//
	send_again_notice[0] = FW_UPDATE_FLAG0;
	send_again_notice[1] = FW_UPDATE_FLAG1;
	send_again_notice[2] = ACK_FW_DATA_FAILED_TYPE;
	send_again_notice[3 + SLAVE_ADDR_LEN] = FW_UPDATE_FLAG1;
	send_again_notice[4 + SLAVE_ADDR_LEN] = FW_UPDATE_FLAG0;
	//--- end of fille send_again_notice ---//
	
	//--- fill notice content ---//
	notice[0] = FW_UPDATE_FLAG0;
	notice[1] = FW_UPDATE_FLAG1;
	notice[2] = NOTICE_UPDATE_TYPE;
	notice[3 + SLAVE_ADDR_LEN] = g_FWInfo.m_SectionSum;
	notice[4 + SLAVE_ADDR_LEN] = (unsigned char)(g_FWInfo.m_LastSectionSize >> 8);
	notice[5 + SLAVE_ADDR_LEN] = (unsigned char)g_FWInfo.m_LastSectionSize;
	res = atoi(g_FWInfo.m_Version);
	notice[6 + SLAVE_ADDR_LEN] = (unsigned char)res;
	notice[7 + SLAVE_ADDR_LEN] = FW_UPDATE_FLAG1;
	notice[8 + SLAVE_ADDR_LEN] = FW_UPDATE_FLAG0;
	
	L_DEBUG("VER = %d\n", notice[6 + SLAVE_ADDR_LEN]);
	//--- end of fill notice content ---//
	
	while(position < slave_sum)	
	{
		SetCurSlavePositionOnTab(aisle, position);	//-- set the current slave address table position --//
		
		res = GetSlaveAddrByPos(position, aisle);	
		
		address[0] = (unsigned char)(res >> 8);
		address[1] = (unsigned char)res;
		
		memcpy(&notice[3], address, SLAVE_ADDR_LEN);	//-- fill slave address --//
		memcpy(data, address, SLAVE_ADDR_LEN);			
		data[SLAVE_ADDR_LEN + 1] = 1; //-- default update ok! --//
		
		SetAisleFlag(aisle, PRO_FW_UPDATE_FLAG);		//-- allow processing update ack data from slave --//

		write(aisle, notice, 11);			//-- send notice updated to slave --//
		
		L_DEBUG("send update notice to %.5d by %d aisle (position = %d)\n",res, aisle,(position+1));	
		GET_SYS_CURRENT_TIME(start1);
		
		while (IsFWUpdateSuccess(aisle))		
		{
			res = IS_TIMEOUT(start1, (10 * 60 * 1000 ));	//-- we will send fw again, if not receive ack in 10 minutes --//		
			if (6 <= send_again_counter || 0 != res) //-- if we send the same section fw 12 times or we can not update ok in 10 minutes, we will give up --//
			{
				send_again_counter = 6;
				printf("%s:%.5d update timeout!\n",__FUNCTION__,((int)(address[0] << 8) | address[1]));
				data[SLAVE_ADDR_LEN + 1] = 0;
				break;
			}
			else if (fw_count != GetCurFwCount(aisle))
			{
				send_again_counter = 0;
			}
			
			GET_SYS_CURRENT_TIME(start);
			
			while (!(PRO_DATA_OK_FLAG & GetAisleFlag(aisle)))
			{
				Delay_ms(5);
				
				res = IS_TIMEOUT(start, (10 * 1000 + 5));	//-- we will send fw again, if not receive ack in 10s --//
				if (0 != res)
				{	
					send_again_counter++;				
									
					fw_count = GetCurFwCount(aisle);
					if (-1 == fw_count)
					{
						if (6 <= send_again_counter)
						{
							break;
						}
						
						printf("%s:rec %.5d update ack timeout, send notice again!\n",__FUNCTION__,	((int)(address[0] << 8) | address[1]));
						
						write(aisle, notice, 11);			//-- send notice updated to slave again --//
					}
					else
					{
						memcpy(&send_again_notice[3], address, SLAVE_ADDR_LEN);
						ProcAisleData(aisle, send_again_notice, 7); 
						printf("%s:rec %.5d update ack timeout, send fw again!\n",__FUNCTION__, ((int)(address[0] << 8) | address[1]));
					}
					
					GET_SYS_CURRENT_TIME(start);
				} //-- end of if (0 != res) --//
			}
	
			SetAisleFlag(aisle, NULL_DATA_FLAG);
			SetAisleFlag(aisle, PRO_FW_UPDATE_FLAG);
			
			Delay_ms(1);			
		}
		
		//--- tell server what we have updated a machine ---//
		res = UploadAckToSer(5, UPDATE_FW_ACK_BACKUP, data, (2 + SLAVE_ADDR_LEN));
		//--- end ---//	
		
		COUNTER(6, send_again_counter);
		send_again_counter = 0;
		ClearFwCount(aisle);
		SetAisleFlag(aisle, NULL_DATA_FLAG);
		position++;	
		send_again_counter = 0;			
		sleep(10);
	
	}
	
	SetCurSlavePositionOnTab(aisle, 0);	//-- reset --//
	SetAisleFlag(aisle, NULL_DATA_FLAG);

	L_DEBUG("%d slaves update successful by %d aisle\n", counter, aisle);

}

/***********************************************************************
**Function Name	: AsyncCmd_AlertSearch
**Description	: if cmd is 'A', we search slaves alert status.
**Parameters	: aisle - communication aisle.
				: id - slave id.
				: IsSingle - 0:search all slaves,1:search one.
**Return		: 0
***********************************************************************/
int AsyncCmd_AlertSearch(int aisle, int id, int IsSingle)
{
	int res = 0;
	
	//--- connect server ---//					
	if (0 == g_IsCommu) 				//-- avoid starting more --//
	{
		g_IsCommu++;
		alarm(1);						//-- start connect --//					
	}
	else
	{
		g_IsCommu++;
	}

	//--- search alert information ---//
	SendCommonReqInfo(aisle, id, IsSingle, ALERT_DATA_TYPE);
	
	//--- a aisle finish task ---//	
	g_IsCommu = g_IsCommu > 0 ? (g_IsCommu - 1) : 0;
	
	if (0 == g_IsCommu)
	{		
		LogoutClient();		
	}
	
	while (0 != g_IsCommu) //-- all aisle exit task at the same time --//
	{
		Delay_ms(5);
	}
	
	ClearCurAsynCmd();
	
	L_DEBUG("====================================================\n");
	L_DEBUG("              all aisle finish task                 \n");	
	L_DEBUG("====================================================\n");	
	return 0;
}

/***********************************************************************
**Function Name	: AsyncCmd_StatusSearch
**Description	: if cmd is 'S', we search slaves status information.
**Parameters	: aisle - communication aisle.
				: id - slave id.
				: IsSingle - 0:search all slaves,1:search one.
**Return		: 0
***********************************************************************/
int AsyncCmd_StatusSearch(int aisle, int id, int IsSingle)
{
	int res = 0;
	
	//--- connect server ---//					
	if (0 == g_IsCommu) 				//-- avoid starting more --//
	{
		g_IsCommu++;
		alarm(1);						//-- start connect --//
	}
	else
	{
		g_IsCommu++;
	}
	
	//--- search status information ---//
	SendCommonReqInfo(aisle, id, IsSingle, STATUS_DATA_TYPE);				

	//--- a aisle finish task ---//	
	g_IsCommu = g_IsCommu > 0 ? (g_IsCommu - 1) : 0;
	
	if (0 == g_IsCommu)
	{		
		LogoutClient();
				
		if ((REMOTE_CMD_FLAG & g_PCurAsyncCmd->m_Flag))
		{
			RemoteCMD_Init();
		}
	}
	
	while (0 != g_IsCommu) //-- all aisle exit task at the same time --//
	{
		Delay_ms(5);
	}
		
	ClearCurAsynCmd();

	L_DEBUG("====================================================\n");
	L_DEBUG("              all aisle finish task                 \n");	
	L_DEBUG("====================================================\n");		
	return 0;
}

/***********************************************************************
**Function Name	: AsyncCmd_RestartSlave
**Description	: if cmd is 'R', we restart slave.
**Parameters	: aisle - communication aisle.
				: id - slave id.
				: IsSingle - 0:search all slaves,1:search one.
**Return		: 0
***********************************************************************/
int AsyncCmd_RestartSlave(int aisle, int id, int IsSingle)
{
	int res = 0;
					
	g_IsCommu++;						

	//--- send restart notice ---//
	SendCommonReqInfo(aisle, id, IsSingle, RESTART_SLAVE_DATA_TYPE);			

	//--- a aisle finish task ---//	
	g_IsCommu = g_IsCommu > 0 ? (g_IsCommu - 1) : 0;
	
	if (0 == g_IsCommu)
	{	
		if (REMOTE_CMD_FLAG == (REMOTE_CMD_FLAG & g_PCurAsyncCmd->m_Flag))
		{
			RemoteCMD_Init();
		}
	}
	
	while (0 != g_IsCommu) //-- all aisle exit task at the same time --//
	{
		Delay_ms(5);
	}
		
	ClearCurAsynCmd();	

	L_DEBUG("====================================================\n");
	L_DEBUG("              all aisle finish task                 \n");	
	L_DEBUG("====================================================\n");	
	return 0;	
}

/***********************************************************************
**Function Name	: AsyncCmd_FWUpdate
**Description	: if cmd is 'F', we update slave.
**Parameters	: aisle - communication aisle.
				: id - slave id.
				: IsSingle - 0:search all slaves,1:search one.
**Return		: 0
***********************************************************************/
int AsyncCmd_FWUpdate(int aisle, int id, int IsSingle)
{
	int res = 0;
	
	//--- connect server ---//					
	if (0 == g_IsCommu) 				//-- avoid starting more --//
	{
		g_IsCommu++;
		alarm(1);						//-- start connect --//			
	}
	else
	{
		g_IsCommu++;
	}
	
	SendFWUpdateNotice(aisle, id, IsSingle);

	//--- a aisle finish task ---//	
	g_IsCommu = g_IsCommu > 0 ? (g_IsCommu - 1) : 0;
	
	if (0 == g_IsCommu)
	{				
		LogoutClient();
	}

	while (0 != g_IsCommu) //-- all aisle exit task at the same time --//
	{
		Delay_ms(5);
	}
	
	AddAsyncCmd('R', REMOTE_CMD_FLAG); //-- we restart slave --//
	ClearCurAsynCmd();

	L_DEBUG("====================================================\n");
	L_DEBUG("              all aisle finish task                 \n");	
	L_DEBUG("====================================================\n");		
	return 0;
}

/***********************************************************************
**Function Name	: AsyncCmd_Config
**Description	: if cmd is 'P', we configure slave.
**Parameters	: aisle - communication aisle.
				: id - slave id.
				: IsSingle - 0:search all slaves,1:search one.
**Return		: 0
***********************************************************************/
int AsyncCmd_Config(int aisle, int id, int IsSingle)
{
	int res = 0;
					
	if (0 == g_IsCommu) 				//-- avoid starting more --//
	{
		g_IsCommu++;
		alarm(1);						//-- start connect --//
	}
	else
	{
		g_IsCommu++;
	}
	
	//--- send configuration information ---//
	SendConfigData(aisle, id, IsSingle);						

	//--- a aisle finish task ---//	
	g_IsCommu = g_IsCommu > 0 ? (g_IsCommu - 1) : 0;
	
	if (0 == g_IsCommu)
	{		
		LogoutClient();	
		
		if (REMOTE_CMD_FLAG == (REMOTE_CMD_FLAG & g_PCurAsyncCmd->m_Flag))
		{
			if (REMOTE_CMD_CONFIG_SLAVE_CURVE == g_RemoteData.m_Type)
			{
				AddAsyncCmd('c', NULL_FLAG); //-- we have finished setting curve of slaves and we would search curve of slaves --//
			}
			else
			{
				RemoteCMD_Init();
			}
		}
	}

	while (0 != g_IsCommu) //-- all aisle exit task at the same time --//
	{
		Delay_ms(5);
	}
	

	ClearCurAsynCmd();
	
	L_DEBUG("====================================================\n");
	L_DEBUG("              all aisle finish task                 \n");	
	L_DEBUG("====================================================\n");	
	return 0;
}

/***********************************************************************
**Function Name	: AsyncCmd_Get
**Description	: if cmd is 'G', we configure slave.
**Parameters	: aisle - communication aisle.
				: id - slave id.
				: IsSingle - 0:search all slaves,1:search one.
**Return		: 0
***********************************************************************/
int AsyncCmd_Get(int aisle, int id, int IsSingle)
{
	int res = 0;
					
	if (0 == g_IsCommu) 				//-- avoid starting more --//
	{
		g_IsCommu++;
		alarm(1);						//-- start connect --//
	}
	else
	{
		g_IsCommu++;
	}
	
	//--- send configuration information ---//
	SendConfigData(aisle, id, IsSingle);						

	//--- a aisle finish task ---//	
	g_IsCommu = g_IsCommu > 0 ? (g_IsCommu - 1) : 0;
	
	if (0 == g_IsCommu)
	{		
		LogoutClient();	
		
		if (REMOTE_CMD_FLAG == (REMOTE_CMD_FLAG & g_PCurAsyncCmd->m_Flag))
		{	
			RemoteCMD_Init();
		}
	}

	while (0 != g_IsCommu) //-- all aisle exit task at the same time --//
	{
		Delay_ms(5);
	}
	
	ClearCurAsynCmd();
	
	L_DEBUG("====================================================\n");
	L_DEBUG("              all aisle finish task                 \n");	
	L_DEBUG("====================================================\n");	
	return 0;
}

/***********************************************************************
**Function Name	: AsyncCmd_StatusSearch
**Description	: if cmd is 'c', we search slaves status information.
**Parameters	: aisle - communication aisle.
				: id - slave id.
				: IsSingle - 0:search all slaves,1:search one.
**Return		: 0
***********************************************************************/
int AsyncCmd_CurveDataSearch(int aisle, int id, int IsSingle)
{
	int res = 0;
	
	//--- connect server ---//					
	if (0 == g_IsCommu) 				//-- avoid starting more --//
	{
		g_IsCommu++;
		alarm(1);						//-- start connect --//
	}
	else
	{
		g_IsCommu++;
	}
	
	//--- search status information ---//
	SendCommonReqInfo(aisle, id, IsSingle, CURVE_DATA_TYPE);
						
	//--- a aisle finish task ---//	
	g_IsCommu = g_IsCommu > 0 ? (g_IsCommu - 1) : 0;
	
	if (0 == g_IsCommu)
	{		
		LogoutClient();	
		
		if (REMOTE_CMD_CONFIG_SLAVE_CURVE == g_RemoteData.m_Type)
		{
			RemoteCMD_Init();
		}						
	}
	
	while (0 != g_IsCommu) //-- all aisle exit task at the same time --//
	{
		Delay_ms(5);
	}
		
	ClearCurAsynCmd();

	L_DEBUG("====================================================\n");
	L_DEBUG("              all aisle finish task                 \n");	
	L_DEBUG("====================================================\n");		
	return 0;
}







