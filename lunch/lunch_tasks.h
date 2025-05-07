
/*-----------------------------------------------------------------------------------------------*/
/* Based on innovaphone App template                                                             */
/* copyright (c) innovaphone 2018                                                                */
/*                                                                                               */
/*-----------------------------------------------------------------------------------------------*/

namespace AppLunch {

    enum MealType {
        MEAL_TYPE_MEAT = 0,
        MEAL_TYPE_VEGGY,
        MEAL_TYPE_MEAL3,
        MEAL_TYPE_MEAL4,
        MEAL_TYPE_LAST
    };

    extern const char * ADMINS[];

    class TaskDatabaseInit : public ITask, public UTask, public UDatabase {
        class TaskPostgreSQLInitTable initLunchs;
        class TaskPostgreSQLInitTable initMeals;
        class TaskPostgreSQLInitTable initDailies;
        class TaskPostgreSQLInitTable initPaninis;
        class TaskPostgreSQLInitTable initDailyOrders;
        class TaskPostgreSQLInitEnum initLunchTypeEnum;

        class IDatabase * database;
        bool tableCheckLunchs;
        dword counter;

        void DatabaseError(IDatabase * const database, db_error_t error) override;
        void DatabaseExecSQLResult(IDatabase * const database, class IDataSet * dataset) override;

        virtual void TaskComplete(class ITask * const task);
        virtual void TaskFailed(class ITask * const task);

    public:
        TaskDatabaseInit(IDatabase * database);
        virtual ~TaskDatabaseInit();

        virtual void Start(class UTask * user);

        static enum MealType CharTypeToEnum(const char * type);
        static const char * EnumTypeToChar(enum MealType type);
    };

    class TaskAddLunch : public ITask, public UDatabase {
        class IDatabase * database;

        char * sip;
        char * removed;
        ulong64 timestamp;
        ulong64 removedTimestamp;
        enum MealType type;
        bool removing;

        void DatabaseError(IDatabase * const database, db_error_t error) override;
        void DatabaseBeginTransactionResult(IDatabase * const database) override;
        void DatabaseExecSQLResult(IDatabase * const database, class IDataSet * dataset) override;
        void DatabaseEndTransactionResult(IDatabase * const database) override;

    public:
        TaskAddLunch(class IDatabase * database, const char * sip, ulong64 timestamp, enum MealType type);
        ~TaskAddLunch();

        void Start(class UTask * user) override;

        const char * GetSip() { return this->sip; }
        ulong64 GetTimestamp() { return this->timestamp; }
        enum MealType GetType() { return this->type; }
        const char * GetRemovedSip() { return this->removed; }
        ulong64 GetRemovedTimestamp() { return this->removedTimestamp; }
    };

    class TaskRemoveLunch : public ITask, public UDatabase {
        class IDatabase * database;
        char * sip;
        ulong64 timestamp;
        enum MealType type;
        class IIoMux * iomux;
        class ISocketProvider * tcpSocketProvider;
        class ISocketProvider * tlsSocketProvider;
        class IInstanceLog * log;
        void DatabaseError(IDatabase * const database, db_error_t error) override;
        void DatabaseExecSQLResult(IDatabase * const database, class IDataSet * dataset) override;

        void StartQuery();

    public:
        TaskRemoveLunch(class IIoMux * iomux, class ISocketProvider * tcpSocketProvider, class ISocketProvider * tlsSocketProvider, class IDatabase * database,
        class IInstanceLog * log,
            const char * sip, ulong64 timestamp, enum MealType type);
        ~TaskRemoveLunch();

        void Start(class UTask * user) override;

        const char * GetSip() { return this->sip; }
        ulong64 GetTimestamp() { return this->timestamp; }
        enum MealType GetType() { return this->type; }
    };

    class TaskGetLunchs : public ITask, public UDatabase {
        class IDatabase * database;
        char * json;
        ulong64 from;
        ulong64 to;

        void DatabaseError(IDatabase * const database, db_error_t error) override;
        void DatabaseExecSQLResult(IDatabase * const database, class IDataSet * dataset) override;

    public:
        TaskGetLunchs(class IDatabase * database, ulong64 from, ulong64 to);
        ~TaskGetLunchs();

        void Start(class UTask * user) override;
        const char * GetJson() { return this->json; }
    };

    class TaskAddDailyOrder : public ITask, public UDatabase {
        class IDatabase * database;

        char * sip;
        ulong64 timestamp;
        
        void DatabaseError(IDatabase * const database, db_error_t error) override;
        void DatabaseExecSQLResult(IDatabase * const database, class IDataSet * dataset) override;
        
    public:
        TaskAddDailyOrder(class IDatabase * database, const char * sip, ulong64 timestamp);
        ~TaskAddDailyOrder();

        void Start(class UTask * user) override;

        const char * GetSip() { return this->sip; }
        ulong64 GetTimestamp() { return this->timestamp; }
    };

    class TaskRemoveDailyOrder : public ITask, public UDatabase {
        class IDatabase * database;
        char * sip;
        ulong64 timestamp;
        class IIoMux * iomux;
        class ISocketProvider * tcpSocketProvider;
        class ISocketProvider * tlsSocketProvider;
        class IInstanceLog * log;
        void DatabaseError(IDatabase * const database, db_error_t error) override;
        void DatabaseExecSQLResult(IDatabase * const database, class IDataSet * dataset) override;

        void StartQuery();

    public:
        TaskRemoveDailyOrder(class IIoMux * iomux, class ISocketProvider * tcpSocketProvider, class ISocketProvider * tlsSocketProvider, class IDatabase * database,
            class IInstanceLog * log,
            const char * sip, ulong64 timestamp);
        ~TaskRemoveDailyOrder();

        void Start(class UTask * user) override;

        const char * GetSip() { return this->sip; }
        ulong64 GetTimestamp() { return this->timestamp; }
    };

    class TaskGetDailyOrders : public ITask, public UDatabase {
        class IDatabase * database;
        char * json;
        ulong64 from;
        ulong64 to;

        void DatabaseError(IDatabase * const database, db_error_t error) override;
        void DatabaseExecSQLResult(IDatabase * const database, class IDataSet * dataset) override;

    public:
        TaskGetDailyOrders(class IDatabase * database, ulong64 from, ulong64 to);
        ~TaskGetDailyOrders();

        void Start(class UTask * user) override;
        const char * GetJson() { return this->json; }
    };

    class TaskAddPanini : public ITask, public UDatabase {
        class IDatabase * database;

        char * sip;
        ulong64 timestamp;

        void DatabaseError(IDatabase * const database, db_error_t error) override;
        void DatabaseExecSQLResult(IDatabase * const database, class IDataSet * dataset) override;

    public:
        TaskAddPanini(class IDatabase * database, const char * sip, ulong64 timestamp);
        ~TaskAddPanini();

        void Start(class UTask * user) override;

        const char * GetSip() { return this->sip; }
        ulong64 GetTimestamp() { return this->timestamp; }
    };

    class TaskRemovePanini : public ITask, public UDatabase {
        class IDatabase * database;
        char * sip;
        ulong64 timestamp;
        class IIoMux * iomux;
        class ISocketProvider * tcpSocketProvider;
        class ISocketProvider * tlsSocketProvider;
        class IInstanceLog * log;
        void DatabaseError(IDatabase * const database, db_error_t error) override;
        void DatabaseExecSQLResult(IDatabase * const database, class IDataSet * dataset) override;

        void StartQuery();

    public:
        TaskRemovePanini(class IIoMux * iomux, class ISocketProvider * tcpSocketProvider, class ISocketProvider * tlsSocketProvider, class IDatabase * database,
            class IInstanceLog * log,
            const char * sip, ulong64 timestamp);
        ~TaskRemovePanini();

        void Start(class UTask * user) override;

        const char * GetSip() { return this->sip; }
        ulong64 GetTimestamp() { return this->timestamp; }
    };

    class TaskGetPaninis : public ITask, public UDatabase {
        class IDatabase * database;
        char * json;
        ulong64 from;
        ulong64 to;

        void DatabaseError(IDatabase * const database, db_error_t error) override;
        void DatabaseExecSQLResult(IDatabase * const database, class IDataSet * dataset) override;

    public:
        TaskGetPaninis(class IDatabase * database, ulong64 from, ulong64 to);
        ~TaskGetPaninis();

        void Start(class UTask * user) override;
        const char * GetJson() { return this->json; }
    };

    class TaskGetMeals : public ITask, public UDatabase {
        class IDatabase * database;
        char * json;
        ulong64 from;
        ulong64 to;

        void DatabaseError(IDatabase * const database, db_error_t error) override;
        void DatabaseExecSQLResult(IDatabase * const database, class IDataSet * dataset) override;

    public:
        TaskGetMeals(class IDatabase * database, ulong64 from, ulong64 to);
        ~TaskGetMeals();

        void Start(class UTask * user) override;
        const char * GetJson() { return this->json; }
    };

    class EditMeal {
        ulong64 timestamp;
        char * veggy;
        char * veggyType;
        char * meat;
        char * meatType;
        char * meal3;        
        char * meal3Type;
        char * meal4;
        char * meal4Type;

    public:
        EditMeal(ulong64 timestamp, const char * veggy, const char * veggyType, const char * meat, const char * meatType, const char * meal3, const char * meal3Type, const char * meal4, const char * meal4Type) {
            this->timestamp = timestamp;
            this->veggy = veggy ? _strdup(veggy) : nullptr;
            this->veggyType = veggyType ? _strdup(veggyType) : nullptr;
            this->meat = meat ? _strdup(meat) : nullptr;
            this->meatType = meatType ? _strdup(meatType) : nullptr;
            this->meal3 = meal3 ? _strdup(meal3) : nullptr;
            this->meal3Type = meal3Type ? _strdup(meal3Type) : nullptr;
            this->meal4 = meal4 ? _strdup(meal4) : nullptr;
            this->meal4Type = meal4Type ? _strdup(meal4Type) : nullptr;
        }

        ~EditMeal() {
            free(this->veggy);
            free(this->veggyType);
            free(this->meat);
            free(this->meatType);
            free(this->meal3);
            free(this->meal3Type);
            free(this->meal4);
            free(this->meal4Type);
        }

        ulong64 GetTimestamp() { return this->timestamp; }
        const char * GetVeggy() { return this->veggy; }
        const char * GetVeggyType() { return this->veggyType; }
        const char * GetMeat() { return this->meat; }
        const char * GetMeatType() { return this->meatType; }
        const char * GetMeal3() { return this->meal3; }
        const char * GetMeal3Type() { return this->meal3Type; }
        const char * GetMeal4() { return this->meal4; }
        const char * GetMeal4Type() { return this->meal4Type; }
    };

    typedef std::list<class EditMeal *> EditMealList;

    class TaskEditMeals : public ITask, public UDatabase, public UIoExec {
        class IIoMux * iomux;
        class IDatabase * database;
        EditMealList list;
        size_t counter;
        
        void DatabaseError(IDatabase * const database, db_error_t error) override;
        void DatabaseExecSQLResult(IDatabase * const database, class IDataSet * dataset) override;

    public:
        TaskEditMeals(class IIoMux * iomux, class IDatabase * database, class json_io & msg, word base);
        ~TaskEditMeals();

        void AddMeal(class EditMeal * meal);
        void Start(class UTask * user) override;
        void IoExec(void * execContext) override;
        EditMealList * GetList() { return &list; }
    };

    class TaskGetDailies : public ITask, public UDatabase {
        class IDatabase * database;
        char * json;

        void DatabaseError(IDatabase * const database, db_error_t error) override;
        void DatabaseExecSQLResult(IDatabase * const database, class IDataSet * dataset) override;

    public:
        TaskGetDailies(class IDatabase * database);
        ~TaskGetDailies();

        void Start(class UTask * user) override;
        const char * GetJson() { return this->json; }
    };

    class Daily {
        char * day;
        char * meal;

    public:
        Daily(const char * day, const char * meal) {
            this->day = _strdup(day);
            this->meal = _strdup(meal);
        }

        ~Daily() {
            free(this->day);
            free(this->meal);
        }

        const char * GetDay() { return this->day; }
        const char * GetMeal() { return this->meal; }
    };

    typedef std::list<class Daily *> DailyList;

    class TaskEditDailies : public ITask, public UDatabase {
        class IDatabase * database;
        size_t counter;
        DailyList list;
        
        void DatabaseError(IDatabase * const database, db_error_t error) override;
        void DatabaseExecSQLResult(IDatabase * const database, class IDataSet * dataset) override;

    public:
        TaskEditDailies(class IDatabase * database, class json_io & msg, word base);
        ~TaskEditDailies();

        void Start(class UTask * user) override;
        DailyList * GetList() { return &list; }
    };   

    class GetMonthCsv : public UWebserverGet, public UDatabase {
        class IDatabase * database;
        IWebserverGet * webserverGet;
        ulong64 from;
        ulong64 to;
        ulong64 sumLunch;
        ulong64 sumPanini;
        ulong64 sumSnack;
        char * month;
        byte counter;
        btree * sips;

        void WebserverGetRequestAcceptComplete(IWebserverGet * const webserverGet) override;
        void WebserverGetSendResult(IWebserverGet * const webserverGet) override;
        void WebserverGetCloseComplete(IWebserverGet * const webserverGet) override;

        void DatabaseError(IDatabase * const database, db_error_t error) override;
        void DatabaseExecSQLResult(IDatabase * const database, class IDataSet * dataset) override;

        void TryClose();

    public:
        GetMonthCsv(class IDatabase * database, IWebserverPlugin * plugin, ulong64 from, ulong64 to, const char * month);
        ~GetMonthCsv();
    };

    class GetRangeCsv : public UWebserverGet, public UDatabase {
        class IDatabase * database;
        IWebserverGet * webserverGet;
        ulong64 from;
        ulong64 to;
        byte counter;
        btree * days;
        
        void WebserverGetRequestAcceptComplete(IWebserverGet * const webserverGet) override;
        void WebserverGetSendResult(IWebserverGet * const webserverGet) override;
        void WebserverGetCloseComplete(IWebserverGet * const webserverGet) override;

        void DatabaseError(IDatabase * const database, db_error_t error) override;
        void DatabaseExecSQLResult(IDatabase * const database, class IDataSet * dataset) override;

        void TryClose();

    public:
        GetRangeCsv(class IDatabase * database, IWebserverPlugin * plugin, ulong64 from, ulong64 to);
        ~GetRangeCsv();
    };

}