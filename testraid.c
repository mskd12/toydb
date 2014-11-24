/* testpf.c */
#include <stdio.h>
#include "pf.h"
#include "pftypes.h"

#define FILE1	"file3"
#define FILE2	"file4"

main()
{
int error;
int i;
int pagenum,*buf;
int *buf1,*buf2;
int fd1,fd2;



	/* create a few files */
	if ((error=PF_CreateFile(FILE1))!= PFE_OK){
		PF_PrintError("file3");
		exit(1);
	}
	printf("file1 created\n");

	if ((error=PF_CreateFile(FILE2))!= PFE_OK){
		PF_PrintError("file4");
		exit(1);
	}
	printf("file2 created\n");

	/* write to file1 */
	writefile(FILE1);

	/* print it out */
	readfile(FILE1);

	/* write to file2 */
	writefile(FILE2);

	/* print it out */
	readfile(FILE2);


}






/************************************************************
Open the File.
allocate as many pages in the file as the buffer
manager would allow, and write the page number
into the data.
then, close file.
******************************************************************/
writefile(fname)
char *fname;
{
int i;
int fd,pagenum;
int *buf;
int error;

	/* open file1, and allocate a few pages in there */
	if ((fd=PF_OpenFile(fname))<0){
		PF_PrintError("open file1");
		exit(1);
	}
	printf("opened %s\n",fname);

	for (i=0; i < PF_MAX_BUFS; i++){
		if ((error=PF_AllocPage(fd,&pagenum,&buf))!= PFE_OK){
			PF_PrintError("first buffer\n");
			exit(1);
		}
		*((int *)buf) = i;
		printf("allocated page %d\n",pagenum);
	}

	if ((error=PF_AllocPage(fd,&pagenum,&buf))==PFE_OK){
		printf("too many buffers, and it's still OK\n");
		exit(1);
	}

	/* unfix these pages */
	for (i=0; i < PF_MAX_BUFS; i++){
		if ((error=PF_UnfixPage(fd,i,TRUE))!= PFE_OK){
			PF_PrintError("unfix buffer\n");
			exit(1);
		}
	}

	/* close the file */
	if ((error=PF_CloseFile(fd))!= PFE_OK){
		PF_PrintError("close file1\n");
		exit(1);
	}

}

/**************************************************************
print the content of file
*************************************************************/
readfile(fname)
char *fname;
{
int error;
int *buf;
int pagenum;
int fd;

	printf("opening %s\n",fname);
	if ((fd=PF_OpenFile(fname))<0){
		PF_PrintError("open file");
		exit(1);
	}
	printfile(fd);
	if ((error=PF_CloseFile(fd))!= PFE_OK){
		PF_PrintError("close file");
		exit(1);
	}
}

printfile(fd)
int fd;
{
int error;
int *buf;
int pagenum;

	printf("reading file\n");
	pagenum = -1;
	while ((error=PF_GetNextPage(fd,&pagenum,&buf))== PFE_OK){
		printf("got page %d, %d\n",pagenum,*buf);
		if ((error=PF_UnfixPage(fd,pagenum,FALSE))!= PFE_OK){
			PF_PrintError("unfix");
			exit(1);
		}
	}
	if (error != PFE_EOF){
		PF_PrintError("not eof\n");
		exit(1);
	}
	printf("eof reached\n");

}

