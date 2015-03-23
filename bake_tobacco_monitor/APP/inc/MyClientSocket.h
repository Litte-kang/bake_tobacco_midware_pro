#ifndef	_MY_CLIENT_SOCKET_H_
#define _MY_CLIENT_SOCKET_H_

//----------------------Define macro for xxx-------------------//

#define HOSTENT			struct hostent
#define SOCKADDR_IN		struct sockaddr_in
#define IN_ADDR			struct in_addr
#define SOCKADDR		struct sockaddr	

#define INACTIVE_CLIENT		0x00
#define CONNECTED_YES		0x01
#define CONNECTED_NO		0x02
#define ACTIVATED_CLIENT 	0x04
#define LOGOUT_CLIENT		0x10

#define INVALID_FD	-1

//---------------------------end-------------------------------//


//---------------------Define new type for xxx-----------------//

typedef struct _CNetParameter
{
	unsigned char m_IPAddr[16];
	unsigned short m_Port;
}CNetParameter;

//---------------------------end-------------------------------//


//-----------------Declaration variable for xxx----------------//


//---------------------------end-------------------------------//


//-------------------Declaration funciton for xxx--------------//

extern int 		MyClientSocketInit(CNetParameter param);
extern int 		ConnectServer(unsigned int timeout);
extern int 		RecDataFromServer(unsigned char *pBuff, unsigned int len);
extern int 		SendDataToServer(unsigned char *pBuff, unsigned int len);
extern void 	LogoutClient();
extern int 		IsConnectedServer();

//---------------------------end-------------------------------//

#endif //-- end of  _MY_CLIENT_SOCKET_H_ --//
