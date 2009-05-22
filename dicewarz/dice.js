/*
	**********************************
	DICE WARZ (2008)
	for use with Synchronet v3.14+
	by Matt Johnson                 
	**********************************

	SET TAB STOPS TO 4 FOR EDITING

	SEARCH "TODO:" ENTRIES FOR
	CODE THAT NEEDS WORK
*/

	load("sbbsdefs.js");
	load("graphic.js");
	load("logging.js");
	var game_dir;
	try { barfitty.barf(barf); } catch(e) { game_dir = e.fileName; }
	game_dir = game_dir.replace(/[^\/\\]*$/,'');
	
	load(game_dir + "diceroll.js");
	load(game_dir + "lock.js");
	load(game_dir + "maps.js");
	load(game_dir + "territory.js");
	load(game_dir + "menu.js");
	load(game_dir + "player.js");
	load(game_dir + "display.js");
	load(game_dir + "ai.js");
	var activeGame="";
	
//######################### InitIALIZE PROGRAM VARIABLES #########################

	var 	pointsToWin=	100;
	var		minScore=		-2;
	var 	maxGames=		100;
	var 	minPlayers=		3;
	var 	maxPlayers=		7;
	var 	maxDice=		8;
	var 	gamelog=		false;
	const 	root=			"game";
	const	scorefile=		"rankings";
	const	halloffame=		"dicehof";
	const	instructions=	"dice.doc";
	const 	bColors=		[BG_BLUE,	BG_CYAN,	BG_RED,		BG_GREEN,	BG_BROWN,	BG_MAGENTA,	BG_LIGHTGRAY]; 		//MAP BACKGROUND COLORS
	const 	bfColors=		[BLUE,		CYAN,		RED,		GREEN,		BROWN,		MAGENTA,	LIGHTGRAY]; 		//MAP BACKGROUND COLORS (FOREGROUND CHARS)
	const	fColors=		["\1h\1w",	"\1h\1c",	"\1h\1r",	"\1h\1g",	"\1h\1y", 	"\1h\1m",	"\1k"];				//MAP FOREGROUND COLORS
	const	border_color=	"\1n\1k\1h";


//######################### DO NOT CHANGE THIS SECTION ##########################	

	var userFileName=game_dir + user.alias + ".usr";
	var userFile=new File(userFileName);
	userFile.open('a');
	userFile.close();

	const	daySeconds=		86400;
	const 	startRow=		4;
	const  startColumn=	3;
	const	menuRow=		1;
	const	menuColumn=		50;
	const 	columns=		9;
	const 	rows=			9;
	const 	blackBG=		console.ansi(ANSI_NORMAL);
	
	//TODO: FIX SCORING SYSTEM... BECAUSE RIGHT NOW IT SUCKS
	const	points=			[-2,-2,-2,-1,0,1,2];			//POINTS GAINED/LOST FOR WINNING/LOSING
	var		scores=			[];
	
	var		messages=		[];								//MESSAGES QUEUED FOR DISPLAY UPON RELOADING MAIN MENU
	var		games=			new GameStatusInfo();
	var		dice=			LoadDice();
	var 	oldpass=		console.ctrlkey_passthru;
	
	console.ctrlkey_passthru="+ACGKLOPQRTUVWXYZ_";
	bbs.sys_status|=SS_MOFF;

//########################## MAIN FUNCTIONS ###################################
function 	ScanProximity(location) 	
{										
	var prox=[];
	var odd=false;
	offset=location%columns;
	if((offset%2)==1) odd=true;

	if(location>=columns) 					//if not in the first row
		prox[0]=(location-columns);				//north
	if(location<(columns*(rows-1)))			//if not in the last row
		prox[1]=(location+columns);				//south
	
	if(odd)
	{
		if(((location+1)%columns)!=1)			//if not in the first column
			prox[2]=location-1;						//northwest
		if(((location+1)%columns)!=0)			//if not in the last column
			prox[3]=location+1;						//northeast
		if(location<(columns*(rows-1)))									//if not in the last row
		{
			if(((location+1)%columns)!=1)			//if not in the first column
				prox[4]=prox[1]-1;						//southwest
			if(((location+1)%columns)!=0)			//if not in the last column
				prox[5]=prox[1]+1;						//southeast		
		}
	}
	else
	{
		if(location>=columns)										//if not in the first row
		{
			if(((location+1)%columns)!=1)			//if not in the first column
				prox[2]=prox[0]-1;						//northwest
			if(((location+1)%columns)!=0)			//if not in the last column
				prox[3]=prox[0]+1;						//northeast		
		}
		if(((location+1)%columns)!=1)			//if not in the first column
			prox[4]=location-1;						//southwest
		if(((location+1)%columns)!=0)			//if not in the last column
			prox[5]=location+1;						//southeast
	}
	return prox;
}
function 	GetNextGameNumber()
{
	var next=1;
	while(games.gameData[next]) {
		next++;
	}
	return next;
}
function	CountSparseArray(data)
{
	var count=0;
	for(tt in data)
	{
		count++;
	}
	if(count==0) return false;
	else return count;
}
function	ViewInstructions()
{
	console.clear();
	console.printfile(game_dir + instructions);
	if(!(user.settings & USER_PAUSE)) console.pause();
	console.aborted=false;
}
function	ViewRankings()
{
	games.HallOfFame();
	games.LoadRankings();
	var scoredata=SortScores();
	console.clear();
	
	printf("\1k\1h\xDA");
	printf(PrintPadded("\xC2",23,"\xC4","right"));
	printf(PrintPadded("\xC2",8,"\xC4","right"));
	printf(PrintPadded("\xC2",8,"\xC4","right"));
	printf(PrintPadded("\xC2",12,"\xC4","right"));
	printf(PrintPadded("\xC2",14,"\xC4","right"));
	printf(PrintPadded("\xBF",13,"\xC4","right"));
	console.crlf();
	printf("\1k\1h\xB3\1n\1g DICE WARZ RANKINGS   \1k\1h\xB3 \1n\1gSCORE \1k\1h\xB3 \1n\1gKILLS \1k\1h\xB3 \1n\1gSOLO WINS \1k\1h\xB3 \1n\1gSOLO LOSSES \1k\1h\xB3 \1n\1gSOLO WIN % \1k\1h\xB3");
	console.crlf();
	printf("\xC3");
	printf(PrintPadded("\xC5",23,"\xC4","right"));
	printf(PrintPadded("\xC5",8,"\xC4","right"));
	printf(PrintPadded("\xC5",8,"\xC4","right"));
	printf(PrintPadded("\xC5",12,"\xC4","right"));
	printf(PrintPadded("\xC5",14,"\xC4","right"));
	printf(PrintPadded("\xB4",13,"\xC4","right"));
	console.crlf();
 	
	for(hs in scoredata)
	{ 
		thisuser=scoredata[hs];
		if(scores[thisuser].score!=0 || scores[thisuser].wins>0 || scores[thisuser].losses>0 || scores[thisuser].kills>0) 
		{
			var winPercentage=0;
			if(scores[thisuser].wins>0) winPercentage=(scores[thisuser].wins/(scores[thisuser].wins+scores[thisuser].losses))*100;
			printf("\1k\1h\xB3");
			printf(PrintPadded("\1y\1h" + system.username(thisuser),22," ","left"));
			printf("\1k\1h\xB3");
			printf(PrintPadded("\1w\1h" + scores[thisuser].score + "\1k\1h\xB3",8," ","right"));
			printf(PrintPadded("\1w\1h" + scores[thisuser].kills + "\1k\1h\xB3",8," ","right"));
			printf(PrintPadded("\1w\1h" + scores[thisuser].wins + "\1k\1h\xB3",12," ","right"));
			printf(PrintPadded("\1w\1h" + scores[thisuser].losses + "\1k\1h\xB3",14," ","right"));
			printf(PrintPadded("\1w\1h" + winPercentage.toFixed(2) + " % \1k\1h\xB3",13," ","right"));
			console.crlf();
		}
	}
	
	printf("\1k\1h\xC0");
	printf(PrintPadded("\xC1",23,"\xC4","right"));
	printf(PrintPadded("\xC1",8,"\xC4","right"));
	printf(PrintPadded("\xC1",8,"\xC4","right"));
	printf(PrintPadded("\xC1",12,"\xC4","right"));
	printf(PrintPadded("\xC1",14,"\xC4","right"));
	printf(PrintPadded("\xD9",13,"\xC4","right"));
	console.crlf();
	
	console.putmsg("\r\n\1y\1h Scores will be reset when a player reaches \1c" + pointsToWin + " \1ypoints.");
	
	console.gotoxy(1,24);
	console.pause();
	console.aborted=false;
}
function	Sort(iiii)
{
	var Sorted=[];
	for(cc in iiii)
	{
		Sorted.push(cc);
	}
	Sorted.sort();
	return Sorted;
}
function	AttackMessage(txt)
{
	var x=menuColumn;
	ClearArea(16,menuColumn,8);
	console.gotoxy(x,16);
	console.putmsg("\1r\1h Choose a territory");
	console.gotoxy(x,17);
	console.putmsg("\1r\1h " + txt);
	console.gotoxy(x,18);
	console.putmsg("\1r\1h or 'Q' to cancel.");
	console.gotoxy(x,20);
	console.putmsg("\1r\1h Use the arrow keys");
	console.gotoxy(x,21);
	console.putmsg("\1r\1h to move, and \1n\1r[\1hENTER\1n\1r]");
	console.gotoxy(x,22);
	console.putmsg("\1r\1h to select a tile.");

	WipeCursor("left");
}

/*
	//TODO: IMPROVE SCORING METHODS
	
	SCORES ARE STORED IN A SPARSE ARRAY. COMPRESS THE INDICES OF THE SCORES INTO A SEQUENTIAL LIST
	THEN SORT THE LIST HIGHEST SCORE TO LOWEST SCORE AND RETURN A NEW ARRAY OF POINTERS TO SCORE INDICES
	I'M SURE THERE'S A BETTER WAY, IT JUST HASN'T OCCURRED TO ME YET
*/
function	CompressScores()
{
	compressed=[];
	for(score in scores)
	{
		compressed.push(score);
	}
	return compressed;
}
function 	SortScores()
{ 
	var data=CompressScores();
	var numScores=data.length;
	for(n=0;n<numScores;n++)
	{
		for(m = 0; m < (numScores-1); m++) 
		{
			if(scores[data[m]].score < scores[data[m+1]].score) 
			{
				holder = data[m+1];
				data[m+1] = data[m];
				data[m] = holder;
			}
		}
	}
	return data;
}
function 	DeliverMessage(nextTurnPlayer,gameNumber)
{
	var nextUserName=system.username(nextTurnPlayer);
	var message="\1r\1hIt is your turn in \1yDice-Warz\1r game #" + gameNumber + "\r\n\r\n";
	var nextUserFileName=game_dir + nextUserName + ".usr";
	if(file_exists(nextUserFileName)) {
		var nextUserFile=new File(nextUserFileName);
		nextUserFile.open('a',true);
		nextUserFile.writeln(message);
		nextUserFile.close();
	}
	else system.put_telegram(nextTurnPlayer, message);
}
function 	DeliverKillMessage(killer,eliminated,gameNumber)
{
	var killed=system.username(eliminated);
	var message="\1r\1h" + killer + " has eliminated you in \1yDice\1r-\1yWarz\1r game #\1y" + gameNumber + "\1r!\r\n\r\n";
	var deaduserfile=game_dir + killed + ".usr";
	if(file_exists(deaduserfile)) {
		var deaduserfile=new File(deaduserfile);
		deaduserfile.open('a',true);
		deaduserfile.writeln(message);
		deaduserfile.close();
	}
	else system.put_telegram(eliminated, message);
}


//########################GAME MENU FUNCTIONS################################
function	SplashScreen()
{
	console.clear();
	var splash_filename=game_dir + "dicewarz.bin";
	var splash_size=file_size(splash_filename);
	splash_size/=2;		// Divide by two for attr/char pairs
	splash_size/=80;	// Divide by 80 cols.  Size should now be height (assuming int)
	var Splash=new Graphic(80,splash_size);
	Splash.load(splash_filename);
	Splash.draw();
	
	console.gotoxy(1,23);
	console.center("\1n\1c[\1hPress any key to continue\1n\1c]");
	while(console.inkey(K_NOECHO|K_NOSPIN)=="");
}
function 	GameMenu()
{
	var gMenu=new Menu(		""								,1,19,GetColor("green"),GetColor("green","high"));
	var gmenu_items=[		"~Rankings"						, 
							"~Select Game"					,
							"~Begin New Game"				,
							"~Instructions"					,
							"~Quit to BBS"					];
	gMenu.add(gmenu_items);
	games.Init();

	while(1)
	{
		//LOOPING MAIN MENU, UPDATES GAME STATUS INFO UPON REFRESH
		//TODO: make recently completed games disappear from "your turn" list upon refresh
		
		games.UpdateGames();
		console.clear();
		console.putmsg("\1r\1hB B S - D I C E - W A R Z ! \1n\1c- Matt Johnson - 2008");
		console.crlf();
		DrawLine("\1n\1w\1h",79);
		console.crlf();
		console.gotoxy(1,18);
		DrawLine("\1n\1w\1h",79);
		
		console.gotoxy(1,24);
		DrawLine("\1n\1w\1h",79);
		gMenu.display();

		console.gotoxy(1,3);
		
		Wrap("\1g Games in progress          ",games.inProgress);
		Wrap("\1g Games needing more players ",games.notFull);
		Wrap("\1g Completed Games            ",games.completed);
		Wrap("\1g You are involved in games  ",games.yourGames);
		Wrap("\1g It is your turn in games   ",games.yourTurn);
		Wrap("\1g You are eliminated in games",games.eliminated);
		Wrap("\1g You have won games         ",games.youWin);
		Wrap("\1g Single-player games        ",games.singleGames);
		
		DisplayMessages();
		
		WipeCursor("right");
		
        var cmd=console.getkey(K_NOSPIN|K_NOECHO|K_NOCRLF|K_UPPER);
        switch(cmd)
        {
        case "R":
            ViewRankings();
            break;
        case "S":
            var choice=ChooseGame();
            if(choice) ProcessSelection(choice);
            break;
        case "I":
            ViewInstructions();
            break;
        case "B":
            CreateNewGame();
            break;
        case "Q":
            Quit(0);
            break;
        default:
            break;
        }
        WipeCursor("right");
	}
}
function	ChooseGame()
{
	x=30; y=19
	while(1)
	{
		console.gotoxy(x,y);
		console.cleartoeol();
		if(!games.gameData.length)
		{
			PutMessage("\1r\1hThere are currently no games.",x,y);
			break;
		}
		console.putmsg("\1n\1gEnter game number or [\1hQ\1n\1g]uit\1h: ");
		game_num=console.getkeys("Q",maxGames);
		if(game_num=="Q") return false;
		if(games.gameData[game_num]) 
		{
			var game_data=games.gameData[game_num];
			var num=game_num;
			if(game_num<10) num="0" + num;
			gamefile=root + num;

			if(Locked(gamefile))
				PutMessage("\1r\1hThat game is in use by another node.",x,y);
			else
			{
				if(game_data.singlePlayer) 
				{
					if(games.gameData[game_num].users[user.number]>0);
					else 
					{
						PutMessage("\1r\1hGame #" + game_num + " is private.",x,y);
						continue;
					}
				}
				return game_num;
			}
		}
		else PutMessage("\1r\1hNo Such Game!",x,y);
	}
	return false;
}
function 	ProcessSelection(gameNumber)
{	
	var num=gameNumber;
	if(gameNumber<10) num="0" + num;
	var gamefile=root + num;

	Lock(gamefile);
	var g=games.gameData[gameNumber];
	var fileName=g.fileName;
	var lastModified=file_date(fileName);
	if(lastModified>g.lastModified) games.gameData[gameNumber]=LoadGame(fileName,gameNumber,lastModified);
	
	if(g.status>=0)
	{
		GameLog("Loading game: " + gameNumber);
		PlayGame(gameNumber);
	}
	else 
	{
		GameLog("Viewing game info: " + gameNumber);
		ViewGameInfo(gameNumber);
		if(g.users[user.number]>=0)
		{
			var gamePlayer=g.users[user.number];
			if(g.fixedPlayers) AskRemove(gameNumber,gamePlayer);
			else ChangeVote(gameNumber,user.number);
		}
		else JoinGame(gameNumber);
	}
	Unlock(gamefile);
	ClearArea(19,30,4);
}
function	ChangeVote(gameNumber,userNumber)
{
	console.gotoxy(30,19);
	console.cleartoeol();
	var g=games.gameData[gameNumber];
	var gamePlayer=g.users[userNumber];
	
	if(console.noyes("\1n\1gChange your vote?"))
	{
		console.gotoxy(30,20);
		console.cleartoeol();
		AskRemove(gameNumber,gamePlayer);
	}
	else 
	{
		var vote=g.players[gamePlayer].vote;
		if(vote==1) g.players[gamePlayer].vote=0;
		else g.players[gamePlayer].vote=1;
		if(g.tallyVotes())
		{
			g.maxPlayers=g.countPlayers();
			StartGame(gameNumber);
		}
		games.StoreGame(gameNumber);
	}
}
function	AskRemove(gameNumber,playerNumber)
{
	g=games.gameData[gameNumber];
	if(console.noyes("\1n\1gRemove yourself from this game?"));
	else
	{
		g.removePlayer(playerNumber);
		if(g.countHumanPlayers()==0) 
		{
			file_remove(g.fileName);
		}
		else games.StoreGame(gameNumber);
	}
}
function	ViewGameInfo(gameNumber)
{
	g=games.gameData[gameNumber];
	ClearArea(3,1,14)
	console.gotoxy(2,4);
	console.putmsg("\1g[ \1hGAME: #" + gameNumber + " \1n\1g]\r\n");

	if(g.maxPlayers>g.minPlayers)
		console.putmsg("\1n\1g Minimum Players: \1h" + g.minPlayers + " \1n\1gMaximum: \1h" + g.maxPlayers + "\r\n");
	else 
		console.putmsg("\1n\1g Players Needed to Start: \1h" + g.maxPlayers + "\r\n");

	console.putmsg("\1n\1g Players In This Game:\r\n");
	for(playerNumber=0;playerNumber<g.players.length;playerNumber++)
	{
		player=g.players[playerNumber];

		console.putmsg("\1g\1h  " + GetUserName(player,playerNumber));
		if(player.vote>=0) {
			if(g.maxPlayers>g.minPlayers)
				console.putmsg(" \1n\1gvotes to \1h" + g.getVote(playerNumber));
		}
		console.crlf();
	}
	console.crlf();
}
function	StartGame(gameNumber)
{
	var maxPlayers=games.gameData[gameNumber].maxPlayers;
	var players=games.gameData[gameNumber].players;
	var oldFn=games.gameData[gameNumber].fileName
	var oldSp=games.gameData[gameNumber].singlePlayer;
	games.gameData[gameNumber]=new Map(columns,rows,maxPlayers,gameNumber);
	games.gameData[gameNumber].fileName = oldFn;
	games.gameData[gameNumber].singlePlayer = oldSp;
	g=games.gameData[gameNumber];
	games.gameData[gameNumber].players=players;
	games.inProgress.push(gameNumber);
	games.gameData[gameNumber].Init();
	games.gameData[gameNumber].lastModified=time();

	QueueMessage("\1r\1hGame " + gameNumber + " Initialized!",30,20);
	games.gameData[gameNumber].Notify();
	/* Set up computer players */
	var aifile=new File(game_dir + "ai.ini");
	aifile.open("r");
	var possibleplayers=aifile.iniGetSections();
	for(i=0; i<g.players.length; i++) {
		if(g.players[i].user==-1) {
			if(possibleplayers.length > 0) {
				var p=random(possibleplayers.length);
				g.players[i].AI.name=possibleplayers[p];
				possibleplayers.splice(p,1);
				g.players[i].AI.sort=aifile.iniGetValue(g.players[i].AI.name, "Sort", "Random");
				g.players[i].AI.check=aifile.iniGetValue(g.players[i].AI.name, "Check", "Random");
				g.players[i].AI.qty=aifile.iniGetValue(g.players[i].AI.name, "Quantity", "Random");
				if(AISortFunctions[g.players[i].AI.sort]==undefined)
					g.players[i].AI.sort="Random";
				if(AICheckFunctions[g.players[i].AI.check]==undefined)
					g.players[i].AI.check="Random";
				if(AIQtyFunctions[g.players[i].AI.qty]==undefined)
					g.players[i].AI.qty="Random";
			}
		}
	}
	aifile.close();
	games.StoreGame(gameNumber);
}
function	JoinGame(gameNumber)
{
	var vote=-1;
	g=games.gameData[gameNumber];
	if(console.noyes("\1n\1gJoin this game?"));
	else
	{
		if(g.maxPlayers>g.minPlayers) vote=GetVote();
		g.addPlayer(user.number,vote);
		if(g.players.length>=g.minPlayers)
		{
			if(g.players.length==g.maxPlayers ||
				g.tallyVotes())
			{
				g.maxPlayers=g.players.length;
				StartGame(gameNumber);
			}
		}
		games.StoreGame(gameNumber);
	}
}
function 	CreateNewGame()
{	
	var minNumPlayers=-1;
	var maxNumPlayers=7;
	var numComputerPlayers=-1;
	var singlePlayer=false;
	var fixedPlayers=false;
	var vote=-1;
	
	var x=30; var y=19
	console.gotoxy(x,y);
	console.cleartoeol();
	if(console.yesno("\1n\1gBegin a new game?"))
	{
		ClearArea(3,1,14);
		ClearArea(19,30,5);
		var gameNumber=GetNextGameNumber();
		var gnum=gameNumber;
		GameLog("creating new game: " + gnum);
		if(gameNumber<10) gnum="0" + gnum;
		var gamefile=root + gnum;
		Lock(gamefile);
	}
	else 
	{
		console.gotoxy(x,y);
		console.cleartoeol();
		return false;
	}
	while(1)
	{
		x=2; y=4;
		console.gotoxy(x,y);
		console.cleartoeol();
		console.putmsg("\1n\1gEnter minimum number of players [\1h" + minPlayers + "\1n\1g-\1h" + maxPlayers + "\1n\1g] or [\1hQ\1n\1g]uit: ");
		var num=console.getkeys("Q",maxPlayers);
		if(num>=minPlayers && num<=maxPlayers) 
		{
			minNumPlayers=num;
			break;
		}
		else if(num=="Q") return false; 
		else PutMessage("\1r\1h Please enter a number within the given range.",x,y);
	}
	if(minNumPlayers<maxPlayers) {
		while(1)
		{
			x=2; y=5;
			console.gotoxy(x,y);
			console.cleartoeol();
			console.putmsg("\1n\1gEnter maximum number of players [\1h" + minNumPlayers + "\1n\1g-\1h" + maxPlayers + "\1n\1g] or [\1hQ\1n\1g]uit: ");
			var num=console.getkeys("Q",maxPlayers);
			if(num>=minNumPlayers && num<=maxPlayers) 
			{
				maxNumPlayers=num;
				break;
			}
			else if(num=="Q") return false;
			else PutMessage("\1r\1h Please enter a number within the given range.",x,y);
		}
	}
	if(minNumPlayers==maxNumPlayers) fixedPlayers=true;
	while(1)
	{
		x=2; y=6;
		console.gotoxy(x,y);
		console.cleartoeol();
		if(console.noyes("\1n\1gInclude computer players?")) break;
		console.gotoxy(x,y);
		console.cleartoeol();
		console.putmsg("\1n\1gHow many? [\1h1\1n\1g-\1h" + (maxNumPlayers-1) + "\1n\1g] or [\1hQ\1n\1g]uit: ");
		cnum=console.getkeys("Q",maxNumPlayers-1);
		if(cnum<=(maxNumPlayers-1) && cnum>0) 
		{
			numComputerPlayers=cnum;
			break;
		}
		else if(cnum=="Q") return false;
		else PutMessage("\1r\1h Please enter a number within the given range.",x,y);
	}
	games.gameData[gameNumber]=new NewGame(minNumPlayers,maxNumPlayers,gameNumber);
	games.gameData[gameNumber].fileName=GetFileName(gameNumber);
	for(cp=0;cp<numComputerPlayers;cp++)
	{
		games.gameData[gameNumber].addPlayer(-1, -1); //NO USER NUMBER, NO VOTE
	}
	if(numComputerPlayers+1==maxNumPlayers) {
		singlePlayer=true;
		GameLog("single player game");
	}
	else {
		if(fixedPlayers) games.gameData[gameNumber].fixedPlayers=true;
		else
		{
			console.crlf();
			vote=GetVote();
		}
	}
	games.gameData[gameNumber].addPlayer(user.number, vote);
	if(singlePlayer) 
	{
		games.gameData[gameNumber].singlePlayer=true;
		StartGame(gameNumber);
		games.singleGames.push(gameNumber);
		games.yourGames.push(gameNumber);
		games.yourTurn.push(gameNumber);
	}
	else games.notFull.push(gameNumber);

	games.StoreGame(gameNumber);
	QueueMessage("\1r\1hGame " + gameNumber + " Created!",30,20);
	Unlock(gamefile);
	return true;
}
function	GetFileName(gameNumber)
{
	var num=gameNumber;
	if(gameNumber<10) num="0" + gameNumber;
	var gamefile=root + num;
	return (game_dir + gamefile + ".dat");
}
function 	GetVote()
{
	var vote=0;
	console.putmsg("\1n\1gDo you wish to start the game immediately or wait for more players?");
	console.crlf();
	if(console.yesno("\1g\1hYes to start, No to wait"))
	{
		vote = 1;
	}
	GameLog("got user vote: " + vote);
	return vote;
}
function 	GetUserName(playerData,playerNumber)
{
	if(playerData.user>=0)
		return(system.username(playerData.user));
	if(playerData.AI.name.length > 0)
		return(playerData.AI.name);
	return("Computer " + (playerNumber+1));
}
//###########################GAMEPLAY FUNCTIONS#############################
function	SelectTile(gameNumber,playerNumber,attackPosition,startPosition)
{
	var g=games.gameData[gameNumber];		
	if(attackPosition>=0) 
	{
		terr=g.grid[attackPosition]; 			//IF A BOARD POSITION HAS BEEN SUPPLIED TO ATTACK FROM, START THERE
	}
	else if(startPosition>=0) 
	{
		terr=g.grid[startPosition];		//IF A BOARD POSITION HAS BEEN SUPPLIED TO START FROM, START THERE
		ShowSelected(terr,"\1n\1w\1h");
	}
	else														//OTHERWISE, START WITH THE FIRST PLAYER TERRITORY THAT IS CAPABLE OF ATTACKING
	{
		var p=g.players[playerNumber];
		for(tile in p.territories)
		{
			terr=g.grid[p.territories[tile]];
			if(terr.dice>1) 
			{
				ShowSelected(terr,"\1n\1w\1h");
				break;
			}
		}
	}
	dir=terr.location;
	while(1)
	{
		var key=console.getkey((K_NOECHO,K_NOCRLF,K_UPPER));
		if(key=="\r" || key==undefined) 
		{
			if(attackPosition>=0)
			{
				if(terr.player==playerNumber);
				else
				{
					if(terr.location==g.grid[attackPosition].North) return terr;
					if(terr.location==g.grid[attackPosition].South) return terr;
					if(terr.location==g.grid[attackPosition].Northeast) return terr;
					if(terr.location==g.grid[attackPosition].Northwest) return terr;
					if(terr.location==g.grid[attackPosition].Southeast) return terr;
					if(terr.location==g.grid[attackPosition].Southwest) return terr;
				}
			}
			else if(terr.player==playerNumber && terr.dice>1) return terr;
		}
		switch(key)
		{
			case KEY_DOWN:
			case '2':
				if(g.grid[terr.South])
					dir=terr.South
				break;
			case KEY_UP:
			case '5':
				if(g.grid[terr.North])
					dir=terr.North;
				break;
			case '1':
				if(g.grid[terr.Southwest])
					dir=terr.Southwest;
				break;
			case '4':
				if(g.grid[terr.Northwest])
					dir=terr.Northwest;
				break;
			case '3':
				if(g.grid[terr.Southeast])
					dir=terr.Southeast;
				break;
			case '6':
				if(g.grid[terr.Northeast])
					dir=terr.Northeast;
				break;
			case KEY_LEFT:
				if(g.grid[terr.Southwest])
					dir=terr.Southwest;
				else if(g.grid[terr.Northwest])
					dir=terr.Northwest;
				break;
			case KEY_RIGHT:
				if(g.grid[terr.Northeast])
					dir=terr.Northeast;
				else if(g.grid[terr.Southeast])
					dir=terr.Southeast;
				break;
			case "Q":
				terr.displayBorder(border_color);
				return false;
			default:
				continue;
		}
		terr.displayBorder(border_color);
		terr=g.grid[dir];
		ShowSelected(terr,"\1n\1w\1h");
	}
	WipeCursor("left");
}
function	ShowSelected(selection,color)
{
	selection.displaySelected(color);
	WipeCursor("left");
}
function	Attack(gameNumber,playerNumber)
{
	ClearArea(16,menuColumn,8);
	AttackMessage("from which to Attack,");
	var attackFrom=SelectTile(gameNumber,playerNumber,-1,games.gameData[gameNumber].lastMapPosition);
	if(attackFrom==false) return false;
	
	ClearArea(16,menuColumn,8);
	AttackMessage("to Attack,");
	var attackTo=SelectTile(gameNumber,playerNumber,attackFrom.location);
	if(attackTo==false) return false;
	var tofrom=Battle(attackFrom,attackTo,gameNumber);
	if(tofrom=true)	games.gameData[gameNumber].lastMapPosition=attackTo.location;
	else games.gameData[gameNumber].lastMapPosition=attackFrom.location;
	return true;
}
function	Battle(attackFrom,attackTo,gameNumber)
{
	var g=games.gameData[gameNumber];
	ClearArea(16,menuColumn,8);
	ShowSelected(attackFrom,"\1n\1r\1h");
	ShowSelected(attackTo,"\1n\1r\1h");
	var totals=RollDice(attackFrom.dice,attackTo.dice);
	GameLog("attackingfrom: " + attackFrom.dice + " attackingTo: " + attackTo.dice);
	
	var defender=attackTo.player;
	var attacker=attackFrom.player;
	var defending=attackTo.location;
	var attacking=attackFrom.location;
	if(totals[0]>totals[1])
	{
		g.players[defender].removeTerritory(defending); 	//REMOVE TILE FROM DEFENDER'S LIST
		g.players[attacker].territories.push(defending);	//ADD TILE TO ATTACKER'S LIST
		
		g.players[defender].totalDice-=(attackTo.dice);
		
		g.grid[defending].assign(attacker,g.players[attacker]);
		g.grid[defending].dice=(attackFrom.dice-1);
		if(g.players[defender].territories.length==0) 
		{
			var killer=g.players[attacker].user;
			var killed=g.players[defender].user;
			g.EliminatePlayer(defender,killer);
			if(killer>=0)
			{
				GameLog("player scored a kill: " + system.username(killer));
				scores[killer].kills++;
				games.StoreRankings();
			}
			if(killed>=0)
			{
				GameLog("player has been killed: " + system.username(killed));
				var kname= (killer<0?g.players[attacker].AI.name:system.username(killer));
				DeliverKillMessage(kname,killed,gameNumber);
			} 
			if(g.CheckElimination()) games.StoreGame(gameNumber); 
		}
	}
	g.grid[attacking].dice=1;
	attackFrom.show();
	attackFrom.displayBorder(border_color);
	attackTo.displayBorder(border_color);
	attackTo.show();
	g.DisplayPlayers();
}
function	EndTurn(gameNumber,pl)
{
	GameLog("###ENDING TURN");
	var g=games.gameData[gameNumber];
	var placed=g.Reinforce(pl);
	ClearArea(16,menuColumn,9);
	console.gotoxy(menuColumn,16);
	console.putmsg("\1r\1hPlaced " + placed.length + " reinforcements");
	mswait(500);
	ClearArea(16,menuColumn,9);
	for(place in placed) g.grid[placed[place]].show();
	g.takingTurn=false;
	games.StoreGame(gameNumber);
}
function	Forfeit(gameNumber,playerNumber)
{
	GameLog("Player " + playerNumber + " forfeitting game");
	var g=games.gameData[gameNumber];
	if(g.singlePlayer) 
	{
		scores[user.number].losses+=1;
		GameLog("assigning loss to user " + user.alias);
		file_remove(g.fileName);
	}
	else 
	{
		var activePlayers=g.countActivePlayers();
		scores[user.number].score+=points[7-activePlayers.length];
		GameLog("giving " + points[7-activePlayers.length] + " points to user " + user.alias);
		if(activePlayers.length==2) 
		{
			g.status=0;
			for(player in activePlayers)
			{
				var ply=activePlayers[player];
				if(g.players[ply].user!=user.number)
				{
					g.winner=g.players[ply].user;
					scores[g.players[ply].user].points+=2;
					GameLog("giving " + pts + " points to user " + system.username(g.players[ply].user));
					break;
				}
			}
		}
		else 
		{
			delete g.users[user.number];
			g.players[playerNumber].AI.name=user.alias+" AI";
			g.players[playerNumber].user=-1;
			games.StoreGame(gameNumber);
		}
	}
	g.DisplayPlayers();
	games.StoreRankings();
}
//MAIN GAMEPLAY FUNCTION
function	PlayGame(gameNumber)
{
	console.clear();
	var g=games.gameData[gameNumber];
	var userInGame;
	var currentPlayer;
	g.Redraw();
	
	var pMenu=new Menu(		""								,1,1,GetColor("green"),GetColor("green","high"));
	var pmenu_items=[		"~Take Turn"					, 
							"~Attack"						,
							"~Forfeit"						,
							"~End"							,
							"~Redraw"						,
							"~Quit"							];
	pMenu.add(pmenu_items);	
	pMenu.disable(["A","E","T","F"]);
	ClearArea(16,menuColumn,9);

	while(1)
	{
		if(g.users[user.number]>=0) 
		{
			userInGame=true;
			currentPlayer=g.users[user.number];
		}
		else 
		{
			currentPlayer=-1;
			userInGame=false;
		}
		var turn=g.turnOrder[g.nextTurn];
		while(g.players[turn].user<0 && userInGame && g.status==1) 
		{
			////////////////////////////////////
				GameLog("####COMPUTER PLAYER TAKING TURN");
				ClearLine(1,48);
				console.gotoxy(2,1);
				console.putmsg("\1r\1hPlease wait. " + g.players[turn].AI.name + " taking turn.");
				mswait(750);
			/////////////////////////////////////
			g.players[turn].AI.turns=0;
			g.players[turn].AI.moves=0;
			while(g.CanAttack(turn))
			{
				if(!TakeTurnAI(gameNumber,turn))
					break;
			}
			g.takingTurn=true;
			EndTurn(gameNumber,turn);
			g.DisplayPlayers();
			turn=g.turnOrder[g.nextTurn];
		}
		if(!g.takingTurn)
		{
			ClearArea(16,50,8);
			console.gotoxy(51,16);
			console.putmsg("\1r\1hGame \1n\1r#\1h" + gameNumber);
			if(g.status==0)
			{
				pMenu.disable(["A","E","T","F"]);
				pMenu.enable(["Q"]);
				ShowWinner(g);
			}
			else if(turn==currentPlayer) 
			{
				pMenu.enable("T");
				GameLog("it is the current user's turn");
			}
			else  
			{
				console.gotoxy(51,18);
				console.putmsg("\1r\1hIt is " + GetUserName(g.players[turn],turn) + "'s turn");
				var daysOld=g.FindDaysOld();
				var hoursOld=parseInt(daysOld*24);
				if(daysOld>0)
				{
					console.gotoxy(51,19);
					console.putmsg("\1r\1hLast turn taken " + hoursOld + " hours ago");
				}
			}
		}
		pMenu.displayHorizontal();
		var cmd=console.getkey((K_NOECHO,K_NOCRLF,K_UPPER));
		WipeCursor("right");
		if(pMenu.items[cmd] && pMenu.items[cmd].enabled)
		{
			switch(cmd)
			{
			case "T":
				pMenu.enable(["F"]);
				pMenu.disable(["Q","T"]);
				g.takingTurn=true;
				GameLog("######TAKING TURN");
				if(g.CanAttack(currentPlayer,-1))
				{
					pMenu.enable("A");
				}
				pMenu.enable("E");
				continue;
			case "F":
				console.home();
				if(console.noyes("\1n\1gForfeit this game?")) continue;
				Forfeit(gameNumber,currentPlayer);
				pMenu.disable(["A","F","E"]);
				pMenu.enable(["Q"]);
				break; 
			case "A":
				if(Attack(gameNumber,currentPlayer));
				else 
				{
					GameLog("player cancelled attack");
					continue; 
				}
				g.DisplayPlayers();
				break;
			case "E":
				pMenu.disable(["A","E","F"]);
				pMenu.enable(["Q"]);
				g.takingTurn=false;
				EndTurn(gameNumber,currentPlayer);
				g.DisplayPlayers();
				break;
			case "R":
				console.clear();
				g.Redraw();
				continue;
			case "Q": 
				return;
				break;
			default:
				WipeCursor("left");
				continue;
			}
		}
	}
}

function 	TakeTurnAI(gameNumber,playerNumber)
{
	g=games.gameData[gameNumber];
	computerPlayer=g.players[playerNumber];
	targets=[];

	/* For each owned territory */
	for(territory in computerPlayer.territories)
	{
		base=computerPlayer.territories[territory];
		/* If we have enough to attack */
		if(g.grid[base].dice>1)
		{
			/* Find places we can attack */
			attackOptions=g.CanAttack(playerNumber,base,computerPlayer,g);
			if(attackOptions!==false) {
				var basetargets=new Array();

				/* Randomize the order to check in */
				attackOptions.sort(RandomSort);
				for(option in attackOptions)
				{
					target=attackOptions[option];
					/* Check if this is an acceptable attack */
					if(AICheckFunctions[computerPlayer.AI.check](gameNumber, playerNumber, base, target))
						basetargets.push({gameNumber:gameNumber, target:target, base:base, target_grid:g.grid[target], base_grid:g.grid[base]});
				}
				/* If we found acceptable attacks, sort them and choose the best one */
				if(basetargets.length > 0) {
					basetargets.sort(AISortFunctions[computerPlayer.AI.sort]);
					targets.push(basetargets.shift());
				}
			}
		}
	}
	/* Randomize the targets array */
	targets.sort(RandomSort);
	attackQuantity=AIQtyFunctions[computerPlayer.AI.qty](targets.length);
	if(attackQuantity < 1)
		return false;
	targets.sort(AISortFunctions[computerPlayer.AI.sort]);
	for(attackNum=0;attackNum<attackQuantity;attackNum++)
	{
		GameLog("computer " + (playerNumber+1) + " attacking: " + targets[attackNum].target + " from: " + targets[attackNum].base);
		attackFrom=g.grid[targets[attackNum].base];
		attackTo=g.grid[targets[attackNum].target];
		if(attackFrom.dice>1 && attackTo.player!=playerNumber) {
			Battle(attackFrom,attackTo,gameNumber);
			computerPlayer.AI.moves++;
		}
	}
	computerPlayer.AI.turns++;
	return true;
}
function 	Quit(err) 
{
	console.ctrlkey_passthru=oldpass;
	bbs.sys_status&=~SS_MOFF;
	file_remove(userFileName);
	console.clear();
	
	//TODO:  CREATE A GAME EXIT ANSI SCREEN
	printf("\n\r\1r\1h  Thanks for playing!\r\n\r\n");
	mswait(500);
	exit(0);
}
//#######################MAIN GAME CLASS###################################
function	LoadSettings()
{
	var sfile=new File(game_dir + "dice.ini");
	sfile.open('r',true);
	pointsToWin=	sfile.iniGetValue(null,"pointstowin");
	minScore=		sfile.iniGetValue(null,"minscore");
	maxGames=		sfile.iniGetValue(null,"maxgames");
	minPlayers=		sfile.iniGetValue(null,"minplayers");
	maxPlayers=		sfile.iniGetValue(null,"maxplayers");
	maxDice=		sfile.iniGetValue(null,"maxdice");
	logEnabled=		sfile.iniGetValue(null,"enablelogging");
	
	if(logEnabled)
	{
		if(!file_isdir(game_dir+"logs")) mkdir(game_dir+"logs");
		gamelog=new Logger(game_dir + "logs/",root);
	}
	sfile.close();
}
function	GameStatusInfo()
{
	this.gameData=[]; //master game data array
	this.inProgress=[];
	this.notFull=[];
	this.completed=[];
	this.youWin=[];
	this.yourGames=[];
	this.yourTurn=[];
	this.eliminated=[];
	this.singleGames=[];

	
	//TODO: REWORK SCOREFILE CODE....BECAUSE IT SUCKS
	this.StoreRankings=function()
	{
		var sfilename=game_dir+scorefile+".dat";
		var sfile=new File(sfilename);
		if(!Locked(scorefile,true))
		{
			GameLog("storing rankings in file: " + sfilename);
			Lock(scorefile);
			sfile.open((file_exists(sfilename)?'r+':'w+'), true);
			for(s in scores)
			{
				var score=scores[s];
				sfile.iniSetValue(s,"score",score.score);
				sfile.iniSetValue(s,"kills",score.kills);
				sfile.iniSetValue(s,"wins",score.wins);
				sfile.iniSetValue(s,"losses",score.losses);
			}
			sfile.close();
			Unlock(scorefile);
		}
	}
	this.LoadRankings=function()
	{
		if(file_exists(game_dir + "dicerank.dat")) this.UpdateScoreFile();
		var sfilename=game_dir+scorefile+".dat";
		if(file_exists(sfilename))
		{
			var lfile=new File(sfilename);
			GameLog("loading scores from file: " + sfilename);
			lfile.open('r',true);
			var plyrs=lfile.iniGetAllObjects("user");
			for(p in plyrs)
			{
				var plyr=plyrs[p];
				scores[plyr.user]=plyr;
				if(plyr.score>=pointsToWin) 
				{	
					lfile.close();
					file_remove(lfile.name);
					this.WinRound(player);
					scores=[];
					break;
				}
			}
			lfile.close();
		}
		else GameLog("score file: " + sfilename + " does not exist");
	}
	this.UpdateScoreFile=function() //FOR BACKWARD COMPATIBILITY WITH OLD SCORE FILES (for sysops who dont want to reset their scores with the latest changes)
	{
		GameLog("updating score file");		
		var sfile=new File(game_dir+"dicerank.dat");
		sfile.open('r',true);
		for(sc=0;!(sfile.eof);sc++) 
		{
			plyr=sfile.readln();
			if(plyr==undefined || plyr=="") break;
			else
			{
				player=parseInt(plyr);
				var score=parseInt(sfile.readln());
				var wins=parseInt(sfile.readln());
				var losses=parseInt(sfile.readln());
				scores[player]={'score':score,'kills':0,'wins':wins,'losses':losses};
				GameLog("loaded old score: " + score + " w: " + wins + " l: " + losses);		
			}
		}
		sfile.close();
		file_remove(sfile.name);
		this.StoreRankings();
	}
	this.WinRound=function(player)
	{	
		hfilename=game_dir+halloffame+".dat";
		var hfile=new File(hfilename);
		if(!file_exists(hfile.name))
		{
			hfile.open('w+');
			hfile.writeln("\1y\1hDICE-WARZ HALL OF FAME (Round Winners):");
			hfile.writeln();
		}
		else hfile.open('a');
		hfile.writeln(system.username(player) + ": " + system.datestr());
		hfile.close();
	}
	this.HallOfFame=function()
	{	
		if(file_exists(game_dir+halloffame+".dat"))
		{
			console.clear();
			console.printfile(game_dir + halloffame + ".dat");
			console.pause();
		}
	}
	this.LoadGame=function(gamefile,gameNumber,lastModified)
	{
		humans=0;
		var gfile=new File(gamefile);
		gfile.open('r',true);
		var lgame;

		var status=parseInt(gfile.readln());
		if(status<0) 
		{
			var minp=parseInt(gfile.readln());
			var maxp=parseInt(gfile.readln());
			lgame=new NewGame(minp,maxp,gameNumber);
			lgame.lastModified=lastModified;
			lgame.fileName=gamefile;
			if(minp==maxp) lgame.fixedPlayers=true;
			
			for(nnn=0;!(gfile.eof) && nnn<maxp;nnn++)
			{
				userNumber=gfile.readln();
				if(userNumber==undefined || userNumber=="") break;
				else
				{
					userNumber=parseInt(userNumber);
					vote=gfile.readln();
					lgame.addPlayer(userNumber,parseInt(vote));
					if(userNumber>0) 
					{
						if(!scores[userNumber]) 
						{
							//INITIALIZE ALL PLAYERS WHO HAVE NO SCORE ENTRY AS ZERO POINTS ZERO WINS ZERO KILLS ZERO LOSSES
							scores[userNumber]={'score':0,'kills':0,'wins':0,'losses':0};
						}
					}
				}
			}
			gfile.close();
			return lgame;
		}
		var np=parseInt(gfile.readln());
		var ms=parseInt(gfile.readln());
		var nt=parseInt(gfile.readln());
		var r=parseInt(gfile.readln());
		var c=parseInt(gfile.readln());
		var pt=parseInt(gfile.readln());

		lgame=new Map(c,r,np,gameNumber);
		lgame.lastModified=lastModified;
		lgame.fileName=gamefile;
		lgame.mapSize=ms;
		lgame.nextTurn=nt;
		lgame.status=status;
		lgame.playerTerr=pt;
		
		for(to=0;to<np;to++)
		{
			ttoo=parseInt(gfile.readln());
			lgame.turnOrder[to]=ttoo;
		}
		var aifile=new File(game_dir + "ai.ini");
		aifile.open("r");
		for(pl=0;pl<np;pl++)
		{
			var uname=gfile.readln();
			var u=-1;
			if(uname.search(/^[0-9]+$/) != -1)
				u=parseInt(uname);
			if(uname=='-1')
				uname='';
			res=parseInt(gfile.readln());

			lgame.players[pl]=new Player(u,-1);
			lgame.players[pl].setColors(pl);
			lgame.players[pl].reserve=res;

			if(u>0) 
			{
				lgame.users[u]=pl;
				humans++;
				if(scores[u]);
				else
				{
					scores[u]={'score':0,'wins':0,'losses':0};
				}
			}
			else {
				/* Set up computer players */
				lgame.players[pl].AI.name=uname;
				lgame.players[pl].AI.sort=aifile.iniGetValue(lgame.players[pl].AI.name, "Sort", "Random");
				lgame.players[pl].AI.check=aifile.iniGetValue(lgame.players[pl].AI.name, "Check", "Random");
				lgame.players[pl].AI.qty=aifile.iniGetValue(lgame.players[pl].AI.name, "Quantity", "Random");
				if(AISortFunctions[lgame.players[pl].AI.sort]==undefined)
					lgame.players[pl].AI.sort="Random";
				if(AICheckFunctions[lgame.players[pl].AI.check]==undefined)
					lgame.players[pl].AI.check="Random";
				if(AIQtyFunctions[lgame.players[pl].AI.qty]==undefined)
					lgame.players[pl].AI.qty="Random";
			}
		}
		aifile.close();
		if(humans<2) 
		{
			lgame.singlePlayer=true;
			GameLog("single player game");
		}
		for(sec=0;sec<ms;sec++)
		{
			spot_player=parseInt(gfile.readln());
			spot_index=parseInt(gfile.readln());
			spot_dice=parseInt(gfile.readln());
			lgame.grid[spot_index]=new Territory(spot_index);
			lgame.used[spot_index]=true;
			lgame.grid[spot_index].assign(spot_player,lgame.players[spot_player]);
			lgame.players[spot_player].territories.push(spot_index);
			lgame.grid[spot_index].dice=spot_dice;
		}
		lgame.SetGrid();
		lgame.SetEliminated();
		lgame.CountDiceAll();
		GameLog("game " + gameNumber + " loaded");
		gfile.close();
		return lgame;
	}
	this.StoreGame=function(gameNumber)
	{
		g=this.gameData[gameNumber];
		GameLog("Storing game: " + gameNumber);
		var gamefullname=GetFileName(gameNumber);
		var gfile=new File(gamefullname);
		gfile.open('w+',false);
		
		gfile.writeln(g.status);
		if(g.status<0)
		{
			gfile.writeln(g.minPlayers);
			gfile.writeln(g.maxPlayers);
			for(nnn=0;nnn<g.players.length;nnn++)
			{
				gfile.writeln(g.players[nnn].user);
				gfile.writeln(g.players[nnn].vote);
			}
			gfile.close();
			return;
		}
		gfile.writeln(g.maxPlayers);
		gfile.writeln(g.mapSize);
		gfile.writeln(g.nextTurn);
		gfile.writeln(g.rows);
		gfile.writeln(g.columns);
		gfile.writeln(g.playerTerr);
		for(to=0;to<g.maxPlayers;to++)
		{
			gfile.writeln(g.turnOrder[to]);
		}
		for(ply in g.players)
		{
			p=g.players[ply];
			if(p.user==-1)
				gfile.writeln(p.AI.name);
			else
				gfile.writeln(p.user);
			gfile.writeln(p.reserve);
		}	
		for(sector in g.used)
		{
			location=sector;
			gfile.writeln(g.grid[location].player);
			gfile.writeln(g.grid[location].location);
			gfile.writeln(g.grid[location].dice);
		}
		gfile.close();
	}	
	this.UpdateGames=function()
	{
		GameLog("updating game data");
		for(gd in this.gameData)
		{
			var fileName=this.gameData[gd].fileName;
			var lastModified=file_date(fileName);

			if(file_exists(fileName))
			{
				if(lastModified>this.gameData[gd].lastModified) 
				{
					GameLog("game " + gd + " needs updating");
					this.gameData[gd]=this.LoadGame(fileName,gd,lastModified);
				}
			}
			else
			{
				GameLog("gamefile " + fileName + " deleted, removing data");
				delete this.gameData[gd];
			}
		}
		this.FilterData();
	}
	this.SortArray=function(data)
	{
		var numItems=data.length;
		for(n=0;n<numItems;n++)
		{
			for(m = 0; m < (numItems-1); m++) 
			{
				if(parseInt(data[m]) > parseInt(data[m+1])) 
				{
					holder = data[m+1];
					data[m+1] = data[m];
					data[m] = holder;
				}
			}
		}
		return data;
	}
	this.FilterData=function()
	{
		GameLog("organizing game data");
		this.singleGames=[]; 
		this.inProgress=[];
		this.notFull=[];
		this.completed=[];
		this.youWin=[];
		this.yourGames=[];
		this.yourTurn=[];
		this.eliminated=[];
		
		for(ggg in this.gameData)
		{
			var gm=this.gameData[ggg];
			var	playerNumber=gm.users[user.number];
			
			if(gm.users[user.number]>=0)
			{
				if(gm.status<0) this.notFull.push(ggg);
				if(gm.singlePlayer) this.singleGames.push(ggg);
				this.yourGames.push(ggg);
				if(gm.status==0) 
				{
					this.completed.push(ggg);
					if(gm.winner==user.number) this.youWin.push(ggg);
				}
				else if(gm.status>0)
				{
					this.inProgress.push(ggg);
					if(gm.turnOrder[gm.nextTurn]==playerNumber || gm.singlePlayer) {
						this.yourTurn.push(ggg);
					}
				}
				if(gm.players[playerNumber].eliminated==true) this.eliminated.push(ggg);
			}
			else 
			{
				if(gm.status<0) this.notFull.push(ggg);
				else if(!gm.singlePlayer)
				{
					if(gm.status==0) 
					{
						this.completed.push(ggg);
						if(gm.winner>=0) 
						{
							if(gm.winner==user.number) this.youWin.push(ggg);
						}
					}
					else this.inProgress.push(ggg);
				}
			}
		}
	}
	this.LoadGames=function()
	{	
		var open_list=directory(game_dir + root + "*.dat"); 	
		GameLog("today's date: " + time());
		if(open_list.length)
		{
			for(lg in open_list)
			{
				var temp_fname=file_getname(open_list[lg]);
				var lastModified=file_date(open_list[lg]);
				var daysOld=(time()-lastModified)/daySeconds;
				var gameNumber=parseInt(temp_fname.substring(4,temp_fname.indexOf(".")),10);
				GameLog("game " + gameNumber + " last modified: " + daysOld + " ago");
				var lgame=this.LoadGame(open_list[lg],gameNumber,lastModified);
				this.gameData[gameNumber]=lgame;
			}
		}
		this.FilterData();
		this.DeleteOld();
	}
	this.DeleteOld=function()
	{
		for(oldgame in this.gameData)
		{
			daysOld=(time()-this.gameData[oldgame].lastModified)/daySeconds;
			if(this.gameData[oldgame].singlePlayer==true && daysOld>=10)
			{
				GameLog("removing old singleplayer game: " + this.gameData[oldgame].gameNumber)
				file_remove(this.gameData[oldgame].fileName);
				delete this.gameData[oldgame];
			}
		}
		for(completed in this.completed)
		{
			gm=this.completed[completed];
			daysOld=(time()-this.gameData[gm].lastModified)/daySeconds;
			GameLog("game was completed " + daysOld + " days ago");
			if(this.gameData[gm].singlePlayer==true)
			{
				if(daysOld>=1) 
				{
					GameLog("removing old singleplayer game: " + this.gameData[gm].gameNumber)
					file_remove(this.gameData[gm].fileName);
					delete this.gameData[gm];
				}
			}
			else
			{
				if(daysOld>=4) 
				{
					GameLog("removing old game: " + gm.gameNumber)
					file_remove(this.gameData[gm].fileName);
					delete this.gameData[gm];
				}
			}
		}
	}
	this.SkipPlayers=function()
	{
		for(inp in this.inProgress)
		{
			gm=this.gameData[this.inProgress[inp]];
			if(gm) daysOld=(time()-gm.lastModified)/daySeconds;
			GameLog("game: " + this.inProgress[inp] + " last turn was: " + daysOld + " ago");
			if(daysOld>=4 && !gm.singlePlayer) 
			{
				nextTurnPlayer=gm.turnOrder[gm.nextTurn];
				GameLog("skipping player: " + system.username(gm.players[nextTurnPlayer].user) + " in game " + this.inProgress[inp]);
				gm.Reinforce(nextTurnPlayer);
				this.StoreGame(this.inProgress[inp]);
			}
		}
	}
	this.Init=function()
	{
		this.LoadRankings();
		this.LoadGames();
		this.StoreRankings();
		this.SkipPlayers();
	}
}
function	GameLog(data)
{
	if(gamelog) gamelog.Log(data);
}

	LoadSettings();
	SplashScreen();
	GameMenu();
	
