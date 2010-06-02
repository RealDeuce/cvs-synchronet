/* $Id$ */

/* Synchronet v3.15 update script (to be executed with jsexec) */

const REVISION = "$Revision$".split(' ')[1];

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
		} else if(!file_rename(f2, f2 + ".old")) {
				printf("!Error %u renaming %s\n", errno, f2);
				return false;
		}
	}
	return true;
}


printf("Synchronet update.js revision %u\n", REVISION);
printf("Updating exec directory: ");
printf("%s\n", update_exec_dir() ? "Success" : "FAILURE");

