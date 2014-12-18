//
// service.h - 服务程序框架
//


#if !defined(AFX_SERVICE_H__40095FEA_5F70_11D4_B3A2_0080C88FC26A__INCLUDED_)
#define AFX_SERVICE_H__40095FEA_5F70_11D4_B3A2_0080C88FC26A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "odsError.h"

////////////////////////////////////////////////////////////////////////
// 函数定义

void WINAPI service_main(DWORD dwArgc, LPTSTR *lpszArgv);
void WINAPI service_ctrl(DWORD dwCtrlCode);
void CmdInstallService();
void CmdRemoveService();
void CmdDebugService(int argc, TCHAR ** argv);
BOOL WINAPI ControlHandler ( DWORD dwCtrlType );
void ServiceStart (DWORD dwArgc, LPTSTR *lpszArgv, STODSERROR* pstError);
void ServiceCleanup();
void ServiceStop();

#endif // !defined(AFX_SERVICE_H__40095FEA_5F70_11D4_B3A2_0080C88FC26A__INCLUDED_)
