if(user.number==0) {
	http_reply.status='401 Premission Denied';
	http_reply.header["WWW-Authenticate"]='Basic realm="'+system.name+'"';
}
write('<html><head>');
write('<meta http-equiv="Pragma" content="no-cache">');
write('<meta http-equiv="expires" content="0">');
write('<meta http-equiv="refresh" content="0; URL=../index.ssjs">');
write('</head></html>');

