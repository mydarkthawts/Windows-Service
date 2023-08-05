# Windows Service Template C++
## After building the project, open a command prompt with
## administrative privileges. To control this service, you can
## pass the program parameters given below in the command prompt.
## 
## - To install the service: C:\path\to\MyWindowsService.exe install
##   - Now, the service will run on boot automatically.
## - Start the service: C:\path\to\MyWindowsService.exe start
## - Stop the service: C:\path\to\MyWindowsService.exe stop
## - Uninstall the service: C:\path\to\MyWindowsService.exe uninstall
## - Print this note section: C:\path\to\MyWindowsService.exe help
## 
## OR you can use the sc commands given below in the command prompt.
## 
## - Install the service: 
##   sc create "My Service" binPath= "C:\path\to\YourService.exe"
## - Uninstall the service: sc delete "My Service"
## - Start the service: sc start "My Service"
## - Stop the service: sc stop "My Service"

## Change the TEXT("MyWindowsService") to your own.