
/*-----------------------------------------------------------------------------------------------*/
/* Based on innovaphone App template                                                             */
/* copyright (c) innovaphone 2016                                                                */
/*                                                                                               */
/*-----------------------------------------------------------------------------------------------*/

#include "platform/platform.h"
#include "common/build/release.h"
#include "common/os/iomux.h"
#include "common/interface/task.h"
#include "common/interface/socket.h"
#include "common/interface/http_client.h"
#include "common/interface/webserver_plugin.h"
#include "common/interface/database.h"
#include "common/interface/json_api.h"
#include "common/interface/random.h"
#include "common/interface/smtp.h"
#include "common/lib/appservice.h"
#include "common/lib/config.h"
#include "common/lib/tasks_postgresql.h"
#include "common/lib/appwebsocket.h"
#include "common/lib/app_updates.h"
#include "common/lib/badgecount_signaling.h"
#include "lunch/lunch_tasks.h"
#include "lunch/lunch.h"

using namespace AppLunch;

int main(int argc, char *argv[])
{
    IRandom::Init(time(nullptr));
    class IIoMux * iomux = IIoMux::Create();
	class ISocketProvider * localSocketProvider = CreateLocalSocketProvider();
	class ISocketProvider * tcpSocketProvider = CreateTCPSocketProvider();
	class ISocketProvider * tlsSocketProvider = CreateTLSSocketProvider(tcpSocketProvider);
	class IWebserverPluginProvider * webserverPluginProvider = CreateWebserverPluginProvider();
	class IDatabaseProvider * databaseProvider = CreatePostgreSQLDatabaseProvider();
	class ISmtpProvider * smtpProvider = CreateSmtpProvider();

    AppServiceArgs  serviceArgs;
    serviceArgs.serviceID = "lunch";
    serviceArgs.Parse(argc, argv);
    AppInstanceArgs instanceArgs;
    instanceArgs.appName = "lunch";
    instanceArgs.appDomain = "example.com";
    instanceArgs.appPassword = "pwd";
    instanceArgs.webserver = "/var/run/webserver/webserver";
    instanceArgs.webserverPath = "/lunch";
    instanceArgs.dbHost = "";
    instanceArgs.dbName = "lunch";
    instanceArgs.dbUser = "lunch";
    instanceArgs.dbPassword = "lunch";
    instanceArgs.Parse(argc, argv);
	
    LunchService * service = new LunchService(iomux, localSocketProvider, tcpSocketProvider, tlsSocketProvider, webserverPluginProvider, databaseProvider, smtpProvider, &serviceArgs);
    if (!serviceArgs.manager) service->AppStart(&instanceArgs);
    iomux->Run();

    delete service;
	delete smtpProvider;
	delete localSocketProvider;
	delete tlsSocketProvider;
	delete tcpSocketProvider;
    delete webserverPluginProvider;
    delete iomux;
    delete debug;
    
    return 0;
}
