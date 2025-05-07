
/// <reference path="../../sdk/web1/lib1/innovaphone.lib1.js" />
/// <reference path="../../sdk/web1/appwebsocket/innovaphone.appwebsocket.Connection.js" />
/// <reference path="../../sdk/web1/ui1.lib/innovaphone.ui1.lib.js" />
/// <reference path="../../sdk/web1/ui1.svg/innovaphone.ui1.svg.js" />

var innovaphone = innovaphone || {};
innovaphone.Lunch = innovaphone.Lunch || function (start, args) {
    var body = this.createNode("body", "display:flex; position:absolute; left:0; right:0; top:0; bottom:0; flex-direction:column"),
        content = body.add(new innovaphone.ui1.Div("display:flex; flex-direction:column; flex-wrap:nowrap; overflow:hidden; flex:1 1 100%")),
        contentScroll = content.add(new innovaphone.ui1.Scrolling("position:relative; width:100%; height:100%", -1, 0, null, "var(--accent)"));

    var mealTypes = {
        meat: "MEAT",
        veggy: "VEGGY",
        meal3: "MEAL3",
        meal4: "MEAL4",
    };

    var colorSchemes = {
        "dark": {
            "--client-bg1": "#232323",
            "--client-bg2": "#161616",
            "--forms": "#1E1E1E",
            "--bg1": "#282828",
            "--bg2": "#333333",
            "--bg3": "#474747",
            "--c1": "#efefef",
            "--c2": "#939393",
            "--highlight-bg": "#595959",
            "--button-bg1": "#3D3D3D",
            "--button-bg2": "#61ABEC",
            "--active-bg": "#256297",
            "--accent": "#08b047",
            "--red": "#ef4d64",
            "--green": "#7cb270",
            "--yellow": "#f59b10",
            "--view-menu-highlight": "#61ABEC",
            "--view-menu-back1": "#494949",
            "--view-menu-back2": "#545454",
            "--view-menu-back3": "#5E5E5E",
            "--job-item-even": "#5E5E5E",
            "--job-item-odd": "#545454",
            "--meat": "#8c2a2a",
            "--veggy": "#3a9616",
            "--fish": "#6fa0ff",
            "--soup": "#125bea",
            "--bg-cancel": "#800000",
            "--menu1": "#ffdf8d",
            "--menu2": "#ffb63b",
            "--menu3": "#e9903b",
            "--menu4": "#b8e3ab"
        },

        "light": {
            "--client-bg1": "#e5e5e5",
            "--client-bg2": "#ffffff",
            "--forms": "#ededed",
            "--bg1": "#e3e3e3",
            "--bg2": "#f5f5f5",
            "--bg3": "#ffffff",
            "--c1": "#000000",
            "--c2": "#404040",
            "--highlight-bg": "#f9f9f9",
            "--button-bg1": "#CCCCCC",
            "--button-bg2": "#61ABEC",
            "--active-bg": "#C6C6C6",
            "--accent": "#08b047",
            "--red": "#ef4d64",
            "--green": "#7cb270",
            "--yellow": "#f59b10",
            "--view-menu-highlight": "#f2eaea",
            "--view-menu-back1": "#f5f5f5",
            "--view-menu-back2": "#f5f5f5",
            "--view-menu-back3": "#ffffff",
            "--job-item-even": "#efefef",
            "--job-item-odd": "#f7f7f7",
            "--meat": "#8c2a2a",
            "--veggy": "#3a9616",
            "--fish": "#6fa0ff",
            "--soup": "#125bea",
            "--bg-cancel": "#E9967A",
            "--menu1": "#886622",
            "--menu2": "#C17900",
            "--menu3": "#DD6600",
            "--menu4": "#2C8420"
        }
    };

    var that = this,
        schemes = new innovaphone.ui1.CssVariables(colorSchemes, start.scheme),
        texts = new innovaphone.lib1.Languages(innovaphone.lunchTexts, start.lang),
        app = new innovaphone.appwebsocket.Connection(start.url, start.name),
        config = null,
        oneDayMs = (24 * 60 * 60 * 1000),
        weeks = [],
        dailies = [],
        mySip = null,
        timer = null,
        evUpdateButtons = new innovaphone.lib1.Event(this),
        evLunchRemoved = new innovaphone.lib1.Event(this),
        evLunchAdded = new innovaphone.lib1.Event(this),
        evDailyOrderRemoved = new innovaphone.lib1.Event(this),
        evDailyOrderAdded = new innovaphone.lib1.Event(this),
        evPaniniRemoved = new innovaphone.lib1.Event(this),
        evPaniniAdded = new innovaphone.lib1.Event(this),
        evMealsEdited = new innovaphone.lib1.Event(this),
        evDailiesEdited = new innovaphone.lib1.Event(this),
        localNow = null,
        dateEnter = null,
        curDay = null,
        admin = false,
        orderWindow = 750,
        months = null;

    start.onschemechanged.attach(function () { schemes.activate(start.scheme) });
    app.checkBuild = true;
    app.onconnected = appConnected;
    app.onmessage = appMessage;
    
    function appConnected(domain, user, dn, appdomain) {
        mySip = user;
        app.send({ mt: "GetAdmin" });
        app.send({ mt: "GetDailies" });
    }

    function updateButtons() {
        localNow = new Date();
        if (curDay == null) {
            curDay = localNow.getDay();
        }
        else if (curDay != localNow.getDay()) {  // day changed => close app ...
            start.close();
            return;
        }
        evUpdateButtons.notify();
        timer = window.setTimeout(updateButtons, 60 * 1000);
    }

    function appMessage(obj) {
        switch (obj.mt) {
            case "GetAdminResult":
                getAdminResult(obj);
                break;
            case "GetDailiesResult":
                getDailiesResult(obj);
                break;
            case "LunchRemoved":
                lunchRemoved(obj);
                break;
            case "LunchAdded":
                lunchAdded(obj);
                break;
            case "DailyOrderRemoved":
                dailyOrderRemoved(obj);
                break;
            case "DailyOrderAdded":
                dailyOrderAdded(obj);
                break;
            case "PaniniRemoved":
                paniniRemoved(obj);
                break;
            case "PaniniAdded":
                paniniAdded(obj);
                break;
            case "MealsEdited":
                mealsEdited(obj);
                break;
            case "DailiesEdited":
                dailiesEdited(obj);
                break;
        }
    }

    function getAdminResult(obj) {
        admin = (obj.admin === true);
        orderWindow = obj.orderWindow;

        if (admin) {
            if (config == null) {
                config = new innovaphone.Config();
                config.evOnConfigLoaded.attach(onConfigLoaded);
                config.evOnConfigSaveResult.attach(onConfigSaveResult);
            }
            if (!config.initialized) config.init(app);
            else onConfigLoaded();
        }
    }

    function onConfigLoaded() {

    }

    function onConfigSaveResult() {

    }

    function getDailiesResult(obj) {
        contentScroll.clear();
        for (var i = 0; i < weeks.length; i++) {
            weeks[i].close();
        }
        weeks.length = 0;
        var date = new Date(),
            tzOffset = date.getTimezoneOffset() * 60000;        
        var from = date.getTime();
        from -= (from % oneDayMs);
        var day = new Date(date.getTime() + tzOffset).getDay();
        console.log('from: ' + from);
        if (day == 0) { // sunday
            from += oneDayMs;
            ++day;
        }
        else {
            while (day > 1) {
                from -= oneDayMs;
                --day;
            }
        }
        console.log('from2: ' + from);
        dateEnter = from + (3 * oneDayMs);  // thursday of the first week
        weeks.push(new WeekNode(from, 1));
        weeks.push(new WeekNode(from + (1 * 7 * oneDayMs), 2));
        weeks.push(new WeekNode(from + (2 * 7 * oneDayMs), 3));
        weeks.push(new WeekNode(from + (3 * 7 * oneDayMs), 4));
        for (var i = 0; i < weeks.length; i++) {
            contentScroll.add(weeks[i]);
        }
        /*var a = contentScroll.add(new innovaphone.ui1.Node("a", "flex:1 1 auto; margin-left:10px", "", "link")).addTranslation(texts, "menu");
        a.container.setAttribute("href", "https://www.stollsteimer.de/menues.html");
        a.container.setAttribute("target", "_blank");*/

        var breakfast = contentScroll.add(new PdfLink("margin-top:20px", "breakfast", "fruehstueck.pdf"));
        var allergens = contentScroll.add(new PdfLink("", "allergens", "allergene.pdf"));

        var snacks = contentScroll.add(new innovaphone.ui1.Div("display:flex; flex:1 1 auto"));
        snacks.add(new innovaphone.ui1.Node("h1", "flex:0 1 auto")).addTranslation(texts, "dailies");
        if (admin) {
            var editSnacks = snacks.add(new innovaphone.LunchSvgTitle("flex:0 0 auto; width:20px; height:0px; fill:var(--accent); margin-left:10px; margin-top:20px; cursor:pointer", "fill:var(--accent)", "0 0 20 20", innovaphone.Lunch.SymbolCodes.Edit, "edit", texts));
            editSnacks.addEvent("click", onEditSnacks);
        }
        if (obj.dailies) {
            dailiesEdited(obj);
        }

        if (admin) {
            months = contentScroll.add(new innovaphone.ui1.Div("display:flex; flex:1 1 auto; margin-left:10px; margin-top:20px"));
            for (var i = 0; i < 4; i++) {
                var d1 = new Date(date.getFullYear(), date.getMonth() - i, 1);      // first day of month
                var d2 = new Date(date.getFullYear(), date.getMonth() - i + 1, 0);  // last day of month
                var dNode = months.add(new MonthNode(d1, d2));
            }

            contentScroll.add(new RangeNode());

        }
        
        if (timer) {
            window.clearTimeout(timer);
        }
        updateButtons();
    }

    function PdfLink(style, translationId, pdfName) {
        this.createNode("div", "display:flex; flex:1 1 auto; overflow:hidden; flex-direction:row;" + style);
        this.add(new innovaphone.ui1.Node("h1", "flex:0 1 auto")).addTranslation(texts, translationId);
        //style, texts, translationKey, translationParams, svgSymbolCode, href, text
        var download = this.add(new innovaphone.Lunch.Link("flex:0 1 auto; margin-top:10px", texts, null, null, innovaphone.Lunch.SymbolCodes.Download, "../" + pdfName));
    }
    PdfLink.prototype = innovaphone.ui1.nodePrototype;

    function RangeNode() {
        this.createNode("div", "display:flex; flex:1 1 auto; margin-top:20px; overflow:hidden; flex-direction:column");

        var d = new Date(),
            tzOffset = d.getTimezoneOffset() * 60000,
            text = this.add(new innovaphone.ui1.Node("h1", "flex:1 1 auto")).addTranslation(texts, "downloadRange"),
            fromOuter = this.add(new innovaphone.ui1.Div("display:flex; flex:1 1 auto; overflow:hidden; margin-left:20px; align-items:center")),
            fromText = fromOuter.add(new innovaphone.ui1.Div("flex:0 1 auto; margin-right:20px; width:100px; font-size:16px")).addTranslation(texts, "from"),
            from = fromOuter.add(new innovaphone.ui1.Input("flex:0 1 auto", null, null, null, "date")),
            toOuter = this.add(new innovaphone.ui1.Div("display:flex; flex:1 1 auto; overflow:hidden; margin-top:10px; margin-left:20px; align-items:center")),
            toText = toOuter.add(new innovaphone.ui1.Div("flex:0 1 auto; margin-right:20px; width:100px; font-size:16px")).addTranslation(texts, "to"),
            to = toOuter.add(new innovaphone.ui1.Input("flex:0 1 auto", null, null, null, "date")),
            download = this.add(new innovaphone.ui1.Node("a", "flex:0 1 auto; margin-top:10px; margin-left:20px", null, "link")).addTranslation(texts, "download");

        function init() {
            from.addEvent("change", onChange);
            to.addEvent("change", onChange);
            download.container.setAttribute("download", "");
            from.container.setAttribute("value", (new Date(d.getTime() - tzOffset - (7 * 24 * 60 * 60 * 1000))).toISOString().substr(0, 10));
            to.container.setAttribute("value", (new Date(d.getTime() - tzOffset)).toISOString().substr(0, 10));
            onChange();
        }

        function onChange() {
            download.container.setAttribute("href", "../csv-range?from=" + (new Date(from.getValue()).getTime()) + "&to=" + (new Date(to.getValue()).getTime()));
        }

        init();
    }
    RangeNode.prototype = innovaphone.ui1.nodePrototype;

    function DailyNode(day, meal) {
        this.createNode("div", "display:flex; flex:1 1 auto; overflow:hidden; margin-left:20px; font-size:14px; flex-wrap:wrap");
        this.add(new innovaphone.ui1.Div("flex:0 1 auto; width:100px")).addTranslation(texts, day);
        this.add(new innovaphone.ui1.Div("flex:1 1 auto", meal));

        this.day = day;
        this.meal = meal;
    }
    DailyNode.prototype = innovaphone.ui1.nodePrototype;

    function onEditSnacks() {
        var before = months;
        if (dailies.length > 0) {
            before = dailies[dailies.length - 1];
        }
        contentScroll.add(new EditSnacksNode(), before);
    }

    function getDailyValue(day) {
        for (var i = 0; i < dailies.length; i++) {
            if (dailies[i].day == day) {
                return dailies[i].meal;
            }
        }
        return "";
    }

    function EditSnacksNode() {
        this.createNode("div", "display:flex; flex: 1 1 auto; overflow:hidden; flex-direction:column; margin:20px");

        var that = this,
            src = new app.Src(),
            mondays = this.add(new innovaphone.ui1.Node("textarea", "flex:1 1 auto", getDailyValue("mondays")).addTranslation(texts, "mondays", "placeholder")),
            tuesdays = this.add(new innovaphone.ui1.Node("textarea", "flex:1 1 auto; margin-top:10px", getDailyValue("tuesdays")).addTranslation(texts, "tuesdays", "placeholder")),
            wednesdays = this.add(new innovaphone.ui1.Node("textarea", "flex:1 1 auto; margin-top:10px", getDailyValue("wednesdays")).addTranslation(texts, "wednesdays", "placeholder")),
            thursdays = this.add(new innovaphone.ui1.Node("textarea", "flex:1 1 auto; margin-top:10px", getDailyValue("thursdays")).addTranslation(texts, "thursdays", "placeholder")),
            fridays = this.add(new innovaphone.ui1.Node("textarea", "flex:1 1 auto; margin-top:10px", getDailyValue("fridays")).addTranslation(texts, "fridays", "placeholder")),
            buttons = this.add(new innovaphone.ui1.Div("display:flex; flex:0 1 auto; flex-wrap:nowrap; overflow:hidden; flex-direction:row; margin-top:10px")),
            buttonSave = buttons.add(new innovaphone.LunchButton("flex:0 1 auto", "save", texts, editSave)),
            buttonCancel = buttons.add(new innovaphone.LunchButton("flex:0 1 auto; margin-left:10px", "cancel", texts, editCancel));

        src.onmessage = onMessage;
        
        function onMessage(obj) {
            switch (obj.mt) {
                case "EditDailiesResult":
                    editDailiesResult(obj);
                    break;
            }
        }

        function editSave() {
            buttonSave.disable();
            buttonCancel.disable();
            var obj = { mt: "EditDailies" };
            var dailies = [];
            dailies.push({ day: "mondays", meal: mondays.container.value.trim() });
            dailies.push({ day: "tuesdays", meal: tuesdays.container.value.trim() });
            dailies.push({ day: "wednesdays", meal: wednesdays.container.value.trim() });
            dailies.push({ day: "thursdays", meal: thursdays.container.value.trim() });
            dailies.push({ day: "fridays", meal: fridays.container.value.trim() });
            obj.dailies = dailies;
            src.send(obj);
        }

        function editDailiesResult() {
            editCancel();
        }

        function editCancel() {
            contentScroll.rem(that);
        }
    }

    EditSnacksNode.prototype = innovaphone.ui1.nodePrototype;

    function MonthNode(dateFrom, dateTo) {
        this.createNode("a", "flex:0 1 auto; margin-right:10px", dateFrom.toLocaleString(start.lang, { month: 'long' }), "link");
        this.container.setAttribute("download", "");
        this.container.setAttribute("href", "../csv-month?from=" + dateFrom.getTime() + "&to=" + dateTo.getTime() + "&m=" + encodeURIComponent(dateFrom.toLocaleString(start.lang, { month: 'long' })));
    }

    MonthNode.prototype = innovaphone.ui1.nodePrototype;

    function lunchAdded(obj) {
        evLunchAdded.notify(obj);
    }

    function lunchRemoved(obj) {
        evLunchRemoved.notify(obj);
    }

    function dailyOrderAdded(obj) {
        evDailyOrderAdded.notify(obj);
    }

    function dailyOrderRemoved(obj) {
        evDailyOrderRemoved.notify(obj);
    }

    function paniniAdded(obj) {
        evPaniniAdded.notify(obj);
    }

    function paniniRemoved(obj) {
        evPaniniRemoved.notify(obj);
    }

    function mealsEdited(obj) {
        evMealsEdited.notify(obj.meals);
    }

    function dailiesEdited(obj) {
        for (var i = 0; i < dailies.length; i++) {
            contentScroll.rem(dailies[i]);
        }
        dailies.length = 0;
        for (var i = 0; i < obj.dailies.length; i++) {
            dailies.push(contentScroll.add(new DailyNode(obj.dailies[i].day, obj.dailies[i].meal), months));
        }
        evDailiesEdited.notify(obj.dailies);
    }

    function WeekNode(weekStart, weekNo) {
        this.createNode("div", "display:flex; flex:1 1 auto; flex-wrap:nowrap; overflow:hidden; flex-direction:column; margin-left:10px; margin-right:20px; margin-bottom:20px; padding:10px; background-color:var(--bg3)");

        var that = this,
            src = new app.Src(),
            header = this.add(new innovaphone.ui1.Div("display:flex; flex:1 1 auto; flex-wrap:nowrap; overflow:hidden; flex-direction:row; cursor:pointer")),
            daysNode = this.add(new innovaphone.ui1.Div("display:none; flex:1 1 auto; flex-wrap:nowrap; overflow:hidden; flex-direction:column")),
            editNode = this.add(new innovaphone.ui1.Div("display:none; flex:1 1 auto; flex-wrap:nowrap; overflow:hidden; flex-direction:column")),
            days = loadDays(),
            daysEdit = [],
            headerText = header.add(new innovaphone.ui1.Div("flex:1 1 auto; font-size:16px", texts.text("calendarWeek") + days[0].date.getWeekNumber() + " " + days[0].date.toLocaleDateString(start.lang) + " - " + days[4].date.toLocaleDateString(start.lang))),
            headerShow = header.add(new innovaphone.ui1.SvgInline("flex:0 0 auto; width:20px; height:20px; fill:var(--accent)", "0 0 20 20", innovaphone.Lunch.SymbolCodes.ArrowDown)),
            headerEdit = (admin ? header.add(new innovaphone.LunchSvgTitle("flex:0 0 auto; width:20px; height:20px; fill:var(--accent); margin-left:10px", "fill:var(--accent)", "0 0 20 20", innovaphone.Lunch.SymbolCodes.Edit, "edit", texts)) : null),
            headerExport = header.add(new innovaphone.LunchSvgTitle("flex:0 0 auto; width:20px; height:20px; fill:var(--accent); margin-left:10px", "fill:var(--accent)", "0 0 20 20", innovaphone.Lunch.SymbolCodes.Export, "export", texts)),
            orderWindowInput = null,
            buttons = this.add(new innovaphone.ui1.Div("display:none; flex:0 1 auto; flex-wrap:nowrap; overflow:hidden; flex-direction:row; margin-top:10px")),
            buttonSave = buttons.add(new innovaphone.LunchButton("flex:0 1 auto", "save", texts, editSave)),
            buttonCancel = buttons.add(new innovaphone.LunchButton("flex:0 1 auto; margin-left:10px", "cancel", texts, editCancel)),
            hidden = true;

        src.onmessage = onMessage;
        headerText.addEvent("click", toggle);
        headerShow.addEvent("click", toggle);
        if (headerEdit) headerEdit.addEvent("click", edit);
        headerExport.addEvent("click", exportCsv);
        src.send({ mt: "GetLunchs", from: weekStart, to: (weekStart + 4 * oneDayMs) });
        src.send({ mt: "GetPaninis", from: weekStart, to: (weekStart + 4 * oneDayMs) });
        src.send({ mt: "GetDailyOrders", from: weekStart, to: (weekStart + 4 * oneDayMs) });
        src.send({ mt: "GetMeals", from: weekStart, to: (weekStart + 4 * oneDayMs) });

        function onMessage(obj) {
            switch (obj.mt) {
                case "GetLunchsResult":
                    getLunchsResult(obj);
                    break;
                case "GetDailyOrdersResult":
                    getDailyOrdersResult(obj);
                    break;
                case "GetPaninisResult":
                    getPaninisResult(obj);
                    break;
                case "GetMealsResult":
                    getMealsResult(obj);
                    break;
                case "EditMealsResult":
                    editMealsResult(obj);
                    break;
            }
        }
        function loadDays() {
            var days = [];
            for (var i = 0; i < 5; i++) {
                var day = new DayNode(weekStart + (i * oneDayMs), weekNo, i + 1);
                days.push(day);
                daysNode.add(day);
            }
            return days;
        }

        function clearEdit() {
            texts.clear(editNode);
            editNode.clear();
            daysEdit.length = 0;
            editNode.container.style.display = "none";
            buttons.container.style.display = "none";
        }

        function edit() {
            clearEdit();
            hide();
            buttons.container.style.display = "flex";
            editNode.container.style.display = "flex";
            for (var i = 0; i < 5; i++) {
                var day = new DayEditNode(weekStart + (i * oneDayMs), days[i]);
                daysEdit.push(day);
                editNode.add(day);
            }
            editNode.add(new innovaphone.ui1.Div("flex:0 1 auto; overflow:hidden; margin-top:20px; font-size:15px", null, "label")).addTranslation(texts, "settingOrderWindow", null);
            orderWindowInput = editNode.add(new innovaphone.ui1.Input("flex:0 1 auto; max-width:80px; overflow:hidden; margin-top:5px", orderWindow / 60.0, "", null, "number"));
            orderWindowInput.setAttribute("step", "0.5");
        }

        function exportCsv() {
            var element = document.createElement("a");
            var text = "\ufeffTag;Anzahl Menü 1;Teilnehmer Menü 1;Anzahl Menü 2;Teilnehmer Menü 2;Anzahl Menü 3;Teilnehmer Menü 3;Anzahl Menü 4;Teilnehmer Menü 4;Anzahl Paninis;Teilnehmer Paninis;Anzahl Warmes Frühstück;Teilnehmer Warmes Frühstück\r\n";
            for (var i = 0; i < days.length; i++) {
                text += days[i].date.toLocaleDateString(start.lang) + ";";
                text += days[i].getCount(mealTypes.meat) + ";";
                text += days[i].getEaters(mealTypes.meat) + ";";
                text += days[i].getCount(mealTypes.veggy) + ";";
                text += days[i].getEaters(mealTypes.veggy) + ";";
                text += days[i].getCount(mealTypes.meal3) + ";";
                text += days[i].getEaters(mealTypes.meal3) + ";";
                text += days[i].getCount(mealTypes.meal4) + ";";
                text += days[i].getEaters(mealTypes.meal4) + ";";
                text += days[i].getCountPaninis() + ";";
                text += days[i].getEatersPanini() + ";";
                text += days[i].getCountDailyOrders() + ";";
                text += days[i].getEatersDailyOrders() + "\r\n";
            }
            element.setAttribute("href", "data:text/csv;charset=utf-8," + encodeURIComponent(text));
            element.setAttribute("download", headerText.container.textContent + ".csv");

            element.style.display = "none";
            document.body.appendChild(element);

            element.click();

            document.body.removeChild(element);
        }

        function editSave() {
            buttonSave.disable();
            buttonCancel.disable();
            var obj = { mt: "EditMeals" };
            var lunchs = [];
            for (var i = 0; i < daysEdit.length; i++) {
                var lunch = { timestamp: daysEdit[i].dayTime };
                lunch.meat = daysEdit[i].getMeat();
                lunch.meatType = daysEdit[i].getMeatType();
                lunch.veggy = daysEdit[i].getVeggy();
                lunch.veggyType = daysEdit[i].getVeggyType();
                lunch.meal3 = daysEdit[i].getMeal3();
                lunch.meal3Type = daysEdit[i].getMeal3Type();
                lunch.meal4 = daysEdit[i].getMeal4();
                lunch.meal4Type = daysEdit[i].getMeal4Type();
                lunchs.push(lunch);
            }
            obj.lunchs = lunchs;

            if (orderWindowInput.changed()) {
                config.orderWindow = orderWindowInput.getValue() * 60;
                config.save();
            }

            src.send(obj);
        }

        function editCancel() {
            clearEdit();
        }

        function close() {
            for (var i = 0; i < days.length; i++) {
                days[i].close();
            }
            texts.clear(that);
            src.close();
        }

        function hide() {
            headerShow.setCode(innovaphone.Lunch.SymbolCodes.ArrowDown);
            daysNode.container.style.display = "none";
            hidden = true;
        }

        function toggle() {
            if (hidden) {
                clearEdit();
                headerShow.setCode(innovaphone.Lunch.SymbolCodes.ArrowUp);
                daysNode.container.style.display = "flex";
                hidden = false;
            }
            else {
                hide();
            }
        }

        function editMealsResult(obj) {
            buttonCancel.enable();
            buttonSave.enable();
            orderWindow = config.orderWindow;
            clearEdit();
        }

        function getLunchsResult(obj) {
            if (obj.lunchs) {
                for (var i = 0; i < obj.lunchs.length; i++) {
                    for (var j = 0; j < days.length; j++) {
                        if (days[j].dayTime == obj.lunchs[i].timestamp) {
                            days[j].addEaters(obj.lunchs[i].eaters, obj.lunchs[i].timestamp);
                            break;
                        }
                    }
                }
            }
        }

        function getDailyOrdersResult(obj) {
            if (obj.dailyOrders) {
                for (var i = 0; i < obj.dailyOrders.length; i++) {
                    for (var j = 0; j < days.length; j++) {
                        if (days[j].dayTime == obj.dailyOrders[i].timestamp) {
                            days[j].addDailyOrderEaters(obj.dailyOrders[i].eaters, obj.dailyOrders[i].timestamp);
                            break;
                        }
                    }
                }
            }
        }

        function getPaninisResult(obj) {
            if (obj.paninis) {
                for (var i = 0; i < obj.paninis.length; i++) {
                    for (var j = 0; j < days.length; j++) {
                        if (days[j].dayTime == obj.paninis[i].timestamp) {
                            days[j].addPaniniEaters(obj.paninis[i].eaters, obj.paninis[i].timestamp);
                            break;
                        }
                    }
                }
            }
        }

        function getMealsResult(obj) {
            if (obj.meals) {
                for (var i = 0; i < obj.meals.length; i++) {
                    for (var j = 0; j < days.length; j++) {
                        if (days[j].dayTime == obj.meals[i].timestamp) {
                            days[j].updateMeals(obj.meals[i]);
                            break;
                        }
                    }
                }
            }
            for (var j = 0; j < days.length; j++) {
                days[j].start();
            }
        }
        this.close = close;
    }

    WeekNode.prototype = innovaphone.ui1.nodePrototype;

    var dateOptions = { weekday: 'long', year: 'numeric', month: 'numeric', day: 'numeric' };

    function DayNode(dayTime, weekNo, dayNo) {
        var date = new Date(dayTime),
            sips = {},
            dailyOrders = {},
            paninis = {},
            src = new app.Src(),
            srcAdmin = new app.Src(),
            dateEnterOver = false;

        this.createNode("div", "display:flex; flex:1 1 auto; flex-wrap:nowrap; overflow:hidden; flex-direction:column; margin-top:20px", null, "day-node");
        var header = this.add(new innovaphone.ui1.Div("display:flex; flex:1 1 auto; flex-wrap:nowrap; overflow:hidden; flex-direction:row; font-size:15px", date.toLocaleDateString(start.lang, dateOptions))),
            buttons = this.add(new innovaphone.ui1.Div("display:flex; flex:1 1 auto; flex-wrap:wrap; overflow:hidden; flex-direction:row; margin-top:5px")),
            meatGroup = buttons.add(new innovaphone.ui1.Div("display:flex; flex:0 1 auto; flex-wrap:nowrap; overflow:hidden; flex-direction:column; margin-right:10px; margin-top:10px")),
            meatOuter = meatGroup.add(new innovaphone.ui1.Div("display:flex; flex:1 1 auto; flex-wrap:nowrap; overflow:hidden; flex-direction:row")),
            mealMeatSvg1 = meatOuter.add((new innovaphone.ui1.SvgInline("flex:0 0 auto; width:20px; height:20px; fill:var(--menu1)", "0 0 20 20", innovaphone.Lunch.SymbolCodes.Menu1))),
            mealMeatSvg = meatOuter.add((new innovaphone.ui1.SvgInline("display:none; flex:0 0 auto; width:20px; height:20px; fill:var(--menu1); margin-right:10px", "0 0 20 20", innovaphone.Lunch.SymbolCodes.Menu1))),
            mealMeat = meatOuter.add(new innovaphone.ui1.Div("flex:1 1 auto; max-width:250px; font-size:14px; text-align:center")),
            buttonMeat = meatGroup.add(new innovaphone.LunchButton("flex:0 1 auto; margin-top:5px", dayNo == 5 ? "fish" : "meat", texts, eatMeat)),
            veggyGroup = buttons.add(new innovaphone.ui1.Div("display:flex; flex:0 1 auto; flex-wrap:nowrap; overflow:hidden; flex-direction:column; margin-right:10px; margin-top:10px")),
            veggyOuter = veggyGroup.add(new innovaphone.ui1.Div("display:flex; flex:1 1 auto; flex-wrap:nowrap; overflow:hidden; flex-direction:row")),
            mealVeggySvg2 = veggyOuter.add((new innovaphone.ui1.SvgInline("flex:0 0 auto; width:20px; height:20px; fill:var(--menu2)", "0 0 20 20", innovaphone.Lunch.SymbolCodes.Menu2))),
            mealVeggySvg = veggyOuter.add((new innovaphone.ui1.SvgInline("display:none; flex:0 0 auto; width:20px; height:20px; fill:var(--menu2); margin-right:10px", "0 0 20 20", innovaphone.Lunch.SymbolCodes.Menu2))),
            mealVeggy = veggyOuter.add(new innovaphone.ui1.Div("flex:1 1 auto; max-width:250px; font-size:14px; text-align:center")),
            buttonVeggy = veggyGroup.add(new innovaphone.LunchButton("flex:0 1 auto; margin-top:5px", "veggy", texts, eatVeggy)),
            buttonRemove = new innovaphone.LunchButton("flex:0 1 auto; margin-top:5px; max-width:250px", "cancelEat", texts, removeLunch, "button-cancel"),
            meal3Group = buttons.add(new innovaphone.ui1.Div("display:none; flex:0 1 auto; flex-wrap:nowrap; overflow:hidden; flex-direction:column; margin-right:10px; margin-top:10px")),
            meal3Outer = meal3Group.add(new innovaphone.ui1.Div("display:flex; flex:1 1 auto; flex-wrap:nowrap; overflow:hidden; flex-direction:row")),
            mealMeal3Svg3 = meal3Outer.add((new innovaphone.ui1.SvgInline("flex:0 0 auto; width:20px; height:20px; fill:var(--menu3)", "0 0 20 20", innovaphone.Lunch.SymbolCodes.Menu3))),
            mealMeal3Svg = meal3Outer.add((new innovaphone.ui1.SvgInline("display:none; flex:0 0 auto; width:20px; height:20px; fill:var(--menu3); margin-right:10px", "0 0 20 20", innovaphone.Lunch.SymbolCodes.Menu3))),
            mealMeal3 = meal3Outer.add(new innovaphone.ui1.Div("flex:1 1 auto; max-width:250px; font-size:14px; text-align:center")),
            buttonEatMeal3 = meal3Group.add(new innovaphone.LunchButton("flex:0 1 auto; margin-top:5px", "eatMeal3", texts, eatMeal3)),
            buttonRemoveMeal3 = meal3Group.add(new innovaphone.LunchButton("flex:0 1 auto; margin-top:5px", "cancelMeal3", texts, removeMeal3, "button-cancel")),
            meal4Group = buttons.add(new innovaphone.ui1.Div("display:none; flex:0 1 auto; flex-wrap:nowrap; overflow:hidden; flex-direction:column; margin-right:10px; margin-top:10px")),
            meal4Outer = meal4Group.add(new innovaphone.ui1.Div("display:flex; flex:1 1 auto; flex-wrap:nowrap; overflow:hidden; flex-direction:row")),
            mealMeal4Svg4 = meal4Outer.add((new innovaphone.ui1.SvgInline("flex:0 0 auto; width:20px; height:20px; fill:var(--menu4)", "0 0 20 20", innovaphone.Lunch.SymbolCodes.Menu4))),
            mealMeal4Svg = meal4Outer.add((new innovaphone.ui1.SvgInline("display:none; flex:0 0 auto; width:20px; height:20px; fill:var(--menu4); margin-right:10px", "0 0 20 20", innovaphone.Lunch.SymbolCodes.Menu4))),
            mealMeal4 = meal4Outer.add(new innovaphone.ui1.Div("flex:1 1 auto; max-width:250px; font-size:14px; text-align:center")),
            buttonEatMeal4 = meal4Group.add(new innovaphone.LunchButton("flex:0 1 auto; margin-top:5px", "eatMeal4", texts, eatMeal4)),
            buttonRemoveMeal4 = meal4Group.add(new innovaphone.LunchButton("flex:0 1 auto; margin-top:5px", "cancelMeal4", texts, removeMeal4, "button-cancel")),
            buttonEatPanini = this.add(new innovaphone.LunchButton("flex:0 1 auto; margin-top:5px; max-width:250px", "eatPanini", texts, eatPanini)),
            buttonRemovePanini = this.add(new innovaphone.LunchButton("flex:0 1 auto; margin-top:5px; max-width:250px", "cancelPanini", texts, removePanini, "button-cancel")),
            buttonEatDaily = this.add(new innovaphone.LunchButton("flex:0 1 auto; margin-top:5px; max-width:250px", "eatDaily", texts, eatDaily)),
            buttonRemoveDaily = this.add(new innovaphone.LunchButton("flex:0 1 auto; margin-top:5px; max-width:250px", "cancelDaily", texts, removeDaily, "button-cancel")),
            eaters = this.add(new innovaphone.ui1.Div("display:flex; flex:1 1 auto; flex-wrap:wrap; overflow:hidden; flex-direction:row")),
            newEater = null,
            newEaterSip = null,
            newEaterVeggy = null,
            newEaterVeggySvg = null,
            newEaterMeat = null,
            newEaterMeatSvg = null,
            newEaterMeal3 = null,
            newEaterMeal3Svg = null,
            newEaterMeal4 = null,
            newEaterMeal4Svg = null,
            newEaterConfirm = null,
            meals = null;

        src.onmessage = onMessage;
        srcAdmin.onmessage = onAdminMessage;
        buttonVeggy.hide();
        buttonMeat.hide();
        buttonEatMeal3.hide();
        buttonRemoveMeal3.hide();
        buttonEatMeal4.hide();
        buttonRemoveMeal4.hide();
        buttonRemove.hide();
        buttonEatPanini.hide();
        buttonRemovePanini.hide();
        buttonEatDaily.hide();
        buttonRemoveDaily.hide();
        evUpdateButtons.attach(updateButtons);
        evMealsEdited.attach(mealsEdited);
        evDailiesEdited.attach(dailiesEdited);
        evLunchRemoved.attach(lunchRemoved);
        evLunchAdded.attach(lunchAdded);
        evDailyOrderRemoved.attach(dailyOrderRemoved);
        evDailyOrderAdded.attach(dailyOrderAdded);
        evPaniniRemoved.attach(paniniRemoved);
        evPaniniAdded.attach(paniniAdded);

        if (admin) {
            newEater = this.add(new innovaphone.ui1.Div("display:flex; flex:1 1 auto; flex-wrap:wrap; overflow:hidden; flex-direction:row; align-items:center; margin-top:10px"));
            newEaterSip = newEater.add(new innovaphone.ui1.Input("flex:0 1 auto; max-width:50px", "", "sip", 6));
            newEaterMeatSvg = newEater.add(new innovaphone.ui1.SvgInline("flex:0 0 auto; width:20px; height:20px; fill:var(--menu1); margin-left:10px", "0 0 20 20", innovaphone.Lunch.SymbolCodes.Menu1));
            newEaterMeat = newEater.add(new innovaphone.ui1.Checkbox("flex:0 0 auto; overflow:hidden; margin-left:2px; cursor:pointer", false, null, "var(--accent)", "var(--c1)", "var(--c1)"));
            newEaterVeggySvg = newEater.add(new innovaphone.ui1.SvgInline("flex:0 0 auto; width:20px; height:20px; fill:var(--menu2); margin-left:10px", "0 0 20 20", innovaphone.Lunch.SymbolCodes.Menu2));
            newEaterVeggy = newEater.add(new innovaphone.ui1.Checkbox("flex:0 0 auto; overflow:hidden; margin-left:2px; cursor:pointer", false, null, "var(--accent)", "var(--c1)", "var(--c1)"));
            newEaterMeal3Svg = newEater.add(new innovaphone.ui1.SvgInline("display:none; flex:0 0 auto; width:20px; height:20px; fill:var(--menu3); margin-left:10px", "0 0 20 20", innovaphone.Lunch.SymbolCodes.Menu3));
            newEaterMeal3 = newEater.add(new innovaphone.ui1.Checkbox("display:none; flex:0 0 auto; overflow:hidden; margin-left:2px; cursor:pointer", false, null, "var(--accent)", "var(--c1)", "var(--c1)"));
            newEaterMeal4Svg = newEater.add(new innovaphone.ui1.SvgInline("display:none; flex:0 0 auto; width:20px; height:20px; fill:var(--menu4); margin-left:10px", "0 0 20 20", innovaphone.Lunch.SymbolCodes.Menu4));
            newEaterMeal4 = newEater.add(new innovaphone.ui1.Checkbox("display:none; flex:0 0 auto; overflow:hidden; margin-left:2px; cursor:pointer", false, null, "var(--accent)", "var(--c1)", "var(--c1)"));
            newEaterConfirm = newEater.add(new innovaphone.ui1.SvgInline("flex:0 0 auto; width:15px; height:15px; fill:var(--c1); margin-left:10px; cursor:pointer", "0 0 20 20", innovaphone.Lunch.SymbolCodes.CheckMark));
            newEaterConfirm.addEvent("click", addNewEater);
        }
        
        function close() {
            src.close();
            evUpdateButtons.detach(updateButtons);
            evMealsEdited.detach(mealsEdited);
            evLunchRemoved.detach(lunchRemoved);
            evLunchAdded.detach(lunchAdded);
            evDailiesEdited.detach(dailiesEdited);
            evDailyOrderRemoved.detach(dailyOrderRemoved);
            evDailyOrderAdded.detach(dailyOrderAdded);
            evPaniniRemoved.detach(paniniRemoved);
            evPaniniAdded.detach(paniniAdded);
        }

        function chkMeal4() {
            if (getMeal4() == "") {
                meal4Group.setStyle("display", "none");
                if (newEaterMeal4Svg) {
                    newEaterMeal4Svg.setStyle("display", "none");
                    newEaterMeal4.setStyle("display", "none");
                }
            }
            else {
                meal4Group.setStyle("display", "flex");
                if (newEaterMeal4Svg) {
                    newEaterMeal4Svg.setStyle("display", "");
                    newEaterMeal4.setStyle("display", "flex");
                }
            }
        }

        function chkMeal3() {
            if (getMeal3() == "") {
                meal3Group.setStyle("display", "none");
                if (newEaterMeal3Svg) {
                    newEaterMeal3Svg.setStyle("display", "none");
                    newEaterMeal3.setStyle("display", "none");
                }
            }
            else {
                meal3Group.setStyle("display", "flex");
                if (newEaterMeal3Svg) {
                    newEaterMeal3Svg.setStyle("display", "");
                    newEaterMeal3.setStyle("display", "flex");
                }
            }
        }

        function dailiesEdited(sender, obj) {

        }

        function updateButtons() {
            dateEnterOver = false;
            dateCancelOver = false;
            var localNowTime = localNow.getTime();

            if (weekNo == 2) {  // second week -> was always wrong on a sunday, as the next week then is the first week? ...
                if (dayNo == 1) {   // lunch on monday?
                    localNowTime += (2 * oneDayMs);
                }
                /*else if (dayNo == 2) {
                    localNowTime += (3 * oneDayMs);
                }*/
            }
            var tzOffset = (localNow.getTimezoneOffset() * 60 * 1000);
            var dayTimeMoved = (dayTime - ((oneDayMs / 24) * (orderWindow / 60.0)) + tzOffset);    // order until 11.30 o'clock of previous day, so -12.5 hours
            if (localNowTime > dayTimeMoved) {
                dateEnterOver = true;
                dateCancelOver = true;
            }

            // code for no time limit
            //if (localNowTime > (dayTime + (oneDayMs / 2) + (oneDayMs / 12))) {
            //    dateEnterOver = true;
            //    dateCancelOver = true;
            //}
            if (dateCancelOver) {
                buttonVeggy.hide();
                buttonMeat.hide();
                buttonRemove.hide();
                buttonEatPanini.hide();
                buttonRemovePanini.hide();
                buttonEatDaily.hide();
                buttonRemoveDaily.hide();
                buttonRemoveMeal3.hide();
                buttonEatMeal3.hide();
                buttonRemoveMeal4.hide();
                buttonEatMeal4.hide();
            }
            else {
                if (dateEnterOver) {
                    buttonVeggy.hide();
                    buttonMeat.hide();
                    buttonEatDaily.hide();
                    buttonEatPanini.hide();
                    buttonEatMeal3.hide();
                    buttonEatMeal4.hide();
                    if (sips[getKey(mySip, mealTypes.meat)]) {
                        meatGroup.add(buttonRemove);
                        buttonRemove.show();
                    }
                    else if (sips[getKey(mySip, mealTypes.veggy)]) {
                        veggyGroup.add(buttonRemove);
                        buttonRemove.show();
                    }
                    else {
                        buttonRemove.hide();
                    }
                    if (sips[getKey(mySip, mealTypes.meal3)]) {
                        buttonRemoveMeal3.show();
                    }
                    else {
                        buttonRemoveMeal3.hide();
                    }
                    if (sips[getKey(mySip, mealTypes.meal4)]) {
                        buttonRemoveMeal4.show();
                    }
                    else {
                        buttonRemoveMeal4.hide();
                    }
                    if (paninis[mySip]) {
                        buttonRemovePanini.show();
                    }
                    else {
                        buttonRemovePanini.hide();
                    }
                    if (dailyOrders[mySip]) {
                        buttonRemoveDaily.show();
                    }
                    else {
                        buttonRemoveDaily.hide();
                    }
                }
                else {
                    if (sips[getKey(mySip, mealTypes.meat)]) {
                        buttonVeggy.hide();
                        buttonMeat.hide();
                        meatGroup.add(buttonRemove);
                        buttonRemove.show();
                    }
                    else if (sips[getKey(mySip, mealTypes.veggy)]) {
                        buttonVeggy.hide();
                        buttonMeat.hide();
                        veggyGroup.add(buttonRemove);
                        buttonRemove.show();
                    }
                    else {
                        buttonVeggy.show();
                        buttonMeat.show();
                        buttonRemove.hide();
                    }
                    if (sips[getKey(mySip, mealTypes.meal3)]) {
                        buttonEatMeal3.hide();
                        buttonRemoveMeal3.show();
                    }
                    else {
                        buttonEatMeal3.show();
                        buttonRemoveMeal3.hide();
                    }
                    if (sips[getKey(mySip, mealTypes.meal4)]) {
                        buttonEatMeal4.hide();
                        buttonRemoveMeal4.show();
                    }
                    else {
                        buttonEatMeal4.show();
                        buttonRemoveMeal4.hide();
                    }
                    if (paninis[mySip]) {
                        buttonEatPanini.hide();
                        buttonRemovePanini.show();
                    }
                    else {
                        buttonEatPanini.show();
                        buttonRemovePanini.hide();
                    }
                    if (dailyOrders[mySip]) {
                        buttonEatDaily.hide();
                        buttonRemoveDaily.show();
                    }
                    else {
                        buttonEatDaily.show();
                        buttonRemoveDaily.hide();
                    }
                }
            }
        }

        function getVeggy() {
            return meals ? meals.veggy : "";
        }

        function getMeat() {
            return meals ? meals.meat : "";
        }

        function getMeal3() {
            return (meals && meals.meal3) ? meals.meal3 : "";
        }

        function getMeal4() {
            return (meals && meals.meal4) ? meals.meal4 : "";
        }

        function getVeggyType() {
            return meals ? meals.veggy_type : "";
        }

        function getMeatType() {
            return meals ? meals.meat_type : "";
        }

        function getMeal3Type() {
            return (meals && meals.meal3_type) ? meals.meal3_type : "";
        }

        function getMeal4Type() {
            return (meals && meals.meal4_type) ? meals.meal4_type : "";
        }

        function onMessage(obj) {
            if (obj.error && obj.error != "") {
                new innovaphone.Lunch.Error(body, obj.error, texts);
                return;
            }
            switch (obj.mt) {
                case "AddLunchResult":
                    addLunchResult(obj.type);
                    break;
                case "RemoveLunchResult":
                    removeLunchResult(obj.type); 
                    break;
                case "AddDailyOrderResult":
                    addDailyOrderResult();
                    break;
                case "RemoveDailyOrderResult":
                    removeDailyOrderResult();
                    break;
                case "AddPaniniResult":
                    addPaniniResult();
                    break;
                case "RemovePaniniResult":
                    removePaniniResult();
                    break;
            }
        }

        function onAdminMessage(obj) {
            if (obj.error && obj.error != "") {
                new innovaphone.Lunch.Error(body, obj.error, texts);
                return;
            }
            switch (obj.mt) {
                case "AddLunchResult":
                    newEaterSip.setValue("");
                    newEaterMeat.setValue(false);
                    newEaterVeggy.setValue(false);
                    newEaterMeal3.setValue(false);
                    newEaterMeal4.setValue(false);
                    newEaterConfirm.addEvent("click", addNewEater);
                    break;
                case "RemoveLunchResult":
                    break;
            }
        }

        function removeLunchResult(type) {
            switch (type) {
                case mealTypes.meal3:
                    buttonRemoveMeal3.enable();
                    buttonRemoveMeal3.hide();
                    if (!dateEnterOver) {
                        buttonEatMeal3.show();
                    }
                    break;
                case mealTypes.meal4:
                    buttonRemoveMeal4.enable();
                    buttonRemoveMeal4.hide();
                    if (!dateEnterOver) {
                        buttonEatMeal4.show();
                    }
                    break;
                default:
                    buttonRemove.enable();
                    buttonRemove.hide();
                    if (!dateEnterOver) {
                        buttonVeggy.show();
                        buttonMeat.show();
                    }
                    break;
            }
        }

        function addLunchResult(type) {
            switch (type) {
                case mealTypes.meal3:
                    buttonEatMeal3.enable();
                    buttonEatMeal3.hide();
                    buttonRemoveMeal3.show();
                    break;
                case mealTypes.meal4:
                    buttonEatMeal4.enable();
                    buttonEatMeal4.hide();
                    buttonRemoveMeal4.show();
                    break;
                default:
                    buttonVeggy.enable();
                    buttonMeat.enable();
                    buttonVeggy.hide();
                    buttonMeat.hide();
                    if (type == mealTypes.meat) {
                        meatGroup.add(buttonRemove);
                    }
                    else {
                        veggyGroup.add(buttonRemove);
                    }
                    buttonRemove.show();
                    break;
            }
        }

        function removeDailyOrderResult() {
            buttonRemoveDaily.enable();
            buttonRemoveDaily.hide();
            if (!dateEnterOver) {
                buttonEatDaily.show();
            }
        }

        function addDailyOrderResult() {
            buttonEatDaily.enable();
            buttonEatDaily.hide();
            buttonRemoveDaily.show();
        }

        function removePaniniResult() {
            buttonRemovePanini.enable();
            buttonRemovePanini.hide();
            if (!dateEnterOver) {
                buttonEatPanini.show();
            }
        }

        function addPaniniResult() {
            buttonEatPanini.enable();
            buttonEatPanini.hide();
            buttonRemovePanini.show();
        }

        function addNewEater() {
            if (newEaterSip.getValue().length > 6) return;
            if (newEaterVeggy.getValue() || newEaterMeal3.getValue() || newEaterMeal4.getValue() || newEaterMeat.getValue()) {
                newEaterConfirm.remEvent("click", addNewEater);
                if (newEaterVeggy.getValue()) {
                    srcAdmin.send({ mt: "AddLunch", sip: newEaterSip.getValue(), timestamp: dayTime, type: mealTypes.veggy, byAdmin: true });
                }
                else if (newEaterMeat.getValue()) {
                    srcAdmin.send({ mt: "AddLunch", sip: newEaterSip.getValue(), timestamp: dayTime, type: mealTypes.meat, byAdmin: true });
                }
                if (newEaterMeal3.getValue()) {
                    srcAdmin.send({ mt: "AddLunch", sip: newEaterSip.getValue(), timestamp: dayTime, type: mealTypes.meal3, byAdmin: true });
                }
                if (newEaterMeal4.getValue()) {
                    srcAdmin.send({ mt: "AddLunch", sip: newEaterSip.getValue(), timestamp: dayTime, type: mealTypes.meal4, byAdmin: true });
                }
            }
        }

        function eatVeggy() {
            buttonVeggy.disable();
            buttonMeat.disable();
            src.send({ mt: "AddLunch", sip: mySip, timestamp: dayTime, type: mealTypes.veggy });
        }

        function eatMeat() {
            buttonVeggy.disable();
            buttonMeat.disable();
            src.send({ mt: "AddLunch", sip: mySip, timestamp: dayTime, type: mealTypes.meat });
        }

        function eatMeal3() {
            buttonEatMeal3.disable();
            src.send({ mt: "AddLunch", sip: mySip, timestamp: dayTime, type: mealTypes.meal3 });
        }

        function eatMeal4() {
            buttonEatMeal4.disable();
            src.send({ mt: "AddLunch", sip: mySip, timestamp: dayTime, type: mealTypes.meal4 });
        }

        function eatPanini() {
            buttonEatPanini.disable();
            src.send({ mt: "AddPanini", sip: mySip, timestamp: dayTime });
        }

        function eatDaily() {
            buttonEatDaily.disable();
            src.send({ mt: "AddDailyOrder", sip: mySip, timestamp: dayTime });
        }

        function removeLunch() {
            if (dateEnterOver) {
                body.add(new Popup(null, texts, doRemoveLunch));
            }
            else {
                doRemoveLunch();
            }
        }

        function doRemoveLunch() {
            buttonRemove.disable();
            src.send({ mt: "RemoveLunch", sip: mySip, timestamp: dayTime, mail: dateEnterOver, type: (sips[getKey(mySip, mealTypes.meat)] ? mealTypes.meat : mealTypes.veggy) });
        }

        function removeMeal3() {
            if (dateEnterOver) {
                body.add(new Popup(null, texts, doRemoveMeal3));
            }
            else {
                doRemoveMeal3();
            }
        }

        function doRemoveMeal3() {
            buttonRemoveMeal3.disable();
            src.send({ mt: "RemoveLunch", sip: mySip, timestamp: dayTime, mail: dateEnterOver, type: mealTypes.meal3 });
        }

        function removeMeal4() {
            if (dateEnterOver) {
                body.add(new Popup(null, texts, doRemoveMeal4));
            }
            else {
                doRemoveMeal4();
            }
        }

        function doRemoveMeal4() {
            buttonRemoveMeal4.disable();
            src.send({ mt: "RemoveLunch", sip: mySip, timestamp: dayTime, mail: dateEnterOver, type: mealTypes.meal4 });
        }
        
        function removeDaily() {
            if (dateEnterOver) {
                body.add(new Popup(null, texts, doRemoveDaily));
            }
            else {
                doRemoveDaily();
            }
        }

        function doRemoveDaily() {
            buttonRemoveDaily.disable();
            src.send({ mt: "RemoveDailyOrder", sip: mySip, timestamp: dayTime, mail: dateEnterOver });
        }

        function removePanini() {
            if (dateEnterOver) {
                body.add(new Popup(null, texts, doRemovePanini));
            }
            else {
                doRemovePanini();
            }
        }

        function doRemovePanini() {
            buttonRemovePanini.disable();
            src.send({ mt: "RemovePanini", sip: mySip, timestamp: dayTime, mail: dateEnterOver });
        }

        function getKey(sip, type) {
            return sip + "." + type;
        }

        function addLunch(sip, type, timestamp) {
            if (!sips[getKey(sip, type)]) {
                sips[getKey(sip, type)] = eaters.add(new EaterNode(sip, type, timestamp, srcAdmin));
                return true;
            }
            return false;
        }

        function addDailyOrder(sip, timestamp) {
            if (!dailyOrders[sip]) {
                dailyOrders[sip] = { sip: sip, timestamp: timestamp };;
                return true;
            }
            return false;
        }

        function addPanini(sip, timestamp) {
            if (!paninis[sip]) {
                paninis[sip] = { sip: sip, timestamp: timestamp };;
                return true;
            }
            return false;
        }

        function compareLunch(a, b) {
            if (sips[a].type != sips[b].type) return sips[a].type > sips[b].type ? 1 : -1;
            else return a > b ? 1 : -1;
        }

        function lunchAdded(sender, obj) {
            if (obj.timestamp == dayTime) {
                if (addLunch(obj.sip, obj.type, obj.timestamp)) {
                    eaters.clear();
                    Object.keys(sips).sort(compareLunch).forEach(function (keyValue, index, map) {
                        eaters.add(sips[keyValue]);
                    });
                }                
                if (obj.sip == mySip) {
                    addLunchResult(obj.type);
                }
            }
        }

        function lunchRemoved(sender, obj) {
            if (obj.timestamp == dayTime) {
                var key = getKey(obj.sip, obj.type);
                if (sips[key]) {
                    eaters.rem(sips[key]);
                    delete sips[key];
                    if (obj.sip == mySip) {
                        removeLunchResult(obj.type);
                    }
                }
            }
        }

        function dailyOrderAdded(sender, obj) {
            if (obj.timestamp == dayTime) {
                if (addDailyOrder(obj.sip, obj.timestamp)) {
                }
                if (obj.sip == mySip) {
                    addDailyOrderResult();
                }
            }
        }

        function dailyOrderRemoved(sender, obj) {
            if (obj.timestamp == dayTime) {
                if (dailyOrders[obj.sip]) {
                    delete dailyOrders[obj.sip];
                    if (obj.sip == mySip) {
                        removeDailyOrderResult();
                    }
                }
            }
        }

        function paniniAdded(sender, obj) {
            if (obj.timestamp == dayTime) {
                if (addPanini(obj.sip, obj.timestamp)) {
                }
                if (obj.sip == mySip) {
                    addPaniniResult();
                }
            }
        }

        function paniniRemoved(sender, obj) {
            if (obj.timestamp == dayTime) {
                if (paninis[obj.sip]) {
                    delete paninis[obj.sip];
                    if (obj.sip == mySip) {
                        removePaniniResult();
                    }
                }
            }
        }

        function addEaters(obj, timestamp) {
            if (obj) {
                for (var i = 0; i < obj.length; i++) {
                    addLunch(obj[i].sip, obj[i].type, timestamp);
                }
            }
        }

        function addDailyOrderEaters(obj, timestamp) {
            if (obj) {
                for (var i = 0; i < obj.length; i++) {
                    addDailyOrder(obj[i].sip, timestamp);
                }
            }
        }

        function addPaniniEaters(obj, timestamp) {
            if (obj) {
                for (var i = 0; i < obj.length; i++) {
                    addPanini(obj[i].sip, timestamp);
                }
            }
        }

        function mealsEdited(sender, obj) {
            for (var i = 0; i < obj.length; i++) {
                if (obj[i].timestamp == dayTime) {
                    updateMeals(obj[i]);
                    break;
                }
            }
        }

        function updateMealType(svg, type) {
            var svgCode = null;
            switch (type) {
                case "fish": svgCode = innovaphone.Lunch.SymbolCodes.Fish; break;
                case "veggy": svgCode = innovaphone.Lunch.SymbolCodes.Veggy; break;
                case "soup": svgCode = innovaphone.Lunch.SymbolCodes.Soup; break;
                case "meat":
                default:
                    svgCode = innovaphone.Lunch.SymbolCodes.Meat;
                    break;                    
            }
            svg.setCode(svgCode);
            svg.setStyle("fill", "var(--" + type + ")");
            svg.setStyle("display", "");
        }

        function updateMeals(obj) {
            meals = obj;
            mealVeggy.addText(obj.veggy);
            mealMeat.addText(obj.meat);
            mealMeal3.addText(obj.meal3);
            mealMeal4.addText(obj.meal4);
            updateMealType(mealMeatSvg, obj.meat_type);
            updateMealType(mealVeggySvg, obj.veggy_type);
            updateMealType(mealMeal3Svg, obj.meal3_type);
            updateMealType(mealMeal4Svg, obj.meal4_type);
            chkMeal3();
            chkMeal4();
        }

        function start() {
            updateButtons();
        }

        function getCount(type) {
            var count = 0;
            Object.keys(sips).forEach(function (keyValue, index, map) {
                if (sips[keyValue].type == type) {
                    if (keyValue.indexOf("+") > 0) {
                        count += parseInt(keyValue.substring(keyValue.indexOf("+") + 1));
                    }
                    else {
                        ++count;
                    }
                }
            });
            return count;
        }

        function getEaters(type) {
            var eaters = "";
            Object.keys(sips).forEach(function (keyValue, index, map) {
                if (sips[keyValue].type == type) {
                    eaters += sips[keyValue].sip.replace("m\+", "+").replace("v\+", "+") + ",";
                }
            });
            if (eaters != "") {
                eaters = eaters.substring(0, eaters.length - 1);
            }
            return eaters;
        }

        function getCountPaninis() {
            var count = 0;
            Object.keys(paninis).forEach(function (keyValue, index, map) {
                ++count;
            });
            return count;
        }

        function getEatersPanini() {
            var eaters = "";
            Object.keys(paninis).forEach(function (keyValue, index, map) {
                eaters += keyValue.replace("m\+", "+").replace("v\+", "+") + ",";
            });
            if (eaters != "") {
                eaters = eaters.substring(0, eaters.length - 1);
            }
            return eaters;
        }

        function getCountDailyOrders() {
            var count = 0;
            Object.keys(dailyOrders).forEach(function (keyValue, index, map) {
                ++count;
            });
            return count;
        }

        function getEatersDailyOrders() {
            var eaters = "";
            Object.keys(dailyOrders).forEach(function (keyValue, index, map) {
                eaters += keyValue.replace("m\+", "+").replace("v\+", "+") + ",";
            });
            if (eaters != "") {
                eaters = eaters.substring(0, eaters.length - 1);
            }
            return eaters;
        }

        this.date = date;
        this.dayTime = dayTime;
        this.lunchAdded = lunchAdded;
        this.lunchRemoved = lunchRemoved;
        this.addEaters = addEaters;
        this.addDailyOrderEaters = addDailyOrderEaters;
        this.addPaniniEaters = addPaniniEaters;
        this.updateMeals = updateMeals;
        this.getVeggy = getVeggy;
        this.getVeggyType = getVeggyType;
        this.getMeat = getMeat;
        this.getMeatType = getMeatType;
        this.getMeal3 = getMeal3;
        this.getMeal3Type = getMeal3Type;
        this.getMeal4 = getMeal4;
        this.getMeal4Type = getMeal4Type;
        this.start = start;
        this.close = close;
        this.getCount = getCount;
        this.getEaters = getEaters;
        this.getCountPaninis = getCountPaninis;
        this.getEatersPanini = getEatersPanini;
        this.getCountDailyOrders = getCountDailyOrders;
        this.getEatersDailyOrders = getEatersDailyOrders;
    }

    DayNode.prototype = innovaphone.ui1.nodePrototype;

    function EaterNode(sip, type, timestamp, srcAdmin) {
        var node = this.createNode("div", "display:flex; flex:0 1 auto; flex-wrap:nowrap; overflow:hidden; flex-direction:row; margin-top:5px; margin-right:5px; padding:2px; border:1px solid var(--bg2); align-items:center");
        node.add(new innovaphone.ui1.Div("flex:0 0 auto; margin-right:2px", sip.replace("m\+", "+").replace("v\+", "+")));

        switch (type) {
            case mealTypes.meat:
                node.add(new innovaphone.ui1.SvgInline("flex:0 0 auto; width:20px; height:20px; fill:var(--menu1)", "0 0 20 20", innovaphone.Lunch.SymbolCodes.Menu1));
                break;
            case mealTypes.veggy:
                node.add(new innovaphone.ui1.SvgInline("flex:0 0 auto; width:20px; height:20px; fill:var(--menu2)", "0 0 20 20", innovaphone.Lunch.SymbolCodes.Menu2));
                break;
            case mealTypes.meal3:
                node.add(new innovaphone.ui1.SvgInline("flex:0 0 auto; width:20px; height:20px; fill:var(--menu3)", "0 0 20 20", innovaphone.Lunch.SymbolCodes.Menu3));
                break;
            case mealTypes.meal4:
                node.add(new innovaphone.ui1.SvgInline("flex:0 0 auto; width:20px; height:20px; fill:var(--menu4)", "0 0 20 20", innovaphone.Lunch.SymbolCodes.Menu4));
                break;
        }        

        if (admin) {
            var remove = node.add(new innovaphone.ui1.SvgInline("flex:0 0 auto; width:15px; height:15px; fill:var(--c1); margin-left:5px; cursor:pointer", "0 0 20 20", innovaphone.Lunch.SymbolCodes.Delete));
            remove.addEvent("click", removeLunch);
        }

        function removeLunch() {
            body.add(new Popup(null, texts, doRemoveLunch, "removeAdmin", sip));
        }

        function doRemoveLunch() {
            srcAdmin.send({ mt: "RemoveLunch", sip: sip, timestamp: timestamp, type: type, byAdmin: true });
        }

        this.sip = sip;
        this.type = type;
    }

    EaterNode.prototype = innovaphone.ui1.nodePrototype;

    function DayEditNodeType(tlId, value, type) {
        this.createNode("div", "display:flex; flex:1 1 auto; flex-wrap:nowrap; overflow:hidden; flex-direction:column; margin-top:10px");

        var text = this.add(new innovaphone.ui1.Node("textarea", "flex:1 1 auto").addTranslation(texts, tlId, "placeholder")),
            types = this.add(new innovaphone.ui1.Div("display:flex; flex:1 1 auto; flex-wrap:wrap; overflow:hidden; flex-direction:row")),
            typeMeatSvg = types.add(new innovaphone.ui1.SvgInline("flex:0 0 auto; width:20px; height:20px; fill:var(--meat); margin-left:10px; margin-top:10px", "0 0 20 20", innovaphone.Lunch.SymbolCodes.Meat)),
            typeMeatChk = types.add(new innovaphone.ui1.Checkbox("flex:0 0 auto; overflow:hidden; margin-left:2px; cursor:pointer; margin-top:10px", type == "meat", null, "var(--accent)", "var(--c1)", "var(--c1)")),
            typeVeggySvg = types.add(new innovaphone.ui1.SvgInline("flex:0 0 auto; width:20px; height:20px; fill:var(--veggy); margin-left:10px; margin-top:10px", "0 0 20 20", innovaphone.Lunch.SymbolCodes.Veggy)),
            typeVeggyChk = types.add(new innovaphone.ui1.Checkbox("flex:0 0 auto; overflow:hidden; margin-left:2px; cursor:pointer; margin-top:10px", type == "veggy", null, "var(--accent)", "var(--c1)", "var(--c1)")),
            typeFishSvg = types.add(new innovaphone.ui1.SvgInline("flex:0 0 auto; width:20px; height:20px; fill:var(--fish); margin-left:10px; margin-top:10px", "0 0 20 20", innovaphone.Lunch.SymbolCodes.Fish)),
            typeFishChk = types.add(new innovaphone.ui1.Checkbox("flex:0 0 auto; overflow:hidden; margin-left:2px; cursor:pointer; margin-top:10px", type == "fish", null, "var(--accent)", "var(--c1)", "var(--c1)")),
            typeSoupSvg = types.add(new innovaphone.ui1.SvgInline("flex:0 0 auto; width:20px; height:20px; fill:var(--soup); margin-left:10px; margin-top:10px", "0 0 20 20", innovaphone.Lunch.SymbolCodes.Soup)),
            typeSoupChk = types.add(new innovaphone.ui1.Checkbox("flex:0 0 auto; overflow:hidden; margin-left:2px; cursor:pointer; margin-top:10px", type == "soup", null, "var(--accent)", "var(--c1)", "var(--c1)"));

        text.container.value = value;

        typeMeatChk.addEvent("change", onChk);
        typeVeggyChk.addEvent("change", onChk);
        typeFishChk.addEvent("change", onChk);
        typeSoupChk.addEvent("change", onChk);

        function onChk(e, sender) {
            if (sender.getValue()) {
                switch (sender) {
                    case typeMeatChk:
                        typeVeggyChk.setValue(false);
                        typeFishChk.setValue(false);
                        typeSoupChk.setValue(false);
                        break;
                    case typeVeggyChk:
                        typeMeatChk.setValue(false);
                        typeFishChk.setValue(false);
                        typeSoupChk.setValue(false);
                        break;
                    case typeFishChk:
                        typeVeggyChk.setValue(false);
                        typeMeatChk.setValue(false);
                        typeSoupChk.setValue(false);
                        break;
                    case typeSoupChk:
                        typeVeggyChk.setValue(false);
                        typeMeatChk.setValue(false);
                        typeFishChk.setValue(false);
                        break;
                }
            }
        }

        function getText() {
            return text.container.value.trim();
        }

        function getType() {
            if (typeMeatChk.getValue()) return "meat";
            else if (typeVeggyChk.getValue()) return "veggy";
            else if (typeFishChk.getValue()) return "fish";
            else if (typeSoupChk.getValue()) return "soup";
            else return "meat";
        }

        this.getText = getText;
        this.getType = getType;
    }
    DayEditNodeType.prototype = innovaphone.ui1.nodePrototype;

    function DayEditNode(dayTime, dayNode) {
        this.createNode("div", "display:flex; flex:1 1 auto; flex-wrap:nowrap; overflow:hidden; flex-direction:column; margin-top:10px");

        var date = new Date(dayTime),
            header = this.add(new innovaphone.ui1.Div("display:flex; flex:1 1 auto; flex-wrap:nowrap; overflow:hidden; flex-direction:row; font-size:15px", date.toLocaleDateString(start.lang))),
            descs = this.add(new innovaphone.ui1.Div("display:flex; flex:1 1 auto; flex-wrap:nowrap; overflow:hidden; flex-direction:row")),
            meat = descs.add(new DayEditNodeType("meat", dayNode.getMeat(), dayNode.getMeatType())),
            veggy = descs.add(new DayEditNodeType("veggy", dayNode.getVeggy(), dayNode.getVeggyType())),
            meal3 = descs.add(new DayEditNodeType("meal3", dayNode.getMeal3(), dayNode.getMeal3Type())),
            meal4 = descs.add(new DayEditNodeType("meal4", dayNode.getMeal4(), dayNode.getMeal4Type()));

        this.date = date;
        this.dayTime = dayTime;
        this.getMeat = meat.getText;
        this.getMeatType = meat.getType;
        this.getVeggy = veggy.getText;
        this.getVeggyType = veggy.getType;
        this.getMeal3 = meal3.getText;
        this.getMeal3Type = meal3.getType;
        this.getMeal4 = meal4.getText;
        this.getMeal4Type = meal4.getType;
    }

    DayEditNode.prototype = innovaphone.ui1.nodePrototype;
}

innovaphone.Lunch.prototype = innovaphone.ui1.nodePrototype;

innovaphone.LunchButton = innovaphone.LunchButton || function (style, translationKey, texts, onClick, addClass) {
    var that = this,
        enabled = false;

    style += "; height:30px; text-align:center; padding:5px 10px 5px 10px; cursor:pointer; font-size:16px;";
    this.createNode("input", style);
    this.setNoSelect();
    this.container.setAttribute("type", "submit");
    if (addClass) {
        that.addClass(addClass);
    }

    this.addTranslation(texts, translationKey, "value");
    
    function enable() {
        if (!enabled) {
            that.remClass("button-disabled");
            that.addEvent("click", onClick);
            that.addClass("button");
            enabled = true;
        }
    }

    function disable() {
        if (enabled) {
            that.remEvent("click", onClick);
            that.remClass("button");
            that.addClass("button-disabled");
            enabled = false;
        }
    }

    function show() {
        that.container.style.display = "";
    }

    function hide() {
        that.container.style.display = "none";
    }
    
    enable();

    this.enable = enable;
    this.disable = disable;
    this.show = show;
    this.hide = hide;
};

innovaphone.LunchButton.prototype = innovaphone.ui1.nodePrototype;

innovaphone.LunchSvgTitle = innovaphone.LunchSvgTitle || function (style, svgStyle, viewbox, code, titleId, texts) {
    var that = this;
    this.createNode("div", style);
    this.addTranslation(texts, titleId, "title");

    var svg = this.add(new innovaphone.ui1.SvgInline(svgStyle, viewbox, code));

    function changeTranslation(titleId) {
        texts.clear(that);
        this.addTranslation(texts, titleId, "title");
    }

    function setFill(fill) {
        svg.container.style.fill = fill;
    }

    this.setCode = svg.setCode;
    this.changeTranslation = changeTranslation;
    this.setFill = setFill;
};

innovaphone.LunchSvgTitle.prototype = innovaphone.ui1.nodePrototype;

innovaphone.Lunch.SymbolCodes = {
    Edit: "<path d=\"M19.67,4.82,16.38,8.19,11.1,3.75,14.4.38A1,1,0,0,1,15.66.15l4.13,3.48A.81.81,0,0,1,19.67,4.82ZM.86,14.25,0,19.3A.54.54,0,0,0,.8,20l5.34-1.26,8.64-8.86L9.51,5.39Z\"/>",
    Export: "<path d=\"M17.07,4.15,13.66.33a1.27,1.27,0,0,0-.95-.42H3.87A1.27,1.27,0,0,0,2.6,1.17V18.83a1.27,1.27,0,0,0,1.27,1.26H16.13a1.27,1.27,0,0,0,1.27-1.26V5A1.28,1.28,0,0,0,17.07,4.15ZM14.92,7h-10V6h10Zm0,4h-10V10h10Zm0,4h-10V14h10Z\"/>",
    ArrowUp: "<path d=\"M18,16,10,9,2,16,0,14,10,5l10,9Z\"/>",
    ArrowDown: "<path d=\"M2,4.5l8,7,8-7,2,2-10,9L0,6.5Z\"/>",
    Meat: "<path d=\"M17.39,2.83l-.12-.11c-.25-.24-.51-.46-.77-.67l-.2-.16a8.84,8.84,0,0,0-.93-.62l-.07,0A7.36,7.36,0,0,0,14.4.8L14.23.73A7,7,0,0,0,13.3.45l-.17,0a4.49,4.49,0,0,0-.8-.1h-.21a3.87,3.87,0,0,0-.77.08l-.14,0a2.78,2.78,0,0,0-.65.23l-.14.06a2.55,2.55,0,0,0-.65.48h0S7.32,3.23,6,8a9.14,9.14,0,0,1-1,2.63,6.49,6.49,0,0,1-.43.69.18.18,0,0,0-.06.09,3.25,3.25,0,0,0-.14,2.15L2,15.88a1.41,1.41,0,0,0-1.45.34,1.42,1.42,0,0,0-.42,1,1.45,1.45,0,0,0,.42,1,1.39,1.39,0,0,0,.68.35,1.39,1.39,0,0,0,.35.68,1.44,1.44,0,0,0,2.45-1,1.61,1.61,0,0,0-.1-.53L6.1,15.54a3.21,3.21,0,0,0,2.61,0h0l0,0a3.11,3.11,0,0,0,.94-.65l.18-.19a4,4,0,0,1,1.28-.49A3.53,3.53,0,0,1,12,14.1h0c1.53,0,4.76-2.11,6.22-3.12l.13-.08.2-.11a3.57,3.57,0,0,0,.38-.3.1.1,0,0,1,.05,0,2.55,2.55,0,0,0,.3-.43C20.53,8.26,19.75,5.19,17.39,2.83ZM18,9.4a1.79,1.79,0,0,1-1.32.46,6.52,6.52,0,0,1-4.21-2.09C10.33,5.65,9.88,3.15,10.8,2.22a1.82,1.82,0,0,1,1.32-.44,6.51,6.51,0,0,1,4.23,2.09,7.08,7.08,0,0,1,2,3.41A2.34,2.34,0,0,1,18,9.4ZM15.52,5c.6.6.74,1.42.33,1.83s-1.24.27-1.84-.32S13.27,5,13.69,4.62,14.92,4.35,15.52,5Z\"/>",
    Veggy: "<path d=\"M18.91,4.22l-1.15.51a3,3,0,0,1,.62.13A1,1,0,0,1,19,6.12a1,1,0,0,1-1.26.64,3.89,3.89,0,0,0-3.47,1.09A7.34,7.34,0,0,1,15,9c.68,1.42-.25,2-1.56,2.73L13.1,12l-1.72-.82.75,1.59L8.09,16l-6.2,3.86A1,1,0,0,1,.6,18.57l3.86-6.2.89-1.1,2.9,1.81L6.34,10,7.85,8.15C8.51,7,9.9,4.72,11.2,5.3A7.78,7.78,0,0,1,12.39,6l0-.09a4,4,0,0,0,.61-4.42A1,1,0,0,1,13.5.13a1,1,0,0,1,1.34.45,6.87,6.87,0,0,1,.63,3l2.64-1.16a1,1,0,0,1,1.32.51A1,1,0,0,1,18.91,4.22Z\"/>",
    Soup: "<path d=\"M10,3c3.86,0,7,3.14,7,7s-3.14,7-7,7-7-3.14-7-7S6.14,3,10,3m0-1h0c-4.42,0-8,3.58-8,8s3.58,8,8,8h0c4.42,0,8-3.58,8-8S14.42,2,10,2h0Zm0,13h0c-2.76,0-5-2.24-5-5h0c0-2.76,2.24-5,5-5h0c2.76,0,5,2.24,5,5h0c0,2.76-2.24,5-5,5Z\"/>",
    Delete: "<path d=\"M8.17,18h-2V8h2Zm6,0h-2V8h2ZM3.17,6V19c0,.89,1,1,1,1h12c1,0,1-.37,1-1V6Zm0-1h14a1,1,0,0,0,1-1,1,1,0,0,0-1-1h-5V1c.05-.22-.59-1-.89-1H9.06c-.28,0-.89.72-.89,1V3h-5a.9.9,0,0,0-1,1A1,1,0,0,0,3.17,5Z\"/>",
    CheckMark: "<path d=\"M6.67,17.5,0,10.81,1.62,9.18l5.05,5.06L18.38,2.5,20,4.13Z\"/>",
    Download: "<path d=\"M14.33,6.21l2.17,2.2L10,15,3.5,8.41l2.17-2.2L8.56,9.14V0h2.88V9.14ZM16.5,18H3.5v2h13Z\"/>",
    Menu1: "<path d=\"m10,17.5c-4.14,0-7.5-3.36-7.5-7.5,0-4.14,3.36-7.5,7.5-7.5,4.14,0,7.5,3.36,7.5,7.5,0,4.14-3.36,7.5-7.5,7.5Zm-.13-9.87v5.87h1.22v-7.26h-1.12l-2.36,1.57.56.91,1.7-1.1h0Z\"/>",
    Menu2: "<path d=\"m10,17.5c-4.14,0-7.5-3.36-7.5-7.5,0-4.14,3.36-7.5,7.5-7.5,4.14,0,7.5,3.36,7.5,7.5,0,4.14-3.36,7.5-7.5,7.5Zm-.4-10.29c.36-.03.73.05,1.04.24.23.2.35.5.32.81,0,.37-.11.74-.33,1.04-.35.46-.73.9-1.14,1.31l-1.79,1.85v1.04h4.68v-1.07h-3.28l1.55-1.51c.47-.41.89-.89,1.24-1.41.24-.42.36-.89.35-1.38,0-1.34-.77-2.02-2.28-2.02-.75,0-1.49.11-2.21.32l.08.97.27-.04c.5-.1,1-.15,1.51-.15Z\"/>",
    Menu3: "<path d=\"m10,17.5c-4.14,0-7.5-3.36-7.5-7.5,0-4.14,3.36-7.5,7.5-7.5,4.14,0,7.5,3.36,7.5,7.5,0,4.14-3.36,7.5-7.5,7.5Zm-2.32-5.17l-.08.95.34.09c.31.07.63.13.95.17.36.05.72.08,1.09.08.66.06,1.32-.14,1.84-.55.42-.47.63-1.09.58-1.71.03-.37-.06-.74-.26-1.05-.23-.26-.52-.46-.84-.6.15-.1.3-.21.44-.34.15-.15.26-.32.35-.51.12-.25.17-.52.17-.79.05-.55-.16-1.1-.55-1.48-.52-.35-1.15-.52-1.78-.47-.76,0-1.52.11-2.24.35l.1.92.28-.05c.59-.1,1.19-.15,1.79-.15.29-.02.59.06.83.23.21.17.32.44.3.71,0,.31-.12.6-.34.82-.2.21-.47.33-.75.33h-1.46v1.02h1.46c.32,0,.64.09.9.29.23.2.36.49.34.8,0,.77-.44,1.16-1.32,1.17-.72-.01-1.43-.08-2.13-.22h0Z\"/>",
    Menu4: "<path d=\"M10,2.5c-4.14,0-7.5,3.36-7.5,7.5s3.36,7.5,7.5,7.5,7.5-3.36,7.5-7.5-3.36-7.5-7.5-7.5ZM12.37,12.17h-.79v1.3h-1.23v-1.3h-3.27v-.93l1.91-5.03h1.35l-2.02,4.89h2.02v-2.13h1.23v2.13h.79v1.07Z \"/>",
    Fish: "<path d=\"m19.95,9.07c-5.49-7.67-13.25-2.64-14.54-1.73-.11.08-.26.06-.36-.04-1.94-1.87-3.44-2.05-4.23-1.98-.28.03-.42.35-.26.58.58.82,1.61,2.68.66,4.57-.61,1.21-.84,2.05-.94,2.53-.05.25.18.47.43.43,1.69-.25,2.5-.96,3.57-1.86.26-.22.71-.24.99-.06,9.43,5.96,13.69.15,14.7-1.59.15-.27.14-.61-.04-.86Zm-3.97-.1c-.41,0-.75-.34-.75-.75s.34-.75.75-.75.75.34.75.75-.34.75-.75.75Zm-2.45,5.14c-.3.02-.62.03-.94.02-2.3-3.86-1.15-7.48-.29-9.25.29,0,.58,0,.87.02-.81,1.48-2.3,5.2.36,9.21Z\"/>",
};

Date.prototype.getWeekNumber = function () {
    var d = new Date(Date.UTC(this.getFullYear(), this.getMonth(), this.getDate()));
    var dayNum = d.getUTCDay() || 7;
    d.setUTCDate(d.getUTCDate() + 4 - dayNum);
    var yearStart = new Date(Date.UTC(d.getUTCFullYear(), 0, 1));
    return Math.ceil((((d - yearStart) / 86400000) + 1) / 7)
};

function Popup(main, str, onOK, text, sip) {
    var that = this,
        popup = null,
        headline = null,
        infoTxt = null,
        btnCont = null,
        cancelBtn = null,
        confirmBtn = null;

    function init() {
        that.createNode("div", null, null, "overlay");
        popup = that.add(new innovaphone.ui1.Div(null, null, "popup"));
        headline = popup.add(new innovaphone.ui1.Node("h1", "margin-left:10px")).addTranslation(str, "reallyRemove");
        infoTxt = popup.add(new innovaphone.ui1.Div(null, null, "info-text"));
        if (text) {
            infoTxt.addTranslation(str, text, null, [sip]);
        }
        else {
            infoTxt.addTranslation(str, "removeInfo");
        }
        btnCont = popup.add(new innovaphone.ui1.Div(null, null, "btn-cont"));
        cancelBtn = btnCont.add(new innovaphone.ui1.Node("button", null, null, "button-row")).addTranslation(str, "cancel");
        cancelBtn.addEvent("click", function () {
            str.clear(that);
            that.container.parentNode.removeChild(that.container);
        });
        confirmBtn = btnCont.add(new innovaphone.ui1.Node("button", null, null, "button-row button-red")).addTranslation(str, "reallyRemove");
        confirmBtn.addEvent("click", function () {
            str.clear(that);
            that.container.parentNode.removeChild(that.container);
            onOK();
        });
    };

    init();
}
Popup.prototype = innovaphone.ui1.nodePrototype;

innovaphone.Lunch.Error = innovaphone.Lunch.Error || function (node, error, texts) {
    var errorContainer = node.add(new innovaphone.ui1.Div("position:fixed; top:00px; left:00px; bottom:0px; right:0px; background-color: rgba(40, 40, 40, 0.5); display: flex; justify-content: center;")),
        errorDiv = errorContainer.add(new innovaphone.ui1.Div(null, null, "confirmation-container")),
        text = new innovaphone.ui1.Div("position:absolute; left:20px; top:20px; padding-right:20px"),
        errorOk = new innovaphone.LunchButton("display:block; margin:100px auto; width:150px", "ok", texts, closeError);

    text.addTranslation(texts, error);

    errorDiv.add(text);
    errorDiv.add(errorOk);

    window.addEventListener("keydown", onKeyDown);

    function onKeyDown(event) {
        switch (event.keyCode) {
            case innovaphone.lib1.keyCodes.enter:
            case innovaphone.lib1.keyCodes.escape:
                closeError();
                break;
        }
        event.stopPropagation();
        event.preventDefault();
        return false;
    }

    function closeError() {
        node.rem(errorContainer);
        window.removeEventListener("keydown", onKeyDown);
    }
};

innovaphone.Lunch.Link = innovaphone.Lunch.Link || function (style, texts, translationKey, translationParams, svgSymbolCode, href, text) {
    this.createNode(href ? "a" : "div", "display:flex; overflow:hidden; align-items:center; cursor:pointer; " + style);

    var that = this,
        arrow = this.add(new innovaphone.ui1.SvgInline("flex:0 0 auto; margin-right:10px; width:20px; height:20px; fill:var(--accent); " + (svgSymbolCode ? "" : "transform:rotate(180deg);"), "0 0 20 20", svgSymbolCode || innovaphone.MyPortal.SymbolCodes.ArrowNavigation)),
        link = this.add(new innovaphone.ui1.Div("flex:1 1 auto", text, "link"));

    if (translationKey) {
        link.addTranslation(texts, translationKey, null, translationParams);
    }
    if (href) {
        this.setAttribute("href", href);
        this.setAttribute("download", link.container.innerText);
    }
}
innovaphone.Lunch.Link.prototype = innovaphone.ui1.nodePrototype;