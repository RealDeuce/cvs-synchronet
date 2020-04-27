js.branch_limit = 0;
js.time_limit = 0;

load('sbbsdefs.js');
load('http.js');
load('json-client.js');
load(argv[0] + '../lib/func.js');

var settings,
	jsonClient;

function validateUser(data) {

	if (typeof data.id !== 'string' || data.id.length < 5) return false;

	if (typeof data.alias !== 'string' ||
		data.alias.length < 1 ||
		data.alias.length > LEN_ALIAS
	) {
		return false;
	}

	if (typeof data.system !== 'string' ||
		data.system.length < 1 ||
		data.system.length > 50
	) {
		return false;
	}

	var u = jsonClient.read(settings.JSONDB.dbName, 'users.' + data.id, 1);
	if (typeof u === 'object') return false;

	return true;

}

function validateMessage(data) {
	if ((	typeof data.alias === 'string' &&
			(data.alias.length < 1 || data.alias.length > LEN_ALIAS)
		) ||
		(typeof data.alias !== 'undefined' && typeof data.alias !== 'string')
	) {
		return false;
	}
	if (typeof data.system !== 'string' ||
		data.system.length < 1 ||
		data.system.length > 50
	) {
		return false;
	}
	if (typeof data.message !== 'string' || data.message.length < 1) {
		return false;
	}
	return true;
}

function processInput(update) {

	if (typeof update.data !== 'object' ||
		typeof update.data.key !== 'string' ||
		typeof update.data.data !== 'object'
	) {
		return false;
	}

	var key = update.data.key;
	var oper = update.data.oper;
	var data = update.data.data;

	switch (key) {

		case 'users':
			if (validateUser(data)) {
				var id = data.id;
				delete data.id;
				data.created = time();
				data.laston = time();
				data.winnings = 0;
				data.answers = { correct : 0, incorrect : 0 };
				jsonClient.write(settings.JSONDB.dbName, 'users.'+id, data, 2);
			} 
			break;

		case 'messages':
			if (validateMessage(data)) {
				data.created = time();
				if (typeof data.store === 'boolean' && data.store) {
					jsonClient.unshift(
						settings.JSONDB.dbName,
						'messages.history',
						data,
						2
					);
				}
				jsonClient.write(
					settings.JSONDB.dbName,
					'messages.latest',
					data,
					2
				);
			}
			break;

		case 'game.users':
			if (typeof data.id === 'string') {
				var ugs = jsonClient.read(
					settings.JSONDB.dbName,
					'game.users.' + data.id,
					1
				);
				if (typeof ugs === 'undefined') {
					var obj = {
						winnings : 0,
						round : 1,
						rounds : {
							'1' : { '0':[], '1':[], '2':[], '3':[], '4':[] },
							'2' : { '0':[], '1':[], '2':[], '3':[], '4':[] },
							'3' : { '0':[] }
						},
						answers : { correct : 0, incorrect : 0 }
					};
					jsonClient.write(
						settings.JSONDB.dbName,
						'game.users.' + data.id,
						obj,
						2
					);
				}
			}
			break;

		case 'game.state':
			if (typeof data.id === 'string' &&
				typeof data.round === 'number' &&
				typeof data.category === 'number' &&
				typeof data.clue === 'number' &&
				data.round >= 1 && data.round <= 3 &&
				data.category >= 0 && data.category < 5 &&
				data.clue >= 0 && data.clue < 5
			) {
				jsonClient.push(
					settings.JSONDB.dbName,
					'game.users.' + data.id +
						'.rounds.' + data.round + '.' + data.category,
					data.clue,
					2
				);
			}
			break;

		case 'game.nextRound':
			if (typeof data.id === 'string') {
				var ugs = jsonClient.read(
					settings.JSONDB.dbName,
					'game.users.' + data.id,
					1
				);
				if (ugs && ugs.round <= 3 && ugs.winnings >= 0) {
					ugs.round++;
					jsonClient.write(
						settings.JSONDB.dbName,
						'game.users.' + data.id + '.round',
						ugs.round,
						2
					);
				}
			}
			break;

		case 'game.report':
			if (typeof data.userID === 'string' &&
				typeof data.clueID === 'number' &&
				typeof data.userAnswer === 'string' &&
				typeof data.realAnswer === 'string'
			) {
				var us = base64_decode(data.userID).split('@');
				if (us.length < 2) us.push('Unknown');
				var body = format(
					'User: %s\r\n' +
					'System: %s\r\n' +
					'Clue ID: %s\r\n' +
					'Given answer: %s\r\n' +
					'Correct answer: %s\r\n' +
					'\r\nThis message was generated by Jeopardized\r\n',
					us[0], us[1], data.clueID, data.userAnswer, data.realAnswer
				);
				notifySysop('Jeopardized clue ' + data.clueID, body);
			}
			break;

		case 'game.answer':
			if (typeof data.id === 'string' &&
				typeof data.round === 'number' &&
				typeof data.category === 'number' &&
				typeof data.clue === 'number' &&
				typeof data.answer === 'string'
			) {
				var category = jsonClient.read(
					settings.JSONDB.dbName,
					'game.round.' + data.round + '.' + data.category,
					1
				);
				var clue = category.clues[data.clue];
				var ugs = jsonClient.read(
					settings.JSONDB.dbName,
					'game.users.' + data.id,
					1
				);
				var usr = jsonClient.read(
					settings.JSONDB.dbName,
					'users.' + data.id,
					1
				);
				if (typeof category === 'undefined' ||
					typeof ugs === 'undefined' ||
					typeof usr === 'undefined'
				) {
					return;
				}
				var value = (
					clue.dd && data.value !== null
					? data.value
					: clue.value
				);
				try {
					var success = compareAnswer(clue.id, data.answer);
					if (success) {
						ugs.winnings = ugs.winnings + value;
						usr.winnings = usr.winnings + value;
						ugs.answers.correct++;
						usr.answers.correct++;
					} else {
						ugs.winnings = ugs.winnings - value;
						usr.winnings = usr.winnings - value;
						ugs.answers.incorrect++;
						usr.answers.incorrect++;
					}
					jsonClient.write(
						settings.JSONDB.dbName,
						'game.users.' + data.id,
						ugs,
						2
					);
					jsonClient.write(
						settings.JSONDB.dbName,
						'users.' + data.id,
						usr,
						2
					);
					if (clue.dd && data.value !== null) {
						var msg = '%s%s@%s bet $%s and %s!';
						if (success) {
							msg = format(
								msg,
								'\1h\1g',
								usr.alias,
								usr.system,
								data.value,
								'won'
							);
						} else {
							msg = format(
								msg,
								'\1h\1r',
								usr.alias,
								usr.system,
								data.value,
								'lost'
							);
						}
						jsonClient.unshift(
							settings.JSONDB.dbName,
							'messages.history',
							{ message : msg },
							2
						);
						jsonClient.write(
							settings.JSONDB.dbName,
							'messages.latest',
							{ message : msg },
							2
						);
					}
				} catch (err) {
					notifySysop('Web API error', err);
				}
			}
			break;

		default:
			break;

	}

}

function processUpdate(update) {

	if (typeof update.location === 'undefined' ||
		typeof update.oper !== 'string' ||
		update.oper !== 'WRITE'
	) {
		return;
	}

	if (update.location === 'input') {
		processInput(update);
	}

}

function loadSettings() {
	settings = {};
	var f = new File(argv[0] + '../settings.ini');
	f.open('r');
	settings.JSONDB = f.iniGetObject('JSONDB');
	settings.WebAPI = f.iniGetObject('WebAPI');
	f.close();
}

function initJSON() {
	jsonClient = new JSONClient(settings.JSONDB.host, settings.JSONDB.port);
	jsonClient.callback = processUpdate;
	jsonClient.subscribe(settings.JSONDB.dbName, 'input');
}

function main() {
	while (!js.terminated) {
		jsonClient.cycle();
		mswait(50);
	}
}

function cleanUp() {
	jsonClient.disconnect();
}

try {
	loadSettings();
	initJSON();
	main();
} catch (err) {
	notifySysop('Service thread encountered an error', err);
} finally {
	cleanUp();
}