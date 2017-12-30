// $Id: hexdump_lib.js,v 1.1 2017/12/30 03:00:13 rswindell Exp $

function hexdump(title, val)
{
	var i;

	if(!val) {
		printf("%s: (null)\n", title);
		return false;
	}
	printf("%s: %u bytes\n", title, val.length);
	var ascii = '';
	for(i=0; i < val.length; i++) {
		var ch = val.charCodeAt(i);
		if(i && i%16 == 0) {
			printf("  %s\n", ascii);
			ascii = '';
		}
		else if(i && i%8 == 0)
			printf("- ");
		printf("%02X ", ch);
		ascii += format("%c", (ch >= 0x20 && ch < 0x7f) ? ch : '.');
	}
	if(ascii) {
		var gap = 0;
		if(i%16) {
			gap = (16-(i%16))*3;
			if((i%16) < 8)
				gap += 2;
		}
		printf("%*s  %s\n", gap, "", ascii);
	}
	print();
	return true;
}

function hexdump_file(file)
{
	var buf = file.read();

	hexdump(file.name, buf);
}