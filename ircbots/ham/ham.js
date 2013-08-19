if(!js.global || js.global.HTTPRequest==undefined)
	js.global.load("http.js");
if(!js.global || js.global.USCallsign==undefined)
	js.global.load("callsign.js");

// Comment these...
//Bot_Commands={};
//function Bot_Command(x,y,z){}
//js.global.config_filename='hambot.ini';

var last_update=0;
var update_interval=2;
var last_wflength=-1;
var last_solar_update=0;
var solar_x;
var contests={};
var last_contest_update=0;

function main(srv,target)
{
	if((time() - last_update) < update_interval) return;

	var config = new File(system.ctrl_dir + js.global.config_filename);
	var watch=new File();
	if(!config.open('r')) {
		log("Unable to open config!");
		return;
	}
	var wfname=config.iniGetValue("module_Ham", 'watchfile');
	config.close();
	if(wfname.length > 0) {
		var wf=new File(wfname);
		var len=wf.length;
		// Ignore contents at start up.
		if(last_wflength==-1)
			last_wflength=len;
		// If the file is truncated, start over
		if(last_wflength > len)
			last_wflength=0;
		if(len > last_wflength) {
			if(wf.open("r")) {
				var l;
				wf.position=last_wflength;
				while(l=wf.readln()) {
					srv.o(target, l);
				}
				last_wflength=wf.position;
				wf.close();
			}
		}
	}
	last_update=time();
}

Bot_Commands["Z"] = new Bot_Command(0, false, false);
Bot_Commands["Z"].command = function (target, onick, ouh, srv, lbl, cmd) {
	// Remove empty cmd args
	for(i=1; i<cmd.length; i++) {
		if(cmd[i].search(/^\s*$/)==0) {
			cmd.splice(i,1);
			i--;
		}
	}

	if(cmd.length == 1) {
		var d=new Date(time()*1000);
		srv.o(target,d.toGMTString());
	}

	return true;
}

function update_contests()
{
	// Only update once per day.
	if((time() - last_contest_update) < 60*60*24) return;
	contests={};
	contest_order=[];

	function parse_contest(cont) {
		var m;
		var c;

		function parse_time(t) {
			var se = t.split(/ to /);
			var ret={};

			if(se.length==1) {
				se=t.match(/([0-9]{4})([^-]*?)-([0-9]{4})([^-]*?), ([A-Za-z]{3}) ([0-9]+)/)
				if(se == null) {
					log("Error parsing range "+t+"!");
					return;
				}
				ret.startTime=se[1];
				ret.zone=se[2].replace(/^\s*(.*?)\s*$/, "$1");
				ret.endTime=se[3];
				ret.startMonth=se[5];
				ret.endMonth=se[5];
				ret.startDay=se[6];
				ret.endDay=se[6];
			}
			else {
				var s=se[0].match(/([0-9]{4})([^-]*?), ([A-Za-z]{3}) ([0-9]+)/);
				if(s == null) {
					log("Error parsing start "+se[0]+"!");
					return;
				}
				ret.startTime=s[1];
				ret.zone=s[2].replace(/^\s*(.*?)\s*$/, "$1");
				ret.startMonth=s[3];
				ret.startDay=s[4];
				s=se[1].match(/([0-9]{4})([^-]*?), ([A-Za-z]{3}) ([0-9]+)/);
				if(s == null) {
					log("Error parsing end "+se[1]+"!");
					return;
				}
				ret.endTime=s[1];
				ret.endMonth=s[3];
				ret.endDay=s[4];
			}
			return ret;
		}

		m=cont.match(/<tr><td colspan="3" class="bgray"><strong><a[^>]*>(.[^<]*)<\/a>([^<]*)<\/strong>/)
		if(m != null) {
			c=m[1];
			var times=m[2].split(/ and /);

			contest_order.push(c);
			contests[c]={};
			contests[c].timeStr=m[2];
			contests[c].times=[];
			for(timeidx in times) {
				contests[c].times.push(parse_time(times[timeidx]));
			}
			var re=/<tr><td [^>]*>&nbsp;<\/td><td class="blightblue twhite">(.*?):<\/td><td>(.*?)<\/td><\/tr>/g;
			var prop;
			while((prop=re.exec(cont))!=null) {
				if(prop[1]=='Bands') {
					prop[2] = prop[2].replace(/ Only$/i,'').replace(/([0-9])m$/,'$1').replace(/VHF\/UHF/,'VHF, UHF');
					contests[c][prop[1]]=prop[2].split(/,\s*/);
				}
				else if(prop[1]=='Mode') {
					contests[c][prop[1]]=prop[2].split(/,\s*/);
				}
				else if(prop[1]=='Find rules at') {
					contests[c][prop[1]]=prop[2].replace(/^.*href="(.*?)">.*$/,"$1");
				}
				else {
					contests[c][prop[1]]=prop[2].split(/<br>/);
				}
			}
		}
	}

	// Fetch current contest cal (next 8 days)
	var req=new HTTPRequest();
	var currcal=req.Get('http://www.hornucopia.com/contestcal/weeklycont.php');
	var matches=currcal.match(/<tr><td colspan="3" class="bgray"><strong>[\x00-\xff]*?<tr><td>&nbsp;<\/td><td class="blightblue">&nbsp;<\/td><td>&nbsp;<\/td><\/tr>/g);
	for(match in matches) {
		parse_contest(matches[match]);
	}
	var currcal=req.Get('http://www.hornucopia.com/contestcal/weeklycont.php?mode=custom&week=next');
	var matches=currcal.match(/<tr><td colspan="3" class="bgray"><strong>[\x00-\xff]*?<tr><td>&nbsp;<\/td><td class="blightblue">&nbsp;<\/td><td>&nbsp;<\/td><\/tr>/g);
	for(match in matches) {
		parse_contest(matches[match]);
	}
}

function update_solar()
{
	// MUST NOT request more than every hour or you will get banned!
	if((time() - last_solar_update) < 60*60) return;
	last_solar_update=time();
	var req=new HTTPRequest();
	var newxml=req.Get('http://www.hamqsl.com/solarxml.php');
	newxml=newxml.replace(/<\?[^?]*\?>/g,'');
	solar_x=new XML(newxml).solardata;
}

function trim(val)
{
	var ret=val.toString().replace(/^\s*/,"");
	ret=ret.replace(/\s*$/,"");
	return(ret);
}

Bot_Commands["GEO"] = new Bot_Command(0, false, false);
Bot_Commands["GEO"].command = function (target, onick, ouh, srv, lbl, cmd) {
	// Remove empty cmd args
	for(i=1; i<cmd.length; i++) {
		if(cmd[i].search(/^\s*$/)==0) {
			cmd.splice(i,1);
			i--;
		}
	}

	if(cmd.length == 1) {
		update_solar();
		srv.o(target,"Solar conditions from "+trim(solar_x.source)+" as of "+trim(solar_x.updated)+":");
		srv.o(target,"Sun Spots: "+trim(solar_x.sunspots)+", Solar Flux: "+trim(solar_x.solarflux)+", A-Index: "+trim(solar_x.aindex)+", K-Index: "+trim(solar_x.kindex)+"/"+trim(solar_x.kindexnt));
		srv.o(target,"X-Ray: "+trim(solar_x.xray)+", Helium Line: "+trim(solar_x.heliumline)+", Proton Flux: "+trim(solar_x.protonflux)+", Electron Flux: "+trim(solar_x.electonflux));
		srv.o(target,"Aurora Activity: "+trim(solar_x.aurora)+"/n="+trim(solar_x.normalization)+", Aurora Latitude: "+trim(solar_x.latdegree));
		srv.o(target,"Solar Wind: "+trim(solar_x.solarwind)+", Magnetic Field: "+trim(solar_x.magneticfield)+", Geomagnetic Field: "+trim(solar_x.geomagfield)+", Noise Level: "+trim(solar_x.signalnoise)+", F2 Critical Frequency: "+trim(solar_x.fof2));
	}

	return true;
}

Bot_Commands["VHF"] = new Bot_Command(0, false, false);
Bot_Commands["VHF"].command = function (target, onick, ouh, srv, lbl, cmd) {
	// Remove empty cmd args
	for(i=1; i<cmd.length; i++) {
		if(cmd[i].search(/^\s*$/)==0) {
			cmd.splice(i,1);
			i--;
		}
	}

	if(cmd.length == 1) {
		update_solar();
		srv.o(target,"VHF conditions from "+trim(solar_x.source)+" as of "+trim(solar_x.updated)+":");
		srv.o(target,"Aurora Latitude: "+trim(solar_x.latdegree)+" ("+trim(solar_x.calculatedvhfconditions.phenomenon.(@name=='vhf-aurora'))+")");
		srv.o(target,"E-Skip North America (2m): "+trim(solar_x.calculatedvhfconditions.phenomenon.(@name=='E-Skip').(@location=='north_america')));
		srv.o(target,"E-Skip Europe (2m): "+trim(solar_x.calculatedvhfconditions.phenomenon.(@name=='E-Skip').(@location=='europe')));
		srv.o(target,"E-Skip Europe (4m): "+trim(solar_x.calculatedvhfconditions.phenomenon.(@name=='E-Skip').(@location=='europe_4m')));
		srv.o(target,"E-Skip Europe (6m): "+trim(solar_x.calculatedvhfconditions.phenomenon.(@name=='E-Skip').(@location=='europe_6m')));
	}
	return true;
}

Bot_Commands["HF"] = new Bot_Command(0, false, false);
Bot_Commands["HF"].command = function (target, onick, ouh, srv, lbl, cmd) {
	// Remove empty cmd args
	for(i=1; i<cmd.length; i++) {
		if(cmd[i].search(/^\s*$/)==0) {
			cmd.splice(i,1);
			i--;
		}
	}

	if(cmd.length == 1) {
		update_solar();
		srv.o(target,"HF conditions from "+trim(solar_x.source)+" as of "+trim(solar_x.updated)+":");
		srv.o(target,"Band    | Day    | Night");
		srv.o(target,format("80m-40m | %-7.7s| %s",solar_x.calculatedconditions.band.(@name == '80m-40m').(@time == 'day'),solar_x.calculatedconditions.band.(@name == '80m-40m').(@time == 'night')));
		srv.o(target,format("30m-20m | %-7.7s| %s",solar_x.calculatedconditions.band.(@name == '30m-20m').(@time == 'day'),solar_x.calculatedconditions.band.(@name == '30m-20m').(@time == 'night')));
		srv.o(target,format("17m-15m | %-7.7s| %s",solar_x.calculatedconditions.band.(@name == '17m-15m').(@time == 'day'),solar_x.calculatedconditions.band.(@name == '17m-15m').(@time == 'night')));
		srv.o(target,format("12m-10m | %-7.7s| %s",solar_x.calculatedconditions.band.(@name == '12m-10m').(@time == 'day'),solar_x.calculatedconditions.band.(@name == '12m-10m').(@time == 'night')));
		srv.o(target,"Signal Noise Level: "+trim(solar_x.signalnoise));
	}
	return true;
}

Bot_Commands["CALLSIGN"] = new Bot_Command(0,false,false);
Bot_Commands["CALLSIGN"].command = function (target,onick,ouh,srv,lvl,cmd) {
	var callsign;
	var i;

	// Remove empty cmd args
	for(i=1; i<cmd.length; i++) {
		if(cmd[i].search(/^\s*$/)==0) {
			cmd.splice(i,1);
			i--;
		}
	}

	if(cmd.length==2)
		callsign=cmd[1].toUpperCase();
	else {
		return true;
	}

	try {
		var matched=CallSign.Lookup.Any(callsign);
		if(matched.string != undefined)
			srv.o(target, matched.string);
		else {
			var str=matched.callsign+": ";
			if(matched.name != undefined)
				str += matched.name;
			if(matched.address != undefined)
				str += ", "+matched.address;
			if(matched.city != undefined)
				str += ", "+matched.city;
			if(matched.provstate != undefined)
				str += ", "+matched.provstate;
			if(matched.postalzip != undefined)
				str += " "+matched.postalzip;
			if(matched.qualifications != undefined)
				str += ". "+matched.qualifications;
			if(matched.status != undefined)
				str += " ("+matched.status+")";
			srv.o(target, str);
		}
	}
	catch(e) {
		srv.o(target,e);
	}
	return true;
}

Bot_Commands["CONTESTS"] = new Bot_Command(0,false,false);
Bot_Commands["CONTESTS"].command = function (target, onick, ouh, srv, lvl, cmd) {
	// Remove empty cmd args
	for(i=1; i<cmd.length; i++) {
		if(cmd[i].search(/^\s*$/)==0) {
			cmd.splice(i,1);
			i--;
		}
	}

	update_contests();

	function match_contests(rules)
	{
		var ret=[];
		var c;
		var rules;
		var matches;

		for(contest in contest_order) {
			c=contests[contest_order[contest]];
			matches=0;
			for(rule in rules) {
				var r=rules[rule].toLowerCase();
				if(r == 'hf' || r.search(/^[0-9]+$/)!= -1 || r == 'vhf'
						|| r == 'uhf') {
					for(band in c.Bands) {
						var b=c.Bands[band].toLowerCase();
						var bn=parseFloat(b);

						if(b==r) {
							matches++;
							break;
						}
						else if(b=='all' || b=='any') {
							matches++;
							break;
						}
						else if(r=='hf' && bn >= 10 && bn <= 160) {
							matches++;
							break;
						}
						else if(r=='vhf' && ((bn <=6 && bn >= 1.25) || (bn >= 219 && bn <= 225))) {
							matches++;
							break;
						}
						else if(r=='UHF' && ((bn > 1.25 && bn < 10) || (bn >= 420))) {
							matches++;
							break;
						}
					}
				}
				else if(r == 'ssb' || r == 'cw' || r == 'phone' || r == 'sstv'
						|| r == 'rtty' || r == 'psk' || r == 'digital'
						|| r == 'data' || r == 'feld hell' || r == 'am' || r=='fm') {
					for(mode in c.Mode) {
						var m=c.Mode[mode].toLowerCase();

						if(m==r) {
							matches++;
							break;
						}
						else if(r=='phone' && (m=='ssb' || m=='am' || m=='fm')) {
							matches++;
							break;
						}
						else if((r=='data' || r=='digital') && (m=='cw' || m=='rtty' || m=='psk' || m == 'feld hell')) {
							matches++;
							break;
						}
					}
				}
				else {
					matches++;
				}
			}
			if(matches==rules.length) {
				ret.push(contest_order[contest]);
			}
		}
		return ret;
	}

	var cl=match_contests(cmd.slice(1));
	var displayed=0;
	for(c in cl) {
		var t=contests[cl[c]];
		srv.o(target, cl[c]+': '+t.timeStr+' '+t['Find rules at']);
		displayed++;
		if(displayed > 8) {
			srv.o(target, "--- Aboring after 8 entries");
			break;
		}
	}
	if(displayed) {
		srv.o(target,'Provided by WA7BNM Contest Calendar');
	}
	else {
		srv.o(target,'No contests found');
	}
}

//var dumb={o:function(x,y) {log(y);}};
//Bot_Commands["GEO"].command(undefined, undefined, undefined,dumb,undefined,['GEO']);
//Bot_Commands["HF"].command(undefined, undefined, undefined,dumb,undefined,['GEO']);
//Bot_Commands["VHF"].command(undefined, undefined, undefined,dumb,undefined,['GEO']);
//Bot_Commands["CALLSIGN"].command(undefined, undefined, undefined,dumb,undefined,['asdf','n0l']);
//Bot_Commands["CALLSIGN"].command(undefined, undefined, undefined,dumb,undefined,['asdf','kj6pxy']);
//Bot_Commands["CALLSIGN"].command(undefined, undefined, undefined,dumb,undefined,['asdf','va6rrx']);
//Bot_Commands["CALLSIGN"].command(undefined, undefined, undefined,dumb,undefined,['asdf','g1xkz']);
