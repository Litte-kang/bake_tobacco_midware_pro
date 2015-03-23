#ifndef _MY_CUST_MADE_JSON_H_
#define _MY_CUST_MADE_JSON_H_

//----------------------Define macro for-------------------//

#define CUST_MADE_TYPE	"\"type\":"
#define CUST_MADE_ADDR	"\"address\":"
#define	CUST_MADE_DATA	"\"data\":"

#define INVAILD_CUST_MADE_JSON 	-1

//---------------------------end---------------------------//


//---------------------Define new type for-----------------//

typedef struct _MyCustMadeJson
{
    int				m_Type;
    unsigned char	m_Addr[11];
    unsigned char	*m_PData;
    unsigned int	m_DataLen;
}MyCustMadeJson;

//---------------------------end---------------------------//


//-----------------Declaration variable for----------------//

//---------------------------end---------------------------//


//-------------------Declaration funciton for--------------//

extern unsigned int 	CustomMadeJson(MyCustMadeJson MyJson, char *pJsonFormat);
extern MyCustMadeJson 	CustMadeJsonParse(const char *pJsonFormat);
extern void 			FreeCustMadeJson(MyCustMadeJson *pJson);

//---------------------------end---------------------------//

#endif //-- end of _MY_CUST_MADE_JSON_H_ --// 
