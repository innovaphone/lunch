/*-----------------------------------------------------------------------------------------------*/
/* Based on innovaphone App template                                                             */
/* copyright (c) innovaphone 2018 - 2023                                                         */
/*                                                                                               */
/*-----------------------------------------------------------------------------------------------*/

#include "platform/platform.h"
#include "common/os/iomux.h"
#include "common/interface/task.h"
#include "common/interface/http_client.h"
#include "common/interface/socket.h"
#include "common/interface/smtp.h"
#include "common/interface/webserver_plugin.h"
#include "common/interface/database.h"
#include "common/interface/httpfile.h"
#include "common/interface/json_api.h"
#include "common/interface/file.h"
#include "common/interface/time.h"
#include "common/interface/timezone.h"
#include "common/ilib/str.h"
#include "common/ilib/http_query_args.h"
#include "common/ilib/json.h"
#include "common/lib/appservice.h"
#include "common/lib/config.h"
#include "common/lib/tasks_postgresql.h"
#include "common/lib/appwebsocket.h"

#include "lunch_tasks.h"
#include "lunch.h"

using namespace AppLunch;

#define DBG(x) //debug->printf x

#define MS_ONE_DAY			24  * 60 * 60 * 1000	// one day in milliseconds

/*-----------------------------------------------------------------------------------------------*/
/* class LunchService                                                                        */
/*-----------------------------------------------------------------------------------------------*/

LunchService::LunchService(class IIoMux * const iomux, class ISocketProvider * localSocketProvider, class ISocketProvider * tcpSocketProvider, class ISocketProvider * tlsSocketProvider, IWebserverPluginProvider * const webserverPluginProvider, IDatabaseProvider * databaseProvider, class ISmtpProvider * smtpProvider, AppServiceArgs * args) : AppService(iomux, localSocketProvider, args)
{
    this->iomux = iomux;
    this->localSocketProvider = localSocketProvider;
	this->tcpSocketProvider = tcpSocketProvider;
	this->tlsSocketProvider = tlsSocketProvider;
    this->webserverPluginProvider = webserverPluginProvider;
	this->smtpProvider = smtpProvider;
    this->databaseProvider = databaseProvider;	
}

LunchService::~LunchService()
{
}

void LunchService::AppServiceApps(istd::list<AppServiceApp> * appList)
{
    appList->push_back(new AppServiceApp("innovaphone-lunch"));
}

void LunchService::AppInstancePlugins(istd::list<AppInstancePlugin> * pluginList)
{
}

class AppInstance * LunchService::CreateInstance(AppInstanceArgs * args)
{
    return new Lunch(iomux, this, args);
}

/*-----------------------------------------------------------------------------------------------*/
/* class Lunch                                                                               */
/*-----------------------------------------------------------------------------------------------*/

Lunch::Lunch(IIoMux * const iomux, class LunchService * service, AppInstanceArgs * args) : AppInstance(service, args),
taskConfigInit(this, &Lunch::TaskConfigInitFinished, &Lunch::TaskConfigInitFinished)
{
    this->stopping = false;
    this->iomux = iomux;
    this->service = service;
    this->webserverPlugin = service->webserverPluginProvider->CreateWebserverPlugin(iomux, service->localSocketProvider, this, args->webserver, args->webserverPath, this);
    this->database = service->databaseProvider->CreateDatabase(iomux, this, this);
    this->database->Connect(args->dbHost, args->dbName, args->dbUser, args->dbPassword);

	this->config = new LunchConfig(this);
	this->RegisterJsonApi(this->config);

    currentTask = nullptr;
    Log("App instance started");
}

Lunch::~Lunch()
{
	delete this->config;
}

void Lunch::DatabaseConnectComplete(IDatabase * const database)
{
    currentTask = new DatabaseInit(this, database);
}

void Lunch::DatabaseInitComplete()
{
    delete currentTask;
	currentTask = nullptr;

	if (this->stopping) {
		TryStop();
		return;
	}
	class ITask * configTask = this->config->CreateInitTask(database);
	configTask->Start(&taskConfigInit);
}

void Lunch::TaskConfigInitFinished(class ITask * task)
{
	delete task;

	webserverPlugin->HttpListen(nullptr, nullptr, nullptr, nullptr, _BUILD_STRING_);
	webserverPlugin->WebsocketListen();
	Log("App instance initialized");
}

void Lunch::LunchSessionClosed(class LunchSession * session)
{
    sessionList.remove(session);
    delete session;
    if (stopping) {
        TryStop();
    }
}

void Lunch::BroadcastMeals(class TaskEditMeals * task)
{
	char buffer[WS_MAX_DATA_SIZE];
	char buff[WS_MAX_DATA_SIZE / 2];
	char * tmp = buff;
	class json_io send(buffer);
	word base = send.add_object(JSON_ID_ROOT, NULL);
	send.add_string(base, "mt", "MealsEdited");
	word a = send.add_array(base, "meals");
	EditMealList * list = task->GetList();
	for (auto item : *list) {
		word o = send.add_object(a, nullptr);
		send.add_ulong64(o, "timestamp", item->GetTimestamp(), tmp);
		send.add_string(o, "veggy", item->GetVeggy());
		send.add_string(o, "veggy_type", item->GetVeggyType());
		send.add_string(o, "meat", item->GetMeat());
		send.add_string(o, "meat_type", item->GetMeatType());
		send.add_string(o, "meal3", item->GetMeal3());
		send.add_string(o, "meal3_type", item->GetMeal3Type());
		send.add_string(o, "meal4", item->GetMeal4());
		send.add_string(o, "meal4_type", item->GetMeal4Type());
	}
	this->BroadcastMessage(send, buffer);
}

void Lunch::BroadcastDailies(class TaskEditDailies * task)
{
	char buffer[WS_MAX_DATA_SIZE];
	class json_io send(buffer);
	word base = send.add_object(JSON_ID_ROOT, NULL);
	send.add_string(base, "mt", "DailiesEdited");
	word a = send.add_array(base, "dailies");
	DailyList * list = task->GetList();
	for (auto item : *list) {
		word o = send.add_object(a, nullptr);
		send.add_string(o, "day", item->GetDay());
		send.add_string(o, "meal", item->GetMeal());
	}
	this->BroadcastMessage(send, buffer);
}

void Lunch::BroadcastMessage(class json_io & json, char * buffer)
{
	for (auto session : sessionList) {
		session->SendMessage(json, buffer);
	}
}

void Lunch::DatabaseShutdown(IDatabase * const database, db_error_t reason)
{
	delete this->config;
	this->config = nullptr;
    this->database = nullptr;
    TryStop();
}

void Lunch::DatabaseError(IDatabase * const database, db_error_t error)
{

}

void Lunch::ServerCertificateUpdate(const byte * cert, size_t certLen)
{
    Log("Lunch::ServerCertificateUpdate cert:%x certLen:%u", cert, certLen);
}

void Lunch::WebserverPluginWebsocketListenResult(IWebserverPlugin * plugin, const char * path, const char * registeredPathForRequest, const char * host)
{
    if (!this->stopping) {
        class LunchSession * session = new LunchSession(this);
        this->sessionList.push_back(session);
    }
    else {
        plugin->Cancel(wsr_cancel_type_t::WSP_CANCEL_UNAVAILABLE);
    }
}

void Lunch::WebserverPluginHttpListenResult(IWebserverPlugin * plugin, ws_request_type_t requestType, char * resourceName, const char * registeredPathForRequest, ulong64 dataSize)
{
    if (requestType == WS_REQUEST_GET) {
		if (strncasecmp("/csv-month", resourceName, strlen("/csv-month")) == 0) {
			if (this->database && this->database->Connected()) {
				ulong64 from = 0, to = 0;
				char * month = nullptr;
				for (HttpQueryArgs args(resourceName); args.Current(); args.Next()) {
					HttpQueryArg * arg = args.Current();
					if (!strcmp(arg->name, "from")) {
						from = strtoull(arg->value, nullptr, 10);
					}
					else if (!strcmp(arg->name, "to")) {
						to = strtoull(arg->value, nullptr, 10);
					}
					else if (!strcmp(arg->name, "m")) {
						month = arg->value;
					}
				}
				new class GetMonthCsv(this->database, plugin, from, to, month);
			}
			else {
				plugin->Cancel(WSP_CANCEL_UNAVAILABLE);
			}
			return;
		}
		else if (strncasecmp("/csv-range", resourceName, strlen("/csv-range")) == 0) {
			if (this->database && this->database->Connected()) {
				ulong64 from = 0, to = 0;
				for (HttpQueryArgs args(resourceName); args.Current(); args.Next()) {
					HttpQueryArg * arg = args.Current();
					if (!strcmp(arg->name, "from")) {
						from = strtoull(arg->value, nullptr, 10);
					}
					else if (!strcmp(arg->name, "to")) {
						to = strtoull(arg->value, nullptr, 10);
					}
				}
				new class GetRangeCsv(this->database, plugin, from, to);
			}
			else {
				plugin->Cancel(WSP_CANCEL_UNAVAILABLE);
			}
			return;
		}
        else if (plugin->BuildRedirect(resourceName, _BUILD_STRING_, strlen(_BUILD_STRING_))) {
            return;
        }
    }
    plugin->Cancel(WSP_CANCEL_NOT_FOUND);
}

void Lunch::WebserverPluginClose(IWebserverPlugin * plugin, wsp_close_reason_t reason, bool lastUser)
{
    Log("WebserverPlugin closed");
    if (lastUser) {
        delete webserverPlugin;
        webserverPlugin = nullptr;
        TryStop();
    }
}

void Lunch::OnConfigChanged(LunchConfig * config)
{
	for (auto i = this->sessionList.begin(); i != this->sessionList.end(); ++i) {
		(*i)->OnConfigChanged(config);
	}
}

void Lunch::Stop()
{
    TryStop();
}

void Lunch::TryStop()
{
    if (!stopping) {
        Log("App instance stopping");
        stopping = true;
		if (webserverPlugin) webserverPlugin->Close();
        if (database) database->Shutdown();
		if (!sessionList.empty()) {
			for (std::list<LunchSession *>::iterator it = sessionList.begin(); it != sessionList.end(); ++it) {
				(*it)->Close();
			}
		}
		if (currentTask) currentTask->Stop();
    }
    
    if (!webserverPlugin && !database && sessionList.empty() && !currentTask) appService->AppStopped(this);
}

/*-----------------------------------------------------------------------------------------------*/
/* class LunchSession                                                                            */
/*-----------------------------------------------------------------------------------------------*/

LunchSession::LunchSession(class Lunch * instance) : AppWebsocket(instance->webserverPlugin, instance, instance),
	taskRemoveLunch(this, &LunchSession::TaskRemoveLunchFinished, &LunchSession::TaskRemoveLunchFinished),
	taskAddLunch(this, &LunchSession::TaskAddLunchFinished, &LunchSession::TaskAddLunchFinished),
	taskGetLunchs(this, &LunchSession::TaskGetLunchsFinished, &LunchSession::TaskGetLunchsFinished),
	taskRemoveDailyOrder(this, &LunchSession::TaskRemoveDailyOrderFinished, &LunchSession::TaskRemoveDailyOrderFinished),
	taskAddDailyOrder(this, &LunchSession::TaskAddDailyOrderFinished, &LunchSession::TaskAddDailyOrderFinished),
	taskGetDailyOrders(this, &LunchSession::TaskGetDailyOrdersFinished, &LunchSession::TaskGetDailyOrdersFinished),
	taskRemovePanini(this, &LunchSession::TaskRemovePaniniFinished, &LunchSession::TaskRemovePaniniFinished),
	taskAddPanini(this, &LunchSession::TaskAddPaniniFinished, &LunchSession::TaskAddPaniniFinished),
	taskGetPaninis(this, &LunchSession::TaskGetPaninisFinished, &LunchSession::TaskGetPaninisFinished),
	taskGetMeals(this, &LunchSession::TaskGetMealsFinished, &LunchSession::TaskGetMealsFinished),
	taskEditMeals(this, &LunchSession::TaskEditMealsFinished, &LunchSession::TaskEditMealsFinished),
	taskGetDailies(this, &LunchSession::TaskGetDailiesFinished, &LunchSession::TaskGetDailiesFinished),
	taskEditDailies(this, &LunchSession::TaskEditDailiesFinished, &LunchSession::TaskEditDailiesFinished)
{
    this->instance = instance;
	
	this->currentTask = nullptr;
	this->currentSrc = nullptr;
	this->closed = false;

	this->closing = false;
	this->admin = false;
}

LunchSession::~LunchSession()
{
}

void LunchSession::AppWebsocketMessage(class json_io & msg, word base, const char * mt, const char * src)
{
    if (currentSrc) free(currentSrc);
    currentSrc = src ? _strdup(src) : nullptr;
	if (!strcmp(mt, "GetAdmin")) {
		char buffer[8192];
		char buf[128];
		char * tmp = buf;
		class json_io send(buffer);
		word base = send.add_object(JSON_ID_ROOT, NULL);
		send.add_string(base, "mt", "GetAdminResult");
		send.add_bool(base, "admin", this->admin);
		send.add_unsigned(base, "orderWindow", this->instance->config->OrderWindow()->Value(), tmp);
		SendMessage(send, buffer, true);
	}
    else if (!strcmp(mt, "AddLunch")) {
		const char * sip = msg.get_string(base, "sip");
		ulong64 timestamp = msg.get_ulong64(base, "timestamp");
		enum MealType type = TaskDatabaseInit::CharTypeToEnum(msg.get_string(base, "type"));
		bool byAdmin = msg.get_bool(base, "byAdmin");
		if (!byAdmin || !this->admin) {
			long64 ts = ITime::TimeStampMilliseconds();
			long64 tsLunchEnter = GetLunchEnterDate((long64)timestamp);
			if (ts >= tsLunchEnter) {
				SendMessage("AddLunchResult", "errorEnterOver");
				return;
			}
		}
		if (sip && *sip && timestamp > 0) {
			this->currentTask = new class TaskAddLunch(this->instance->database, sip, timestamp, type);
			this->currentTask->Start(&taskAddLunch);
		}
		else {
			AppWebsocketMessageComplete();
		}
    }
	else if (!strcmp(mt, "RemoveLunch")) {
		const char * sip = msg.get_string(base, "sip");
		enum MealType type = TaskDatabaseInit::CharTypeToEnum(msg.get_string(base, "type"));
		ulong64 timestamp = msg.get_ulong64(base, "timestamp");
		bool byAdmin = msg.get_bool(base, "byAdmin");
		if (!byAdmin || !this->admin) {
			long64 ts = ITime::TimeStampMilliseconds();
			long64 tsLunchCancel = GetLunchCancelDate((long64)timestamp);
			if (ts >= tsLunchCancel) {
				SendMessage("RemoveLunchResult", "errorCancelOver");
				return;
			}
			/*if (!sendMail) {	// check timestamp on server side ...
				long64 tsLunchEnter = GetLunchEnterDate((long64)timestamp);
				if (ts >= tsLunchEnter) {
					sendMail = true;	// we're already after the official enter date, so send an e-mail
				}
			}*/
		}
		if (sip && *sip && timestamp > 0) {
			this->currentTask = new class TaskRemoveLunch(this->instance->service->iomux, 
				this->instance->service->tcpSocketProvider, 
				this->instance->service->tlsSocketProvider, 
				this->instance->database,
				this->instance,
				sip, timestamp, type);
			this->currentTask->Start(&taskRemoveLunch);
		}
		else {
			AppWebsocketMessageComplete();
		}
	}
	else if (!strcmp(mt, "GetDailyOrders")) {
		ulong64 from = msg.get_ulong64(base, "from");
		ulong64 to = msg.get_ulong64(base, "to");
		if (!to || !from) {
			AppWebsocketMessageComplete();
		}
		else {
			this->currentTask = new class TaskGetDailyOrders(this->instance->database, from, to);
			this->currentTask->Start(&taskGetDailyOrders);
		}
	}
	else if (!strcmp(mt, "AddDailyOrder")) {
		const char * sip = msg.get_string(base, "sip");
		ulong64 timestamp = msg.get_ulong64(base, "timestamp");
		bool byAdmin = msg.get_bool(base, "byAdmin");
		if (!byAdmin || !this->admin) {
			long64 ts = ITime::TimeStampMilliseconds();
			long64 tsLunchEnter = GetLunchEnterDate((long64)timestamp);
			if (ts >= tsLunchEnter) {
				SendMessage("AddDailyOrderResult", "errorEnterOver");
				return;
			}
		}
		if (sip && *sip && timestamp > 0) {
			this->currentTask = new class TaskAddDailyOrder(this->instance->database, sip, timestamp);
			this->currentTask->Start(&taskAddDailyOrder);
		}
		else {
			AppWebsocketMessageComplete();
		}
	}
	else if (!strcmp(mt, "RemoveDailyOrder")) {
		const char * sip = msg.get_string(base, "sip");
		ulong64 timestamp = msg.get_ulong64(base, "timestamp");
		bool byAdmin = msg.get_bool(base, "byAdmin");
		if (!byAdmin || !this->admin) {
			long64 ts = ITime::TimeStampMilliseconds();
			long64 tsLunchCancel = GetLunchCancelDate((long64)timestamp);
			if (ts >= tsLunchCancel) {
				SendMessage("RemoveDailyOrderResult", "errorCancelOver");
				return;
			}
			/*if (!sendMail) {	// check timestamp on server side ...
				long64 tsLunchEnter = GetLunchEnterDate((long64)timestamp);
				if (ts >= tsLunchEnter) {
					sendMail = true;	// we're already after the official enter date, so send an e-mail
				}
			}*/
		}
		if (sip && *sip && timestamp > 0) {
			this->currentTask = new class TaskRemoveDailyOrder(this->instance->service->iomux,
				this->instance->service->tcpSocketProvider,
				this->instance->service->tlsSocketProvider,
				this->instance->database,
				this->instance,
				sip, timestamp);
			this->currentTask->Start(&taskRemoveDailyOrder);
		}
		else {
			AppWebsocketMessageComplete();
		}
	}
	else if (!strcmp(mt, "GetPaninis")) {
		ulong64 from = msg.get_ulong64(base, "from");
		ulong64 to = msg.get_ulong64(base, "to");
		if (!to || !from) {
			AppWebsocketMessageComplete();
		}
		else {
			this->currentTask = new class TaskGetPaninis(this->instance->database, from, to);
			this->currentTask->Start(&taskGetPaninis);
		}
	}
	else if (!strcmp(mt, "AddPanini")) {
		const char * sip = msg.get_string(base, "sip");
		ulong64 timestamp = msg.get_ulong64(base, "timestamp");
		bool byAdmin = msg.get_bool(base, "byAdmin");
		if (!byAdmin || !this->admin) {
			long64 ts = ITime::TimeStampMilliseconds();
			long64 tsLunchEnter = GetLunchEnterDate((long64)timestamp);
			if (ts >= tsLunchEnter) {
				SendMessage("AddPaniniResult", "errorEnterOver");
				return;
			}
		}
		if (sip && *sip && timestamp > 0) {
			this->currentTask = new class TaskAddPanini(this->instance->database, sip, timestamp);
			this->currentTask->Start(&taskAddPanini);
		}
		else {
			AppWebsocketMessageComplete();
		}
	}
	else if (!strcmp(mt, "RemovePanini")) {
		const char * sip = msg.get_string(base, "sip");
		ulong64 timestamp = msg.get_ulong64(base, "timestamp");
		bool byAdmin = msg.get_bool(base, "byAdmin");
		if (!byAdmin || !this->admin) {
			long64 ts = ITime::TimeStampMilliseconds();
			long64 tsLunchCancel = GetLunchCancelDate((long64)timestamp);
			if (ts >= tsLunchCancel) {
				SendMessage("RemovePaniniResult", "errorCancelOver");
				return;
			}
			/*if (!sendMail) {	// check timestamp on server side ...
				long64 tsLunchEnter = GetLunchEnterDate((long64)timestamp);
				if (ts >= tsLunchEnter) {
					sendMail = true;	// we're already after the official enter date, so send an e-mail
				}
			}*/
		}
		if (sip && *sip && timestamp > 0) {
			this->currentTask = new class TaskRemovePanini(this->instance->service->iomux,
				this->instance->service->tcpSocketProvider,
				this->instance->service->tlsSocketProvider,
				this->instance->database,
				this->instance,
				sip, timestamp);
			this->currentTask->Start(&taskRemovePanini);
		}
		else {
			AppWebsocketMessageComplete();
		}
	}
	else if (!strcmp(mt, "GetLunchs")) {
		ulong64 from = msg.get_ulong64(base, "from");
		ulong64 to = msg.get_ulong64(base, "to");
		if (!to || !from) {
			AppWebsocketMessageComplete();
		}
		else {
			this->currentTask = new class TaskGetLunchs(this->instance->database, from, to);
			this->currentTask->Start(&taskGetLunchs);
		}
	}
	else if (!strcmp(mt, "GetMeals")) {
		ulong64 from = msg.get_ulong64(base, "from");
		ulong64 to = msg.get_ulong64(base, "to");
		if (!to || !from) {
			AppWebsocketMessageComplete();
		}
		else {
			this->currentTask = new class TaskGetMeals(this->instance->database, from, to);
			this->currentTask->Start(&taskGetMeals);
		}
	}
	else if (!strcmp(mt, "EditMeals") && this->admin) {	// only admins are allowed to edit meals
		this->currentTask = new class TaskEditMeals(this->instance->iomux, this->instance->database, msg, base);
		this->currentTask->Start(&taskEditMeals);
	}
	else if (!strcmp(mt, "GetDailies")) {
		this->currentTask = new class TaskGetDailies(this->instance->database);
		this->currentTask->Start(&taskGetDailies);
	}
	else if (!strcmp(mt, "EditDailies") && this->admin) {	// only admins are allowed to edit meals
		this->currentTask = new class TaskEditDailies(this->instance->database, msg, base);
		this->currentTask->Start(&taskEditDailies);
	}
    else {
        AppWebsocketMessageComplete();
    }
}

long64 LunchSession::GetLunchCancelDate(long64 tsLunch)
{
	return this->GetLunchEnterDate(tsLunch);
}

long64 LunchSession::GetLunchEnterDate(long64 tsLunch)
{
	//return tsLunch + (MS_ONE_DAY / 2) + (MS_ONE_DAY / 12);
	time_tm_t tm;
	ITime::GetTimeStruct(tsLunch, &tm);
	if (tm.tmWDay == 1) {				// check if lunch is on monday
		tsLunch -= (2 * MS_ONE_DAY);	// remove two days weekend
	}
	/*else if (tm.tmWDay == 2) {			// check if lunch is on tuesday
		tsLunch -= (3 * MS_ONE_DAY);	// remove three days weekend+monday
	}*/
	tsLunch -= (long64)((MS_ONE_DAY / 24) * (this->instance->config->OrderWindow()->Value() / 60.0));		// allow just up to 11.30 o'clock of previous day (-12.5 hours)
	return tsLunch;
}

void LunchSession::AppWebsocketAppInfo(const char * app, class json_io & msg, word base)
{

}

bool LunchSession::AppWebsocketApiPermission(const char * api)
{
	if ((this->admin && !strcmp(api, "Config"))) {
		return true;
	}
	return false;
}

bool LunchSession::AppWebsocketConnectComplete(class json_io & msg, word info)
{
	if (info != JSON_ID_NONE) {
		const char * appObj = msg.get_string(info, "appobj");
		if (appObj) {
			const char * tilde = strchr(appObj, '~');
			if (tilde) {
				++tilde;
				if (strcmp(tilde, "admin") == 0) {
					this->admin = true;
				}
			}
		}
	}
	return true;
}

void LunchSession::AppWebsocketClosed()
{
    closed = true;
    TryClose();
}

void LunchSession::TaskRemoveLunchFinished(class TaskRemoveLunch * task)
{
	this->instance->Log("LunchSession::TaskRemoveLunchFinished task->error:%u this->closing:%u this->closed:%u", task->error, this->closing, this->closed);
	this->currentTask = nullptr;
	if (this->closing) {
		TryClose();
	}
	else if (task->error) {
		SendMessage("RemoveLunchResult", "errorDatabase");
	}
	else {
		char buffer[8192];
		char buf[128];
		char * tmp = buf;
		class json_io send(buffer);
		word base = send.add_object(JSON_ID_ROOT, NULL);
		send.add_string(base, "mt", "LunchRemoved");
		send.add_string(base, "sip", task->GetSip());
		send.add_string(base, "type", TaskDatabaseInit::EnumTypeToChar(task->GetType()));
		send.add_ulong64(base, "timestamp", task->GetTimestamp(), tmp);
		instance->BroadcastMessage(send, buffer);

		send.reset();
		base = send.add_object(JSON_ID_ROOT, NULL);
		send.add_string(base, "mt", "RemoveLunchResult");
		send.add_string(base, "src", this->currentSrc);
		send.add_string(base, "sip", task->GetSip());
		send.add_string(base, "type", TaskDatabaseInit::EnumTypeToChar(task->GetType()));
		SendMessage(send, buffer, true);
	}
	delete task;
}

void LunchSession::TaskAddLunchFinished(class TaskAddLunch * task)
{
	this->currentTask = nullptr;
	if (this->closing) {
		TryClose();
	}
	else if (task->error) {
		SendMessage("AddLunchResult", "errorDatabase");
	}
	else {
		char buffer[8192];
		char buf[128];
		char * tmp = buf;
		class json_io send(buffer);
		word base = JSON_ID_ROOT;
		if (task->GetRemovedSip() != nullptr) {
			base = send.add_object(JSON_ID_ROOT, NULL);
			send.add_string(base, "mt", "LunchRemoved");
			send.add_string(base, "sip", task->GetRemovedSip());
			send.add_ulong64(base, "timestamp", task->GetRemovedTimestamp(), tmp);
			instance->BroadcastMessage(send, buffer);
			send.reset();
		}
		base = send.add_object(JSON_ID_ROOT, NULL);
		send.add_string(base, "mt", "LunchAdded");
		send.add_string(base, "sip", task->GetSip());
		send.add_string(base, "type", TaskDatabaseInit::EnumTypeToChar(task->GetType()));
		send.add_ulong64(base, "timestamp", task->GetTimestamp(), tmp);
		instance->BroadcastMessage(send, buffer);

		send.reset();
		base = send.add_object(JSON_ID_ROOT, NULL);
		send.add_string(base, "mt", "AddLunchResult");
		send.add_string(base, "src", this->currentSrc);
		send.add_string(base, "sip", task->GetSip());
		send.add_string(base, "type", TaskDatabaseInit::EnumTypeToChar(task->GetType()));
		SendMessage(send, buffer, true);
	}
	delete task;
}

void LunchSession::TaskGetLunchsFinished(class TaskGetLunchs * task)
{
	this->currentTask = nullptr;
	if (this->closing) {
		TryClose();
	}
	else if (task->error) {
		SendMessage("GetLunchsResult", "errorDatabase");
	}
	else {
		char buffer[WS_MAX_DATA_SIZE];
		class json_io send(buffer);
		word base = send.add_object(JSON_ID_ROOT, NULL);
		send.add_string(base, "mt", "GetLunchsResult");
		send.add_string(base, "src", this->currentSrc);
		const char * lunchsJson = task->GetJson();
		if (lunchsJson && *lunchsJson) {
			send.add_json(base, "lunchs", task->GetJson());
		}
		this->SendMessage(send, buffer, true);
	}
	delete task;
}

void LunchSession::TaskRemoveDailyOrderFinished(class TaskRemoveDailyOrder * task)
{
	this->instance->Log("LunchSession::TaskRemoveDailyOrderFinished task->error:%u this->closing:%u this->closed:%u", task->error, this->closing, this->closed);
	this->currentTask = nullptr;
	if (this->closing) {
		TryClose();
	}
	else if (task->error) {
		SendMessage("RemoveDailyOrderResult", "errorDatabase");
	}
	else {
		char buffer[8192];
		char buf[128];
		char * tmp = buf;
		class json_io send(buffer);
		word base = send.add_object(JSON_ID_ROOT, NULL);
		send.add_string(base, "mt", "DailyOrderRemoved");
		send.add_string(base, "sip", task->GetSip());
		send.add_ulong64(base, "timestamp", task->GetTimestamp(), tmp);
		instance->BroadcastMessage(send, buffer);

		SendMessage("RemoveDailyOrderResult"); // send answer after broadcasts
	}
	delete task;
}

void LunchSession::TaskAddDailyOrderFinished(class TaskAddDailyOrder * task)
{
	this->currentTask = nullptr;
	if (this->closing) {
		TryClose();
	}
	else if (task->error) {
		SendMessage("AddDailyOrderResult", "errorDatabase");
	}
	else {
		char buffer[8192];
		char buf[128];
		char * tmp = buf;
		class json_io send(buffer);
		word base = JSON_ID_ROOT;
		base = send.add_object(JSON_ID_ROOT, NULL);
		send.add_string(base, "mt", "DailyOrderAdded");
		send.add_string(base, "sip", task->GetSip());
		send.add_ulong64(base, "timestamp", task->GetTimestamp(), tmp);
		instance->BroadcastMessage(send, buffer);

		SendMessage("AddDailyOrderResult");	// send answer after broadcasts
	}
	delete task;
}

void LunchSession::TaskGetDailyOrdersFinished(class TaskGetDailyOrders * task)
{
	this->currentTask = nullptr;
	if (this->closing) {
		TryClose();
	}
	else if (task->error) {
		SendMessage("GetDailyOrdersResult", "errorDatabase");
	}
	else {
		char buffer[WS_MAX_DATA_SIZE];
		class json_io send(buffer);
		word base = send.add_object(JSON_ID_ROOT, NULL);
		send.add_string(base, "mt", "GetDailyOrdersResult");
		send.add_string(base, "src", this->currentSrc);
		const char * dailyOrdersJson = task->GetJson();
		if (dailyOrdersJson && *dailyOrdersJson) {
			send.add_json(base, "dailyOrders", task->GetJson());
		}
		this->SendMessage(send, buffer, true);
	}
	delete task;
}

void LunchSession::TaskRemovePaniniFinished(class TaskRemovePanini * task)
{
	this->instance->Log("LunchSession::TaskRemovePaniniFinished task->error:%u this->closing:%u this->closed:%u", task->error, this->closing, this->closed);
	this->currentTask = nullptr;
	if (this->closing) {
		TryClose();
	}
	else if (task->error) {
		SendMessage("RemovePaniniResult", "errorDatabase");
	}
	else {
		char buffer[8192];
		char buf[128];
		char * tmp = buf;
		class json_io send(buffer);
		word base = send.add_object(JSON_ID_ROOT, NULL);
		send.add_string(base, "mt", "PaniniRemoved");
		send.add_string(base, "sip", task->GetSip());
		send.add_ulong64(base, "timestamp", task->GetTimestamp(), tmp);
		instance->BroadcastMessage(send, buffer);

		SendMessage("RemovePaniniResult"); // send answer after broadcasts
	}
	delete task;
}

void LunchSession::TaskAddPaniniFinished(class TaskAddPanini * task)
{
	this->currentTask = nullptr;
	if (this->closing) {
		TryClose();
	}
	else if (task->error) {
		SendMessage("AddPaniniResult", "errorDatabase");
	}
	else {
		char buffer[8192];
		char buf[128];
		char * tmp = buf;
		class json_io send(buffer);
		word base = JSON_ID_ROOT;
		base = send.add_object(JSON_ID_ROOT, NULL);
		send.add_string(base, "mt", "PaniniAdded");
		send.add_string(base, "sip", task->GetSip());
		send.add_ulong64(base, "timestamp", task->GetTimestamp(), tmp);
		instance->BroadcastMessage(send, buffer);

		SendMessage("AddPaniniResult");	// send answer after broadcasts
	}
	delete task;
}

void LunchSession::TaskGetPaninisFinished(class TaskGetPaninis * task)
{
	this->currentTask = nullptr;
	if (this->closing) {
		TryClose();
	}
	else if (task->error) {
		SendMessage("GetPaninisResult", "errorDatabase");
	}
	else {
		char buffer[WS_MAX_DATA_SIZE];
		class json_io send(buffer);
		word base = send.add_object(JSON_ID_ROOT, NULL);
		send.add_string(base, "mt", "GetPaninisResult");
		send.add_string(base, "src", this->currentSrc);
		const char * paninisJson = task->GetJson();
		if (paninisJson && *paninisJson) {
			send.add_json(base, "paninis", task->GetJson());
		}
		this->SendMessage(send, buffer, true);
	}
	delete task;
}

void LunchSession::TaskGetMealsFinished(class TaskGetMeals * task)
{
	this->currentTask = nullptr;
	if (this->closing) {
		TryClose();
	}
	else if (task->error) {
		SendMessage("GetMealsResult", "errorDatabase");
	}
	else {
		char buffer[WS_MAX_DATA_SIZE];
		class json_io send(buffer);
		word base = send.add_object(JSON_ID_ROOT, NULL);
		send.add_string(base, "mt", "GetMealsResult");
		send.add_string(base, "src", this->currentSrc);
		const char * mealsJson = task->GetJson();
		if (mealsJson && *mealsJson) {
			send.add_json(base, "meals", task->GetJson());
		}
		this->SendMessage(send, buffer, true);
	}
	delete task;
}

void LunchSession::TaskEditMealsFinished(class TaskEditMeals * task)
{
	this->currentTask = nullptr;
	if (this->closing) {
		TryClose();
	}
	else if (task->error) {
		SendMessage("EditMealsResult", "errorDatabase");
	}
	else {
		SendMessage("EditMealsResult");
		instance->BroadcastMeals(task);
	}
	delete task;
}

void LunchSession::TaskGetDailiesFinished(class TaskGetDailies * task)
{
	this->currentTask = nullptr;
	if (this->closing) {
		TryClose();
	}
	else if (task->error) {
		SendMessage("GetDailiesResult", "errorDatabase");
	}
	else {
		char buffer[WS_MAX_DATA_SIZE];
		class json_io send(buffer);
		word base = send.add_object(JSON_ID_ROOT, NULL);
		send.add_string(base, "mt", "GetDailiesResult");
		send.add_string(base, "src", this->currentSrc);
		const char * dailiesJson = task->GetJson();
		if (dailiesJson && *dailiesJson) {
			send.add_json(base, "dailies", task->GetJson());
		}
		this->SendMessage(send, buffer, true);
	}
	delete task;
}

void LunchSession::TaskEditDailiesFinished(class TaskEditDailies * task)
{
	this->currentTask = nullptr;
	if (this->closing) {
		TryClose();
	}
	else if (task->error) {
		SendMessage("EditDailiesResult", "errorDatabase");
	}
	else {
		SendMessage("EditDailiesResult");
		instance->BroadcastDailies(task);
	}
	delete task;
}


void LunchSession::SendMessage(const char * mt, const char * error)
{
	char buffer[8192];
	class json_io send(buffer);
	word base = send.add_object(JSON_ID_ROOT, NULL);
	send.add_string(base, "mt", mt);
	send.add_string(base, "src", this->currentSrc);
	if (error != nullptr) {
		send.add_string(base, "error", error);
	}
	SendMessage(send, buffer, true);
}

void LunchSession::SendMessage(class json_io & json, char * buffer, bool recvNext)
{
	AppWebsocketMessageSend(json, buffer);
	if (recvNext) {
		AppWebsocketMessageComplete();
	}
}

void LunchSession::SendMessage(class json_io & json, char * buffer)
{
	this->SendMessage(json, buffer, false);
}

void LunchSession::OnConfigChanged(class LunchConfig * config)
{

}

void LunchSession::TryClose()
{
    if (!closed) {
        this->AppWebsocketClose();
        return;
    }
    if (closed && !currentTask) {
        instance->LunchSessionClosed(this);
    }
}

void LunchSession::Close()
{
	closing = true;
	TryClose();
}


/*-----------------------------------------------------------------------------------------------*/
/* class LunchConfig                                                                             */
/*-----------------------------------------------------------------------------------------------*/

LunchConfig::LunchConfig(class Lunch * instance)
	: ConfigContext(instance->database, instance),
	appInstance(instance),
	orderWindow(this, "orderWindow", 750, false, 0, 2880)
{
}

LunchConfig::~LunchConfig()
{
}


void LunchConfig::ConfigChanged()
{
	this->appInstance->OnConfigChanged(this);
}
