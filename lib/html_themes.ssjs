/* $Id: html_themes.ssjs,v 1.9 2005/06/24 09:01:44 runemaster Exp $ */

/* List of HTML theme packs installed */

Themes=new Object;

load('../web/templates/html_themes.ssjs');
var CurrTheme=DefaultTheme;

/* Read in current users selected theme if it exists */
if(file_exists(system.data_dir+'user/'+format("%04d.html_prefs",user.number))) {
  themefile=new File(system.data_dir+'user/'+format("%04d.html_prefs",user.number));
  if(themefile.open("r",false)) {
  CurrTheme=themefile.iniGetValue(null, 'CurrTheme');
  themefile.close();
  }
}
    

if(Themes[CurrTheme] == undefined || Themes[CurrTheme].dir == undefined)
    CurrTheme=DefaultTheme;

if(http_request.query.force_ssjs_theme != undefined) {
    if(http_request.query.force_ssjs_theme[0] != undefined && Themes[http_request.query.force_ssjs_theme[0]] != undefined)
        CurrTheme=http_request.query.force_ssjs_theme[0];
}
