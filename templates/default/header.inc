<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
    <!-- $Id: header.inc,v 1.9 2005/03/22 04:20:57 runemaster Exp $ -->
<head>
<title>%%title%%</title>
<meta http-equiv="Content-Type" content="text/html; charset=iso-8859-1" />
@@JS:if(http_request.virtual_path=="/nodelist.ssjs")'<meta http-equiv=refresh content="30">';@@
<link rel="stylesheet" href="/synchronet.css" type="text/css" />
</head>
<body>

  <h1 id="siteName">@@system:name@@</h1> 
   <div id="Greeting"> 
        @@user_greeting@@
   </div>
   
   <table width="100%"  border="0" cellspacing="0" cellpadding="0">
  <tr>
    <td>
