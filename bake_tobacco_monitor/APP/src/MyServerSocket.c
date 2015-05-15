#include "MyServerSocket.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <pthread.h>
#include <signal.h>
#include "MyPublicFunction.h"

//----------------------Define macro for xxx-------------------//



//---------------------------end-------------------------------//


//--------------------Define variable for xxx------------------//

/*
Description			: socket fd
Default value		: INVAILD_SOCK_FD
The scope of value	: /
First used			: MyClientSocketInit();
*/
static int g_SocketFD = INVAILD_SOCK_FD;

//---------------------------end-------------------------------//


//------------Declaration function for xxx--------------//

static void* ListenThrd(void *pArg);
static int CreateRecThrd(ClientInfo info); 

//---------------------------end------------------------//


/***********************************************************************
**Function Name	: MyServerSocketInit
**Description	: create a socket and bind socket.
**Parameters	: param - in.
**Return		: 0 - ok, other value - failed.
***********************************************************************/
int MyServerSocketInit(SNetParameter param)
{
	int tmp = 1;
	SOCKADDR_IN server_addr;

	g_SocketFD = socket(AF_INET, SOCK_STREAM, 0);
	if (-1 == g_SocketFD)
	{
		printf("%s:create socket failed!\n", __FUNCTION__);
		return -1;
	}

	setsockopt(g_SocketFD, SOL_SOCKET, SO_REUSEADDR, &tmp, sizeof(tmp));
	
	bzero(&(server_addr.sin_zero), 8);
	
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(param.m_Port);
	server_addr.sin_addr.s_addr = INADDR_ANY;
	
	tmp = bind(g_SocketFD, (SOCKADDR *)&server_addr, sizeof(SOCKADDR));
	if (-1 == tmp)
	{
		printf("%s:bind socket failed!\n", __FUNCTION__);
		return -2;
	}
	
	return 0;
}

/***********************************************************************
**Function Name	: ListenThrd
**Description	: listen client connect thread.
**Parameters	: pArg - in.
**Return		: none.
***********************************************************************/
static void* ListenThrd(void *pArg)
{
	fd_set inset;
	fd_set tmp_inset;
	SOCKADDR_IN client_addr;
	int tmp = 0;
	int client_fd = 0;
	int max_fd = 0;

	L_DEBUG("Listen thread!\n");

	FD_ZERO(&inset);

	FD_SET(g_SocketFD, &inset);	
	
	max_fd = g_SocketFD + 1;

	while (1)
	{
		tmp_inset = inset;

		tmp = select(max_fd, &tmp_inset, NULL, NULL, NULL);
		if (-1 == tmp)
		{
			printf("%s: select listen socket failed!\n",__FUNCTION__);
		}
		else if (0 < FD_ISSET(g_SocketFD, &tmp_inset))
		{	
			tmp = sizeof(SOCKADDR_IN);
			client_fd = accept(g_SocketFD, (SOCKADDR *)&client_addr, &tmp);
			if (-1 == client_fd)
			{
				printf("%s:accept failed!\n", __FUNCTION__);
			}
			else
			{
				ClientInfo info;
				
				L_DEBUG("accept a client!\n");
				
				info.m_fd = client_fd;
				info.m_IPAddr = (int)client_addr.sin_addr.s_addr;

				CreateRecThrd(info);
			}

		}
		
		sleep(1);
	}
	
	pthread_exit(NULL);
}

/***********************************************************************
**Function Name	: Listen
**Description	: listen client connect.
**Parameters	: none.
**Return		: none.
***********************************************************************/
int Listen()
{
	int res = 0;
	pthread_t thread;
	pthread_attr_t attr;
	void *thrd_ret = NULL;

	res = listen(g_SocketFD, MAX_QUE_CONN_SIZE);
	if (-1 == res)
	{
		printf("%s: listen failed!\n", __FUNCTION__);
		return -1;
	}
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

	res = pthread_create(&thread, &attr, ListenThrd, (void*)0);
	if (0 != res)
	{
		printf("%s:create connect listen thread failed!\n",__FUNCTION__);
		return -4;
	}
	
	pthread_attr_destroy(&attr);	
	
	return 0;
}

/***********************************************************************
**Function Name	: RecDataFromClient
**Description	: receive data from client by fd.
**Parameters	: indx - in.
				: pBuff - save data buffer.
				: len - the length expired.
				: timeout - in.
**Return		: 0 - ok, other value - failed.
***********************************************************************/
int RecDataFromClient(int fd, unsigned char *pBuff, unsigned int len, int timeout)
{
	fd_set inset;
	int max_fd = 0;
	int res = 0;
	struct timeval tv;
	
	if (NULL == pBuff)
	{
		printf("%s:buffer error!\n", __FUNCTION__);
		return -1;
	}
	
	if (INVAILD_SOCK_FD == fd)
	{
		printf("%s:socket fd error!\n", __FUNCTION__);
		return -2;
	}

	FD_ZERO(&inset);
	FD_SET(fd, &inset);
		
	max_fd = fd + 1;

	tv.tv_sec = timeout;
	tv.tv_usec = 0;

	res = select(max_fd, &inset, NULL, NULL, (timeout ? &tv : NULL));

	if (!FD_ISSET(fd, &inset))
	{
		printf("%s:select error!\n",__FUNCTION__);
		return -4;
	}	

	FD_CLR(fd, &inset);

	res = recv(fd, pBuff, len, 0);
	if (0 >= res)
	{
		printf("%s:recieve data from client failed!\n",__FUNCTION__);
		return -3;
	}
		
	return 0;
}

/***********************************************************************
**Function Name	: SendDataToClient
**Description	: send data to client.
**Parameters	: fd - in.
				: pBuff - to be sent.
				: len - the length of data.
**Return		: 0 - ok, other value - failed.
***********************************************************************/
int SendDataToClient(int fd, unsigned char *pBuff, unsigned int len)
{
	int send_len = 0;

	if (NULL == pBuff)
	{
		printf("%s:buffer error!\n",__FUNCTION__);
		return -1;
	}

	if (INVAILD_SOCK_FD == fd)
	{
		printf("%s:socket fd error!\n", __FUNCTION__);
		return -2;
	}

	send_len = send(fd, pBuff, len, 0);
	if (0 >= send_len)
	{
		printf("%s:send data to client failed!\n",__FUNCTION__);
		return -3;
	}

	return 0;
}

/***********************************************************************
**Function Name	: CreateRecThrd
**Description	: create a receive thread.
**Parameters	: info - in.
**Return		: 0 - ok, other value - failed.
***********************************************************************/
static int CreateRecThrd(ClientInfo info)
{
	int res = 0;
	pthread_t thread;
	pthread_attr_t attr;
	void *thrd_ret = NULL;

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

	res = pthread_create(&thread, &attr, RecThrds, (void*)&info);
	if (0 != res)
	{
		printf("%s:create connect listen thread failed!\n",__FUNCTION__);
		return -4;
	}
	
	pthread_attr_destroy(&attr);	
	
	return 0;	
}











