load("json-client.js");
load("event-timer.js");

var root = argv[0];

var jsonClient, users, systems, state, timer, month, dbName, historyPath, httpPath;
var dummy = [ { 'x' : 1, 'y' : 1, 'c' : "CLEAR", 'a' : 0, 'u' : 0, 's' : 0 } ];

var newMonth = function() {

	var thisMonth = strftime("%m%Y");
	if(thisMonth == month)
		return;

	log(LOG_INFO, "SyncWall service: performing monthly maintenance.");

	var oldMonth = {
		'MONTH' : month,
		'USERS' : users,
		'SYSTEMS' : systems,
		'SEQUENCE' : [],
		'STATE' : state
	};

	var len = jsonClient.read(dbName, "SEQUENCE.length", 1);
	for(var i = 0; i < len; i = i + ((len - 1 > 100) ? 100 : len - i)) {
		var history = jsonClient.splice(
			dbName,
			"SEQUENCE",
			i,
			((len - i > 100) ? 100 : len)
		);
		oldMonth.SEQUENCE = oldMonth.SEQUENCE.concat(history);
	}

	var state = {};
	jsonClient.write(dbName, "STATE", state, 2);
	jsonClient.write(dbName, "SEQUENCE", dummy,	2);
	jsonClient.write(dbName, "LATEST", dummy, 2);
	jsonClient.write(dbName, "MONTH", thisMonth, 2);

	if(!file_exists(historyPath) || !file_isdir(historyPath))
		mkdir(historyPath);

	var f = new File(historyPath + "/" + month + ".json");
	f.open("w");
	f.write(JSON.stringify(oldMonth));
	f.close();

	jsonClient.push(dbName, "MONTHS", httpPath + "/" + month + ".json", 2);

	month = thisMonth;

}

var processUpdate = function(update) {

	if(typeof update.location == "undefined" || update.oper != "WRITE")
		return;

	data = update.data;

	if(update.location == "USERS" && users.indexOf(data) < 0) {
		users.push(data);
		return;
	}

	if(update.location == "SYSTEMS" && systems.indexOf(data) < 0) {
		systems.push(data);
		return;
	}

	if(	update.location == "LATEST"
		&&
		data.u < users.length && data.u >= 0
		&&
		data.s < systems.length && data.s >= 0
	) {
		if(data.c == "CLEAR") {
			state = {};
			jsonClient.write(dbName, "STATE", state, 2);
		} else {
			if(typeof state[data.y] == "undefined")
				state[data.y] = {};
			state[data.y][data.x] = { 'c' : data.c, 'a' : data.a };
			jsonClient.write(
				dbName,
				"STATE." + data.y + "." + data.x,
				state[data.y][data.x],
				2
			);
		}
		jsonClient.push(dbName, "SEQUENCE", data, 2);
		return;
	}

}

var init = function() {

	timer = new Timer();

	var f = new File(root + "service.ini");
	f.open("r");
	var serviceIni = f.iniGetObject();
	f.close();
	dbName = serviceIni.dbName;
	historyPath = serviceIni.historyPath;
	httpPath = serviceIni.httpPath;

	var usr = new User(1);
	jsonClient = new JSONClient("localhost", 10088);
	jsonClient.ident("ADMIN", usr.alias, usr.security.password);
	jsonClient.callback = processUpdate;

	month = jsonClient.read(dbName, "MONTH", 1);
	if(!month) {
		month = strftime("%m%Y");
		jsonClient.write(dbName, "MONTH", month, 2);
	}

	var historyFiles = directory(historyPath + "/*.json");
	for(var h = 0; h < historyFiles.length; h++)
		historyFiles[h] = httpPath + "/" + file_getname(historyFiles[h]);
	var months = jsonClient.read(dbName, "MONTHS", 1);
	if(!months) {
		var months = historyFiles;
		jsonClient.write(dbName, "MONTHS", months, 2);
	}
	for(var h = 0; h < historyFiles.length; h++) {
		if(months.indexOf(historyFiles[h]) < 0)
			jsonClient.push(dbName, "MONTHS", historyFiles[h], 2);
	}

	users = jsonClient.read(dbName, "USERS", 1);
	if(!users) {
		users = [ usr.alias ];
		jsonClient.write(dbName, "USERS", users, 2);
	}
	jsonClient.subscribe(dbName, "USERS");

	systems = jsonClient.read(dbName, "SYSTEMS", 1);
	if(!systems) {
		systems = [ system.name ];
		jsonClient.write(dbName, "SYSTEMS", systems, 2);
	}
	jsonClient.subscribe(dbName, "SYSTEMS");

	if(!jsonClient.read(dbName, "SEQUENCE", 1)) {
		jsonClient.write(
			dbName,
			"SEQUENCE",
			dummy,
			2
		);
	}

	state = jsonClient.read(dbName, "STATE", 1);
	if(!state)
		state = {};

	jsonClient.subscribe(dbName, "LATEST");

	// Monthly maintenance will happen on startup, or between midnight and
	// one o'clock AM, depending on start time.
	var event = timer.addEvent(3600000, true, newMonth);
	event.run();

}

var main = function() {
	while(!js.terminated) {
		timer.cycle();
		jsonClient.cycle();
		mswait(5);
	}
}

var cleanUp = function() {
	jsonClient.disconnect();
}

try {
	init();
	main();
	cleanUp();
	exit();
} catch(err) {
	log("SyncWall service error: " + err);
}
