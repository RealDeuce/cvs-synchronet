<!-- $Id: posted.inc,v 1.5 2005/03/04 01:01:57 runemaster Exp $ -->

<!-- Main Content -->  
    
    <td class="main" valign="top"><br />

<table class="posted" cellspacing="2" cellpadding="2">
    <tbody>
        <tr>
        <td class="posted">
            @@JS:if(sub!='mail')'<h3 align="center">Message posted successfully</h3>'; else '<h3 align="center">E-Mail sent successfully</h3>';@@
            <p class="posted"><a href="msgs.ssjs?msg_sub=^^sub:code^^">Return to %%sub:description%%</a></p>
        </td>
        </tr>
    </tbody>
</table>
