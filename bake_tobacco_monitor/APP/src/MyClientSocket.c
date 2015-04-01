#include "MyClientSocket.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/time.h>

//----------------------Define macro for xxx-------------------//

//---------------------------end-------------------------------//


//--------------------Define variable for xxx------------------//

/*
Description			: socket fd
Default value		: -1
The scope of value	: /
First used			: MyClientSocketInit();
*/
static int g_SocketFD = -1;

/*
Description			: /
Default value		: CONNECTED_NO
The scope of value	: /
First used			: /
*/
static char g_ClientState = CONNECTED_NO;


//---------------------------end-------------------------------//


//------------Declaration function for xxx--------------//

static void 	CacthSig(int SigNum);

//---------------------------end-----------------------//

/***********************************************************************
**Function Name	: CacthSig
**Description	: this is callback,when a specified signal come, call it.
**Parameters	: SigNum - signal type.
**Return		: none.
***********************************************************************/
static void CacthSig(int SigNum)
{
	switch (SigNum)
	{
		case SIGPIPE:
			printf("catch SIGPIPE!\n");
			g_ClientState = CONNECTED_NO;
			break;
		default:
			break;
	}
}

/***********************************************************************
**Function Name	: ConnectServer
**Description	: connect server.
**Parameters	: times - in.
				: param - in.
**Return		: -1 - failed, 0 - ok.
***********************************************************************/
int ConnectServer(unsigned int times, CNetParameter param)
{	
	SOCKADDR_IN serv_addr = {0};
	int tmp = 0;
	
	if (0 >= times)
	{		
		printf("%s:param error\n", __FUNCTION__);
		return -1;
	}
	
	if (CONNECTED_YES == (CONNECTED_YES & g_ClientState))
	{
		printf("%s:client has connected!\n", __FUNCTION__);
		return -1;
	}
		
	g_SocketFD = socket(AF_INET, SOCK_STREAM, 0);
	
	if (-1 == g_SocketFD)
	{
		printf("%s:create socket failed!\n",__FUNCTION__);
		return -1;
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(param.m_Port);
	serv_addr.sin_addr.s_addr = inet_addr(param.m_IPAddr);

	bzero(&(serv_addr.sin_zero), 8);
	
	do
	{
		tmp = connect(g_SocketFD, (SOCKADDR*)&serv_addr, sizeof(SOCKADDR));
		
		if (0 == tmp)
		{
			g_ClientState = CONNECTED_YES;
			
			printf("connect %s:%d sucessful!\n", param.m_IPAddr, param.m_Port);
			
			return 0;
		}
		
		sleep(1);
	}while(--times);
	
	printf("connect %s:%d failed!\n", param.m_IPAddr, param.m_Port);
	
	return -1;
}

/***********************************************************************
**Function Name	: RecDataFromServer
**Description	: recieve data from server.
**Parameters	: pBuff - store data recieved.
				: len - expectations.
**Return		: none.
***********************************************************************/
int RecDataFromServer(unsigned char *pBuff, unsigned int len)
{
	int rec_len = 0;
	int max_fd = 0;
	fd_set inset;

	if (CONNECTED_YES != (CONNECTED_YES & g_ClientState))	
	{
		printf("%s:disconnected server!\n",__FUNCTION__);
		return -3;
	}

	if (NULL == pBuff)
	{
		printf("%s:memory error!\n",__FUNCTION__);
		return -1;
	}
	
	FD_ZERO(&inset);
	FD_SET(g_SocketFD, &inset);	
	max_fd = g_SocketFD + 1;
	
	//--- wait data from server ---//
	select(max_fd,  &inset, NULL, NULL, NULL);	

	if (!FD_ISSET(g_SocketFD, &inset))
	{
		printf("%s:socket error!\n",__FUNCTION__);
		return -4;
	}

	FD_CLR(g_SocketFD, &inset);	
	
	rec_len = recv(g_SocketFD, pBuff, len, 0);	
	if (-1 == rec_len)
	{
		printf("%s:recieve data from server failed!\n",__FUNCTION__);
		return -2;
	}
	else if (0 == rec_len)
	{
		g_ClientState = CONNECTED_NO;
		return -5;
	}	

	return 0;
	
}

/***********************************************************************
**Function Name	: SendDataToServer
**Description	: send data to server.
**Parameters	: pBuff - store data recieved.
				: len - expectations.
**Return		: none.
***********************************************************************/
int SendDataToServer(unsigned char *pBuff, unsigned int len)
{
	int send_len = 0;	

	if (CONNECTED_YES != (CONNECTED_YES & g_ClientState))	
	{
		printf("%s:disconnected server!\n",__FUNCTION__);
		return -3;
	}

	if (NULL == pBuff)
	{
		printf("%s:memory error!\n",__FUNCTION__);
		return -1;
	}

	send_len = send(g_SocketFD, pBuff, len, 0);
	if (-1 == send_len)
	{
		printf("%s:send data to server failed!\n",__FUNCTION__);
		return -2;
	}

	return 0;
}

/***********************************************************************
**Function Name	: LogoutClient
**Description	: logout client free source.
**Parameters	: none.
**Return		: none.
***********************************************************************/
void LogoutClient()
{
	if (INVALID_FD != g_SocketFD && CONNECTED_YES == g_ClientState)
	{
		close(g_SocketFD);
		g_ClientState = CONNECTED_NO;
		
		printf("logout client sucessful!\n");	
	}	
}









