// $Id: mimehdr.js,v 1.3 2019/05/01 02:50:25 rswindell Exp $

// Support for RFC2047:
//
//        MIME (Multipurpose Internet Mail Extensions) Part Three:
//              Message Header Extensions for Non-ASCII Text

require("utf8_cp437.js", 'utf8_cp437');

// Returns an array of 'encoded-words' 
function decode(hvalue)
{
	var result = [];
	var regex = /(\=\?[a-zA-Z0-9-]+\?.\?[^ ?]+\?\=|\S+\s*)/g;
	var word;
	
	while(hvalue && (word = regex.exec(hvalue)) !== null) {
		var str = word[1];
		var retval = { charset: 'unspecified (US-ASCII)', data: str };

		var match = str.match(/^\=\?([a-zA-Z0-9-]+)\?(.)\?([^ ?]+)\?\=$/);
		if(!match) {
			result.push(retval);
			continue;
		}
		
		retval.charset = match[1].toLowerCase();
		switch(match[2].toLowerCase()) {
			case 'q':	// Quoted-printable
				retval.data = match[3].replace(/=([0-9A-F][0-9A-F])/g, function(str, p1) { return(ascii(parseInt(p1,16))); });
				break;
			case 'b':	// Base64
				retval.data = base64_decode(match[3]);
				break;
		}
		result.push(retval);
	}
	return result;
}

// Translate a MIME-encoded header field value to a CP437 string
function to_cp437(val)
{
	var result = [];
	var words = mimehdr.decode(val);
	for(i in words) {
		var word = words[i];
		switch(word.charset) {
			case 'utf-8':
				result.push(utf8_cp437(word.data));
				break;
			default:
				result.push(word.data);
				break;
		}
	}
	return result.join('');
}

this;