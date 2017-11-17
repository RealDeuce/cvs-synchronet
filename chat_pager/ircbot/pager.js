function get_config(module) {
	var cfg = null;
	var fn = system.ctrl_dir + 'ircbot.ini';
	if (file_exists(fn)) {
		var f = new File(fn);
		if (f.open('r')) {
			var cfg = f.iniGetObject(module);
			f.close();
			if (typeof cfg.channels !== 'undefined') {
				cfg.channels = cfg.channels.split(',').map(
					function (e) { return e.replace(/\s.*/g, ''); }
				);
			} else {
				cfg.channels = [];
			}
		}
	}
	return cfg;
}

var bot_cfg = get_config('module_pager');
var settings = load_settings(fullpath(this.dir + '../settings.ini'));

if (bot_cfg !== null && settings !== null) {

	if (!settings.queue.disabled) {
		var queue = new Queue(settings.queue.queue_name);
	}
	var timer = new Timer();
	var scanner = new Scanner(settings.scanner);
	var messages = [];

	function scan_nodes() {
		var nodes_paging = scanner.scan();
		nodes_paging.forEach(
			function (e, i) {
				var un = user_online(i);
				if (e == PAGE_NEW && un > 0) {
					var u = new User(un);
					messages.push(
						format(
							settings.ircbot.notification_format, u.alias, i + 1
						)
					);
				}
			}
		);
	}

	timer.addEvent(settings.scanner.semaphore_scan_interval, true, scan_nodes);

	function main(srv, target) {
		timer.cycle();
		while (messages.length > 0) {
			bot_cfg.channels.forEach(
				function (e) {
					srv.o(e, messages.shift());
				}
			);
		}
	}

	Bot_Commands["CHAT"] = new Bot_Command(90, false, false);
	Bot_Commands["CHAT"].usage = get_cmd_prefix() + "!chat [node]";
	Bot_Commands["CHAT"].command = function (target, onick, ouh, srv, lbl, cmd) {
		if (cmd.length > 0) {
			var nn = parseInt(cmd[1]);
			if (isNaN(nn) || nn < 1 || nn > system.node_list.length) {
				return;
			} else if (!settings.queue.disabled) {
				var valname = "chat_" + cmd[1];
				queue.write(system.timer, valname);
			} else {
				var valname = system.ctrl_dir + "syspage_response." + cmd[1];
				file_touch(valname);
			}
		}
	}

} else {

	log(LOG_ERR, 'Unable to load pager settings.');

}
