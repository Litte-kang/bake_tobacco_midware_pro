#ifndef _MY_SERVER_SOCKET_H_
#define _MY_SERVER_SOCKET_H_

//----------------------Define macro for xxx-------------------//

#define SOCKADDR_IN		struct sockaddr_in
#define SOCKADDR		struct sockaddr

#define MAX_QUE_CONN_SIZE	2
#define QUEUE_SIZE			MAX_QUE_CONN_SIZE

#define INVAILD_SOCK_FD 	-1
#define SER_DISCONNECTED			-3

//---------------------------end-------------------------------//


//---------------------Define new type for xxx-----------------//

typedef struct _SNetParameter
{
	unsigned char m_IPAddr[16];
	unsigned short m_Port;
}SNetParameter;

typedef struct _ClientInfo
{
	int m_fd;
	int m_IPAddr;
}ClientInfo;

//---------------------------end-------------------------------//


//-----------------Declaration variable for xxx----------------//

//---------------------------end-------------------------------//


//-------------------Declaration funciton for xxx--------------//

extern int 		MyServerSocketInit(SNetParameter param);
extern int 		Listen();
extern void* 	RecThrds(void *pArg);
extern int 		RecDataFromClient(int fd, unsigned char *pBuff, unsigned int len, int timeout);
extern int 		SendDataToClient(int fd, unsigned char *pBuff, unsigned int len); 

//---------------------------end-------------------------------//


#endif //-- _MY_SERVER_SOCKET_H_ --//
