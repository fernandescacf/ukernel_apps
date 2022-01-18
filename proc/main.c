#include <stdio.h>
#include <dispatch.h>

#include <connection.h>
#include <proc.h>
#include <proc_io.h>

#define PROC_SERVER_PATH    "/proc"
#define SERVER_BUFFER_SIZE  2048
#define SERVER_TASKS        1

int32_t ProcServerStart()
{
    // Set connection handler close call back
    ConnectionSetCloseCallBack(_io_CloseCallBack);

    // Install Server Dispatcher to handle client messages
    dispatch_attr_t attr = { 0x0, SERVER_BUFFER_SIZE, SERVER_BUFFER_SIZE, SERVER_TASKS };
    ctrl_funcs_t ctrl_funcs = { ConnectionAttach, ConnectionDetach, NULL };
    io_funcs_t io_funcs = { _io_ProcInfo,  _io_FileRead, _io_FileWrite, _io_FileOpen,     _io_FileClose,
                            _io_FileShare, NULL,         _io_FileSeek,  _io_FileTruncate, NULL
                          };
    dispatch_t* disp = DispatcherAttach(PROC_SERVER_PATH, &attr, &io_funcs, &ctrl_funcs);

    // Sart the dispatcher, will not return
    DispatcherStart(disp,  TRUE);

    return E_OK;
}

int main()
{
    StdOpen();

    ProcFileSystemBuild();

    ProcServerStart();

    StdClose();

    return 0;
}