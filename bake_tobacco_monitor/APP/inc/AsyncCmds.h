#ifndef _ASYNC_CMDS_H_
#define _ASYNC_CMDS_H_

//----------------------Define macro for-------------------//

#define MAX_ASYNC_CMD_SUM	5
#define IDLE_ASYNC_CMD		0

#define NULL_FLAG			0x00	
#define INNER_CMD_FLAG		0x01
#define REMOTE_CMD_FLAG		0x02	//-- the async cmd come from server --//
#define CUTED_FLAG			0x04	//-- a async cmd was cuted --//

//---------------------------end---------------------------//


//---------------------Define new type for-----------------//

typedef struct _AsyncCmd
{
	unsigned char m_Cmd;
	unsigned char m_Flag;
}AsyncCmd;

//---------------------------end---------------------------//


//-----------------Declaration variable for----------------//

/*
Description			: current async cmd .
Default value		: 0
The scope of value	: 0 - nothing to do.
					: the current define cmd:
					: 'S' - search slave status information.
					: 'F' - update slave fw.
					: 'A' - search slave alert information.
					: 'C' - configure slave.
					: 'c' - search slave curve value.
					: 'P' - get data from slaves.
First used			: /
*/
extern const AsyncCmd *g_PCurAsyncCmd;

/*
Description			: to force end a cmd.
Default value		: 0.
The scope of value	: /
First used			: /
*/
extern char g_IsForceEndCurCmd;

//---------------------------end---------------------------//


//-------------------Declaration funciton for--------------//

extern int 		AsyncCmdsInit(void);
extern int 		AddAsyncCmd(const unsigned char cmd, const unsigned char flag);
extern void		ClearCurAsynCmd();

//---------------------------end---------------------------//

#endif	//--_ASYNC_CMDS_H_--//
