#ifndef _AISLE_MANAGE_H_
#define _AISLE_MANAGE_H_

#include "MyPublicFunction.h"
#include "RemoteCmds.h"
#include "uart_api.h"

//------------------------------------------------------MACRO------------------------------------------------//

#define SLAVES_ADDR_CONF			"./conf/slaves_addr/aisle_"

#define STATUS_INFO_BACKUP			"./data/backup/status_info_backup"
#define ALERT_INFO_BACKUP			"./data/backup/alert_info_backup"
#define CONF_DATA_ACK_BACKUP		"./data/backup/conf_data_ack_backup"
#define UPDATE_FW_ACK_BACKUP		"./data/backup/update_fw_ack_backup"
#define CUVRE_INFO_BACKUP			"./data/backup/curve_info_backup"

/****************************************************************
*					communication protocol(server)		        *
*****************************************************************/

#define UPLOAD_SER_SIZE		256

/****************************************************************
*				end communication protocol(server)		        *
*****************************************************************/


#define REC_DATA_FLAG			0x01
#define PRO_DATA_OK_FLAG		0x02
#define PRO_FW_UPDATE_FLAG		0x04
#define NULL_DATA_FLAG			0x00

//-----------------------------------------------------MACRO END ---------------------------------------------//

//-------------------------------------------NEW TYPE------------------------------------//

typedef struct _FWInformation
{
	int 	m_SectionSum;
	short 	m_LastSectionSize;
	char 	m_Version[5];
}FWInformation;

typedef struct _AisleInfo
{
	unsigned char 	m_Flag;	
	int 			m_FwCount;
	int 			m_Aisle;
	unsigned int	m_CurSlavePosition;
	unsigned int	m_SlaveSum;	
	unsigned char	m_SlavesAddrTab[MAX_SLAVE_SUM][SLAVE_ADDR_LEN];
}AisleInfo;

//-----------------------------------------NEW TYPE END-----------------------------------//

//---------------------------------------DECLARATION VARIAVLE--------------------------------------------//

/*
Description			: to store fw information
Default value		: /.
The scope of value	: /.
First used			: /.
*/
extern FWInformation g_FWInfo;

//-------------------------------------DECLARATION VARIABLE END-------------------------------------------//

//--------------------------------------------------DECLARATION FUNCTION----------------------------------------//

extern void 			AisleManageInit();
extern void 			ProcAisleData(int aisle, unsigned char *pData, unsigned int len);
extern int 				IsFwUpdateSuccess(int aisle);
extern void 			ClearFwCount(int aisle);
extern int 				GetCurFwCount(int aisle);
extern void 			SetCurSlavePositionOnTab(int aisle, unsigned int position);
extern unsigned int 	GetCurSlavePositionOnTab(int aisle);
extern unsigned int 	GetSlaveSumOnAisle(int aisle);
extern void 			SetAisleFlag(int aisle, unsigned char flag);
extern unsigned char	GetAisleFlag(int aisle);
extern int 				GetSlavePositionOnTab(int addr, int *pPos ,int aisle);
extern int				GetSlaveAddrByPos(int pos, int aisle);
extern void				SaveTmpData(unsigned char *pData);

//-----------------------------------------------DECLARATION FUNCTION END--------------------------------------------//


#endif //-- _AISLE_MANAGE_H_ --//
