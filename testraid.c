/* testpf.c */
#include <stdio.h>
#include "pf.h"
#include "pftypes.h"

#define FILE1	"file3"
#define FILE2	"file4"


int reads, writes, erases;
int filesize;

main(int argc, char *argv[])
{
int error;
int i,j;
int pagenum,*buf;
int *buf1,*buf2;
int fd1,fd2;
int nowrites;

	if( argc == 3 )
	{
		printf("output The argument supplied are %s, %s\n", argv[1], argv[2]);
		//char temp[] = *argv[1];
		//nowrites = *argv[2];
		sscanf(argv[1], "%d", &filesize); 
		sscanf(argv[2], "%d", &nowrites);
		filesize = filesize*PF_MAX_BUFS; 
		printf("output final The argument supplied are %d, %d\n", filesize, nowrites);
	}
	else
	{
		printf("One argument expected.\n");
		filesize = 2*PF_MAX_BUFS;
		nowrites = 20;
	}

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

	//writing to both file since RAID1
	/* write to file1 */
	writefile(FILE1);

	/* print it out */
	readfile(FILE1);

	/* write to file2 */
	writefile(FILE2);

	/* print it out */
	readfile(FILE2);

	printf("output file size - %d\n", filesize);
	printf("output Number of updates - %d\n", nowrites);
	printf("output Sequential/Random updates\n");
	reads = 0;
	writes = 0;
	for(i=0; i<nowrites; i++)
	{
		j = rand() % filesize;
		updatefile(FILE1, FILE2, j);
	}
	printf("output final reads - %d\n", reads);
	printf("output final writes - %d\n", writes);
	
	readfile(FILE1);
	
	readfile(FILE2);

}


updatefile(fname1, fname2, pagenum)
char *fname1;
char* fname2;
int pagenum;
{
int error;
int fd1, fd2;
int* buf;
int* buf2;

	printf("opening %s\n",fname1);
	if ((fd1=PF_OpenFile(fname1))<0){
		PF_PrintError("open file");
		exit(1);
	}
	if ((fd2=PF_OpenFile(fname2))<0){
		PF_PrintError("open file");
		exit(1);
	}
	printf("Updating pagenum value - %d\n", pagenum);
	reads++;
	if((error=PF_GetThisPage(fd1,pagenum,&buf))== PFE_OK)
	{
		printf("got page in file %s, %d, value is %d\n",fname1,pagenum,*buf);
		writes++;
		*((int *)buf) = -(*buf);
		if((error=PF_GetThisPage(fd2,pagenum,&buf2))== PFE_OK)
		{
			printf("got page in file %d, %d, value is %d\n",fname2,pagenum,*buf2);
			writes++;
			*((int *)buf2) = -(*buf2);
			if ((error=PF_UnfixPage(fd2,pagenum,TRUE))!= PFE_OK){
				PF_PrintError("unfix");
				exit(1);
			}
		}
		if(error != PFE_OK)
		{
			PF_PrintError("error in getting the page");
			exit(1);
		}		
		if ((error=PF_UnfixPage(fd1,pagenum,TRUE))!= PFE_OK){
			PF_PrintError("unfix");
			exit(1);
		}
	}
	
	if(error != PFE_OK)
	{
		PF_PrintError("error in getting the page");
		exit(1);
	}
	
	if ((error=PF_CloseFile(fd1))!= PFE_OK){
		PF_PrintError("close file3");
		exit(1);
	}
	if ((error=PF_CloseFile(fd2))!= PFE_OK){
		PF_PrintError("close file4");
		exit(1);
	}
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
int i,j;
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
	
	for(j=1; j<(filesize/PF_MAX_BUFS); j++){
		for (i=j*PF_MAX_BUFS; i < (j+1)*PF_MAX_BUFS; i++){
			if ((error=PF_AllocPage(fd,&pagenum,&buf))!= PFE_OK){
				PF_PrintError("first buffer\n");
				exit(1);
			}
			*((int *)buf) = i;
			printf("allocated page %d\n",pagenum);
		}
	
		/* unfix these pages */
		for (i=j*PF_MAX_BUFS; i < (j+1)*PF_MAX_BUFS; i++){
			if ((error=PF_UnfixPage(fd,i,TRUE))!= PFE_OK){
				PF_PrintError("unfix buffer\n");
				exit(1);
			}
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

