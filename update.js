/* $Id: update.js,v 1.2 2014/09/17 04:41:26 rswindell Exp $ */

/* Synchronet v3.15 update script (to be executed with jsexec) */

const REVISION = "$Revision: 1.2 $".split(' ')[1];

function update_exec_dir()
{
	var files;
	var fi,f1,f2;

	files=directory(system.exec_dir + "load/*.js");

	for(fi in files) {
		f1 = files[fi];
		f2 = system.exec_dir + file_getname(f1);
		if(!file_exists(f2))
			continue;
//		print("\nDuplicate detected: " +f1);
		if(file_compare(f1, f2)) {
			if(!file_remove(f2)) {
				printf("!Error %u removing %s\n", errno, f2);
				return false;
			}
		} else if(!file_backup(f2, /* levels: */100, /* rename: */true)) {
				printf("!Error backing-up %s\n", f2);
				return false;
		}
	}
	return true;
}


printf("Synchronet update.js revision %u\n", REVISION);
printf("Updating exec directory: ");
printf("%s\n", update_exec_dir() ? "Success" : "FAILURE");

