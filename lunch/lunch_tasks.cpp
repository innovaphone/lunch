
/*-----------------------------------------------------------------------------------------------*/
/* Based on innovaphone App template                                                             */
/* copyright (c) innovaphone 2018                                                                */
/*                                                                                               */
/*-----------------------------------------------------------------------------------------------*/

#include "platform/platform.h"
#include "common/os/iomux.h"
#include "common/ilib/json.h"
#include "common/interface/database.h"
#include "common/interface/http_client.h"
#include "common/interface/task.h"
#include "common/interface/file.h"
#include "common/interface/socket.h"
#include "common/interface/process.h"
#include "common/interface/time.h"
#include "common/interface/webserver_plugin.h"
#include "common/lib/tasks_postgresql.h"
#include "common/ilib/uri.h"
#include "common/ilib/str.h"
#include "common/lib/database_switch.h"
#include "lunch_tasks.h"

using namespace AppLunch;

/*---------------------------------------------------------------------------*/
/* DevicesDatabaseHandler                                                    */
/*---------------------------------------------------------------------------*/

TaskDatabaseInit::TaskDatabaseInit(IDatabase * database)
	: initLunchs(database, "lunchs"),
	initMeals(database, "meals"),
	initDailies(database, "dailies"),
	initPaninis(database, "paninis"),
	initDailyOrders(database, "daily_orders"),
	initLunchTypeEnum(database, "lunch_type_t")
{
	this->database = database;
	this->tableCheckLunchs = false;
	this->counter = 0;

	for (int i = 0; i < MEAL_TYPE_LAST; i++) {
		initLunchTypeEnum.AddValue(TaskDatabaseInit::EnumTypeToChar((MealType)i));
	}

	// meals
	initMeals.AddColumn("id", "BIGSERIAL PRIMARY KEY NOT NULL");
	initMeals.AddColumn("timestamp", "BIGINT UNIQUE NOT NULL");
	initMeals.AddColumn("meat", "TEXT NOT NULL");
	initMeals.AddColumn("meat_type", "VARCHAR");
	initMeals.AddColumn("veggy", "TEXT NOT NULL");
	initMeals.AddColumn("veggy_type", "VARCHAR");
	initMeals.AddColumn("meal3", "TEXT");
	initMeals.AddColumn("meal3_type", "VARCHAR");
	initMeals.AddColumn("meal4", "TEXT");
	initMeals.AddColumn("meal4_type", "VARCHAR");
	initMeals.AddIndex("meals_timestamp_index", "timestamp", false);

	// dailies
	initDailies.AddColumn("day", "TEXT PRIMARY KEY NOT NULL");
	initDailies.AddColumn("meal", "TEXT NOT NULL");
	
    // lunchs
    initLunchs.AddColumn("id", "BIGSERIAL PRIMARY KEY NOT NULL");
    initLunchs.AddColumn("sip", "VARCHAR(256) NOT NULL");
    initLunchs.AddColumn("timestamp", "BIGINT NOT NULL");
	initLunchs.AddColumn("veggy", "BOOLEAN DEFAULT true");	// leave this column for legacy reasons, replaced by enum type
	initLunchs.AddColumn("type", "lunch_type_t DEFAULT NULL");
	initLunchs.AddConstraint("lunch_unique", "UNIQUE(sip,timestamp)");
	initLunchs.AddIndex("lunchs_sip_index", "sip", false);
	initLunchs.AddIndex("lunchs_sip_lower_index", "LOWER(sip)", false);
	initLunchs.AddIndex("lunchs_timestamp_index", "timestamp", false);

	// paninies
	initPaninis.AddColumn("id", "BIGSERIAL PRIMARY KEY NOT NULL");
	initPaninis.AddColumn("sip", "VARCHAR(256) NOT NULL");
	initPaninis.AddColumn("timestamp", "BIGINT NOT NULL");
	initPaninis.AddConstraint("paninies_unique", "UNIQUE(sip,timestamp)");
	initPaninis.AddIndex("paninies_sip_index", "sip", false);
	initPaninis.AddIndex("paninies_sip_lower_index", "LOWER(sip)", false);
	initPaninis.AddIndex("paninies_timestamp_index", "timestamp", false);

	// daily_orders
	initDailyOrders.AddColumn("id", "BIGSERIAL PRIMARY KEY NOT NULL");
	initDailyOrders.AddColumn("sip", "VARCHAR(256) NOT NULL");
	initDailyOrders.AddColumn("timestamp", "BIGINT NOT NULL");
	initDailyOrders.AddConstraint("daily_orders_unique", "UNIQUE(sip,timestamp)");
	initDailyOrders.AddIndex("daily_orders_sip_index", "sip", false);
	initDailyOrders.AddIndex("daily_orders_sip_lower_index", "LOWER(sip)", false);
	initDailyOrders.AddIndex("daily_orders_timestamp_index", "timestamp", false);
}

TaskDatabaseInit::~TaskDatabaseInit()
{
}

void TaskDatabaseInit::Start(class UTask * user)
{
    this->user = user;
	initMeals.Start(this);
}

void TaskDatabaseInit::TaskComplete(class ITask * const task)
{
	if (task == &initMeals) initLunchTypeEnum.Start(this);
	else if (task == &initLunchTypeEnum) initDailies.Start(this);
	else if (task == &initDailies) initLunchs.Start(this);
	else if (task == &initLunchs) initPaninis.Start(this);
	else if (task == &initPaninis) initDailyOrders.Start(this);
	else if (task == &initDailyOrders) {
		this->database->ExecSQL(this, DB_EXEC_FETCH_ALL, "SELECT id FROM lunchs WHERE type IS NULL LIMIT 1");
	}
    else ASSERT(false, "TaskDatabaseInit::TaskComplete");
}

void TaskDatabaseInit::TaskFailed(class ITask * const task)
{
    Failed();
}

void TaskDatabaseInit::DatabaseError(IDatabase * const database, db_error_t error)
{
	Failed();
}

void TaskDatabaseInit::DatabaseExecSQLResult(IDatabase * const database, class IDataSet * dataset)
{
	if (!this->tableCheckLunchs) {
		if (dataset->Eot()) {
			// nothing to upgrade
			Complete();
		}
		else {
			this->database->ExecSQL(this, DB_EXEC_FETCH_ALL, "ALTER TABLE lunchs ALTER COLUMN veggy DROP NOT NULL");
			this->database->ExecSQL(this, DB_EXEC_FETCH_ALL, "ALTER TABLE lunchs ALTER COLUMN veggy SET DEFAULT true");
			this->database->ExecSQL(this, DB_EXEC_FETCH_ALL, "UPDATE lunchs SET type=%s WHERE type IS NULL AND veggy=false", TaskDatabaseInit::EnumTypeToChar(MealType::MEAL_TYPE_MEAT));
			this->database->ExecSQL(this, DB_EXEC_FETCH_ALL, "UPDATE lunchs SET type=%s WHERE type IS NULL AND veggy=true", TaskDatabaseInit::EnumTypeToChar(MealType::MEAL_TYPE_VEGGY));
			this->counter = 4;
		}
		this->tableCheckLunchs = true;
	}
	else {
		if (--this->counter == 0) {
			Complete();
		}
	}
	delete dataset;
}

enum MealType TaskDatabaseInit::CharTypeToEnum(const char * type)
{
	if (type) {
		if (strcmp(type, "MEAT") == 0) return MEAL_TYPE_MEAT;
		else if (strcmp(type, "VEGGY") == 0) return MEAL_TYPE_VEGGY;
		else if (strcmp(type, "MEAL3") == 0) return MEAL_TYPE_MEAL3;
		else if (strcmp(type, "MEAL4") == 0) return MEAL_TYPE_MEAL4;
		else {
			debug->printf("TaskDatabaseInit::CharTypeToEnum invalid type %s", type);
			return MEAL_TYPE_LAST;
		}
	}
	return MEAL_TYPE_LAST;
}

const char * TaskDatabaseInit::EnumTypeToChar(enum MealType type)
{
	switch (type)
	{
	case MEAL_TYPE_MEAT: return "MEAT";
	case MEAL_TYPE_VEGGY: return "VEGGY";
	case MEAL_TYPE_MEAL3: return "MEAL3";
	case MEAL_TYPE_MEAL4: return "MEAL4";
	default:
		debug->printf("TaskDatabaseInit::EnumTypeToChar invalid type %u", type);
		return "VEGGY";
	}
}

/*---------------------------------------------------------------------------*/
/* TaskAddLunch                                                              */
/*---------------------------------------------------------------------------*/

TaskAddLunch::TaskAddLunch(class IDatabase * database, const char * sip, ulong64 timestamp, enum MealType type)
{
	this->database = database;
	this->sip = _strdup(sip);
	this->timestamp = timestamp;
	this->type = type;
	this->removing = false;
	this->removed = nullptr;
}
	
TaskAddLunch::~TaskAddLunch()
{
	free(this->sip);
	free(this->removed);
}

void TaskAddLunch::Start(class UTask * user)
{
	this->user = user;
	this->database->BeginTransaction(this);
	const char * plus = strstr(this->sip, "+");
	if (plus) {
		this->removing = true;
		char * sipPlus = (char *)alloca(strlen(this->sip) + 1);
		_sprintf(sipPlus, "%.*s", (int)(plus - this->sip + 1), this->sip);
		this->database->ExecSQL(this, DB_EXEC_FETCH_ALL, "DELETE FROM lunchs WHERE sip LIKE '%e%%' RETURNING sip, timestamp", sipPlus);
	}
	this->database->ExecSQL(this, DB_EXEC_FETCH_ALL, "INSERT INTO lunchs (sip, timestamp, type) VALUES (%s,%llu,%s) ON CONFLICT(sip,timestamp) DO NOTHING", this->sip, this->timestamp, TaskDatabaseInit::EnumTypeToChar(this->type));
	this->database->EndTransaction(this);
}

void TaskAddLunch::DatabaseBeginTransactionResult(IDatabase * const database)
{

}

void TaskAddLunch::DatabaseError(IDatabase * const database, db_error_t error) 
{
	this->user->TaskFailed(this);
}

void TaskAddLunch::DatabaseExecSQLResult(IDatabase * const database, class IDataSet * dataset)
{
	if (this->removing) {
		this->removing = false;
		if (!dataset->Eot()) {
			this->removed = _strdup(dataset->GetStringValue(0));
			this->removedTimestamp = dataset->GetULong64Value(1);
		}
	}
	delete dataset;
}

void TaskAddLunch::DatabaseEndTransactionResult(IDatabase * const database)
{
	this->user->TaskComplete(this);
}

/*---------------------------------------------------------------------------*/
/* TaskRemoveLunch                                                           */
/*---------------------------------------------------------------------------*/

TaskRemoveLunch::TaskRemoveLunch(class IIoMux * iomux, class ISocketProvider * tcpSocketProvider, class ISocketProvider * tlsSocketProvider,
	class IDatabase * database, class IInstanceLog * log, const char * sip, ulong64 timestamp, enum MealType type)
{
	this->iomux = iomux;
	this->tcpSocketProvider = tcpSocketProvider;
	this->tlsSocketProvider = tlsSocketProvider;
	this->database = database;
	this->log = log;
	this->sip = _strdup(sip);
	this->timestamp = timestamp;
	this->type = type;
}

TaskRemoveLunch::~TaskRemoveLunch()
{
	free(this->sip);
}

void TaskRemoveLunch::Start(class UTask * user)
{
	this->user = user;
	this->StartQuery();
}

void TaskRemoveLunch::StartQuery() {
	this->database->ExecSQL(this, DB_EXEC_FETCH_ALL, "DELETE FROM lunchs WHERE sip=%s AND timestamp=%llu AND type=%s", this->sip, this->timestamp, TaskDatabaseInit::EnumTypeToChar(this->type));
}

void TaskRemoveLunch::DatabaseError(IDatabase * const database, db_error_t error)
{
	this->user->TaskFailed(this);
}

void TaskRemoveLunch::DatabaseExecSQLResult(IDatabase * const database, class IDataSet * dataset)
{
	delete dataset;
	this->user->TaskComplete(this);
}

/*---------------------------------------------------------------------------*/
/* TaskGetLunchs		                                                     */
/*---------------------------------------------------------------------------*/

TaskGetLunchs::TaskGetLunchs(class IDatabase * database, ulong64 from, ulong64 to)
{
	this->database = database;
	this->json = nullptr;
	this->from = from;
	this->to = to;
}

TaskGetLunchs::~TaskGetLunchs()
{
}

void TaskGetLunchs::Start(class UTask * user)
{
	this->user = user;
	this->database->ExecSQL(this, DB_EXEC_FETCH_ALL, "SELECT array_to_json(array_agg(b)) FROM (" \
		"SELECT l1.timestamp, (SELECT array_to_json(array_agg(t)) " \
			"FROM(SELECT l.sip, l.type " \
				"FROM lunchs l WHERE l.timestamp = l1.timestamp " \
				"ORDER BY l.veggy, LOWER(l.sip) ASC) t) AS eaters " \
		"FROM lunchs l1 " \
		"WHERE l1.timestamp>=%llu AND l1.timestamp<=%llu GROUP BY l1.timestamp) b", this->from, this->to);
}

void TaskGetLunchs::DatabaseError(IDatabase * const database, db_error_t error)
{
	this->user->TaskFailed(this);
}

void TaskGetLunchs::DatabaseExecSQLResult(IDatabase * const database, class IDataSet * dataset)
{
	if (!dataset->Eot()) {
		this->json = _strdup(dataset->GetStringValue(0));
	}
	delete dataset;
	this->user->TaskComplete(this);
}


/*---------------------------------------------------------------------------*/
/* TaskAddDailyOrder                                                         */
/*---------------------------------------------------------------------------*/

TaskAddDailyOrder::TaskAddDailyOrder(class IDatabase * database, const char * sip, ulong64 timestamp)
{
	this->database = database;
	this->sip = _strdup(sip);
	this->timestamp = timestamp;
}

TaskAddDailyOrder::~TaskAddDailyOrder()
{
	free(this->sip);
}

void TaskAddDailyOrder::Start(class UTask * user)
{
	this->user = user;
	this->database->ExecSQL(this, DB_EXEC_FETCH_ALL, "INSERT INTO daily_orders (sip, timestamp) VALUES (%s,%llu) ON CONFLICT(sip,timestamp) DO NOTHING", this->sip, this->timestamp);
}

void TaskAddDailyOrder::DatabaseError(IDatabase * const database, db_error_t error)
{
	this->user->TaskFailed(this);
}

void TaskAddDailyOrder::DatabaseExecSQLResult(IDatabase * const database, class IDataSet * dataset)
{
	delete dataset;
	this->user->TaskComplete(this);
}

/*---------------------------------------------------------------------------*/
/* TaskRemoveDailyOrder                                                      */
/*---------------------------------------------------------------------------*/

TaskRemoveDailyOrder::TaskRemoveDailyOrder(class IIoMux * iomux, class ISocketProvider * tcpSocketProvider, class ISocketProvider * tlsSocketProvider,
	class IDatabase * database, class IInstanceLog * log, const char * sip, ulong64 timestamp)
{
	this->iomux = iomux;
	this->tcpSocketProvider = tcpSocketProvider;
	this->tlsSocketProvider = tlsSocketProvider;
	this->database = database;
	this->log = log;
	this->sip = _strdup(sip);
	this->timestamp = timestamp;	
}

TaskRemoveDailyOrder::~TaskRemoveDailyOrder()
{
	free(this->sip);
}

void TaskRemoveDailyOrder::Start(class UTask * user)
{
	this->user = user;
	this->StartQuery();
}

void TaskRemoveDailyOrder::StartQuery() {
	this->database->ExecSQL(this, DB_EXEC_FETCH_ALL, "DELETE FROM daily_orders WHERE sip=%s AND timestamp=%llu", this->sip, this->timestamp);
}

void TaskRemoveDailyOrder::DatabaseError(IDatabase * const database, db_error_t error)
{
	this->user->TaskFailed(this);
}

void TaskRemoveDailyOrder::DatabaseExecSQLResult(IDatabase * const database, class IDataSet * dataset)
{
	delete dataset;
	this->user->TaskComplete(this);
}

/*---------------------------------------------------------------------------*/
/* TaskGetDailyOrders		                                                 */
/*---------------------------------------------------------------------------*/

TaskGetDailyOrders::TaskGetDailyOrders(class IDatabase * database, ulong64 from, ulong64 to)
{
	this->database = database;
	this->json = nullptr;
	this->from = from;
	this->to = to;
}

TaskGetDailyOrders::~TaskGetDailyOrders()
{
}

void TaskGetDailyOrders::Start(class UTask * user)
{
	this->user = user;
	this->database->ExecSQL(this, DB_EXEC_FETCH_ALL, "SELECT array_to_json(array_agg(b)) FROM (" \
		"SELECT l1.timestamp, (SELECT array_to_json(array_agg(t)) " \
		"FROM(SELECT l.sip " \
		"FROM daily_orders l WHERE l.timestamp = l1.timestamp " \
		"ORDER BY LOWER(l.sip) ASC) t) AS eaters " \
		"FROM daily_orders l1 " \
		"WHERE l1.timestamp>=%llu AND l1.timestamp<=%llu GROUP BY l1.timestamp) b", this->from, this->to);
}

void TaskGetDailyOrders::DatabaseError(IDatabase * const database, db_error_t error)
{
	this->user->TaskFailed(this);
}

void TaskGetDailyOrders::DatabaseExecSQLResult(IDatabase * const database, class IDataSet * dataset)
{
	if (!dataset->Eot()) {
		this->json = _strdup(dataset->GetStringValue(0));
	}
	delete dataset;
	this->user->TaskComplete(this);
}


/*---------------------------------------------------------------------------*/
/* TaskAddPanini                                                             */
/*---------------------------------------------------------------------------*/

TaskAddPanini::TaskAddPanini(class IDatabase * database, const char * sip, ulong64 timestamp)
{
	this->database = database;
	this->sip = _strdup(sip);
	this->timestamp = timestamp;
}

TaskAddPanini::~TaskAddPanini()
{
	free(this->sip);
}

void TaskAddPanini::Start(class UTask * user)
{
	this->user = user;
	this->database->ExecSQL(this, DB_EXEC_FETCH_ALL, "INSERT INTO paninis (sip, timestamp) VALUES (%s,%llu) ON CONFLICT(sip,timestamp) DO NOTHING", this->sip, this->timestamp);
}

void TaskAddPanini::DatabaseError(IDatabase * const database, db_error_t error)
{
	this->user->TaskFailed(this);
}

void TaskAddPanini::DatabaseExecSQLResult(IDatabase * const database, class IDataSet * dataset)
{
	delete dataset;
	this->user->TaskComplete(this);
}

/*---------------------------------------------------------------------------*/
/* TaskRemovePanini                                                          */
/*---------------------------------------------------------------------------*/

TaskRemovePanini::TaskRemovePanini(class IIoMux * iomux, class ISocketProvider * tcpSocketProvider, class ISocketProvider * tlsSocketProvider,
	class IDatabase * database, class IInstanceLog * log, const char * sip, ulong64 timestamp)
{
	this->iomux = iomux;
	this->tcpSocketProvider = tcpSocketProvider;
	this->tlsSocketProvider = tlsSocketProvider;
	this->database = database;
	this->log = log;
	this->sip = _strdup(sip);
	this->timestamp = timestamp;	
}

TaskRemovePanini::~TaskRemovePanini()
{
	free(this->sip);
}

void TaskRemovePanini::Start(class UTask * user)
{
	this->user = user;
	this->StartQuery();
}

void TaskRemovePanini::StartQuery() {
	this->database->ExecSQL(this, DB_EXEC_FETCH_ALL, "DELETE FROM paninis WHERE sip=%s AND timestamp=%llu", this->sip, this->timestamp);
}

void TaskRemovePanini::DatabaseError(IDatabase * const database, db_error_t error)
{
	this->user->TaskFailed(this);
}

void TaskRemovePanini::DatabaseExecSQLResult(IDatabase * const database, class IDataSet * dataset)
{
	delete dataset;
	this->user->TaskComplete(this);
}

/*---------------------------------------------------------------------------*/
/* TaskGetPaninis		                                                     */
/*---------------------------------------------------------------------------*/

TaskGetPaninis::TaskGetPaninis(class IDatabase * database, ulong64 from, ulong64 to)
{
	this->database = database;
	this->json = nullptr;
	this->from = from;
	this->to = to;
}

TaskGetPaninis::~TaskGetPaninis()
{
}

void TaskGetPaninis::Start(class UTask * user)
{
	this->user = user;
	this->database->ExecSQL(this, DB_EXEC_FETCH_ALL, "SELECT array_to_json(array_agg(b)) FROM (" \
		"SELECT l1.timestamp, (SELECT array_to_json(array_agg(t)) " \
		"FROM(SELECT l.sip " \
		"FROM paninis l WHERE l.timestamp = l1.timestamp " \
		"ORDER BY LOWER(l.sip) ASC) t) AS eaters " \
		"FROM paninis l1 " \
		"WHERE l1.timestamp>=%llu AND l1.timestamp<=%llu GROUP BY l1.timestamp) b", this->from, this->to);
}

void TaskGetPaninis::DatabaseError(IDatabase * const database, db_error_t error)
{
	this->user->TaskFailed(this);
}

void TaskGetPaninis::DatabaseExecSQLResult(IDatabase * const database, class IDataSet * dataset)
{
	if (!dataset->Eot()) {
		this->json = _strdup(dataset->GetStringValue(0));
	}
	delete dataset;
	this->user->TaskComplete(this);
}

/*---------------------------------------------------------------------------*/
/* TaskGetMeals		                                                         */
/*---------------------------------------------------------------------------*/

TaskGetMeals::TaskGetMeals(class IDatabase * database, ulong64 from, ulong64 to)
{
	this->database = database;
	this->json = nullptr;
	this->from = from;
	this->to = to;
}

TaskGetMeals::~TaskGetMeals()
{
}

void TaskGetMeals::Start(class UTask * user)
{
	this->user = user;
	this->database->ExecSQL(this, DB_EXEC_FETCH_ALL, "SELECT array_to_json(array_agg(b)) FROM (" \
		"SELECT m.timestamp, m.meat, m.veggy, m.meal3, m.meal4, m.meat_type, m.veggy_type, m.meal3_type, m.meal4_type " \
		"FROM meals m " \
		"WHERE m.timestamp>=%llu AND m.timestamp<=%llu) b", this->from, this->to);
}

void TaskGetMeals::DatabaseError(IDatabase * const database, db_error_t error)
{
	this->user->TaskFailed(this);
}

void TaskGetMeals::DatabaseExecSQLResult(IDatabase * const database, class IDataSet * dataset)
{
	if (!dataset->Eot()) {
		this->json = _strdup(dataset->GetStringValue(0));
	}
	delete dataset;
	this->user->TaskComplete(this);
}


/*---------------------------------------------------------------------------*/
/* TaskEditMeals		                                                     */
/*---------------------------------------------------------------------------*/

TaskEditMeals::TaskEditMeals(class IIoMux * iomux, class IDatabase * database, class json_io & msg, word base)
{
	this->iomux = iomux;
	this->database = database;
	this->counter = 0;
	word a = msg.get_array(base, "lunchs");
	if (a != JSON_ID_NONE) {
		word last = 0;
		while (msg.get_object(a, last) != JSON_ID_NONE) {
			ulong64 timestamp = msg.get_ulong64(last, "timestamp");
			const char * veggy = msg.get_string(last, "veggy");
			const char * veggyType = msg.get_string(last, "veggyType");
			const char * meat = msg.get_string(last, "meat");
			const char * meatType = msg.get_string(last, "meatType");
			const char * meal3 = msg.get_string(last, "meal3");
			const char * meal3Type = msg.get_string(last, "meal3Type");
			const char * meal4 = msg.get_string(last, "meal4");
			const char * meal4Type = msg.get_string(last, "meal4Type");
			if (timestamp && veggy && meat && meal3 && meal4 && veggyType && meatType && meal3Type && meal4Type) {
				this->list.push_back(new class EditMeal(timestamp, veggy, veggyType, meat, meatType, meal3, meal3Type, meal4, meal4Type));
			}
		}
	}
}

TaskEditMeals::~TaskEditMeals()
{
	while (list.size() > 0) {
		class EditMeal * item = list.front();
		delete item;
		list.pop_front();
	}
}

void TaskEditMeals::AddMeal(class EditMeal * meal)
{
	this->list.push_back(meal);
}

void TaskEditMeals::Start(class UTask * user)
{
	this->user = user;
	if (this->list.size() == 0) {
		this->iomux->SetExec(this, nullptr);
	}
	else {
		for (auto item : this->list) {
			this->database->ExecSQL(this, DB_EXEC_FETCH_ALL, "INSERT INTO meals (timestamp,meat,veggy,meal3,meal4,meat_type,veggy_type,meal3_type,meal4_type) VALUES (%llu,%s,%s,%s,%s,%s,%s,%s,%s) " \
				"ON CONFLICT (timestamp) DO UPDATE SET meat=%s, veggy=%s, meal3=%s, meal4=%s, meat_type=%s, veggy_type=%s, meal3_type=%s, meal4_type=%s",
				item->GetTimestamp(), 
				item->GetMeat(), item->GetVeggy(), item->GetMeal3(), item->GetMeal4(), item->GetMeatType(), item->GetVeggyType(), item->GetMeal3Type(), item->GetMeal4Type(),
				item->GetMeat(), item->GetVeggy(), item->GetMeal3(), item->GetMeal4(), item->GetMeatType(), item->GetVeggyType(), item->GetMeal3Type(), item->GetMeal4Type());
		}
		this->counter = this->list.size();
	}
}

void TaskEditMeals::IoExec(void * execContext)
{
	this->user->TaskComplete(this);
}

void TaskEditMeals::DatabaseError(IDatabase * const database, db_error_t error)
{
	this->user->TaskFailed(this);
}

void TaskEditMeals::DatabaseExecSQLResult(IDatabase * const database, class IDataSet * dataset)
{
	delete dataset;
	if (--this->counter == 0) {
		this->user->TaskComplete(this);
	}
}

/*---------------------------------------------------------------------------*/
/* TaskGetDailies    														 */
/*---------------------------------------------------------------------------*/

TaskGetDailies::TaskGetDailies(class IDatabase * database)
{
	this->database = database;
	this->json = nullptr;
}

TaskGetDailies::~TaskGetDailies()
{
	free(this->json);
}

void TaskGetDailies::Start(class UTask * user)
{
	this->user = user;
	this->database->ExecSQL(this, DB_EXEC_FETCH_ALL, "SELECT array_to_json(array_agg(b)) FROM (" \
		"SELECT d.day, d.meal " \
		"FROM dailies d) b");
}

void TaskGetDailies::DatabaseError(IDatabase * const database, db_error_t error)
{
	this->user->TaskFailed(this);
}

void TaskGetDailies::DatabaseExecSQLResult(IDatabase * const database, class IDataSet * dataset)
{
	this->json = _strdup(dataset->GetStringValue(0));
	delete dataset;
	this->user->TaskComplete(this);
}

/*---------------------------------------------------------------------------*/
/* TaskEditDailies															 */
/*---------------------------------------------------------------------------*/

TaskEditDailies::TaskEditDailies(class IDatabase * database, class json_io & msg, word base)
{
	this->database = database;
	this->counter = 0;

	word a = msg.get_array(base, "dailies");
	if (a != JSON_ID_NONE) {
		word last = 0;
		while (msg.get_object(a, last) != JSON_ID_NONE) {
			const char * day = msg.get_string(last, "day");
			const char * meal = msg.get_string(last, "meal");
			if (day && meal) {
				this->list.push_back(new class Daily(day, meal));
			}
		}
	}
}

TaskEditDailies::~TaskEditDailies()
{
	while (list.size() > 0) {
		class Daily * item = list.front();
		delete item;
		list.pop_front();
	}
}

void TaskEditDailies::Start(class UTask * user)
{
	this->user = user;
	this->database->ExecSQL(this, DB_EXEC_FETCH_ALL, "DELETE FROM dailies");
	for (auto item : this->list) {
		this->database->ExecSQL(this, DB_EXEC_FETCH_ALL, "INSERT INTO dailies (day, meal) VALUES (%s,%s)",
			item->GetDay(), item->GetMeal());
	}
	this->counter = this->list.size() + 1;
}

void TaskEditDailies::DatabaseError(IDatabase * const database, db_error_t error)
{
	this->user->TaskFailed(this);
}

void TaskEditDailies::DatabaseExecSQLResult(IDatabase * const database, class IDataSet * dataset)
{
	delete dataset;
	if (--this->counter == 0) {
		this->user->TaskComplete(this);
	}
}

/*---------------------------------------------------------------------------*/
/* GetMonthCsv																 */
/*---------------------------------------------------------------------------*/

class MonthSip : public btree {
public:
	char * sip;
	ulong64 lunch;
	ulong64 panini;
	ulong64 snacks;

	MonthSip(const char * sip) {
		this->sip = _strdup(sip);
		this->lunch = 0;
		this->panini = 0;
		this->snacks = 0;
	}

	~MonthSip() {
		free(this->sip);
	}

	int btree_compare(void * key)
	{
		return strcmp((const char *)key, this->sip);
	}

	int btree_compare(class btree * b)
	{
		return this->btree_compare((void *)((class MonthSip *)b)->sip);
	}
};

GetMonthCsv::GetMonthCsv(class IDatabase * database, IWebserverPlugin * plugin, ulong64 from, ulong64 to, const char * month)
{
	this->database = database;
	this->from = from;
	this->to = to;
	this->month = _strdup(month);
	this->webserverGet = nullptr;
	this->counter = 0;
	this->sumLunch = 0;
	this->sumPanini = 0;
	this->sumSnack = 0;
	this->sips = nullptr;
	plugin->Accept(this);
}

GetMonthCsv::~GetMonthCsv() 
{
	free(this->month);
	class btree * item = nullptr;
	while ((item = this->sips->btree_find_left()) != nullptr) {
		this->sips = this->sips->btree_get(item);
		delete item;
	}
}

void GetMonthCsv::WebserverGetRequestAcceptComplete(IWebserverGet * const webserverGet) 
{
	this->webserverGet = webserverGet;

	this->database->ExecSQL(this, DB_EXEC_FETCH_ALL,
		"SELECT COUNT(id), sip FROM lunchs " \
		"WHERE timestamp>=%llu AND timestamp<=%llu " \
		"GROUP BY sip ORDER BY LOWER(sip) ASC", this->from, this->to);
	this->database->ExecSQL(this, DB_EXEC_FETCH_ALL,
		"SELECT COUNT(id), sip FROM paninis " \
		"WHERE timestamp>=%llu AND timestamp<=%llu " \
		"GROUP BY sip ORDER BY LOWER(sip) ASC", this->from, this->to);
	this->database->ExecSQL(this, DB_EXEC_FETCH_ALL,
		"SELECT COUNT(id), sip FROM daily_orders " \
		"WHERE timestamp>=%llu AND timestamp<=%llu " \
		"GROUP BY sip ORDER BY LOWER(sip) ASC", this->from, this->to);
}

void GetMonthCsv::WebserverGetSendResult(IWebserverGet * const webserverGet) 
{

}

void GetMonthCsv::WebserverGetCloseComplete(IWebserverGet * const webserverGet) 
{
	delete webserverGet;
	this->webserverGet = nullptr;
	TryClose();
}

void GetMonthCsv::DatabaseError(IDatabase * const database, db_error_t error) 
{
	if (this->webserverGet) {
		this->webserverGet->Cancel(wsr_cancel_type_t::WSP_CANCEL_INTERNAL_ERROR);
	}
	this->database = nullptr;
	TryClose();
}

void GetMonthCsv::DatabaseExecSQLResult(IDatabase * const database, class IDataSet * dataset) 
{
	while (!dataset->Eot()) {
		ulong64 count = dataset->GetULong64Value(0);
		const char * sip = dataset->GetStringValue(1);
		class MonthSip * monthSip = (class MonthSip *)this->sips->btree_find(sip);
		if (monthSip == nullptr) {
			monthSip = new class MonthSip(sip);
			this->sips = this->sips->btree_put(monthSip);
		}
		
		switch (this->counter) {
		case 0: 
			monthSip->lunch = count; 
			sumLunch += count;
			break;
		case 1: 
			monthSip->panini = count; 
			sumPanini += count;
			break;
		case 2: 
			monthSip->snacks = count; 
			sumSnack += count;
			break;
		}
		dataset->Next();
	}
	delete dataset;
	this->counter++;
	if (this->counter < 3) return;

	this->database = nullptr;
	
	this->webserverGet->SetTransferInfo(wsr_type_t::WSP_RESPONSE_TEXT, WS_RESPONSE_CHUNKED);
	char tmp[1024];
	_snprintf(tmp, sizeof(tmp), "lunch-%s.csv", this->month);
	this->webserverGet->ForceDownloadResponse(tmp);
	const char * header = "Name;Mittagessen;Panini;Warmes Frühstück\r\n";
	this->webserverGet->Send(header, strlen(header));
	
	for (class MonthSip * monthSip = (class MonthSip *)this->sips->btree_find_left(); monthSip; monthSip = (class MonthSip *)this->sips->btree_find_next_left(((class MonthSip *)monthSip)->sip)) {
		size_t len = _snprintf(tmp, sizeof(tmp), "%s;%llu;%llu;%llu\r\n", monthSip->sip, monthSip->lunch, monthSip->panini, monthSip->snacks);
		this->webserverGet->Send(tmp, len);
	}
	
	size_t len = _snprintf(tmp, sizeof(tmp), "\r\nSumme;%llu;%llu;%llu", this->sumLunch, this->sumPanini, this->sumSnack);
	this->webserverGet->Send(tmp, len);
	TryClose();
}

void GetMonthCsv::TryClose()
{
	if (this->webserverGet) {
		this->webserverGet->Close();
	}
	if (this->database || this->webserverGet) return;
	
	delete this;
}

/*---------------------------------------------------------------------------*/
/* GetRangeCsv																 */
/*---------------------------------------------------------------------------*/

class RangeDay : public btree {
public:
	ulong64 timestamp;
	ulong64 countLunchsMeat;
	ulong64 countLunchsVeggy;
	ulong64 countLunchsMeal3;
	ulong64 countLunchsMeal4;
	ulong64 countPaninis;
	ulong64 countDailyOrders;
	char * lunchsMeat;
	char * lunchsVeggy;
	char * lunchsMeal3;
	char * lunchsMeal4;
	char * paninis;
	char * dailyOrders;

	RangeDay(ulong64 timestamp) {
		this->timestamp = timestamp;
		this->countLunchsMeat = 0;
		this->countLunchsVeggy = 0;
		this->countLunchsMeal3 = 0;
		this->countLunchsMeal4 = 0;
		this->countPaninis = 0;
		this->countDailyOrders = 0;
		this->lunchsMeat = nullptr;
		this->lunchsVeggy = nullptr;
		this->lunchsMeal3 = nullptr;
		this->lunchsMeal4 = nullptr;
		this->paninis = nullptr;
		this->dailyOrders = nullptr;
	}

	~RangeDay() {
		free(this->lunchsMeat);
		free(this->lunchsVeggy);
		free(this->lunchsMeal3);
		free(this->lunchsMeal4);
		free(this->paninis);
		free(this->dailyOrders);
	}

	int btree_compare(void * key)
	{
		ulong64 keyVal = *(ulong64 *)key;
		return (int)(keyVal - this->timestamp);
	}

	int btree_compare(class btree * b)
	{
		return this->btree_compare(&((class RangeDay *)b)->timestamp);
	}
};

GetRangeCsv::GetRangeCsv(class IDatabase * database, IWebserverPlugin * plugin, ulong64 from, ulong64 to)
{
	this->database = database;
	this->from = from;
	this->to = to;
	this->webserverGet = nullptr;
	this->days = nullptr;
	this->counter = 0;
	plugin->Accept(this);
}

GetRangeCsv::~GetRangeCsv()
{
	class btree * item = nullptr;
	while ((item = this->days->btree_find_left()) != nullptr) {
		this->days = this->days->btree_get(item);
		delete item;
	}
}

void GetRangeCsv::WebserverGetRequestAcceptComplete(IWebserverGet * const webserverGet)
{
	this->webserverGet = webserverGet;

	this->database->ExecSQL(this, DB_EXEC_FETCH_ALL,
		"SELECT timestamp, COUNT(sip), array_to_string(array_agg(sip ORDER BY sip ASC),',') AS sips " \
		"FROM lunchs " \
		"WHERE type=%s AND timestamp>=%llu AND timestamp<=%llu " \
		"GROUP BY timestamp", TaskDatabaseInit::EnumTypeToChar(MealType::MEAL_TYPE_MEAT), this->from, this->to);
	this->database->ExecSQL(this, DB_EXEC_FETCH_ALL,
		"SELECT timestamp, COUNT(sip), array_to_string(array_agg(sip ORDER BY sip ASC),',') AS sips " \
		"FROM lunchs " \
		"WHERE type=%s AND timestamp>=%llu AND timestamp<=%llu " \
		"GROUP BY timestamp", TaskDatabaseInit::EnumTypeToChar(MealType::MEAL_TYPE_VEGGY), this->from, this->to);
	this->database->ExecSQL(this, DB_EXEC_FETCH_ALL,
		"SELECT timestamp, COUNT(sip), array_to_string(array_agg(sip ORDER BY sip ASC),',') AS sips " \
		"FROM lunchs " \
		"WHERE type=%s AND timestamp>=%llu AND timestamp<=%llu " \
		"GROUP BY timestamp", TaskDatabaseInit::EnumTypeToChar(MealType::MEAL_TYPE_MEAL3), this->from, this->to);
	this->database->ExecSQL(this, DB_EXEC_FETCH_ALL,
		"SELECT timestamp, COUNT(sip), array_to_string(array_agg(sip ORDER BY sip ASC),',') AS sips " \
		"FROM lunchs " \
		"WHERE type=%s AND timestamp>=%llu AND timestamp<=%llu " \
		"GROUP BY timestamp", TaskDatabaseInit::EnumTypeToChar(MealType::MEAL_TYPE_MEAL4), this->from, this->to);
	this->database->ExecSQL(this, DB_EXEC_FETCH_ALL,
		"SELECT timestamp, COUNT(sip), array_to_string(array_agg(sip ORDER BY sip ASC),',') AS sips " \
		"FROM paninis " \
		"WHERE timestamp>=%llu AND timestamp<=%llu " \
		"GROUP BY timestamp", this->from, this->to);
	this->database->ExecSQL(this, DB_EXEC_FETCH_ALL,
		"SELECT timestamp, COUNT(sip), array_to_string(array_agg(sip ORDER BY sip ASC),',') AS sips  " \
		"FROM daily_orders " \
		"WHERE timestamp>=%llu AND timestamp<=%llu " \
		"GROUP BY timestamp", this->from, this->to);
}

void GetRangeCsv::WebserverGetSendResult(IWebserverGet * const webserverGet)
{

}

void GetRangeCsv::WebserverGetCloseComplete(IWebserverGet * const webserverGet)
{
	delete webserverGet;
	this->webserverGet = nullptr;
	TryClose();
}

void GetRangeCsv::DatabaseError(IDatabase * const database, db_error_t error)
{
	if (this->webserverGet) {
		this->webserverGet->Cancel(wsr_cancel_type_t::WSP_CANCEL_INTERNAL_ERROR);
	}
	this->database = nullptr;
	TryClose();
}

void GetRangeCsv::DatabaseExecSQLResult(IDatabase * const database, class IDataSet * dataset)
{
	while (!dataset->Eot()) {
		ulong64 timestamp = dataset->GetULong64Value(0);
		ulong64 count = dataset->GetULong64Value(1);
		const char * sips = dataset->GetStringValue(2);
		class RangeDay * rangeDay = (class RangeDay *)this->days->btree_find(&timestamp);
		if (rangeDay == nullptr) {
			rangeDay = new class RangeDay(timestamp);
			this->days = this->days->btree_put(rangeDay);
		}

		switch (this->counter) {
		case 0:
			rangeDay->lunchsMeat = _strdup(sips);
			rangeDay->countLunchsMeat = count;
			break;
		case 1:
			rangeDay->lunchsVeggy = _strdup(sips);
			rangeDay->countLunchsVeggy = count;
			break;
		case 2:
			rangeDay->lunchsMeal3 = _strdup(sips);
			rangeDay->countLunchsMeal3 = count;
			break;
		case 3:
			rangeDay->lunchsMeal4 = _strdup(sips);
			rangeDay->countLunchsMeal4 = count;
			break;
		case 4:
			rangeDay->paninis = _strdup(sips);
			rangeDay->countPaninis = count;
			break;
		case 5:
			rangeDay->dailyOrders = _strdup(sips);
			rangeDay->countDailyOrders = count;
			break;
		}
		dataset->Next();
	}
	delete dataset;
	this->counter++;
	if (this->counter < 6) return;

	this->database = nullptr;

	this->webserverGet->SetTransferInfo(wsr_type_t::WSP_RESPONSE_TEXT, WS_RESPONSE_CHUNKED);
	char tmp[WS_MAX_DATA_SIZE];
	char d1[128];
	char d2[128];
	ITime::FormatTimeStamp(d1, sizeof(d1), "%d.%m.%Y", this->from);
	ITime::FormatTimeStamp(d2, sizeof(d2), "%d.%m.%Y", this->to);
	_snprintf(tmp, sizeof(tmp), "lunch-%s-%s.csv", d1, d2);
	this->webserverGet->ForceDownloadResponse(tmp);
	const char * header = "Tag;Anzahl Menü 1;Teilnehmer Menü 1;Anzahl Menü 2;Teilnehmer Menü 2;Anzahl Menü 3;Teilnehmer Menü 3;Anzahl Paninis;Teilnehmer Paninis;Anzahl Warmes Frühstück;Teilnehmer Warmes Frühstück\r\n";
	this->webserverGet->Send(header, strlen(header));

	for (class RangeDay * rangeDay = (class RangeDay *)this->days->btree_find_left(); rangeDay; rangeDay = (class RangeDay *)this->days->btree_find_next_left(&((class RangeDay *)rangeDay)->timestamp)) {
		ITime::FormatTimeStamp(d1, sizeof(d1), "%d.%m.%Y", rangeDay->timestamp);
		size_t len = _snprintf(tmp, sizeof(tmp), "%s;%llu;%s;%llu;%s;%llu;%s;%llu;%s;%llu;%s;%llu;%s\r\n", d1, 
			rangeDay->countLunchsMeat, rangeDay->lunchsMeat ? rangeDay->lunchsMeat : "",
			rangeDay->countLunchsVeggy, rangeDay->lunchsVeggy ? rangeDay->lunchsVeggy : "",
			rangeDay->countLunchsMeal3, rangeDay->lunchsMeal3 ? rangeDay->lunchsMeal3 : "",
			rangeDay->countLunchsMeal4, rangeDay->lunchsMeal4 ? rangeDay->lunchsMeal4 : "",
			rangeDay->countPaninis, rangeDay->paninis ? rangeDay->paninis : "",
			rangeDay->countDailyOrders, rangeDay->dailyOrders ? rangeDay->dailyOrders : "");
		this->webserverGet->Send(tmp, len);
	}

	TryClose();
}

void GetRangeCsv::TryClose()
{
	if (this->webserverGet) {
		this->webserverGet->Close();
	}
	if (this->database || this->webserverGet) return;

	delete this;
}