<!-- $Id: groups.inc,v 1.9 2005/03/04 01:01:57 runemaster Exp $ -->

<!-- Main Content -->  
    
    <td class="main" valign="top"><br />

<table class="grouplist" border="0" cellpadding="2" cellspacing="2">
<tr>
<th class="grouplist">Message Group</th><th class="grouplist">Subs</th>
</tr>
<<REPEAT groups>>
<tr>
    <td class="grouplist"><a class="grouplist" href="subs.ssjs?msg_grp=^^groups:name^^">%%groups:description%%</a></td>
    <td class="grouplist" align="right">@@JS:msg_area.grp['@@groups:name@@'].sub_list.length@@</td>
</tr>
<<END REPEAT groups>>
</table>
<br />

<!-- end Main Content -->


