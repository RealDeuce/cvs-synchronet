
<!-- $Id: changepw.inc,v 1.1 2005/03/22 04:05:39 runemaster Exp $ -->

<!-- Main Content --> 
</td>

    <td class="main" valign="top"><br />
    
      <h1>@@resultmsg@@</h1>
      @@JS:if(success)'Please <a href="/logout.ssjs">Logout</a> and re-login with your new password.'; else'<a href="@@backurl@@">Back to Change Password Page</a>';@@
      <br />
      @@JS:writeln(today);@@
    
