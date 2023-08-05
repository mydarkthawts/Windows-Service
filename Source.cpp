/*
COPYRIGHT:          Copyleft (C) 2023

ORIGINAL AUTHOR(S): https://github.com/mydarkthawts

LICENSE:            Open Source, just give credit where it is due and pay it forward.

DESCRIPTION:        A C++ Windows Service code template with controllable features
                    using nothing but Windows API calls and no third party libraries.
                    This service is coded to log events to the Windows Event Viewer.
                    It's easy to read and adapt to future projects depending on use 
                    case scenarios.

                    It will accept parameters to install, uninstall, stop, and start
                    without needing to use the SC commands to register the service. 
                    Both methods work with this Windows Service, installing from the
                    service program directly in the Command Prompt will set the 
                    service to automatically run on system boot up.


NOTES:              After building the project, open a command prompt with
                    administrative privileges. To control this service, you can
                    pass the program parameters given below in the command prompt.
                    
                    - To install the service: C:\path\to\MyWindowsService.exe install
                      - Now, the service will run on boot automatically.
                    - Start the service: C:\path\to\MyWindowsService.exe start
                    - Stop the service: C:\path\to\MyWindowsService.exe stop
                    - Uninstall the service: C:\path\to\MyWindowsService.exe uninstall
                    - Print this note section: C:\path\to\MyWindowsService.exe help

                    OR you can use the sc commands given below in the command prompt.

                    - Install the service: 
                      sc create "My Service" binPath= "C:\path\to\YourService.exe"
                    - Uninstall the service: sc delete "My Service"
                    - Start the service: sc start "My Service"
                    - Stop the service: sc stop "My Service"

                    Change the TEXT("MyWindowsService") to your own.
*/

#include <Windows.h>
#include <tchar.h>
#include <string>

SERVICE_STATUS        g_ServiceStatus = { 0 };
SERVICE_STATUS_HANDLE g_StatusHandle = NULL;
HANDLE                g_ServiceStopEvent = INVALID_HANDLE_VALUE;
HANDLE                g_EventSource = NULL;

// Function prototypes
void ReportStatus(DWORD currentState, DWORD win32ExitCode, DWORD waitHint);
void ReportEventToEventViewer(const TCHAR* message, WORD eventType);
void ServiceMain(DWORD argc, LPTSTR* argv);
void ControlHandler(DWORD request);
int InitService();
int InstallService();
int UninstallService();
int HelpPrompt();

#define SERVICE_NAME_MAX_LEN 256

int _tmain(int argc, LPTSTR* argv)
{
    // If the service is being installed or uninstalled
    if (argc > 1)
    {
        if (_tcscmp(argv[1], TEXT("install")) == 0)
        {
            return InstallService();
        }
        else if (_tcscmp(argv[1], TEXT("uninstall")) == 0)
        {
            return UninstallService();
        }
        else if (_tcscmp(argv[1], TEXT("help")) == 0)
        {
            return HelpPrompt();
        }
        else
        {
            _tprintf(TEXT("Invalid command.\n"));
            return -1;
        }
    }

    // If not installing or uninstalling, start the service
    TCHAR szServiceName[SERVICE_NAME_MAX_LEN];
    _tcscpy_s(szServiceName, SERVICE_NAME_MAX_LEN, TEXT("MyWindowsService"));

    // Register the event source for the service
    g_EventSource = RegisterEventSource(NULL, szServiceName);
    if (g_EventSource == NULL)
    {
        _tprintf(TEXT("Failed to register the event source. Error: %d\n"), GetLastError());
        return -1;
    }

    SERVICE_TABLE_ENTRY ServiceTable[] =
    {
        {szServiceName, (LPSERVICE_MAIN_FUNCTION)ServiceMain},
        {NULL, NULL}
    };

    if (StartServiceCtrlDispatcher(ServiceTable) == FALSE)
    {
        // If the service is not started from the SCM, it can be run as a console application here
        // Add the code to handle the service as a console application
        // For example: RunServiceAsConsole();
        // See Microsoft documentation for details on how to implement RunServiceAsConsole.
        // The service must be installed to be started from the SCM.
        return GetLastError();
    }

    // Deregister the event source after the service is stopped
    DeregisterEventSource(g_EventSource);

    return 0;
}

// Function to report the service status to the SCM
void ReportStatus(DWORD currentState, DWORD win32ExitCode, DWORD waitHint)
{
    static DWORD checkPoint = 1;

    g_ServiceStatus.dwCurrentState = currentState;
    g_ServiceStatus.dwWin32ExitCode = win32ExitCode;
    g_ServiceStatus.dwWaitHint = waitHint;

    if (currentState == SERVICE_START_PENDING)
    {
        g_ServiceStatus.dwControlsAccepted = 0;
    }
    else
    {
        g_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    }

    if ((currentState == SERVICE_RUNNING) || (currentState == SERVICE_STOPPED))
    {
        g_ServiceStatus.dwCheckPoint = 0;
    }
    else
    {
        g_ServiceStatus.dwCheckPoint = checkPoint++;
    }

    SetServiceStatus(g_StatusHandle, &g_ServiceStatus);
}

// Function to report an event to the Windows Event Viewer
void ReportEventToEventViewer(const TCHAR* message, WORD eventType)
{
    const TCHAR* messageStrings[1] = { message };
    ReportEvent(g_EventSource, eventType, 0, 0, NULL, 1, 0, messageStrings, NULL);
}

// ServiceMain function - Entry point for the service
void ServiceMain(DWORD argc, LPTSTR* argv)
{
    // Register the service control handler
    g_StatusHandle = RegisterServiceCtrlHandler(argv[0], ControlHandler);
    if (g_StatusHandle == NULL)
    {
        // If registration fails, report the error event to the Event Viewer
        ReportEventToEventViewer(TEXT("Service control handler registration failed."), EVENTLOG_ERROR_TYPE);
        return;
    }

    // Initialize the service status
    g_ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    g_ServiceStatus.dwServiceSpecificExitCode = 0;

    // Report that the service is in the starting state
    ReportStatus(SERVICE_START_PENDING, NO_ERROR, 3000);

    // Perform service initialization
    if (InitService() != 0)
    {
        // If initialization fails, report the error event to the Event Viewer
        ReportEventToEventViewer(TEXT("Service initialization failed."), EVENTLOG_ERROR_TYPE);
        ReportStatus(SERVICE_STOPPED, NO_ERROR, 0);
        return;
    }

    // Initialization successful, report the informational event to the Event Viewer
    ReportEventToEventViewer(TEXT("Service initialized successfully."), EVENTLOG_INFORMATION_TYPE);

    // Report the service as running
    ReportStatus(SERVICE_RUNNING, NO_ERROR, 0);

    while (WaitForSingleObject(g_ServiceStopEvent, 0) != WAIT_OBJECT_0)
    {
        // Service logic goes here.
        // Replace the Sleep with your service's main functionality.
        Sleep(1000);
    }

    // Report the informational event to the Event Viewer
    ReportEventToEventViewer(TEXT("Service stopped."), EVENTLOG_INFORMATION_TYPE);

    // Report the service as stopped
    ReportStatus(SERVICE_STOPPED, NO_ERROR, 0);
}

// ControlHandler function - Handles service control requests (e.g., stop)
void ControlHandler(DWORD request)
{
    switch (request)
    {
    case SERVICE_CONTROL_STOP:
        ReportStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);
        SetEvent(g_ServiceStopEvent);
        ReportStatus(g_ServiceStatus.dwCurrentState, NO_ERROR, 0);
        return;

    case SERVICE_CONTROL_INTERROGATE:
        // For the interrogate control request, do nothing (status will be queried)
        break;

    default:
        // For other control requests, do nothing
        break;
    }
}

// InitService function - Perform service-specific initialization here
int InitService()
{
    // Initialization code for your service can be added here.
    // Return 0 on success, non-zero on failure.
    // You can create a separate thread here to run your service.

    g_ServiceStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (g_ServiceStopEvent == NULL)
    {
        return -1;
    }

    return 0;
}

// Function to install the service
int InstallService()
{
    SC_HANDLE schSCManager, schService;
    TCHAR szPath[MAX_PATH];

    // Get the path of the current executable
    if (!GetModuleFileName(NULL, szPath, MAX_PATH))
    {
        _tprintf(TEXT("GetModuleFileName failed, error: %d\n"), GetLastError());
        return -1;
    }

    // Open the Service Control Manager (SCM) database
    schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (schSCManager == NULL)
    {
        _tprintf(TEXT("OpenSCManager failed, error: %d\n"), GetLastError());
        return -1;
    }

    // Create the service with the given service name, display name, and executable path
    schService = CreateService(schSCManager, TEXT("MyWindowsService"), TEXT("My Windows Service"),
        SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS, SERVICE_AUTO_START, SERVICE_ERROR_NORMAL,
        szPath, NULL, NULL, NULL, NULL, NULL);

    if (schService == NULL)
    {
        _tprintf(TEXT("CreateService failed, error: %d\n"), GetLastError());
        CloseServiceHandle(schSCManager);
        return -1;
    }

    // The service is now installed and can be started using the Service Control Manager (SCM).
    // The service will start automatically at system boot (SERVICE_AUTO_START).

    _tprintf(TEXT("Service installed successfully.\n"));

    // Close the service and the Service Control Manager handles
    CloseServiceHandle(schService);
    CloseServiceHandle(schSCManager);

    return 0;
}

// Function to uninstall the service
int UninstallService()
{
    SC_HANDLE schSCManager, schService;
    SERVICE_STATUS ssSvcStatus;

    // Open the Service Control Manager (SCM) database
    schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (schSCManager == NULL)
    {
        _tprintf(TEXT("OpenSCManager failed, error: %d\n"), GetLastError());
        return -1;
    }

    // Open the service with SERVICE_ALL_ACCESS rights
    schService = OpenService(schSCManager, TEXT("MyWindowsService"), SERVICE_ALL_ACCESS);
    if (schService == NULL)
    {
        _tprintf(TEXT("OpenService failed, error: %d\n"), GetLastError());
        CloseServiceHandle(schSCManager);
        return -1;
    }

    // Send a stop control to the service
    if (ControlService(schService, SERVICE_CONTROL_STOP, &ssSvcStatus))
    {
        _tprintf(TEXT("Stopping service...\n"));
        Sleep(1000);

        // Wait for the service to stop
        while (QueryServiceStatus(schService, &ssSvcStatus))
        {
            if (ssSvcStatus.dwCurrentState == SERVICE_STOP_PENDING)
            {
                _tprintf(TEXT("."));
                Sleep(1000);
            }
            else
            {
                break;
            }
        }

        if (ssSvcStatus.dwCurrentState == SERVICE_STOPPED)
        {
            _tprintf(TEXT("\nService stopped successfully.\n"));
        }
        else
        {
            _tprintf(TEXT("\nService could not be stopped.\n"));
        }
    }

    // Delete the service from the SCM database
    if (!DeleteService(schService))
    {
        _tprintf(TEXT("DeleteService failed, error: %d\n"), GetLastError());
        CloseServiceHandle(schService);
        CloseServiceHandle(schSCManager);
        return -1;
    }

    // The service is now uninstalled and removed from the Service Control Manager (SCM).

    _tprintf(TEXT("Service uninstalled successfully.\n"));

    // Close the service and the Service Control Manager handles
    CloseServiceHandle(schService);
    CloseServiceHandle(schSCManager);

    return 0;
}

// Help prompt
int HelpPrompt()
{
    _tprintf(TEXT("To control this service, you can pass the program parameters\n"
    "given below in the command prompt with Admin Permissions.\n"
    "\n"
    "- To install the service: C:\\path\\to\\MyWindowsService.exe install\n"
    "  - Now, the service will run on boot automatically.\n"
    "- Start the service : C:\\path\\to\\MyWindowsService.exe start\n"
    "- Stop the service : C:\\path\\to\\MyWindowsService.exe stop\n"
    "- Uninstall the service : C:\\path\\to\\MyWindowsService.exe uninstall\n"
    "- Print this note section: C:\\path\\to\\MyWindowsService.exe help\n"
    "\n"
    "OR you can use the sc commands given below in the command prompt.\n"
    "\n"
    "- Install the service :\n"
    "sc create \"My Service\" binPath = \"C:\\path\\to\\YourService.exe\"\n"
    "- Uninstall the service : sc delete \"My Service\"\n"
    "- Start the service : sc start \"My Service\"\n"
    "- Stop the service : sc stop \"My Service\"\n"));
    return 0;
}
