/* Example Dynamic-HTML Content Parser */

/* $Id$ */

var filename;

if(this.http_request!=undefined)	/* Requested through web-server */
	filename = http_request.real_path;
else
	filename = argv[0];

var ssjs_filename=filename+".ssjs";

// Probably a race condition on Win32
if(file_exists(ssjs_filename)) {
	if(file_date(ssjs_filename)<=file_date(filename)) {
		file_remove(ssjs_filename);
	}
}

if(!file_exists(ssjs_filename)) {
	var file = new File(filename);
	if(!file.open("r",true,8192)) {
		writeln("!ERROR " + file.error + " opening " + filename);
		exit();
	}
	var text = file.readAll(8192);
	file.close();

	var script="";

	var in_xjs=false;
	for (line in text) {
		var str=text[line];
		while(str != '') {
			if(!in_xjs) {
				if(str=='<?xjs' || str=='<?') {
					in_xjs=true;
					str='';
				}
				else {
					if(str.search(/<\?(xjs)?\s+/)==-1) {
						script += "writeln("+escape_quotes(str)+");";
						str='';
					}
					else {
						str=str.replace(/^(.*?)<\?(xjs)?\s+/,
							function (str, p1, p2, offset, s) {
								if(p1 != '')
									script += "write("+escape_quotes(p1)+");";
								in_xjs=true;
								return '';
							}
						);
					}
				}
			}
			else {
				if(str.search(/\?>/)==-1) {
					script += str;
					str='';
				}
				else {
					str=str.replace(/^(.*?)\?>/,
						function (str, p1, offset, s) {
							script += p1+";";
							in_xjs=false;
							return '';
						}
					);
				}
			}
		}
		script += '\n';
	}

	var f=new File(ssjs_filename);
	if(f.open("w",false)) {
		f.write(script);
		f.close();
	}
}

load(ssjs_filename);

function escape_quotes(arg) {
	return("'"+arg.replace(/'/g,"'+"+'"'+"'"+'"+'+"'")+"'");
}
