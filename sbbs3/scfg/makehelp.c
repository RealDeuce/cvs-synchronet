/* MAKEHELP.C */

/* Developed 1990-1997 by Rob Swindell; PO Box 501, Yorba Linda, CA 92885 */

/* Makes SCFG.HLP for Synchronet configuration program */

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#if defined(__unix__)
	#define O_BINARY 0
#else
	#include <io.h>	/* open */
#endif

int main(void)
{
	char *files[]={
                 "scfg.c"
				,"scfgsys.c"
				,"scfgmsg.c"
				,"scfgsub.c"
				,"scfgnode.c"
				,"scfgchat.c"
				,"scfgxfr1.c"
				,"scfgxfr2.c"
				,"scfgnet.c"
				,"scfgxtrn.c"
				,NULL };
	char str[256],tmp[256];
	int i,j,k,ixb;
	unsigned short line;
	long l;
	FILE *stream,*out;

	if((out=fopen("scfghelp.dat","wb"))==NULL) {
		printf("error opening scfghelp.dat\r\n");
		return(-1); }

	if((ixb=open("scfghelp.ixb",O_WRONLY|O_CREAT|O_BINARY,S_IWRITE|S_IREAD))==-1) {
		printf("error opening scfghelp.ixb\r\n");
		return(-1); }

	for(i=0;files[i];i++) {
		if((stream=fopen(files[i],"rb"))==NULL) {
			printf("error opening %s\r\n",files[i]);
			return(-3); }
		printf("\r\n%s ",files[i]);
		line=0;
		while(!feof(stream)) {
			if(!fgets(str,128,stream))
				break;
			line++;
			if(strstr(str,"SETHELP(WHERE);")) {
				/* printf("%s:%u\n",files[i],line); */
				l=ftell(out);
				write(ixb,files[i],12);
				write(ixb,&line,2);
				write(ixb,&l,4);
				fgets(str,128,stream);	 /* skip start comment */
				line++;
				while(!feof(stream)) {
					if(!fgets(str,128,stream))
						break;
					if(strchr(str,9)) { /* Tab expansion */
						strcpy(tmp,str);
						k=strlen(tmp);
						for(j=l=0;j<k;j++) {
							if(tmp[j]==9) {
								str[l++]=32;
								while(l%4)
									str[l++]=32; }
							else
								str[l++]=tmp[j]; }
						str[l]=0; }
					line++;
					if(strstr(str,"*/"))    /* end comment */
						break;
					fputs(str,out); }
				fputc(0,out); } }
		fclose(stream); }
	fclose(out);
	printf("\n");
	return(0);
}
