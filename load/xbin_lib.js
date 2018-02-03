// $Id: xbin_lib.js,v 1.1 2018/02/03 10:19:09 rswindell Exp $

load('xbin_defs.js');

function read(file)
{
	if(file.read(this.ID_LEN) != this.id)
		return false;
	var image = {};
	image.width = file.readBin(2);
	image.height = file.readBin(2);
	image.charheight =file.readBin(1);
	image.flags = file.readBin(1);
	image.font_count = 0;
	if(image.flags&this.FLAG_FONT_NORMAL)
		image.font_count++;
	if(image.flags&this.FLAG_FONT_HIGH)
		image.font_count++;
	if(image.flags&this.FLAG_FONT_BLINK)
		image.font_count++;
	if(image.flags&this.FLAG_FONT_HIGHBLINK)
		image.font_count++;

	if(image.flags&this.FLAG_FONT_NORMAL)
		image.normal = (image.font_count-1)&2;	// Either 0 or 2
	if(image.flags&this.FLAG_FONT_HIGH)
		image.high = (image.font_count-1)&3;	// Either 1 or 3
	if(image.flags&this.FLAG_FONT_BLINK)
		image.blink = 0;
	if(image.flags&this.FLAG_FONT_HIGHBLINK)
		image.highblink = 1;

	if(image.flags&this.FLAG_PALETTE)
		image.palette = file.read(this.PALETTE_LEN);

//	printf("image.font_count = %d\r\n", image.font_count);
	image.font = [];
	for(var i = 0; i < image.font_count; i++)
		image.font[i] = file.read(0x100 * image.charheight);

	if(!(image.flags&this.FLAG_COMPRESSED))
		image.bin = file.read(image.width * image.height * 2);
	else {
		image.bin = '';
		while(image.bin.length < image.width * image.height * 2 && !file.error) {
			var byte = file.readBin(1);
			var type = byte >> 6;
			var repeat = (byte&0x3f) + 1;
			switch(type) {
				case 0:	// No compression
					for(var i = 0; i < repeat; i++) {
						image.bin += file.read(1);
						image.bin += file.read(1);
					}
					break;
				case 1:	// char compression
					var char = file.read(1);
					for(var i = 0; i < repeat; i++) {
						image.bin += char;
						image.bin += file.read(1);
					}
					break;
				case 2:	// attr compression
					var attr = file.read(1);
					for(var i = 0; i < repeat; i++) {
						image.bin += file.read(1);
						image.bin += attr;
					}
					break;
				case 3:	// char & attr compression
					var char = file.read(1);
					var attr = file.read(1);
					for(var i = 0; i < repeat; i++) {
						image.bin += char;
						image.bin += attr;
					}
					break;
			}
		}		
	}

	return image;
}

// Leave as last line:
this;