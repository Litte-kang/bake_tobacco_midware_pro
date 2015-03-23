#ifndef _TOBA_MACHINE_CMDS_HANDLE_H_
#define _TOBA_MACHINE_CMDS_HANDLE_H_

//----------------------Define macro for-------------------//
//---------------------------end---------------------------//


//---------------------Define new type for-----------------//
//---------------------------end---------------------------//


//-----------------Declaration variable for----------------//

/*
Description			: communicate with slaves g_IsCommu > 0, otherwise = 0.
Default value		: 0
The scope of value	: /
First used			: /
*/
extern int g_IsCommu;

//---------------------------end---------------------------//


//-------------------Declaration funciton for--------------//

extern int 		AsyncCmd_AlertSearch(int aisle, int id, int IsSingle);
extern int 		AsyncCmd_StatusSearch(int aisle, int id, int IsSingle);
extern int 		AsyncCmd_FWUpdate(int aisle, int id, int IsSingle);
extern int 		AsyncCmd_Config(int aisle, int id, int IsSingle);
extern int 		AsyncCmd_Get(int aisle, int id, int IsSingle);
extern int 		AsyncCmd_RestartSlave(int aisle, int id, int IsSingle);
extern int 		AsyncCmd_CurveDataSearch(int aisle, int id, int IsSingle);

//---------------------------end---------------------------//

#endif	//--_TOBA_MACHINE_CMDS_HANDLE_H_--//
