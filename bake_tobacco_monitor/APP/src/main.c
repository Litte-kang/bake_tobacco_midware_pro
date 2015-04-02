#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <ifaddrs.h>
#include <netinet/in.h> 
#include <string.h> 
#include <arpa/inet.h>
#include "uart_api.h"
#include "AisleManage.h"
#include "MyPublicFunction.h"
#include "MyClientSocket.h"
#include "MyServerSocket.h"
#include "MyCustMadeJson.h"
#include "RemoteCmds.h"
#include "AsyncCmdActions.h"
#include "AsyncCmds.h"


//----------------------Define macro for xxx-------------------//

#define IDLE_THRD_NUM 					0		
#define REC_UART0_DATA_NUM				1
//#define REC_UART1_DATA_NUM 				2
#define SEND_UART0_DATA_NUM		 		3
//#define SEND_UART1_DATA_NUM				4
#define THREAD_SIZE      				3
#define CONNECT_TIMEOUT					5
#define SEARCH_ALERT_INFO_INTERVAL		2
#define SEARCH_STATUS_INFO_INTERVAL		6
#define SEARCH_CURVE_INFO_INTERVAL		12
#define START_ASYNC_CMD_MAX_INTERVAL	SEARCH_CURVE_INFO_INTERVAL

#define SER_ADDR						"./conf/ser_ip"

//---------------------------end-------------------------------//

//------------Declaration static function for xxx--------------//

//--- thread function ---//

static void* 	Thrds(void *pArg);
static void* 	IdleThrd(void *pArg);

//--- general function ---//

static void 	AppInit();
static void 	SendUartData(int fd);
static void 	RecUartData(int fd);
static void 	TimerCallback(int SigNum);
static void 	UploadBackData(void);
//static void 	ReadBackData(void);

//--------------------Define variable for xxx------------------//

/*
Description			: connect server parameter.
Default value		: /
The scope of value	: /
First used			: AppInit();
*/
CNetParameter g_Param = {0};

/*
Description			: server parameter.
Default value		: /
The scope of value	: /
First used			: AppInit();
*/
static SNetParameter g_SParam = {0};

/*
Description			: communicate with slaves g_IsCommu > 0, otherwise = 0 or -1.
Default value		: 0
The scope of value	: /
First used			: /
*/
int g_IsCommu = 0;

/*
Description			: middleware machine id
Default value		: 0
The scope of value	: /
First used			: AppInit()
*/
unsigned char g_MyLocalID[11] = {0};


//---------------------------end-------------------------------//


//---------------------------end-------------------------------//

void TestFunc()
{

}

int main()
{	

    AppInit();
    
    return 0;
}

/***********************************************************************
**Function Name	: AppInit
**Description	: initialze some variable and device
**Parameter		: none.
**Return		: none.
***********************************************************************/
static void AppInit()
{
    pthread_t thread[THREAD_SIZE];
#if (COM_TYPE == GNR_COM)
	int target_com[USER_COM_SIZE] = {TARGET_COM_PORT0};
#else
	int target_com[USER_COM_SIZE] = {HOST_COM_PORT0}; 
#endif
	int thrd_num[THREAD_SIZE] = {REC_UART0_DATA_NUM, SEND_UART0_DATA_NUM, IDLE_THRD_NUM};
    int i = 0;
    int res = 0;
    void *thrd_ret;
    FILE *fp = NULL;
   	
   	//--- open uart ---//
	for (i = 0; i < USER_COM_SIZE; ++i)
	{	
		if((g_UartFDS[i] = open_port(target_com[i])) < 0)
		{
		   g_UartFDS[i] = -1;
		   exit(1);
		}
	

		if( -1 != g_UartFDS[i]) 
		{
		    set_com_config(g_UartFDS[i], 9600, 8, 'N', 1);
		}
	}
	//--- end ---//
	
	RemoteCMD_Init();
	
	AisleManageInit();
	
	if (AsyncCmdsInit())
	{
		printf("async cmds init failed!\n");
		exit(1);
	}
	
	//--- get net parameter ---//
	{
		char ip_addr[16] = {0};
		int ip = 0;

		fp = fopen(SER_ADDR,"r");
		fscanf(fp, "%s", ip_addr);
		fclose(fp);
		
		memcpy(g_Param.m_IPAddr, ip_addr, strlen(ip_addr));
		g_Param.m_Port = 8124;
		g_SParam.m_Port = 4000;

		printf("server ip %s\n", g_Param.m_IPAddr);
		
		MyServerSocketInit(g_SParam);
		Listen();
		sleep(1);
	}
	//--- end ---//

	//--- set alarm signal to send local id to server per 1 min ---//
	{
		struct sigaction sa;
		struct sigaction sig;

		sa.sa_handler = SIG_IGN;
		sig.sa_handler = TimerCallback;

		sigemptyset(&sig.sa_mask);
		sig.sa_flags = 0;

		sigaction(SIGALRM, &sa, 0);
		sigaction(SIGALRM, &sig, 0);

		alarm(60);
	}
	//--- end ---//
	
	//--- read middleware id ---//
	{
		fp = fopen(MID_ID_PATH, "r");
		fscanf(fp, "%s", g_MyLocalID);
		fclose(fp);	
		
		L_DEBUG("MID_ID = %s\n",g_MyLocalID);
	}	
	//--- end ---//

	for (i = 0; i < (THREAD_SIZE - 1); ++i)
	{
		res = pthread_create(&thread[i], NULL, Thrds, (void*)thrd_num[i]);
		if (0 != res)
		{
			printf("%s:create rec data thread failed!\n",__FUNCTION__);
			exit(1);
		}
	}

    res = pthread_create(&thread[i], NULL, IdleThrd, (void*)thrd_num[i]);

    if (0 != res)
    {
		printf("%s:create idle thread failed!\n",__FUNCTION__);
        exit(1);
    }

    for (i = 0; i < THREAD_SIZE; i++)
    {
        res = pthread_join(thread[i], &thrd_ret);
    }

}

/***********************************************************************
**Function Name	: SendUartData
**Description	: send uart data.
**Parameter		: fd - in.
**Return		: none.
***********************************************************************/
static void SendUartData(int fd)
{
	int id = 0x0000ffff;
	int tmp = 0;
	
	while (1)
	{	
		id = 0x0000ffff; //-- default all slaves --//
		
		if ((REMOTE_CMD_FLAG &g_PCurAsyncCmd->m_Flag))
		{
			id = (int)(g_RemoteData.m_Addr[0] << 8);
			id |= (int)g_RemoteData.m_Addr[1];
		}
						
		switch (g_PCurAsyncCmd->m_Cmd)
		{
			case 'F':	//-- send fw update notice and update --//	
				{						
					AsyncCmd_FWUpdate(fd, id, (id >= 0x0000ffff ? 0 : 1 ));			
				}
				break;
			case 'S':	//-- send status information request --//
				{	
					AsyncCmd_StatusSearch(fd, id, (id >= 0x0000ffff ? 0 : 1 ));				
				}
				break;
			case 'A':	//-- send alert information request --//
				{	
					AsyncCmd_AlertSearch(fd, id, (id >= 0x0000ffff ? 0 : 1 ));
				}
				break;
			case 'P':	//-- modify slaves configuration --//
				{
					AsyncCmd_Config(fd, id, (id >= 0x0000ffff ? 0 : 1 ));
				}
				break;
			case 'G':	//-- get slaves data --//
				{
					AsyncCmd_Get(fd, id, (id >= 0x0000ffff ? 0 : 1 ));
				}
				break;
			case 'c':	//-- send curve information request --//
				{
					AsyncCmd_CurveDataSearch(fd, id, (id >= 0x0000ffff ? 0 : 1 ));
				}
				break;				
			case 'R':	//-- send restart notice --//
				{
					AsyncCmd_RestartSlave(fd, id, (id >= 0x0000ffff ? 0 : 1 ));
				}
				break;
			default:
				sleep(1);
				break;
		}	
	}
}

/***********************************************************************
**Function Name	: RecUartData
**Description	: received uart data.
**Parameter		: fd - in.
**Return		: none.
***********************************************************************/
static void RecUartData(int fd)
{
    unsigned char buff[BUFFER_SIZE];
    int max_fd = 0;
    int res = 0;
    int real_read = 0;
    int i = 0;
    struct timeval tv;
    fd_set inset;
    fd_set tmp_inset;
    
    FD_ZERO(&inset); 
	FD_SET(fd, &inset);
    
	max_fd = fd;
    
	tv.tv_sec = TIME_DELAY;
	tv.tv_usec = 0;   
	
    while (1)
    {
    	while ((FD_ISSET(fd, &inset)))
    	{   
    		tmp_inset = inset;
    		res = select(max_fd + 1, &tmp_inset, NULL, NULL, NULL);

    		switch(res)
    		{
    			case -1:
    			{
    				perror("select");
    			}
    			break;
    			
    			case 0: 
    			{
    				perror("select time out");
    			}
    			break;
    			
    			default:
    			{
					printf("----------------------------------%d\n",fd);
					if (FD_ISSET(fd, &tmp_inset))
					{
						memset(buff, 0, BUFFER_SIZE);
						real_read = read(fd, buff, BUFFER_SIZE);
						///*
						{
						    int j = 0;
						    
						    for (; j < real_read; ++j)
						    {
						        L_DEBUG("0x%.2x,",buff[j]);
						    }
						    L_DEBUG("\n");
						}
					    //*/    
                        if (0 < real_read)
                        {
                            ProcAisleData(fd, buff, real_read);
                        }//-- end of if (0 < real_read) --//
					}
    			} 
    		}
    	} 
    }

}

/***********************************************************************
**Function Name	: RecThrds
**Description	: receive client data.
**Parameter		: pArg - in.
**Return		: none.
***********************************************************************/
void* RecThrds(void *pArg)
{
	ClientInfo info;
	char str[500] = {0};
	int res = 0;
	char data = 0;
	MyCustMadeJson json = {0};
	
	info.m_fd = ((ClientInfo*)pArg)->m_fd;
	info.m_IPAddr = ((ClientInfo*)pArg)->m_IPAddr;
	
	L_DEBUG("FD = %d,ip = %d\n",info.m_fd, info.m_IPAddr);
	
	while (1)
	{
#if 1
		L_DEBUG("receive data from Client!\n");
		
		memset(str, 0, 500);
		res = RecDataFromClient(info.m_fd, str, 500);
		if (0 == res)
		{	
			json = CustMadeJsonParse(str);
			L_DEBUG("type = %d\n",json.m_Type);
			
			RemoteCMD_Pro(info.m_fd, json);	
			
			FreeCustMadeJson(&json);
			
			break;
					
		}
		else if (SER_DISCONNECTED == res)
		{
			break;
		}
#endif			
		sleep(1);		
	}
	
	close(info.m_fd);
	
	L_DEBUG("FD(%d) disconnect with server!\n", info.m_fd);
	
	pthread_exit(NULL);
}

/***********************************************************************
**Function Name	: Thrds
**Description	: this is thread.
**Parameter		: pArg - unkown.
**Return		: none.
***********************************************************************/
static void* Thrds(void *pArg)
{
   	int thrd_num = (int)pArg;

	L_DEBUG("Thrds %d\n", thrd_num);	
	
	switch (thrd_num)
	{
		case REC_UART0_DATA_NUM:
			RecUartData(g_UartFDS[0]);
			break;
		case SEND_UART0_DATA_NUM:
			SendUartData(g_UartFDS[0]);
			break;
		default:
			break;
	}

    pthread_exit(NULL);
}

/***********************************************************************
**Function Name	: IdleThrd
**Description	: this is a idle thread.
**Parameter		: pArg - unkown.
**Return		: none.
***********************************************************************/
static void* IdleThrd(void *pArg)
{
	int thrd_num = (int)pArg;
#if 1	
	MyCustMadeJson json = {0};
	unsigned char state_dat[11] = {0,50,0,44,0,25,0,25,1,255,3};
	unsigned char alert_dat[4] = {7,5,0,0};
	char addr_buffer[UPLOAD_SER_SIZE] = {0};
	unsigned int len = 0;
	int i = 0;
#endif
    //--- do here ---//
	L_DEBUG("idle thread %d\n", thrd_num);
	
	while (1)
	{
#if 0	
		L_DEBUG("SEND STATE SIMULATION DATA!\n");
		alarm(1);
		while (0 == IsConnectedServer())
		{
			Delay_ms(5);
		}
		
		sprintf(addr_buffer, "{\"midAddress\":\"0000000001\",\"type\":0,\"address\":\"00036\",\"data\":[0,0,0,0,1,90,1,90]}");	
		SendDataToServer(addr_buffer, strlen(addr_buffer));	
		sleep(1);
	
		sprintf(addr_buffer, "{\"midAddress\":\"0000000001\",\"type\":0,\"address\":\"00100\",\"data\":[0,0,0,0,2,26,2,26]}");	
		SendDataToServer(addr_buffer, strlen(addr_buffer));	
		sleep(1);
		LogoutClient();
		
		//-----------------------------------------------//
		
		sleep(1*60);
		alarm(1);	
		L_DEBUG("SEND ALERT SIMULATION DATA!\n");
		while (0 == IsConnectedServer())
		{
			Delay_ms(5);
		}
		
		sprintf(addr_buffer, "{\"midAddress\":\"0000000001\",\"type\":2,\"address\":\"00036\",\"isBelow\":0,\"data\":[1,90,1,90,1,90,1,90,0,20,0]}");	
		SendDataToServer(addr_buffer, strlen(addr_buffer));	
		sleep(1);
	
		sprintf(addr_buffer, "{\"midAddress\":\"0000000001\",\"type\":2,\"address\":\"00100\",\"isBelow\":0,\"data\":[2,26,2,26,2,26,2,26,0,20,0]}");	
		SendDataToServer(addr_buffer, strlen(addr_buffer));	
		sleep(1);
				
		LogoutClient();
		
#endif	
		sleep(2*60);
	}

    pthread_exit(NULL);
}

/***********************************************************************
**Function Name	: TimerCallback
**Description	: alarm() event.
**Parameter		: SigNum - unkown.
**Return		: none.
***********************************************************************/
static void TimerCallback(int SigNum)
{
	static int minute = 0;
	unsigned char async_cmd_start_interval[3][2] = 
	{
		SEARCH_ALERT_INFO_INTERVAL, 	'A',
		SEARCH_STATUS_INFO_INTERVAL, 	'S',
		SEARCH_CURVE_INFO_INTERVAL,		'c'
	};
	int res = 0;
	int i = 0;

	if (SIGALRM == SigNum)
	{	
		UploadBackData();
		
		//-----------------------------------------------------------------//
		
		if (0 == g_IsCommu)
		{	
			minute++;
			
			for (i = 0; i < 3; ++i)
			{
				if (0 == (minute % async_cmd_start_interval[i][0]))
				{
					AddAsyncCmd(async_cmd_start_interval[i][1], NULL_FLAG);
				}
			}
		}
		
		minute = minute % START_ASYNC_CMD_MAX_INTERVAL;
		
		//---------------------------------------------------------------//				
	} //--- end of if (SIGALRM == SigNum) ---//
	
	alarm(62);	
}

/***********************************************************************
**Function Name	: UploadBackData
**Description	: upload back data to server.
**Parameter		: none.
**Return		: none.
***********************************************************************/
static void UploadBackData(void)
{
	FILE *fp = NULL;
	unsigned char upload_buf[UPLOAD_SER_SIZE] = {0};
	char *file_names[4] = 
	{
		STATUS_INFO_BACKUP,
		ALERT_INFO_BACKUP,
		CONF_DATA_ACK_BACKUP,
		UPDATE_FW_ACK_BACKUP
	};
	int i = 0;
		
	if (0 == g_IsCommu)
	{		
		if(0 == ConnectServer(CONNECT_TIMEOUT, g_Param))
		{
			for (i = 0; i < 4; ++i)
			{
				fp = fopen(file_names[i], "r");
				if (NULL != fp)
				{
					do
					{
						fscanf(fp, "%s\n", upload_buf);
	
						if (0 != upload_buf[0])
						{
							SendDataToServer(upload_buf, strlen(upload_buf));
						}
	
					}while(!feof(fp));

					fclose(fp);
	
					remove(file_names[i]);				
				}
			}	

	
			if (0 == g_IsCommu)
			{
				LogoutClient();	
			}
		}
	}	
}





















