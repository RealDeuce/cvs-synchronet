// Used for handling the input of /X commands

// $Id: getkeye.js,v 1.1 2005/09/12 20:44:18 deuce Exp $

load("sbbsdefs.js");

function getkeye()
{
	var key;
	var key2;

	while(1) {
		key=console.getkey(K_UPPER);
		if(key=='/') {
			print(key);
			key2=console.getkey(K_UPPER);
			if(key2=="\b" || key2=="\e") {
				print("\b \b");
				continue;
			}
			key=key+key2;
		}
		break;
	}
	return(key);
}
