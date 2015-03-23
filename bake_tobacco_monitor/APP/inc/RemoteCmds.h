#ifndef _REMOTE_CMD_PRO_H_
#define _REMOTE_CMD_PRO_H_

#include "MyCustMadeJson.h"

//----------------------Define macro for-------------------//

#define REMOTE_CMD_NEW_FW_NOTICE			4
#define REMOTE_CMD_SEARCH_SLAVE_STATUS		8
#define REMOTE_CMD_CONFIG_SLAVE_CURVE		12
#define REMOTE_CMD_CONFIG_SLAVE_TOBA_SIZE	13
#define REMOTE_CMD_CONFIG_SLAVE_TIME		14
#define REMOTE_CMD_SET_SLAVE_ADDR_TAB		15

//---------------------------end---------------------------//


//---------------------Define new type for-----------------//

//---------------------------end---------------------------//


//-----------------Declaration variable for----------------//

/*
Description			: server maybe send data that send to slave,
					: we need save.
Default value		: /.
The scope of value	: /.
First used			: /.
*/
extern MyCustMadeJson g_RemoteData;

//---------------------------end---------------------------//


//-------------------Declaration funciton for--------------//

extern int 		RemoteCMD_Init();
extern int		RemoteCMD_Pro(int fd, MyCustMadeJson CMDInfo);

//---------------------------end---------------------------//

#endif	//--_REMOTE_CMD_PRO_H_--//
