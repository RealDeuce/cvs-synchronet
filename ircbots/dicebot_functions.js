/* 	IRC Bot Module - Server Commands
	You would place all of your module functions in this file.	*/
	
this.Server_command=function(srv,cmdline,onick,ouh) 
{
	var cmd=IRC_parsecommand(cmdline);
	switch (cmd[0]) {
		case "JOIN":
			if (onick == srv.curnick) break;
			
			// Someone else joining? Let's send them a private welcome message!
			srv.o(onick,"Welcome to the DiceBot show!");
			srv.o(onick,"This is a module for IRCBot - by Cyan");
			break;
		case "PRIVMSG":
			if ((cmd[1][0] == "#") || (cmd[1][0] == "&")) {
				var chan = srv.channel[cmd[1].toUpperCase()];
				if (!chan)
					break;
				if (!chan.is_joined)
					break;
				if(srv.users[onick.toUpperCase()]) {
					/* 	You can do special command processing here, if you like.
						This is currently set up to parse public room messages for
						things like trivia answers, or other responses that
						are inconvenient for users to submit with a command prefix */
				}
			}
			break;
		default:
			break;
	}
}

function roll_them_dice(num_dice,num_sides)
{
	var total=0;
	for(var d=0;d<num_dice;d++) {
		total+=random(num_sides)+1;
	}
	return total;
}