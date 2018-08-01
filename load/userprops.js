// $Id: userprops.js,v 1.2 2018/08/01 22:34:07 rswindell Exp $

function filename(usernum)
{
	return system.data_dir + format("user/%04u.ini", usernum);
}

function get(section, key, deflt, usernum)
{
	if(!usernum)
		usernum = user.number;
	var file = new File(filename(usernum));
	if(!file.open('r'))
		return false;
	var result;
	if(!section)
		result = file.iniGetAllObjects();
	else if(!key)
		result = file.iniGetObject(section);
	else 
		result = file.iniGetValue(section, key, deflt);
	file.close();
	return result;
}

function set(section, key, value, usernum)
{
	if(!usernum)
		usernum = user.number;
	var file = new File(filename(usernum));
	if(!file.open(file.exists ? 'r+':'w+'))
		return false;
	file.iniSetValue(section, key, value);
	file.close();
}

this;
