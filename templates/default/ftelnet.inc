<!-- $Id: ftelnet.inc,v 1.3 2018/03/05 17:02:49 ree Exp $ -->

    <td class="main" valign="top"><br />
        <br />
		<h3 align='center'>fTelnet</h3>

<!-- fTelnet --> 

	<div id="fTelnetContainer" class="fTelnetContainer"></div>
	<script>document.write('<script src="//embed-v2.ftelnet.ca/js/ftelnet-loader.norip.noxfer.js?v=' + (new Date()).getTime() + '"><\/script>');</script>
	<script>
		var Options = new fTelnetOptions();
		Options.BareLFtoCRLF = false;
		Options.BitsPerSecond = 57600;
		Options.ConnectionType = 'telnet';
		Options.Emulation = 'ansi-bbs';
		Options.Enter = '\r';
		Options.Font = 'CP437';
		Options.ForceWss = false;
		Options.Hostname = '@@HostName@@';
		Options.LocalEcho = false;
		Options.Port = @@Port@@;
		Options.ScreenColumns = 80;
		Options.ScreenRows = 25;
		var fTelnet = new fTelnetClient('fTelnetContainer', Options);
	</script>

<!-- end fTelnet -->

