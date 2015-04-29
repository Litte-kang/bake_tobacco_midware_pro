#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "AsyncCmds.h"
#include "MyPublicFunction.h"

//----------------------Define macro for-------------------//
//---------------------------end---------------------------//


//--------------------Define variable for------------------//

/*
Description			: async cmd queue.
Default value		: IDLE_ASYNC_CMD.
The scope of value	: /
First used			: AsyncCmdsInit
*/
static AsyncCmd g_AsyncCmds[MAX_ASYNC_CMD_SUM];

/*
Description			: the number of async cmd.
Default value		: 0.
The scope of value	: 0 ~ MAX_ASYNC_CMD_SUM.
First used			: /
*/
static unsigned char g_CurAsyncCmdsSum = 0;

/*
Description			: position where read async cmd from queue.
Default value		: 0.
The scope of value	: 0 ~ (MAX_ASYNC_CMD_SUM - 1).
First used			: /
*/
static unsigned char g_CurReadPos = 0;

/*
Description			: position where write async cmd to queue.
Default value		: 0.
The scope of value	: 0 ~ (MAX_ASYNC_CMD_SUM - 1).
First used			: /
*/
static unsigned g_CurWritePos = 0;

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
First used			: /
*/
static AsyncCmd g_CurAsyncCmd = {IDLE_ASYNC_CMD, NULL_FLAG};

/*
Description			: pointer g_CurAsyncCmd.
Default value		: NULL.
The scope of value	: /
First used			: AsyncCmdsInit
*/
const AsyncCmd *g_PCurAsyncCmd = NULL;

/*
Description			: to force end a cmd to run a high priority cmd.
Default value		: 0.
The scope of value	: /
First used			: /
*/
char g_IsForceEndCurCmd = 0;

//---------------------------end--------------------------//


//------------Declaration static function for--------------//

static void* 	ReadAsyncCmdsThrd(void *pArg);
static void		MyDelay_ms(unsigned int xms);

//---------------------------end---------------------------//


/***********************************************************************
**Function Name	: AsyncCmdsInit
**Description	: initialize async cmd queue,create a thread to read a cmd.
**Parameters	: none.
**Return		: 0 - initialize ok, other value - failed.
***********************************************************************/
int AsyncCmdsInit()
{
	int res = 0;
	pthread_t thread;
	pthread_attr_t attr;
	void *thrd_ret = NULL;
	
	g_PCurAsyncCmd = &g_CurAsyncCmd;

	res = pthread_attr_init(&attr);
	if (0 != res)
	{
		printf("%s:create thread attribute failed!\n",__FUNCTION__);
		return -2;
	}

	res = pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
	if (0 != res)
	{
		printf("%s:bind attribute failed!\n", __FUNCTION__);
	}
	
	res = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	if (0 != res)
	{
		printf("%s:setting attribute failed!\n",__FUNCTION__);
		return -3;
	}

	res = pthread_create(&thread, &attr, ReadAsyncCmdsThrd, (void*)0);
	if (0 != res)
	{
		printf("%s:create connect \"ReadAsyncCmdsThrd\" failed!\n",__FUNCTION__);
		return -4;
	}
	
	pthread_attr_destroy(&attr);	
	
	return 0;
}

/***********************************************************************
**Function Name	: MyDelay_ms
**Description	: delay ? ms,but it is not exact.
**Parameters	: xms - in.
**Return		: none.
***********************************************************************/
static void MyDelay_ms(unsigned int xms)
{
	struct timeval delay;
	
	delay.tv_sec = 0;
	delay.tv_usec = xms * 1000;
	
	select(0, NULL, NULL, NULL, &delay);
}

/***********************************************************************
**Function Name	: ReadAsyncCmdsThrd
**Description	: read async cmd from async cmd queue.
**Parameters	: pArg - in.
**Return		: none.
***********************************************************************/
static void* ReadAsyncCmdsThrd(void *pArg)
{
	printf("%s\n", __FUNCTION__);
	
	while (1)
	{
		if (0 < g_CurAsyncCmdsSum && IDLE_ASYNC_CMD == g_CurAsyncCmd.m_Cmd && 0 == g_IsForceEndCurCmd)
		{			
			g_CurAsyncCmd.m_Cmd = g_AsyncCmds[g_CurReadPos].m_Cmd;
			g_CurAsyncCmd.m_Flag = g_AsyncCmds[g_CurReadPos].m_Flag;
			
			g_CurReadPos++;
			g_CurAsyncCmdsSum--;
			
			if (MAX_ASYNC_CMD_SUM <= g_CurReadPos)
			{
				g_CurReadPos = 0;
			}	
		}

		MyDelay_ms(50);		
	}
		
	pthread_exit(NULL);
}

/***********************************************************************
**Function Name	: AddAsyncCmd
**Description	: add a async cmd.
**Parameters	: cmd - in.
				: flag - in.
**Return		: 0 - add ok, -1 - failed.
***********************************************************************/
int AddAsyncCmd(const unsigned char cmd, const unsigned char flag)
{
	
	if (MAX_ASYNC_CMD_SUM > g_CurAsyncCmdsSum)
	{		
		if ((REMOTE_CMD_FLAG & flag))
		{		
			g_IsForceEndCurCmd = 1;
			
			if (INNER_CMD_FLAG & flag)
			{
				ClearCurAsynCmd();
			}
								
			while (IDLE_ASYNC_CMD != g_CurAsyncCmd.m_Cmd)
			{
				MyDelay_ms(5);
			}
			
			g_CurAsyncCmd.m_Cmd = cmd;
			g_CurAsyncCmd.m_Flag = flag;
			
			g_IsForceEndCurCmd = 0;
			
			return 0;
			
		}

		g_AsyncCmds[g_CurWritePos].m_Cmd = cmd;
		g_AsyncCmds[g_CurWritePos].m_Flag = flag;
		
		g_CurWritePos++;
		g_CurAsyncCmdsSum++;
			
		if (MAX_ASYNC_CMD_SUM <= g_CurWritePos)
		{
			g_CurWritePos = 0;
		}
		
		return 0;
	}
	
	printf("%s: async cmd queue fulled!\n", __FUNCTION__);
	
	return -1;
}

/***********************************************************************
**Function Name	: ClearCurAsynCmd
**Description	: clear g_CurAsyncCmd.
**Parameters	: none.
**Return		: none.
***********************************************************************/
void ClearCurAsynCmd()
{
	g_CurAsyncCmd.m_Flag = NULL_FLAG;
	g_CurAsyncCmd.m_Cmd = IDLE_ASYNC_CMD;
	
}












