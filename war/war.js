var orig_exec_dir = js.exec_dir;
var game_dir = orig_exec_dir;
/*
    Solomoriah's WAR!

    war.js -- main procedure file for war user interface

    Copyright 2013 Stephen Hurd
    All rights reserved.

    3 Clause BSD License

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:

    Redistributions of source code must retain the above copyright
    notice, self list of conditions and the following disclaimer.

    Redistributions in binary form must reproduce the above copyright
    notice, self list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

    Neither the name of the author nor the names of any contributors
    may be used to endorse or promote products derived from self software
    without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
    FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
    AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
    LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


// Based on:
/*
    Solomoriah's WAR!

    war.c -- main procedure file for war user interface

    Copyright 1993, 1994, 2001, 2013 Chris Gonnerman
    All rights reserved.

    3 Clause BSD License

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:

    Redistributions of source code must retain the above copyright
    notice, self list of conditions and the following disclaimer.

    Redistributions in binary form must reproduce the above copyright
    notice, self list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

    Neither the name of the author nor the names of any contributors
    may be used to endorse or promote products derived from self software
    without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
    FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
    AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
    LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
load("sbbsdefs.js");
load(game_dir+'/warcommon.js');

/*
 * Granularity of map centering... doesn't move the map if the new
 * focus is inside a gran x gran square at the centre
 */
const gran = 2;

const terrain = "~.%#^";        /* standard types only. */
const terr_attr = {
	' ':'HB4',
	'~':'HB4',
	'.':'HY3',
	'%':'HG2',
	'#':'HW1',
	'^':'HW7',
	'*':'HW',
	'!':'HR'
};
const genattrs = {
	'help':'N',
	'prompt':'N',
	'border':'NW',
	'header':'N',
	'title':'N'
};

const attrs = {
	'main_screen':'NW',
	'status_area':'N',
	'world_bar':'NK7',
	'reader_pointer':'N',
	'reader_topbottom':'N',
	'reader_deleted':'N',
	'viewer_deleted':'N',
	'viewer_text':'N',
	'army_area':'NW',
	'army_list':'NW',
	'army_total':'NW',
	'city_name':'NW',
	'nation_status':'N',
	'army_types':'N',
	'title_text':'N',
	'title_copyright':'N',
	'title_world':'N',
	'title_newcity':'N',
	'title_retry':'N',
	'title_symbollist':'N',
	'title_symboltitle':'N',
	'title_symbolprompt':'N',
	'title_welcome':'N',
	'title_anykey':'N',
	'title_nameprompt':'N',
	'mailer':'N'
};

const terr_names = [
    "Water",
    "Plains",
    "Forest",
    "Hills",
    "Mountains",
];

const special_desc = [
	"None",
	"Transport Hero",
	"Transport One",
	"Transport Stack"
];

// Player move file.  All commands are written here.
var pfile;

function nationname(n)
{
    return nations[n].name;
}

function getpath(fn)
{
	return backslash(game_dir)+fn;
}

function mainscreen()
{
    var i;

    console.clear(attrs.main_screen);
	console.attributes = genattrs.border;
	console.gotoxy(2, 1);
	console.print(ascii(201));
	console.gotoxy(2, 18);
	console.print(ascii(200));
	console.gotoxy(34, 1);
	console.print(ascii(205)+ascii(187));
	console.gotoxy(34, 18);
	console.print(ascii(205)+ascii(188));
    for(i = 0; i < 16; i++) {
		console.gotoxy(2, i+2);
		console.print(ascii(186));
		console.gotoxy(34, i+2);
		console.print(' '+ascii(186));
		console.gotoxy(i*2+3, 1);
		console.print(ascii(205)+ascii(205));
		console.gotoxy(i*2+3, 18);
		console.print(ascii(205)+ascii(205));
    }
    console.gotoxy(1, 20);
	console.attributes = genattrs.border;
    console.print((new Array(81)).join(ascii(196)));
	console.attributes = attrs.status_area;
	for(i=21; i<=console.screen_rows; i++) {
    	console.gotoxy(1, i);
		console.cleartoeol();
	}
    console.gotoxy(41, 1);
    console.attributes = attrs.world_bar;
    console.print(format("   %-20s    Turn %d   ", world, gen));
}

function saystat(msg)
{
    console.gotoxy(2, 21);
	console.attributes = attrs.status_area;
    console.print(msg);
    console.cleartoeol();
}

function mailer(from, to)
{
	var heading;
	var logf;
	var fp;
	var f;
	var fname;
	var inbuf;

	if(to > 0) {
		heading = format("From %s of %s to %s of %s  (Turn %d)",
			nationname(from), nationcity(from),
			nationname(to), nationcity(to), gen);
	} else {
		heading = format("From %s of %s  (Turn %d)",
			nationname(from), nationcity(from), gen);
	}
	console.clear(attrs.mailer);
	console.gotoxy(1,1);
	f = new File(getpath(format(TMPMAILFL, from)));
	if(!f.open("wb")) {
    	mainscreen();
		saystat("Message Creation Failed (System Error)");
		return;
	}
	f.close();
	if(console.editfile(f.name)) {
		if(file_size(f.name) > 0) {
			if(to > 0)
				fname = getpath(format(MAILFL, to));
			else
				fname = getpath(NEWSFL);
			if(!f.open('rb')) {
				file_remove(f.name);
    			mainscreen();
				saystat("Mail Could Not Be Read...  Cancelled.");
				return;
			}
			fp = new File(fname);
			if(!fp.open('ab')) {
				file_remove(f.name);
    			mainscreen();
				saystat("Mail Could Not Be Sent...  Cancelled.");
				return;
			}
			fp.write(HEADERMARK);
			fp.write(heading);
			fp.write("\n\n");
			while((inbuf = f.readln()) != null)
				fp.writeln(inbuf);
			f.close();
			fp.close();
			logf = new File(getpath(MAILLOG));
			if(logf.open('ab')) {
				logf.write(HEADERMARK);
				logf.write(heading);
				logf.write("\n\n");
			}
		}
	}
	file_remove(f.name);
    mainscreen();
	saystat("Mail Sent.");
}

/*
    showpage() shows a page of text from the current file, starting at
    the file position in pages[pg], for 19 lines.
*/

function showpage(fp, pg, pages)
{
    var i, len;
    var inbuf;

	fp.position = pages[pg];
	len = HEADERMARK.length;
	console.attributes = attrs.viewer_text;
    for(i = 0; i < 19; i++) {
        if((inbuf = fp.readln(1024)) == null)
            break;
		inbuf = inbuf.substr(0, 77);
        if(inbuf.substr(0, 2) == HEADERMARK)
            break;
        console.gotoxy(3, i+1);
        console.print(inbuf);
        console.cleartoeol();
    }
    for(; i < 19; i++) {
        console.gotoxy(3, i+1);
        console.cleartoeol();
    }
    console.gotoxy(56, 22);
	console.attributes = genattrs.help;
    console.print(format("Page %d of %d", pg + 1, pages.length));
    console.cleartoeol();
    console.gotoxy(71, 22);
	console.attribtes = attrs.reader_prompt;
    console.print("< >");
}

function show_killed(killed)
{
    console.gotoxy(41, 22);
    if(killed) {
		console.attributes = attrs.viewer_deleted;
        console.print("[Deleted]");
	}
    else {
		console.attributes = genattrs.help;
        console.print("         ");
	}
}

function viewerscr(mode)
{
    console.clear(genattrs.header);
    console.gotoxy(3, 21);
	console.attributes = genattrs.help;
    if(mode)
        console.print("Mail:  (d)elete  ");
    else
        console.print("News:  ");
    console.print("(z)down  (a)up");
    console.gotoxy(3, 22);
    console.print("Press SPACE to Exit.");
    console.gotoxy(71, 22);
	console.attributes = attrs.viewer_prompt;
    console.print("< >");
}

/*
    delete_msgs() deletes messages from the named file, using the
    information in the index[] array.
*/

function delete_msgs(fn, index)
{
    var i, done, len;
    var tmp, inbuf;
    var inf, outf;

    len = HEADERMARK.length;
	tmp = 'tmp'+fn;
	inf = new File(getpath(fn));
	outf = new File(getpath(tmp));
	if(!inf.open("rb")) {
		log("Message Deletion Failed (System Error)");
		saystat("Message Deletion Failed (System Error)");
		return;
	}
	if(!outf.open("wb")) {
		inf.close();
		log("Message Deletion Failed (System Error)");
		saystat("Message Deletion Failed (System Error)");
		return;
	}
    for(i = 0; i < index.length; i++) {
        if(!index[i].killed) {
			outf.write(HEADERMARK);
			inf.position = index[i].pos;
			done = false;
			while((inbuf = inf.readln(1024)) != null && !done) {
				inbuf = inbuf.substr(0, 77);
				if(inbuf.substr(0, len) == HEADERMARK)
					done = true;
				else
					outf.write(inbuf+'\n');
			}
        }
	}
	inf.close();
	outf.close();
    file_remove(inf.name);
    file_rename(outf.name, inf.name);
}

function readerscr(mode)
{
    console.clear(genattrs.header);
    console.gotoxy(3, 21);
	console.attributes = genattrs.help;
    if(mode)
        console.print("Mail:  (v)iew current  (d)elete  ");
    else
        console.print("News:  (v)iew current  ");
    console.print("(z)down  (a)up  (]) next  ([) previous");
    console.gotoxy(3, 22);
    console.print("Press SPACE to Exit.");
}

/*
    viewer() displays text from the given file, from the given
    index offset to the end of file or until a line beginning with
    a HEADERMARK is encountered.
*/

function viewer(fp, mode, index)
{
    var ch, i, len, done, pg, opg;
    var dummy;
    var pages;

    len = HEADERMARK.length;
	fp.position = index.pos;
    viewerscr(mode);
    show_killed(index.killed);
    done = false;
    pages = [];
    do {
        pages.push(fp.position);
        for(i = 0; i < 18; i++) {
            if((dummy = fp.readln(1024)) == null) {
                done = true;
                break;
            }
			dummy = dummy.substr(0, 77);
            if(dummy.substr(0, len) == HEADERMARK) {
                done = true;
                break;
            }
        }
    } while(!done);
    if(i <= 1)
		pages.pop();
    pg = 0;
    showpage(fp, pg, pages);
    do {
        console.gotoxy(72, 22);
        ch = console.getkey();
        opg = pg;
        switch(ch) {
        case 'z' :
        case KEY_DOWN:
            pg++;
            break;
        case 'a' :
        case KEY_UP:
            pg--;
            break;
        case 'd' :
            index.killed = !index.killed;
            show_killed(index.killed);
            break;
        case ' ' :
        case 'q' :
            ch = '\033';
            break;
        }
        if(pg < 0)
			pg = 0;
        if(pg >= pages.length)
			pg = pages.length - 1;
        if(pg != opg)
            showpage(fp, pg, pages);
    } while(ch != '\033');
    readerscr(mode);
}

/*
    show_screen() shows headers from the given file, centering at the
    header number given.
*/
function show_screen(pos, fp, index)
{
    var i,m;
    var inbuf;

    pos -= 8;
    for(i = 0; i < 18; i++) {
		m = pos + i;
        console.gotoxy(3, i+1);
        if(m < index.length && m >= 0) {
            if(index[m].killed) {
				console.attributes = attrs.reader_deleted;
                console.print('*');
			}
			else {
				console.attributes = genattrs.header;
                console.print(' ');
			}
			fp.position = index[m].pos;
			inbuf = fp.readln(1024);
			if(inbuf != null) {
				inbuf = inbuf.substr(0, 77);
           		console.print(inbuf);
			}
        } else {
			console.attributes = attrs.reader_topbottom;
			if(m == -1)
           		console.print("*** Top of List ***");
        	else if(m == index.length)
            	console.print("*** End of List ***");
		}
        console.cleartoeol();
    }
}

/*
    indexer() reads the given file, scanning for headings.  headings
    are flagged with a space-backspace combination (" \b") which is
    not shown on most printouts but is easily recognized.

    the array index[] contains the seek positions of each heading.
*/

function indexer(fp)
{
	var inbuf, len, pos;
	var index = [];

    len = HEADERMARK.length;
    fp.rewind();
    for(pos = fp.position; (inbuf = fp.readln(1024)) != null; pos = fp.position) {
		inbuf=inbuf.substr(0, 77);
		if(inbuf.substr(0, len)==HEADERMARK) {
			index.push({killed:false, pos:(pos+len)});
		}
	}
	return index;
}

/*
    reader() displays a list of headings from the given file.  the
    user may page or "arrow" up or down to view long listings.  when
    a heading is selected with the (v)iew command, viewer() is called.

    mode is 0 for news, 1 for mail.  mode 1 enables deletion of headings.
*/

function reader(fname, mode)
{
    var top, pos, ch, killcnt, index;
    var fp = new File(getpath(fname));

    if(!fp.open("rb")) {
		switch(mode) {
		case 2:
            saystat("Help file missing.");
            break;
        case 1:
            saystat("No Mail in your box.");
            break;
        default:
            saystat("No News to Read.");
		}
        return;
    }
    readerscr(mode);
    index = indexer(fp);
	pos = index.length - 1;
    top = pos - 8;
    show_screen(top, fp, index);
    do {
        console.gotoxy(2, pos - top + 9);
		console.attributes = attrs.reader_pointer;
        console.print('>');
        ch = console.getkey();
        console.gotoxy(2, pos - top + 9);
		console.attributes = genattrs.header;
        console.print(' ');
        switch(ch) {
        case 'z' :
        case KEY_DOWN:
            pos++;
            break;
        case 'a' :
        case KEY_UP:
            pos--;
            break;
        case ']' :
        //case KEY_PAGEDOWN:
            pos += 11;
            break;
        case '[' :
        //case KEY_PAGEUP:
            pos -= 10;
            break;
        case 'd' :
            if(!mode)
                break;
            index[pos].killed = !index[pos].killed;
            show_screen(top, fp, index, index);
            break;
        case 'v' :
		case '\r' :
            viewer(fp, mode, index[pos]);
            show_screen(top, fp, index);
            break;
        case ' ' :
        case 'q' :
            ch = '\033';
            break;
        }
        if(pos < 0)
			pos = 0;
        if(pos >= index.length)
			pos = index.length - 1;
        if(pos > top + 8 || pos < top - 7) {
            top = pos;
            show_screen(top, fp, index);
        }
    } while(ch != '\033');
    fp.close();
    mainscreen();

    /* handle deletion. */
    if(mode) {
        for(killcnt = 0, pos = 0; pos < index.length; pos++)
            killcnt += index[pos].killed;
        if(killcnt != 0) { /* deleted some... */
            if(killcnt == index.length) /* deleted ALL */
                file_remove(getpath(fname));
            else
                delete_msgs(fname, index);
        }
    }
}

function analyze_stack(ms)
{
    var i, j, a, b, mmode;

    mmode = 0;
    for(i = 0; i < ms.length; i++) {
        if(armies[ms[i].id].special_mv > mmode)
            mmode = armies[ms[i].id].special_mv;
	}

    switch(mmode) {
    case TRANS_ALL :
        /* locate fastest transporter */
        j = -1;
        for(i = 0; i < ms.length; i++) {
            a = ms[i].id;
            if(armies[a].special_mv == TRANS_ALL) {
                if(j == -1 || armies[a].move_left > armies[j].move_left)
                    j = a;
                ms[i].dep = a;
            }
        }

        /* set (almost) all */
        for(i = 0; i < ms.length; i++) {
            a = ms[i].id;
            if(armies[a].special_mv == TRANS_ALL)
                ms[i].dep = a;
            else if(ms[i].dep == -1)
                ms[i].dep = j;
        }
        break;
    case TRANS_SELF :
        /* easy... all move by themselves */
        for(i = 0; i < ms.length; i++)
            ms[i].dep = ms[i].id;
        break;
    default :
        /* transport hero and/or one army */
        /* set all heros for transportation */
        for(i = 0; i < ms.length; i++) {
            a = ms[i].id;
            if(armies[a].hero > 0) {
                for(j = 0; j < ms.length; j++) {
                    b = ms[j].id;
                    if(armies[b].special_mv == TRANS_HERO
                    && ms[j].dep == -1) {
                        ms[j].dep = b;
                        ms[i].dep = b;
                        break;
                    }
                }
            }
        }

        /* set remaining for transportation */
        for(i = 0; i < ms.length; i++) {
            a = ms[i].id;
            if(ms[i].dep == -1
            && armies[a].special_mv == TRANS_SELF) {
                for(j = 0; j < ms.length; j++) {
                    b = ms[j].id;
                    if(armies[b].special_mv == TRANS_ONE
                    && ms[j].dep == -1) {
                        ms[j].dep = b;
                        ms[i].dep = b;
                        break;
                    }
                }
            }
        }

        /* leftovers transport self. */
        for(i = 0; i < ms.length; i++)
            if(ms[i].dep == -1)
                ms[i].dep = ms[i].id;
        break;
    }
}

function movecost(a, r, c)
{
    var t, tbl, mv, city, cost, a2;

    city = city_at(r, c);
    if(city == -1) {
        /* not a city */
        /* search for a TRANS_ALL unit there */
        if(!(armies[a].r == r && armies[a].c == c)
				&& armies[a].special_mv != TRANS_ALL) {
            for(a2 = 0; a2 < armies.length; a2++) {
                if(a != a2
						&& armies[a2].r == r && armies[a2].c == c
						&& armies[a2].nation == armies[a].nation
						&& armies[a2].special_mv == TRANS_ALL) {
                    return 1;
				}
			}
		}

        /* else do this: */
		t = terrain.indexOf(map[r][c]);
        if(t != -1) {
            tbl = armies[a].move_tbl;
            mv = move_table[tbl].cost[t];
            if(mv == 0
					/* otherwise can't move, and */
					&& map[r][c] != '~' 
					/* target space is not water, and */
					&& map[armies[a].r][armies[a].c] == '~'
					/* current space is water, and */
					&& (cost = move_table[tbl].cost[0]) > 0)
					/* this unit moves on water, then */
				mv = cost * 2; /* beaching is allowed */
        }
        else {
            if(map[r][c] == '+')
                mv = 1;
            else
                mv = 3;
		}
    }
    else {
        /* city */
        if(armies[a].nation == cities[city].nation)
            mv = 1;
        else
            mv = 2;
    }
    return mv;
}

function clearstat(mode)
{
	console.attributes = attrs.status_area;
    if(mode == -1) {
        console.gotoxy(2, 21); console.cleartoeol();
        console.gotoxy(2, 22); console.cleartoeol();
        console.gotoxy(2, 23); console.cleartoeol();
        console.gotoxy(2, 24); console.cleartoeol();
    } else {
        console.gotoxy(2, 21+mode); console.cleartoeol();
    }
}

function unit_mark(nation_mark)
{
	if(marks[0].indexOf(nation_mark) == -1)
		return '?';
	return marks[1].substr(marks[0].indexOf(nation_mark), 1);
}

/*
 * This returns a list of armies AND updates the overlay
 */
function get_armylist(ntn, r, c)
{
	var e;
	var alist = {};
	var i,k;

	for(i = 0; i<map_height; i++) {
		for(k = 0; k < map_width; k++) {
			mapovl[i][k] = ' ';
		}
	}
	alist.view=[];
	alist.enemies=[];
    for(i = 0; i < armies.length; i++) {
        if(r == armies[i].r && c == armies[i].c) {
            if(armies[i].nation == ntn || ntn == -1) {
				alist.view.push({id:i, marked: false});
            }
            else {
				if(alist.enemies.length == 0) {
					alist.enemies.push({
						nation:armies[i].nation, 
						hero:0, 
						armies:0, 
						multiple:false
					});
					e = alist.enemies[0];
				}
				else if(alist.enemies[0].nation == armies[i].nation) {
					e = alist.enemies[0];
				}
				else {
					if(alist.enemies.length == 1) {
						alist.enemies.push({
							nation:armies[i].nation, 
							hero:0, 
							armies:0, 
							multiple:false
						});
					}
					e = alist.enemies[alist.enemies.length-1];
				}
				if(e.nation != armies[i].nation)
					e.multiple = true;
                if(armies[i].hero > 0)
                    e.heros++;
                else
                    e.armies++;
            }
        }
        
        /* Mark all armies on the overlay */
        if(mapovl[armies[i].r][armies[i].c] == ' ' 
				|| mapovl[armies[i].r][armies[i].c] == unit_mark(nations[armies[i].nation].mark)) {
			mapovl[armies[i].r][armies[i].c] = unit_mark(nations[armies[i].nation].mark);
		}
		else if(mapovl[armies[i].r][armies[i].c] != nations[armies[i].nation].mark) {
			mapovl[armies[i].r][armies[i].c] = '!';
		}
    }

	/* Update city marks on the overlay */
    for(i = 0; i < cities.length; i++) {
        if(mapovl[cities[i].r][cities[i].c] == ' ' 
				|| mapovl[cities[i].r][cities[i].c] == unit_mark(nations[cities[i].nation].mark)) {
            mapovl[cities[i].r][cities[i].c] = nations[cities[i].nation].mark;
        }
        else {
            mapovl[cities[i].r][cities[i].c] = '!';
		}
    }

	alist.view = alist.view.sort(function(a,b) { return isgreater(b.id, a.id) ? 1 : 0; });
	return alist;
}

function showarmies(full_list, pointer)
{
    var i, a;
    var buff;

	console.attributes = attrs.army_area;
    for(i = 0; i < full_list.view.length && i < 12; i++) {
        console.gotoxy(38, i + 3);
        if(i < full_list.view.length) {
            a = full_list.view[i].id;
			console.print((i==pointer)?'=> ':'   ');
			console.print((full_list.view[i].mark)?'* ':'  ');
            buff = armyname(a);
            if(armies[a].name.length == 0 && armies[a].hero > 0)
                buff = "(Nameless Hero)";
            else if(armies[a].hero > 0)
                buff += " (Hero)";
            console.print(format("%-31.31s %d:%d", buff,
                armies[a].move_rate, armies[a].move_left));
        }
        console.cleartoeol();
    }
    for(;i < 12;i++) {
        console.gotoxy(38, i + 3);
        console.cleartoeol();
	}
    console.gotoxy(38, 15);
	console.attributes = attrs.army_total;
    if(full_list.view.length > 0)
        console.print(format("     Total %d Armies", full_list.view.length));
    console.cleartoeol();
	console.attributes = attrs.army_area;
    for(i = 0; i < full_list.enemies.length; i++) {
        console.gotoxy(43, i + 17);
        if(full_list.enemies[i].nation != -1) {
            console.print(format("%-15.15s %3d Armies, %3d Heros",
                (i == 1 && full_list.enemies[i].multiple)
                    ? "(Other Nations)"
                    : nationcity(full_list.enemies[i].nation),
                full_list.enemies[i].armies, full_list.enemies[i].heros));
		}
        console.cleartoeol();
    }
    for(;i<2; i++) {
        console.gotoxy(43, i + 17);
        console.cleartoeol();
	}
}

function gmapspot(r, c, terr, mark, extra_attr)
{
	var attr;
    if(mark == ' ')
        mark = terr;
    if(terr == '~')
        terr = ' ';
    if(mark == '~')
        mark = ' ';
    console.gotoxy(c * 2 + 3, r + 2);
	if(terr_attr[mark] == undefined)
		attr = 'N';
	else {
   		attr = terr_attr[terr];
		if(terr_attr[mark] != undefined)
    		attr += terr_attr[mark];
	}
	console.attributes = attr+extra_attr;
    console.print(mark);
    console.attributes = terr_attr[terr]+extra_attr;
    console.print(terr);
    console.gotoxy(c * 2 + 3, r + 2);
    return 0;
}

function cityowner(c)
{
    var n;

    n = cities[c].nation;
    if(n == 0)
		return "Neutral";
    return cities[nations[n].city].name;
}

function showcity(r, c)
{
    var i;
    var buff = '';

    console.gotoxy(4, 19);
    i = city_at(r, c);
    if(i != -1) {
        buff = format("City:  %s (%s)",
            cities[i].name, cityowner(i));
    }
	console.attributes = attrs.city_name;
    console.print(format("%-40.40s", buff));
}

/*
 * Adjusts the passed row for wrapping.
 */
function fixrow(r)
{
	return (r + map_height) % map_width;
}

/*
 * Adjusts the passed column for wrapping.
 */
function fixcol(c)
{
	return (c + map_width) % map_height;
}

var old_ul_r = -1;
var old_ul_c = -1;
function showmap(ntn, r, c, force, pointer)
{
    var ul_r, ul_c, f_r, f_c;
    var i, j, zr, zc;
    var rem;
	var alist;

	full_list = get_armylist(ntn, r, c);
    showarmies(full_list, pointer);
    rem = parseInt((16 - gran) / 2);
    f_r = r % gran;
    f_c = c % gran;
    ul_r = fixrow(parseInt(r / gran) * gran - rem);
    ul_c = fixcol(parseInt(c / gran) * gran - rem);
    if(old_ul_r != ul_r || old_ul_c != ul_c || force) {
        for(i = 0; i < 16; i++) {
            for(j = 0; j < 16; j++) {
                zr = fixrow(ul_r+i);
                zc = fixcol(ul_c+j);
                gmapspot(i, j, map[zr][zc], mapovl[zr][zc], '');
            }
		}
	}
	console.attributes='N';
    old_ul_r = ul_r;
    old_ul_c = ul_c;
    if(mapovl[r][c] != ' ')
        showcity(r, c);
    console.gotoxy(f_c * 2 + 16, f_r + 8);
    return full_list.view;
}

/*
 * Redraws the current focus point of the map
 */
function showfocus(r, c)
{
    var f_r, f_c, rem;

    rem = parseInt((16 - gran) / 2);
    f_r = (r % gran) + rem;
    f_c = (c % gran) + rem;
    gmapspot(f_r, f_c, map[r][c], mapovl[r][c], '');
    console.attributes='N';
}

function move_mode(alist, avpnt, ntn, rp, cp)
{
    var i, j, mv, ch, city, army, t_r, t_c, a, b, ok, cnt, max, flag;
    var ac, hc, mmode;
    var mvc = new Array(TRANS_ALL+1);
    var gap, temp;
	var movestack = [];
    var unmarked = [];
    var buff;

	if(turn_done) {
		saystat("Turn is currently done, cannot issue orders");
        return {r:rp,c:cp};
	}
    if(alist.length < 1) {
        saystat("No Army to Move.");
        return {r:rp,c:cp};
    }
    flag = 0;
    for(i = 0; i < alist.length; i++) {
        if(alist[i].mark)
            flag = 1;
        if(alist[i].mark
				&& (armies[alist[i].id].move_left > 0 || ntn == -1)) {
            if(movestack.length >= 10 && ntn >= 0) {
                saystat("Too Many Armies for Group.");
        		return {r:rp,c:cp};
            }
            movestack.push({
				id:alist[i].id,
				moved:0,
				dep:-1
			});
        }
    }
    if(movestack.length < 1 && !flag
			&& (armies[alist[avpnt].id].move_left > 0 || ntn == -1)) {
		movestack.push({
			id:alist[avpnt].id,
			moved:0,
			dep:-1
		});
        alist[avpnt].mark = 1;
    }
    if(movestack.length < 1 && ntn > -1) {
        saystat("Armies Have No Movement Left.");
        return {r:rp,c:cp};
    }

    /* 
        prevent stranding:  
            analyze the unmarked armies
    
        create a "movestack" of the unmarked.
        if they can move, stranding can't occur.
    */
    if(ntn > -1) {
        for(i = 0; i < alist.length; i++) {
            if(!alist[i].mark) {
				unmarked.push({
					moved: 0,
					dep: -1,
					id: alist[i].id
				});
            }
        }
        analyze_stack(unmarked);
        for(i = 0; i < unmarked.length; i++) {
            a = unmarked[i].id;
            if(unmarked[i].dep == a
					&& armies[a].special_mv != TRANS_ALL
					&& movecost(a, armies[a].r, armies[a].c) == 0) {
                saystat("Armies Would Be Stranded...  Movement Cancelled.");
                return {r:rp,c:cp};
            }
        }
    }

    /* analyze move stack */
    if(ntn > -1)
        analyze_stack(movestack);

    /* perform movement */
    clearstat(-1);
    console.gotoxy(2, 22);
	console.attributes = attrs.status_area;
    console.print(format("Move %s", movestack.length > 1 ? "Armies" : "Army"));
    console.gotoxy(21, 23);
    console.print("4   6  or  d   g");
    console.gotoxy(21, 24);
    console.print("1 2 3      c v b");
    console.gotoxy(21, 22);
    console.print("7 8 9      e r t    SPACE to Stop.  ");
    alist = showmap(ntn, rp, cp, false, avpnt);
    while(movestack.length > 0 && (ch = console.getkey()) != ' '
			&& ch != 'q' && ch != '\x1b') {
        showfocus(rp, cp);
        clearstat(0);
        t_r = rp;
        t_c = cp;
        switch(ch) { /* directions */
        case '7' :
        case 'e' :
            t_r--;
            t_c--;
            break;
        case '8' :
        case 'r' :
            t_r--;
            break;
        case '9' :
        case 't' :
            t_r--;
            t_c++;
            break;
        case '4' :
        case 'd' :
            t_c--;
            break;
        case '6' :
        case 'g' :
            t_c++;
            break;
        case '1' :
        case 'c' :
            t_r++;
            t_c--;
            break;
        case '2' :
        case 'v' :
            t_r++;
            break;
        case '3' :
        case 'b' :
            t_r++;
            t_c++;
            break;
        case '\f' :
			// TODO: Refresh
            break;
        }
        t_r = fixrow(t_r);
        t_c = fixcol(t_c);
        if(t_r != rp || t_c != cp) {
            /* actual move code... */
            ok = 1;

            /* verify that all units can make the move. */
            if(ntn > -1) {
                for(i = 0; i < movestack.length; i++) {
                    a = movestack[i].id;
                    if(movestack[i].dep == a) {
                        mv = movecost(a, t_r, t_c);
                        if(mv > armies[a].move_left
                        || mv == 0) {
                            ok = 0;
                            buff = format("%s Can't Move There.",
                                armyname(a));
                            saystat(buff);
                            break;
                        }
                    }
                }
            }

            /* prevent overstacking. */
            if(ok && ntn > -1) {
                cnt = 0;
                for(i = 0; i < armies.length; i++)
                if(armies[i].nation == ntn
						&& armies[i].r == t_r && armies[i].c == t_c)
                    cnt++;
                city = city_at(t_r, t_c);
                max = 10;
                if(city != -1
                && cities[city].nation == ntn)
                    max = 12;
                if(movestack.length + cnt > max) {
                    ok = 0;
                    saystat("Too Many Armies There.");
                }
            }

            /* can't leave combat. */
            if(ok && ntn > -1) {
                for(i = 0; i < armies.length; i++) {
                    if(armies[i].nation != ntn
							&& armies[i].r == rp && armies[i].c == cp) {
                        ok = 0;
						saystat("Can't Leave Combat.");
                        break;
                    }
                }
            }

            /* do it! */
            if(ok) {
                for(i = 0; i < movestack.length; i++) {
                    a = movestack[i].id;
                    if(ntn > -1) {
                        if(movestack[i].dep == a)
                            mv = movecost(a, t_r, t_c);
                        else
                            mv = armies[a].move_left ? 1 : 0;
                    } else
                        mv = 0;
                    movestack[i].moved += mv;
                    armies[a].move_left -= mv;
                    armies[a].r = t_r;
                    armies[a].c = t_c;
                }
                rp = t_r;
                cp = t_c;
            }

            /* redo screen. */
            alist = showmap(ntn, rp, cp, ok, avpnt);
        }
        showfocus(rp, cp);
    }
    if(ntn > -1 && ch != 'q' && ch != '\x1b') {
        for(i = 0; i < movestack.length; i++) {
            if(movestack[i].moved > 0 || ntn == -1) {
                pfile.write(format("move-army %d %d %d %d %d\n",
                    movestack[i].id, movestack[i].moved,
                    rp, cp, movestack[i].dep));
            }
		}
	}
    clearstat(-1);
    return {r:rp,c:cp};
}

/*
 * Returns the first army belonging to n at r/c
 */
function my_army_at(r, c, n)
{
    var i;

    for(i = 0; i < armies.length; i++) {
        if(armies[i].nation == n
				&& armies[i].r == r && armies[i].c == c)
            return i;
	}
    return -1;
}

/*
 * Shows the information about the specified tile in the status area
 */
function show_info(r, c)
{
    var buff;
    var i, city, ac, hc;

    buff = '';
    city = city_at(r, c);
    if(city != -1) {
        buff = format("City:  %s (%s)", cities[city].name,
            nationcity(cities[city].nation));
    }
    else {
		i=terrain.indexOf(map[r][c]);
		if(i==-1 && map[r][c]==' ')
			buff = "Ocean";
		else
			buff = terr_names[i];
    }
    ac = 0;
    hc = 0;
    for(i = 0; i < armies.length; i++) {
        if(armies[i].r == r && armies[i].c == c) {
            if(armies[i].hero > 0)
                hc++;
            else
                ac++;
		}
    }
    buff += format("  %d Armies, %d Heros", ac, hc);
    saystat(buff);
}

function info_mode(rp, cp, n, ch, avpnt)
{
    var done, r, c, ul_r, ul_c, f_r, f_c, t_r, t_c, a_r, a_c;
    var city, army, i, focus;
    var rem;

    rem = parseInt((16 - gran) / 2);
    showfocus(rp, cp);
    clearstat(-1);
    console.gotoxy(21,23);
	console.attributes = attrs.status_area;	
    console.print("4   6  or  d   g");
    console.gotoxy(21,24);
    console.print("1 2 3      c v b");
    console.gotoxy(2,22);
    console.print("Info Mode:         7 8 9      e r t      ESC to Stop.  ");
    r = a_r = rp;
    c = a_c = cp;
    ul_r = fixrow(parseInt(r / gran) * gran - rem);
    ul_c = fixcol(parseInt(c / gran) * gran - rem);
    f_r = (r % gran) + rem;
    f_c = (c % gran) + rem;
    t_r = fixrow(ul_r + f_r);
    t_c = fixcol(ul_c + f_c);
    done = false;
    do {
    	gmapspot(f_r, f_c, map[t_r][t_c], mapovl[t_r][t_c], '');
		console.attributes='N';
        switch(ch) {
		case '' :
			break;
        case '7' :
        case 'e' :
            f_r--;
            f_c--;
            break;
        case '8' :
        case 'r' :
            f_r--;
            break;
        case '9' :
        case 't' :
            f_r--;
            f_c++;
            break;
        case '4' :
        case 'd' :
            f_c--;
            break;
        case '6' :
        case 'g' :
            f_c++;
            break;
        case '3' :
        case 'b' :
            f_r++;
            f_c++;
            break;
        case '2' :
        case 'v' :
            f_r++;
            break;
        case '1' :
        case 'c' :
            f_r++;
            f_c--;
            break;
		case 'i' :
			break;
        case '\f' :
	    // TODO: refresh
            break;
        default :
            done = true;
        }
        if(f_r < gran-1) {
			f_r += gran;
			ul_r -= gran;
			rp = fixrow(rp - gran);
			focus = true;
		}
        if(f_c < gran-1) {
			f_c += gran;
			ul_c -= gran;
			cp = fixcol(cp - gran);
			focus = true;
		}
        if(f_r > 15 - gran + 1) {
			f_r -= gran;
			ul_r += gran;
			rp = fixrow(rp + gran);
			focus = true;
		}
        if(f_c > 15 - gran + 1) {
			f_c -= gran;
			ul_c += gran;
			cp = fixcol(cp + gran);
			focus = true;
		}
        t_r = fixrow(ul_r + f_r);
        t_c = fixcol(ul_c + f_c);
        city = my_city_at(t_r, t_c, n);
        army = my_army_at(t_r, t_c, n);
        if(focus || army >= 0 || city >= 0) {
			if(army >= 0|| city >= 0) {
				a_r = t_r;
				a_c = t_c;
			}
			showmap(n, rp, cp, false, avpnt);
			focus = false;
		}
		show_info(t_r, t_c);
    	gmapspot(f_r, f_c, map[t_r][t_c], mapovl[t_r][t_c], 'I');
        console.gotoxy(f_c * 2 + 3, f_r + 2);
        if(!done) {
			do {
				if(turn_done) {
					if(!file_exists(pfile.name)) {
						ch = '';
						done = true;
					}
				}
				ch = console.inkey(5000);
			} while(ch=='' && !done);
		}
    } while(!done);
   	gmapspot(f_r, f_c, map[t_r][t_c], mapovl[t_r][t_c], '');
	console.attributes='N';
    return {r:a_r,c:a_c,ch:ch};
}

function groupcmp(r1, c1, r2, c2)
{
    /* quadrant check */
    if(parseInt(r1 / 16) > parseInt(r2 / 16))
        return 1;
    if(parseInt(r1 / 16) < parseInt(r2 / 16))
        return -1;
    if(parseInt(c1 / 16) > parseInt(c2 / 16))
        return 1;
    if(parseInt(c1 / 16) < parseInt(c2 / 16))
        return -1;

    /* exact check */
    if(r1 > r2)
        return 1;
    if(r1 < r2)
        return -1;
    if(c1 > c2)
        return 1;
    if(c1 < c2)
        return -1;
    return 0;
}

function skipto(next, group, ntn, rp, cp)
{
    var i, t_r, t_c;
	var cmp;

	if(next)
		cmp = function(x) { return x > 0; };
	else
		cmp = function(x) { return x < 0; };

    t_r = -1;
    t_c = -1;
    if(group) {
		for(i = 0; i < cities.length; i++) {
			if(cities[i].nation == ntn) {
				if(cmp(groupcmp(cities[i].r, cities[i].c, rp, cp))
						&& (t_r == -1 || !cmp(groupcmp(cities[i].r, cities[i].c, t_r, t_c)))) {
					t_r = cities[i].r;
					t_c = cities[i].c;
				}
			}
		}
	}
    for(i = 0; i < armies.length; i++) {
		if(!group && armies[i].move_left <= 0)
			continue;
        if(armies[i].nation == ntn) {
            if(cmp(groupcmp(armies[i].r, armies[i].c, rp, cp))
					&& (t_r == -1 || !cmp(groupcmp(armies[i].r, armies[i].c, t_r, t_c)))) {
                t_r = armies[i].r;
                t_c = armies[i].c;
			}
		}
	}
    if(t_r != -1)
		return {r:t_r,c:t_c,ret:true};
    return {r:rp,c:cp,ret:false};
}

function prevgroup(ntn, rp, cp)
{
	return skipto(false, true, ntn, rp, cp);
}

function prevarmy(ntn, rp, cp)
{
	return skipto(false, false, ntn, rp, cp);
}

function nextgroup(ntn, rp, cp)
{
	return skipto(true, true, ntn, rp, cp);
}

function nextarmy(ntn, rp, cp)
{
	return skipto(true, false, ntn, rp, cp);
}

var help_mode = 0;
function help()
{
    console.gotoxy(2, 21);
	console.attributes = attrs.status_area;
    console.print(format("Commands, Page %d  (Press ? for More Help)", help_mode + 1));
    console.cleartoeol();
    switch(help_mode) {
    case 0 :
        console.gotoxy(2, 22);
        console.print("  (])next  ([)previous  (q)uit game  (u)pdates  (s)tatus display (x)end turn");
        console.cleartoeol();
        console.gotoxy(2, 23);
        console.print("  (})last  ({)first  (A)rmy capabilities   army (p)roduction  (h)elp");
        console.cleartoeol();
        console.gotoxy(2, 24);
        console.print("  (CTRL-]) Next movable (CTRL-[) Previous movable  (?) toggle quick help");
        console.cleartoeol();
        break;
    case 1 :
        console.gotoxy(2, 22);
        console.print("  (z/Down)pointer down  (a/Up)pointer up  (SPACE)mark/unmark  (n)ame hero");
        console.cleartoeol();
        console.gotoxy(2, 23);
        console.print("  (*)mark all  (/)unmark all  (m)ove marked armies  (f/5)mark all and move");
        console.cleartoeol();
        console.gotoxy(2, 24);
        console.print("  (I)nformation about current army  Read (N)ews  (S)end message  read (M)ail");
        console.cleartoeol();
        break;
    }
    help_mode++;
    help_mode %= 2;
}

function status()
{
    var ch, pos, i, j, hc, ac, cc;

    /* set up screen */
    console.clear(attrs.nation_status);
    console.gotoxy(21, 2);
	console.attributes = genattrs.title;
    console.print("Nation Status Display");
	console.attributes = genattrs.header;
    console.gotoxy(2, 4);    console.print("Nation:");
    console.gotoxy(2, 5);    console.print("Ruler:");
    console.gotoxy(2, 6);    console.print("Mark:");
    console.gotoxy(2, 8);    console.print("Cities:");
    console.gotoxy(2, 9);    console.print("Heros:");
    console.gotoxy(2, 10);   console.print("Armies:");
	console.attributes = genattrs.help;
    console.gotoxy(2, 12);   console.print("<]> next page  <[> previous page  (SPACE) to exit");

    /* display loop */
    pos = 0;
    do {
        /* show the nations. */
        for(i = 4; i <= 11; i++) {
            console.gotoxy(17, i);
            console.cleartoeol();
        }
		console.attributes = attrs.nation_status;
        for(i = 0; i < 4; i++) {
            if(pos + 1 + i < nations.length) {
                console.gotoxy(i * 16 + 17, 4);
                console.print(nationcity(pos+1+i));
                console.gotoxy(i * 16 + 17, 5);
                console.print(nations[pos+1+i].name);
                console.gotoxy(i * 16 + 20, 6);
                console.print(nations[pos+1+i].mark);
                cc = 0;
                for(j = 0; j < cities.length; j++) {
                    if(cities[j].nation == pos + 1 + i)
                        cc++;
				}
                hc = 0;
                ac = 0;
                for(j = 0; j < armies.length; j++) {
                    if(armies[j].nation == pos + 1 + i) {
                        if(armies[j].hero > 0)
                            hc++;
                        else
                            ac++;
                    }
                }
                console.gotoxy(i * 16 + 17, 8);
                console.print(format("%5d", cc));
                console.gotoxy(i * 16 + 17, 9);
                console.print(format("%5d", hc));
                console.gotoxy(i * 16 + 17, 10);
                console.print(format("%5d", ac));
            }
        }

        /* handle input. */
        console.gotoxy(71, 12);
		console.attributes = attrs.nstatus_prompt;
        console.print("< >\b\b");
        switch((ch = console.getkey())) {
        case ' ' :
        case 'q' :
            ch = '\033';
            break;
        case ']' :
            if(pos + 5 < nations.length)
                pos += 4;
            break;
        case '[' :
            if(pos - 3 > 0)
                pos -= 4;
            break;
        }
    } while(ch != '\033');

    /* clean up */
    mainscreen();
}

function armytypes()
{
    var ch, pos, i, j, hc, ac, cc;

    /* set up screen */
    console.clear(attrs.army_types);
    console.gotoxy(21, 2);
	console.attributes = genattrs.title;
    console.print("Army Type Display");
	console.attributes = genattrs.header;
    console.gotoxy(2, 4);    console.print("Name:");
    console.gotoxy(2, 5);    console.print("Combat:");
    console.gotoxy(2, 6);    console.print("Move Rate:");
    for(i = 0; terr_names[i] != undefined; i++) {
		console.gotoxy(2, 7+i);   console.print(terr_names[i]+":");
	}
    console.gotoxy(2, 7+i);   console.print("Special:");
	console.attributes = genattrs.help;
    console.gotoxy(2, 9+i);   console.print("<]> next page  <[> previous page  (SPACE) to exit");

    /* display loop */
    pos = 0;
    do {
        /* show the types. */
		console.attributes = attrs.army_types;
        for(i = 4; i <= 8+terr_names.length; i++) {
            console.gotoxy(17, i);
            console.cleartoeol();
        }
        for(i = 0; i < 4; i++) {
            if(pos + i < ttypes.length) {
                console.gotoxy(i * 16 + 17, 4);
                console.print(ttypes[pos+i].name);
                console.gotoxy(i * 16 + 17, 5);
                console.print(format("%5d",ttypes[pos+i].combat));
                console.gotoxy(i * 16 + 17, 6);
                console.print(format("%5d",ttypes[pos+i].move_rate));
                cc = 0;
                for(j = 0; j < terr_names.length; j++) {
					console.gotoxy(i * 16 + 17, 7+j);
					if(move_table[ttypes[pos+i].move_tbl].cost[j] == 0)
						console.print("Impassable");
					else
						console.print(format("%5d", move_table[ttypes[pos+i].move_tbl].cost[j]));
				}
                console.gotoxy(i * 16 + 17, 7+j);
                console.print(special_desc[ttypes[pos+i].special_mv]);
            }
        }

        /* handle input. */
        console.gotoxy(71, 9+j);
		console.attributes = attrs.atype_prompt;
        console.print("< >\b\b");
        switch((ch = console.getkey())) {
        case ' ' :
        case 'q' :
            ch = '\033';
            break;
        case ']' :
            if(pos + 5 < ttypes.length)
                pos += 4;
            break;
        case '[' :
            if(pos - 3 > 0)
                pos -= 4;
            break;
        }
    } while(ch != '\033');

    /* clean up */
    mainscreen();
}

/*
 * Select the unit a city iwll produce
 */
function produce(city)
{
    var i, t, ch;
    var buff='';
    var okstring='';

	if(turn_done) {
		saystat("Turn is currently done, cannot issue orders");
		return;
	}
	console.attributes = attrs.status_area;
    console.gotoxy(2, 22);  console.cleartoeol();
    console.gotoxy(2, 23);  console.cleartoeol();
    for(i = 0; i < cities[city].ntypes; i++) {
        if(cities[city].types[i] != -1) {
            console.gotoxy(parseInt(i / 2) * 30 + 2, (i % 2) + 22);
            console.print(format("%d) %-14.14s (%d)", i + 1,
                ttypes[cities[city].types[i]].name,
                cities[city].times[i]));
            okstring += (i+1);
        }
    }
	console.gotoxy(2, 24);
    t = cities[city].types[cities[city].prod_type];
	console.print(format("Current:  %s (%d turns left)    ESC to Cancel",
        ttypes[t].name,
        cities[city].turns_left));
    console.cleartoeol();
    saystat(format("Set Army Production for %s:  ", cities[city].name));
    if(okstring.indexOf(ch = console.getkeys(okstring+'\x1b', 0)) != -1) {
        buff = format("set-produce %d %d\n", city, ch - 1);
        pfile.write(buff);
        execpriv(buff);
    }
    clearstat(-1);
}

var upd_pos=0;
var upd_top=0;
function update(ntn, or, oc)
{
	var fp = new File(getpath(format(TURNFL, ntn)));
	var lines;
	var lpos = -1;
	var ltop = -1;
	var i;
	var r=or,c=oc;
	var ch;
	var m;
	var alist;

	alist = showmap(ntn, r, c, true, -1);
	console.attributes = attrs.status_area;
	if(fp.open('rb')) {
		lines=fp.readAll();
		fp.close();
		clearstat(-1);
		console.gotoxy(45, 20);
		console.attributes = genattrs.border;
		console.print(ascii(180)+"(u) to move to the event location"+ascii(195));
		for(;;) {
			if(ltop != upd_top) {
				for(i=0; i<4; i++) {
					console.gotoxy(2, 21+i);
					if(upd_top + i < lines.length) {
						if((m = lines[upd_top+i].match(/^([0-9]+) ([0-9]+) (.*)$/))!=null) {
							if(upd_top+i == upd_pos) {
								console.print('=> ');
								r=m[1];
								c=m[2];
							}
							else
								console.print('   ');
							console.print(m[3].substr(0,75));
						}
					}
					console.cleartoeol();
				}
				showfocus(or, oc);
			}
			else if(lpos != upd_pos) {
				console.gotoxy(2, 21+(lpos-upd_top));
				console.print('  ');
				console.gotoxy(2, 21+(upd_pos-upd_top));
				console.print('=>');
				if((m = lines[upd_pos].match(/^([0-9]+) ([0-9]+) (.*)$/))!=null) {
					r=m[1];
					c=m[2];
				}
				showfocus(or, oc);
			}
			lpos = upd_pos;
			ltop = upd_top;
			switch((ch=console.inkey(5000))) {
			case '':
				if(turn_done) {
					if(!file_exists(pfile.name)) {
						console.gotoxy(45, 20);
						console.attributes = genattrs.border;
						console.print((new Array(36)).join(ascii(196)));
						return {r:or,c:oc,ch:''};
					}
				}
				break;
			case 'a':
			case KEY_UP:
				upd_pos--;
				if(upd_pos < 0)
					upd_pos = 0;
				if(upd_pos < upd_top)
					upd_top = upd_pos;
				break;
			case 'z':
			case KEY_DOWN:
				upd_pos++;
				if(upd_pos >= lines.length)
					upd_pos = lines.length-1;
				if(upd_top+3 < upd_pos)
					upd_top = upd_pos-3;
				break;
			case 'u':
				or = r;
				oc = c;
				alist = showmap(ntn, r, c, false, -1);
				showfocus(r, c);
				break;
			default:
				console.gotoxy(45, 20);
				console.attributes = genattrs.border;
				console.print((new Array(36)).join(ascii(196)));
				return {r:or,c:oc,ch:ch};
			}
		}
	}
	else
		saystat("No updates this turn!");
	return {r:or,c:oc,ch:''};
}

function mainloop(ntn)
{
    var ch, r, c, i, n, city, army, force, obj;
    var inbuf, buff;
	var keep_ch = false;
	var orig_pt = console.ctrlkey_passthru;
	var alist;
	var avpnt = 0;

    r = -1;
    c = -1;
    army = -1;

    /* find the player's capitol city */
    if(cities[nations[ntn].city].nation == ntn) {
        i = nations[ntn].city;
        r = cities[i].r;
        c = cities[i].c;
    }

    /* find the player's first city */
    city = city_at(r, c);
    if(city != -1 && cities[city].nation != ntn)
        city = -1;
    if(city == -1) {
        for(i = 0; i < cities.length; i++) {
            if(cities[i].nation == ntn) {
                r = cities[i].r;
                c = cities[i].c;
                break;
            }
		}
	}

    /* find the player's first army */
    if(city == -1) {
        for(i = 0; i < armies.length; i++) {
            if(armies[i].nation == ntn) {
                r = armies[i].r;
                c = armies[i].c;
                army = i;
                break;
            }
		}
	}
    if(army == -1 && city == -1) {
        saystat("Nation is Destroyed.  Press Any Key:  ");
        console.getkey();
		console.ctrlkey_passthru = orig_pt;
        return;
    }
    console.ctrlkey_passthru = '+@ABCDEFGHIJKLMNOPQRSTUVWXYZ\\]^_-[';

	/* Check for messages */
	inbuf = format(MAILFL, ntn);
	alist = showmap(ntn, r, c, true, -1);
	showfocus(r, c);
	obj = update(ntn, r, c);
	r = obj.r;
	c = obj.c;
	ch = obj.ch;
	if(ch != '')
		keep_ch = true;

    /* enter the loop */
    saystat("Welcome to Solomoriah's WAR!  Press <h> for Help.");
    force = false;
    for(;;) {
        alist = showmap(ntn, r, c, force, avpnt);
        if(alist.length <= avpnt)
			avpnt = alist.length - 1;
        force = false;
        showfocus(r, c);
		if(!keep_ch) {
			do {
				ch = console.inkey(5000);

				/* Handle a new turn starting */
				if(turn_done) {
					if(!file_exists(pfile.name)) {
						if(pfile.open('ab', 0)) {
							turn_done = false;
							var orig_gen = gen;
							while(orig_gen == gen) {
								loadmap();
								loadsave();
							}
							// Check if game is over!
							city = -1;
					        for(i = 0; i < cities.length; i++) {
								if(cities[i].nation == ntn) {
									city=0;
									break;
								}
							}
							if(city == -1) {
								for(i = 0; i < armies.length; i++) {
									if(armies[i].nation == ntn) {
										city = 0;
										break;
									}
								}
							}

							if(city == -1) {
								saystat("Nation is Destroyed.  Press Any Key:  ");
								console.getkey();
								console.ctrlkey_passthru = orig_pt;
								return;
							}
							mainscreen();
							alist = showmap(ntn, r, c, true, avpnt);
							showfocus(r, c);
							upd_pos=0;
							upd_top=0;
							obj = update(ntn, r, c);
							r = obj.r;
							c = obj.c;
							ch = obj.ch;
						}
					}
				}
			} while(ch=='');
		}
		keep_ch = false;
        showfocus(r, c);
        clearstat(-1);
        switch(ch) {
        case 'p' : /* production */
            city = city_at(r, c);
            if(city != -1 && cities[city].nation == ntn)
                produce(city);
            else
                saystat("Must View Your Own City First.");
            break;
		case 'A' : /* army types */
			armytypes();
			force = true;
			break;
        case ctrl(']') : /* next army */
			obj = nextarmy(ntn, r, c);
			r = obj.r;
			c = obj.c;
            if(!obj.ret)
                saystat("No Next Army with Movement Found");
            break;
        case ']' : /* next group */
			obj = nextgroup(ntn, r, c);
			r = obj.r;
			c = obj.c;
            if(!obj.ret)
                saystat("No More Groups Remain.");
            break;
        case '}' : /* last group */
            while((obj = nextgroup(ntn, r, c)).ret) {
				r = obj.r;
				c = obj.c;
			}
			r = obj.r;
			c = obj.c;
            break;
        case ctrl('[') : /* prev army */
			obj = prevarmy(ntn, r, c);
			r = obj.r;
			c = obj.c;
            if(!obj.ret)
                saystat("No Previous Army with Movement Found");
            break;
        case '[' : /* previous group */
			obj = prevgroup(ntn, r, c);
			r = obj.r;
			c = obj.c;
            if(!obj.ret)
                saystat("No Previous Groups Remain.");
            break;
        case '{' : /* first group */
            while((obj = prevgroup(ntn, r, c)).ret) {
				r = obj.r;
				c = obj.c;
			}
			r = obj.r;
			c = obj.c;
            break;
        case 'n' : /* name hero */
			if(turn_done) {
				saystat("Turn is currently done, cannot issue orders");
				break;
			}
            if(alist.length > 0 && armies[alist[avpnt].id].name.length == 0) {
                saystat("Enter Hero's Name:  ");
                inbuf = console.getstr(16);

                buff = format("name-army %d '%s'\n", alist[avpnt].id, inbuf);
                pfile.write(buff);
                execpriv(buff);

                clearstat(-1);
            } else
                saystat("Can Only Rename Heros.");
            break;
        case 'z' : /* next army */
        case KEY_DOWN:
            if(alist.length > 0) {
                avpnt++;
             avpnt += alist.length;
                avpnt %= alist.length;
            } else
                saystat("No Armies!");
            break;
        case 'a' : /* previous army */
        case KEY_UP:
            if(alist.length > 0) {
                avpnt--;
                avpnt += alist.length;
                avpnt %= alist.length;
            } else
                saystat("No Armies!");
            break;
        case ' ' : /* mark */
            if(alist.length > 0)
                alist[avpnt].mark = !alist[avpnt].mark;
            break;
        case 'I' : /* army information */
            if(alist.length > 0) {
                var id = alist[avpnt].id;
                buff = format("%s: Combat %d / Hero %d %s",
                    armyname(id), armies[id].combat, armies[id].hero,
                    ((armies[id].hero > 0 && armies[id].eparm1) ? "(Loyal)" : ""));
                saystat(buff);
            } else
                saystat("No Armies!");
            break;
        case '*' : /* mark all */
        case 'f' : /* mark all and move */
        case '5' : /* mark all and move */
            for(i = 0; i < alist.length; i++) {
                if(armies[alist[i].id].move_left > 0)
                    alist[i].mark = 1;
            }
            if(ch == '*')
                break;
            /* fall through */
        case 'm' : /* move */
            obj = move_mode(alist, avpnt, ntn, r, c);
            r = obj.r;
            c = obj.c;
            break;
        case '/' : /* unmark all */
            for(i = 0; i < alist.length; i++)
                alist[i].mark = 0;
            break;
        case 'i' : /* information */
        case '1' :
        case '2' :
        case '3' :
        case '4' :
        case '6' :
        case '7' :
        case '8' :
        case '9' :
        case 'e' :
        case 'r' :
        case 't' :
        case 'd' :
        case 'g' :
        case 'c' :
        case 'v' :
        case 'b' :
            obj = info_mode(r, c, ntn, ch, avpnt);
            r = obj.r;
            c = obj.c;
			ch = obj.ch;
			if(ch != '')
				keep_ch = true;
            break;
		case 'u' :
            obj = update(ntn, r, c);
            r = obj.r;
            c = obj.c;
			ch = obj.ch;
			if(ch != '')
				keep_ch = true;
            break;
        case 'N' : /* read news... */
            reader(NEWSFL, 0);
			force = true;
            break;
        case 'M' : /* mail... */
            inbuf = format(MAILFL, ntn);
            reader(inbuf, 1);
            force = true;
            break;
        case 'S' : /* send mail */
            saystat("Send Mail -- Enter Ruler's Name, or All to Post News:  ");
            inbuf = console.getstr(16);
            n = -1;
            for(i = 0; i < nations.length; i++) {
                if(nations[i].name.toUpperCase() == inbuf.toUpperCase())
                    n = i;
            }
            if(n == -1 && "all" == inbuf.toLowerCase())
                n = 0;
            if(n == -1) {
                saystat("No Such Ruler.");
                break;
            }
            mailer(ntn, n);
            force = true;
            clearstat(-1);
            break;
       case 's' : /* status */
            status();
            force = true;
            break;
        case '?' : /* help */
            help();
            break;
        case 'h' : /* help */
		case 'H' : /* help */
			reader('help.news', 2);
			force = true;
			break;
        case '\f' :
			force = true;
            break;
		case 'x' :
			if(turn_done) {
				saystat("Turn is currently done, cannot issue orders");
				break;
			}
            saystat("End Turn?  (Y/N)  ");
            if(console.getkey().toLowerCase() == 'y') {
				pfile.write("end-turn\n");
				pfile.close();
				turn_done = true;
			}
            clearstat(-1);
            break;
        case 'q' : /* quit */
            saystat("Quit?  (Y/N/X)  ");
	   		switch(console.getkey().toLowerCase()) {
			case 'y':
				console.ctrlkey_passthru = orig_pt;
                return;
			case 'x':
				if(!turn_done)
					pfile.write("end-turn\n");
				return;
			}
            clearstat(-1);
            break;
		}
    }
	console.ctrlkey_passthru = orig_pt;
}

function titlescreen()
{
    var i;
	var title = [
		"             S O L O M O R I A H ' S",
		"",
		"           #    #    ##    #####    ###",
		"           #    #   #  #   #    #   ###",
		"           #    #  #    #  #    #   ###",
		"           #    #  ######  #####     # ",
		"           # ## #  #    #  #   #       ",
		"           ##  ##  #    #  #    #   ###",
		"            #  #   #    #  #    #   ###",
	];

    console.clear(attrs.title_text);
    for(i=0; i<title.length; i++) {
        console.gotoxy(11, i+3);
        console.print(title[i]);
    }
	console.attributes = attrs.title_copyright;
    console.gotoxy(11, i+3);
    console.print('JSWAR Version '+(major_ver)+'.'+(minor_ver)+'             "Code by Solomoriah"');
    console.gotoxy(11, i+5);
    console.print("Original Copyright 1993, 1994, 2001, 2013, Chris Gonnerman");
    console.gotoxy(11, i+6);
    console.print("JavaScript Version Copyright 2013, Stephen Hurd");
    console.gotoxy(11, i+7);
    console.print("All Rights Reserved.");
}

function newcity()
{
	var i;
	var clusters = [];
	var cc;
	var c;
	var cl;

	for(i = 0; i<MAXCITIES; i++)
		clusters[i] = -1;
	for(i=0; i<cities.length; i++) {
		if(cities[i].nation > 0)
			clusters[cities[i].cluster % MAXCITIES] = 1;
		else if(clusters[cities[i].cluster % MAXCITIES] == -1)
			clusters[cities[i].cluster % MAXCITIES] = 0;
	}
    cc = 0;
    for(i = 0; i < MAXCITIES; i++) {
        if(clusters[i] == 0)
            cc++;
    }
    if(cc == 0) {   /* any city will do... */
        cc = 0;
        for(i = 0; i < cities.length; i++)
            if(cities[i].nation == 0)
                cc++;
        if(cc == 0)
            return -1;
        c = random(cc);
        for(i = 0; i < cities.length; i++) {
            if(cities[i].nation == 0) {
                if(c == 0)
                    return i;
                else
                    c--;
            }
        }

        /* should not get here... */
        return -1;
    }
    else {        /* find an empty cluster... */
        c = random(cc);
        for(i = 0; i < MAXCITIES; i++) {
            if(clusters[i] == 0) {
                if(c == 0)
                    break;
                else
                    c--;
            }
        }
        if(i < MAXCITIES) {
            cl = i;
            cc = 0;
            for(i = 0; i < cities.length; i++) {
                if(cities[i].cluster == cl)
                    cc++;
            }
            c = random(cc);
            for(i = 0; i < cities.length; i++) {
                if(cities[i].cluster == cl) {
                    if(c == 0)
                        return i;
                    else
                        c--;
                }
            }
        }

        /* should not get here. */
        return -1;
    }
}

function main(argc, argv)
{
	var rc;
	var uid;
	var n;
	var c;
	var ch;
	var t;
	var trys;
	var mtbl;
	var line;
	var offset;
	var confirmed;
	var fp=new File();
	var pp=new File();
	var filename='';
	var inbuf='';
	var name='';
	var p;
	var u;
	var pass='';
	var cmd='';
	var gamedir='';
	var st_buf;
	var i;

	if(argc)
		set_game(argv[0]);
	load("loadfont.js", '-H', getpath('terrain.fnt'));
	if(bbs.mods.CTerm_Version != null) {
		console.write("\x1b[?31h\x1b[1;255 D");
		js.on_exit('console.write("\x1b[?31l")');
	}
	titlescreen();

	/* load map file */
	if((rc = loadmap()) != 0) {
		console.gotoxy(11,21);
		console.print("Error Loading Map ("+rc+")\r\n");
		console.getkey();
		exit(1);
	}
	if((rc = loadsave()) != 0) {
		console.gotoxy(11,21);
		console.print("Error Loading Game Save ("+rc+")\r\n");
		console.getkey();
		exit(2);
	}
	console.gotoxy(11,17);
	console.attributes = attrs.title_world;
	if(world.length > 0)
		console.print("World: "+world+"    ");
	console.print("Turn:  "+gen);
	uid = user.number;
	if(uid == undefined || uid == 0) {
		console.gotoxy(11, 21);
		console.print("UID Not Available; Aborting.");
		console.getkey();
		exit(3);
	}
	console.gotoxy(11, 21);
	console.attributes = attrs.status_area;
	console.print("Reading Master Commands...  ");
	fp = new File(getpath(MASTERFL));
	if(fp.open('rb')) {
		for(i=0; (inbuf = fp.readln())!=null; i++) {
			if((rc=execpriv(inbuf)==0)) {
				console.gotoxy(49,21);
				console.print(format("%3d lines"));
			}
			else {
				console.gotoxy(11,21);
				console.print("Master Cmd Failed, Line "+(i+1)+", Code "+rc+"  ");
				console.getkey();
				exit(4);
			}
		}
		fp.close();
	}

    /* check if nation is entered. */
    n = -1;
    for(i = 0; i < nations.length; i++) {
        if(nations[i].uid == uid) {
			/* TODO: Check for the canary file! */
            n = i;
            break;
        }
	}
    if(n == -1) { /* nation not entered yet... */
        c = newcity();
        if(c >= 0) {
            fp = new File(getpath(MASTERFL));
            if(!fp.open('ab')) {
				console.gotoxy(11,21);
				console.print("Can't Write Master Cmd's  ");
				console.cleartoeol();
				console.getkey();
				exit(5);
			}
            n = nations.length;
            confirmed = false;
            while(!confirmed) {
				console.gotoxy(11,19);
				console.attributes = attrs.title_newcity;
				console.print("Your City is "+ cities[c].name);
				console.cleartoeol();
				inbuf='';
				while(inbuf.length==0) {
					console.gotoxy(11, 21);
					console.attributes = attrs.title_retry;
					console.print("(Enter ! to Retry)");
					console.cleartoeol();
					console.gotoxy(11, 20);
					console.attributes = attrs.title_nameprompt;
					console.print("Enter your name:  ");
					console.cleartoeol();
					inbuf = console.getstr(NATIONNMLEN);
				}
				if(inbuf == '!')
					c = newcity();
				else
					confirmed = true;
            }
			name=inbuf;
			console.gotoxy(11,22);
			console.attributes = attrs.title_symboltitle;
			console.print("Available:  ");
			inbuf = marks[0];
			for(i=0; i<nations.length; i++)
				inbuf = inbuf.replace(nations[i].mark, '');
			console.attributes = attrs.title_symbollist;
			console.print(inbuf);
			ch='';
			while(ch == '' || ch==null || inbuf.indexOf(ch)==-1) {
				console.gotoxy(11, 21);
				console.attributes = attrs.title_symbolprompt;
				console.print("Select Nation Mark:  ");
				console.cleartoeol();
				ch = console.getkeys(inbuf);
			}
			console.gotoxy(11,22);
			console.cleartoeol();
			inbuf = "new-nation '"+name+"' "+uid+' '+c+' '+ch;
			fp.writeln(inbuf);
			execpriv(inbuf);
			
			/* ... and the hero... */
			t = cities[c].types[0];
			mtbl = ttypes[t].move_tbl;
			inbuf = "make-army '"+name+"' "+n+' '+cities[c].r+' '+cities[c].c+' '+(random(3)+4)+' '+(random(2)+1)+' 8 '+(mtbl)+' 0 1';
			fp.writeln(inbuf);
			execpriv(inbuf);

			/* ...and take the city. */
			inbuf = 'control-city '+(n)+' '+(c);
			fp.writeln(inbuf);
			execpriv(inbuf);
			fp.close();
		}
		else {
			console.gotoxy(11, 21);
			console.print("No Cities Available.  ");
			console.cleartoeol();
			console.getkey();
			exit(6);
		}
	}

	/* execute player commands */
	console.gotoxy(11, 21);
	console.attributes = attrs.status_area;
	console.print("Reading Player Commands...  ");
	console.cleartoeol();
	fp = new File(getpath(format(PLAYERFL, n)));
	if(fp.open('rb')) {
		for(i=0; (inbuf = fp.readln()) != null; i++) {
			if((rc = execpriv(inbuf))==0) {
				if((i+1) % 10 == 0) {
					console.gotoxy(49, 21);
					console.print(format("%3u lines", i));
				}
			}
			else {
				console.gotoxy(11, 21);
				console.print("Player Cmd Failed, Line "+(i+1)+", Code "+(rc)+"  ");
				console.getkey();
				exit(7);
			}
		}
		fp.close();
	}

	/* append to player file... */
	pfile = new File(getpath(format(PLAYERFL, n)));
	if(!turn_done) {
		if(!pfile.open('ab', 0 /* Not buffered */)) {
			console.gotoxy(11, 21);
			console.print("Can't Create or Append Player Cmd's  ");
			console.cleartoeol();
			console.getkey();
			exit(8);
		}
	}

	/* main loop */
	for(i=0; i<nations.length; i++) {
		if(nations[i].uid == uid)
			n = i;
	}
	console.gotoxy(11, 20);
	console.attributes = attrs.title_welcome;
	console.print("Welcome, "+nations[n].name+" of "+nationcity(n)+"!");
	console.cleartoeol();
	console.gotoxy(11, 21);
	console.attributes = attrs.title_anykey;
	console.print("Press Any Key to Begin...  ");
	console.cleartoeol();
	console.getkey();
	mainscreen();
	mainloop(n);

	/* clean up */
	if(pfile.is_open)
		pfile.close();
	exit(0);
}

main(argc, argv);

/* end of file. */
