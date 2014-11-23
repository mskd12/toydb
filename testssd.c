/* testpf.c */
#include <stdio.h>
#include "pf.h"
#include "pftypes.h"

#define FILE1	"file1"
#define FILE2	"file2"
#define PAGEINBLOCK 4

main()
{
int error;
int i;
int pagenum,*buf;
int *buf1,*buf2;
int fd1,fd2;
int filesize = 2*PF_MAX_BUFS;
//int stale_file[2][];
	/* create a few files */
	if ((error=PF_CreateFile(FILE1))!= PFE_OK){
		PF_PrintError("file1");
		exit(1);
	}
	printf("file size - %d", filesize);
	int stale_file1[40];
	for (i=0; i<filesize; i++)
	{
		stale_file1[i] = 0;
	}
	printf("file1 created\n");

	if ((error=PF_CreateFile(FILE2))!= PFE_OK){
		PF_PrintError("file2");
		exit(1);
	}
	int stale_file2[40];
	for (i=0; i<filesize; i++)
	{
		stale_file2[i] = 0;
	}
	printf("file2 created\n");

	/* write to file1 */
	writefile(FILE1,stale_file1);

	/* print it out */
	readfile(FILE1,stale_file1);

	/* write to file2 */
	writefile(FILE2,stale_file2);

	/* print it out */
	readfile(FILE2,stale_file2);
	
	updatefile(FILE1,stale_file1,1);
	
	readfile(FILE1,stale_file1);
	
	garbagecollect(FILE1,stale_file1);
	
	readfile(FILE1,stale_file1);
}


garbagecollect(fname,stale)
char* fname;
int stale[];
{
int error;
int fd;
int* buf;
int* buf2;
int pagenum, pagenum2;
int NumPages;
NumPages = 0;
	printf("opening %s\n",fname);
	if ((fd=PF_OpenFile(fname))<0){
		PF_PrintError("open file");
		exit(1);
	}
	int* block_stale;
	pagenum = -1;
	while ((error=PF_GetNextPage(fd,&pagenum,&buf))== PFE_OK){
		NumPages = NumPages+1;
		if ((error=PF_UnfixPage(fd,pagenum,FALSE))!= PFE_OK){
			PF_PrintError("unfix");
			exit(1);
		}
		/*printf("got page %d, %d, %d\n",pagenum,*buf,stale[pagenum]);
		if(stale[pagenum]==1)
		{
			printf("Stale block");
		}*/
	}
	printf("Total num pages - %d\n", NumPages);
	int NumBlock;
	for(NumBlock = 0; NumBlock<(NumPages/PAGEINBLOCK); NumBlock++){
		int var = 0;
		for(pagenum = NumBlock*PAGEINBLOCK; pagenum<(NumBlock+1)*PAGEINBLOCK; pagenum++){
			if(stale[pagenum] == 1) var = 1;
		}
		if(var == 1){
			printf("Block number which has to be erased is %d\n", NumBlock);
			for(pagenum = NumBlock*PAGEINBLOCK; pagenum<(NumBlock+1)*PAGEINBLOCK; pagenum++){
				if(stale[pagenum] == 1) {
					printf("Disposing of page - %d\n", pagenum);
					stale[pagenum] = 0;
					if((error = PF_DisposePage(fd,pagenum))!=PFE_OK) {
						PF_PrintError("Check the Error\n");
						exit(1);
					}
				}
				else {
					if((error=PF_GetThisPage(fd,pagenum,&buf))== PFE_OK){
						printf("Got the page to be moved\n");
						if ((error=PF_AllocPage(fd,&pagenum2,&buf2))!= PFE_OK){
							PF_PrintError("first buffer alloc\n");
							exit(1);
						}
						*((int *)buf2) = (*buf);
						printf("Wrote pagenum - %d into pageNo - %d\n",pagenum,pagenum2);
						printf("Unfixing both the pages\n");
						if ((error=PF_UnfixPage(fd,pagenum2,TRUE))!= PFE_OK){
							PF_PrintError("unfix buffer\n");
							exit(1);
						}
						if ((error=PF_UnfixPage(fd,pagenum,TRUE))!= PFE_OK){
							PF_PrintError("unfix buffer\n");
							exit(1);
						}
						printf("Disposing of the original page\n");
						if((error = PF_DisposePage(fd,pagenum))!=PFE_OK) {
							PF_PrintError("Check the Error\n");
							exit(1);
						}
					}
					else {
						PF_PrintError("Can't get the page\n");
						exit(1);
					}
				}
			}
		}
	}
	
	if ((error=PF_CloseFile(fd))!= PFE_OK){
		PF_PrintError("close file");
		exit(1);
	}
}


/*****************
 * update value in pagenum
 * As in SSD, we mark it as stale and insert the new value else-where
 *****************/
updatefile(fname,stale,pagenum)
char* fname;
int stale[];
int pagenum;
{
int error;
int fd;
int* buf;
int* buf2;
int pagenum2;

	printf("opening %s\n",fname);
	if ((fd=PF_OpenFile(fname))<0){
		PF_PrintError("open file");
		exit(1);
	}
	printf("Updating pagenum value - %d\n", pagenum);
	if((error=PF_GetThisPage(fd,pagenum,&buf))== PFE_OK)
	{
		printf("Changing stale of pagenum \n");
		stale[pagenum] = 1;
		printf("got page %d, value is %d, stale value is %d\n",pagenum,*buf,stale[pagenum]);
		if ((error=PF_UnfixPage(fd,pagenum,FALSE))!= PFE_OK){
			PF_PrintError("unfix");
			exit(1);
		}
	}
	if(error != PFE_OK)
	{
		PF_PrintError("");
		exit(1);
	}
	printf("Allocating a new free page \n");
	if ((error=PF_AllocPage(fd,&pagenum2,&buf2))!= PFE_OK){
		PF_PrintError("first buffer\n");
		exit(1);
	}
	*((int *)buf2) = -(*buf);
	printf("new allocated page is %d, new value is %d\n",pagenum2,(*buf2));
	
	if ((error=PF_UnfixPage(fd,pagenum2,TRUE))!= PFE_OK){
		PF_PrintError("unfix buffer\n");
		exit(1);
	}
		
	if ((error=PF_CloseFile(fd))!= PFE_OK){
		PF_PrintError("close file");
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
writefile(fname,stale)
char *fname;
int stale[];
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
	
	for (i=0; i < PF_MAX_BUFS; i++){
		if ((error=PF_AllocPage(fd,&pagenum,&buf))!= PFE_OK){
			PF_PrintError("first buffer\n");
			exit(1);
		}
		*((int *)buf) = i+PF_MAX_BUFS;
		printf("allocated page %d\n",pagenum);
	}

	/* unfix these pages */
	for (i=0; i < PF_MAX_BUFS; i++){
		if ((error=PF_UnfixPage(fd,i+PF_MAX_BUFS,TRUE))!= PFE_OK){
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
readfile(fname,stale)
char *fname;
int stale[];
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
	printfile(fd,stale);
	if ((error=PF_CloseFile(fd))!= PFE_OK){
		PF_PrintError("close file");
		exit(1);
	}
}

printfile(fd,stale)
int* stale;
int fd;
{
int error;
int *buf;
int pagenum;
//int stale;

	printf("reading file\n");
	pagenum = -1;
	while ((error=PF_GetNextPage(fd,&pagenum,&buf))== PFE_OK){
		printf("got page %d, %d, %d\n",pagenum,*buf,stale[pagenum]);
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
	//int num;
	//PFnumpages(fd,num);
	//printf("number of pages in file is %d\n", num);

}
