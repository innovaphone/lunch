
/*-----------------------------------------------------------------------------------------------*/
/* Based on innovaphone App template                                                             */
/* copyright (c) innovaphone 2018                                                                */
/*                                                                                               */
/*-----------------------------------------------------------------------------------------------*/

namespace AppLunch {

    class LunchService : public AppService {
        class AppInstance * CreateInstance(AppInstanceArgs * args) override;
        void AppServiceApps(istd::list<AppServiceApp> * appList) override;
        void AppInstancePlugins(istd::list<AppInstancePlugin> * pluginList) override;

    public:
        LunchService(class IIoMux * const iomux, class ISocketProvider * localSocketProvider, class ISocketProvider * tcpSocketProvider, class ISocketProvider * tlsSocketProvider, IWebserverPluginProvider * const webserverPluginProvider, IDatabaseProvider * databaseProvider, class ISmtpProvider * smtpProvider, AppServiceArgs * args);
        ~LunchService();

        class IIoMux * iomux;
        class ISocketProvider * localSocketProvider;
        class ISocketProvider * tcpSocketProvider;
        class ISocketProvider * tlsSocketProvider;
        class IWebserverPluginProvider * webserverPluginProvider;
        class IDatabaseProvider * databaseProvider;
        class ISmtpProvider * smtpProvider;
    };

    class Lunch : public AppInstance, public JsonApiContext, public UDatabase, public UWebserverPlugin
    {
        UTaskTemplate<Lunch, class ITask> taskConfigInit;

        void DatabaseConnectComplete(IDatabase * const database) override;
        void DatabaseShutdown(IDatabase * const database, db_error_t reason) override;
        void DatabaseError(IDatabase * const database, db_error_t error) override;

        void TaskConfigInitFinished(class ITask * task);

        void WebserverPluginClose(IWebserverPlugin * plugin, wsp_close_reason_t reason, bool lastUser) override;
        void WebserverPluginWebsocketListenResult(IWebserverPlugin * plugin, const char * path, const char * registeredPathForRequest, const char * host) override;
        void WebserverPluginHttpListenResult(IWebserverPlugin * plugin, ws_request_type_t requestType, char * resourceName, const char * registeredPathForRequest, ulong64 dataSize) override;

        void ServerCertificateUpdate(const byte * cert, size_t certLen) override;

        void Stop() override;

        void TryStop();

        bool stopping;
        class ITask * currentTask;
        std::list<class LunchSession *> sessionList;

    public:
        Lunch(IIoMux * const iomux, LunchService * service, AppInstanceArgs * args);
        ~Lunch();

        void DatabaseInitComplete();
        void LunchSessionClosed(class LunchSession * session);
        void BroadcastMeals(class TaskEditMeals * task);
        void BroadcastDailies(class TaskEditDailies * task);
        void BroadcastMessage(class json_io & json, char * buffer);
        void OnConfigChanged(class LunchConfig * config);

        const char * appPwd() { return args.appPassword; };

        class IIoMux * iomux;
        class LunchService * service;
        class IWebserverPlugin * webserverPlugin;
        class IDatabase * database;
        class LunchConfig * config;
    };

    class DatabaseInit : public TaskDatabaseInit {
        class Lunch * instance;
    public:
        DatabaseInit(class Lunch * instance, IDatabase * database) : TaskDatabaseInit(database) {
            this->instance = instance;
            Start(nullptr);
        }
        void Complete() { instance->DatabaseInitComplete(); };
    };

    class LunchSession : public AppWebsocket {
        UTaskTemplate<LunchSession, class TaskRemoveLunch> taskRemoveLunch;
        UTaskTemplate<LunchSession, class TaskAddLunch> taskAddLunch;
        UTaskTemplate<LunchSession, class TaskGetLunchs> taskGetLunchs;
        UTaskTemplate<LunchSession, class TaskRemoveDailyOrder> taskRemoveDailyOrder;
        UTaskTemplate<LunchSession, class TaskAddDailyOrder> taskAddDailyOrder;
        UTaskTemplate<LunchSession, class TaskGetDailyOrders> taskGetDailyOrders;
        UTaskTemplate<LunchSession, class TaskRemovePanini> taskRemovePanini;
        UTaskTemplate<LunchSession, class TaskAddPanini> taskAddPanini;
        UTaskTemplate<LunchSession, class TaskGetPaninis> taskGetPaninis;
        UTaskTemplate<LunchSession, class TaskGetMeals> taskGetMeals;
        UTaskTemplate<LunchSession, class TaskEditMeals> taskEditMeals;
        UTaskTemplate<LunchSession, class TaskGetDailies> taskGetDailies;
        UTaskTemplate<LunchSession, class TaskEditDailies> taskEditDailies;

        void AppWebsocketAccept(class UWebsocket * uwebsocket) { instance->webserverPlugin->WebsocketAccept(uwebsocket); };
        char * AppWebsocketPassword() override { return (char *)instance->appPwd(); };
        void AppWebsocketMessage(class json_io & msg, word base, const char * mt, const char * src) override;
        bool AppWebsocketApiPermission(const char * api) override;
        void AppWebsocketAppInfo(const char * app, class json_io & msg, word base) override;
        bool AppWebsocketConnectComplete(class json_io & msg, word info) override;
        void AppWebsocketSendResult() override {};
        void AppWebsocketClosed() override;

        long64 GetLunchEnterDate(long64 tsLunch);
        long64 GetLunchCancelDate(long64 tsLunch);

        void TaskRemoveLunchFinished(class TaskRemoveLunch * task);
        void TaskAddLunchFinished(class TaskAddLunch * task);
        void TaskGetLunchsFinished(class TaskGetLunchs * task);
        void TaskRemoveDailyOrderFinished(class TaskRemoveDailyOrder * task);
        void TaskAddDailyOrderFinished(class TaskAddDailyOrder * task);
        void TaskGetDailyOrdersFinished(class TaskGetDailyOrders * task);
        void TaskRemovePaniniFinished(class TaskRemovePanini * task);
        void TaskAddPaniniFinished(class TaskAddPanini * task);
        void TaskGetPaninisFinished(class TaskGetPaninis * task);
        void TaskGetMealsFinished(class TaskGetMeals * task);
        void TaskEditMealsFinished(class TaskEditMeals * task);
        void TaskGetDailiesFinished(class TaskGetDailies * task);
        void TaskEditDailiesFinished(class TaskEditDailies * task);

        void TryClose();

        void SendMessage(const char * mt, const char * error = nullptr);
        void SendMessage(class json_io & json, char * buffer, bool recvNext);

        bool closed;
        bool closing;
        bool admin;

    public:
        LunchSession(class Lunch * instance);
        ~LunchSession();

        class Lunch * instance;
        char * currentSrc;
        class ITask * currentTask;
        void Close();
        void SendMessage(class json_io & json, char * buffer);
        void OnConfigChanged(class LunchConfig * config);
    };

    class LunchConfig : public ConfigContext {
    private:
        class Lunch * appInstance = nullptr;

        ConfigDword orderWindow;    // in minutes

    protected:
        void ConfigChanged() override;

    public:
        LunchConfig(class Lunch * instance);
        virtual ~LunchConfig();

        const ConfigDword * OrderWindow() { return &this->orderWindow; }
    };

}
