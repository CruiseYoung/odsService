//
// service.cpp - 服务程序框架
//

//#include "stdafx.h"
#include <tchar.h>
//#include <WinSock2.h>
#include <windows.h>
#include "main.h"
#include "odsService.h"
#include "odsError.h"
#include "odsLog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// 和服务相关的宏定义，具体的服务程序实现应当修改这几个宏的内容

// 服务程序名称
#define SRV_APPNAME						_T("myServer")
// 要安装的 NT 服务名称
#define SRV_SERVICENAME					_T("myService")
// 要安装的 NT 服务的显示名称
#define SRV_SERVICEDISPLAYNAME			_T("_我的服务_")
// 服务程序的依赖关系
#define SRV_DEPENDENCIES				_T("\0\0")

#ifdef __AFXWIN_H__
CWinApp theApp;
#endif

static SERVICE_STATUS_HANDLE g_sshStatusHandle;
static BOOL g_bDebug = FALSE;
static DWORD g_dwCurrentState;

//
// 主函数，如不带参数，则以服务方式运行；
// 也可以带以下参数而以命令行方式运行
//
// options:
//		-install -remove -debug
//
//		-install 表示在 NT 系统中安装服务
//		-remove 表示在 NT 系统中删除服务
//		-debug 表示以命令行方式运行此程序，以便于调试
//
void _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
	int nRetCode = 0;

#ifdef __AFXWIN_H__
	// initialize MFC and print and error on failure
	if (!AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0))
	{		
		odsAddToSystemLog( SRV_APPNAME, ODS_MSG_INITMFC, EVENTLOG_ERROR_TYPE );
		return;
	}
#endif

	// 参数判断
	if ( (argc > 1) && ( *argv[1] == _T('-') || *argv[1] == _T('/') ) )
    {
        if ( _tcsicmp( _T("install"), argv[1] + 1 ) == 0 )
        {
            CmdInstallService();
        }
        else if ( _tcsicmp( _T("remove"), argv[1] + 1 ) == 0 )
        {
            CmdRemoveService();
        }
        else if ( _tcsicmp( _T("debug"), argv[1] + 1 ) == 0 )
        {
            g_bDebug = TRUE;
            CmdDebugService(argc, argv);
        }
        else
        {
            goto dispatch;
        }
        return;
    }

    // 如果没有参数，则以服务方式运行
dispatch:
	SERVICE_TABLE_ENTRY dispatchTable[] =
    {
        { SRV_SERVICENAME, (LPSERVICE_MAIN_FUNCTION)service_main },
        { NULL, NULL }
    };

    if (!StartServiceCtrlDispatcher(dispatchTable))
	{
        odsAddToSystemLog( SRV_APPNAME, ODS_MSG_STARTSVCDISP, EVENTLOG_ERROR_TYPE );
		return;
	}
}

void WINAPI service_main(DWORD dwArgc, LPTSTR *lpszArgv)
{
	STODSERROR err;
	err.nCode = 0;

    // register our service control handler:
    //
    g_sshStatusHandle = RegisterServiceCtrlHandler( SRV_SERVICENAME, service_ctrl );

    if (!g_sshStatusHandle)
	{
		odsAddToSystemLog( SRV_APPNAME, ODS_MSG_REGSVCHCTRL, EVENTLOG_ERROR_TYPE );
        return;
	}

    // report the status to the service control manager.
    //
	g_dwCurrentState = SERVICE_START_PENDING;
    if (!odsReportStatusToSCMgr( g_sshStatusHandle, 
			g_dwCurrentState, 0, 5000, NULL ))
        goto cleanup;

    ServiceStart( dwArgc, lpszArgv, &err );

cleanup:

	ServiceCleanup();

    // try to report the stopped status to the service control manager.
    //
	g_dwCurrentState = SERVICE_STOPPED;
    odsReportStatusToSCMgr( g_sshStatusHandle, 
                            g_dwCurrentState, err.nCode, 0, NULL );

    return;
}

void WINAPI service_ctrl(DWORD dwCtrlCode)
{
    // Handle the requested control code.
    //
    switch(dwCtrlCode)
    {
        // Stop the service.
        //
        case SERVICE_CONTROL_STOP:
		case SERVICE_CONTROL_SHUTDOWN:

			g_dwCurrentState = SERVICE_STOP_PENDING;
            odsReportStatusToSCMgr(g_sshStatusHandle, g_dwCurrentState, 0, 5000, NULL);
            ServiceStop();
            return;

        // Update the service status.
        //
        case SERVICE_CONTROL_INTERROGATE:
            break;

        // invalid control code
        //
        default:
            break;

    }    
	odsReportStatusToSCMgr(g_sshStatusHandle, g_dwCurrentState, 0, 5000, NULL);
}

void CmdInstallService()
{
    TCHAR szPath[512];
	STODSERROR err;

    if ( GetModuleFileName( NULL, szPath, 512 ) == 0 )
    {
        _tprintf(_T("Unable to install %s - can not get moudle file name\n"), SRV_SERVICEDISPLAYNAME );
        return;
    }

    if (!odsInstallService( NULL, SRV_SERVICENAME, szPath, SRV_SERVICEDISPLAYNAME, 
							SERVICE_DEMAND_START, SRV_DEPENDENCIES, &err) )
	{
		_tprintf(_T("(%d)%s\n"), err.nCode, err.strMsg );
		return;
	}
}

void CmdRemoveService()
{
	STODSERROR err;

	if (!odsRemoveService( NULL, SRV_SERVICENAME, 30000, &err ))
	{
		_tprintf(_T("(%d)%s\n"), err.nCode, err.strMsg );
		return;
	}    
}

void CmdDebugService(int argc, TCHAR ** argv)
{
	STODSERROR err;

    _tprintf(_T("Debugging %s.\n"), SRV_SERVICEDISPLAYNAME);

    SetConsoleCtrlHandler( ControlHandler, TRUE );

    ServiceStart( (DWORD) argc, argv, &err );

	ServiceCleanup();
}

BOOL WINAPI ControlHandler ( DWORD dwCtrlType )
{
    switch( dwCtrlType )
    {
        case CTRL_BREAK_EVENT:  // use Ctrl+C or Ctrl+Break to simulate
        case CTRL_C_EVENT:      // SERVICE_CONTROL_STOP in debug mode
            _tprintf(_T("Stopping %s.\n"), SRV_SERVICEDISPLAYNAME);
            ServiceStop();
            return TRUE;
            break;
    }
    return FALSE;
}

///////////////////////////////////////////////////////////////////////////////////////
// ServiceStart, ServiceCleanup and ServiceStop


static HANDLE hStopEvent;

void ServiceStart (DWORD dwArgc, LPTSTR *lpszArgv, STODSERROR* pstError)
{	
	// 服务初始化

	// 每一个初始化步骤前，都向服务控制器报告下一步操作最长要多长时间完成，
	// 以便服务控制器判断服务程序是否已失去响应
	if (!g_bDebug)
	{
		g_dwCurrentState = SERVICE_START_PENDING;
		if (!odsReportStatusToSCMgr(g_sshStatusHandle, g_dwCurrentState, 0, 5000, NULL ))            
			return;
	}

	// …………………………………………
	// 初始化终止事件
	hStopEvent = CreateEvent( NULL, FALSE, FALSE, _T("_=STOP=_") );
	

	// 进入服务程序的主流程

	// 向服务控制器报告服务程序已经处于正常运行的状态
	if (!g_bDebug)
	{
		g_dwCurrentState = SERVICE_RUNNING;
		if (!odsReportStatusToSCMgr(g_sshStatusHandle, g_dwCurrentState, 0, 5000, NULL ))            
			 return;
	}

	// ………………………………………… 

	// 下面是主流程的一个简单例子，每隔 10 秒就在 NT 的日志中写一条信息
	// 并检查退出标记，如果已发出终止服务的命令，就停止循环
	int i = 1;

	// 服务程序的主流程通常都是含有某个特定终止条件的无限循环结构
	for(;;)
	{
		// 如果 hStopEvent 被激活，则停止循环
		if ( WaitForSingleObject( hStopEvent, 10000 ) != WAIT_TIMEOUT )
			break;

		TCHAR tmp[100];
        _stprintf(tmp, _T("%d"), i++);
		odsAddToSystemLog( SRV_APPNAME, tmp, EVENTLOG_INFORMATION_TYPE );
	}    
     
	return;
}

void ServiceCleanup()
{
	// 做服务终止时的系统清理工作

	// 每一个清理步骤前，都向服务控制器报告下一步操作最长要多长时间完成，
	// 以便服务控制器判断服务程序是否已失去响应
	if (!g_bDebug)
	{
		g_dwCurrentState = SERVICE_STOP_PENDING;
		odsReportStatusToSCMgr(g_sshStatusHandle, g_dwCurrentState, 0, 5000, NULL );
	}    

	// …………………………………………
	CloseHandle( hStopEvent );
}

void ServiceStop()
{
    // 停止服务，在这里激活终止主流程的特定条件

	// …………………………………………
	SetEvent( hStopEvent );
}

