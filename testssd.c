/* testpf.c */
#include <stdio.h>
#include "pf.h"
#include "pftypes.h"
//#include "amcppheader.h"

#define FILE1	"file1"
#define FILE2	"file2"
#define PAGEINBLOCK 4

int reads, writes, erases;
int filesize, numtotal,numstale;
main()
{
int error;
int i,fd;
int pagenum,*buf;
int *buf1,*buf2;
int fd1,fd2;
filesize = 100*PF_MAX_BUFS;
int nowrites = 500;

//int stale_file[2][];
	/* create a few files */
	if ((error=PF_CreateFile(FILE1))!= PFE_OK){
		PF_PrintError("file1");
		exit(1);
	}
	printf("file size - %d", filesize);
	int stale_file1[20000];
	for (i=0; i<filesize; i++)
	{
		stale_file1[i] = 0;
	}
	printf("file1 created\n");

	if ((error=PF_CreateFile(FILE2))!= PFE_OK){
		PF_PrintError("file2");
		exit(1);
	}
	int stale_file2[20000];
	for (i=0; i<filesize; i++)
	{
		stale_file2[i] = 0;
	}
	printf("file2 created\n");

	
	/* write to file1 */
	writefile(FILE1,stale_file1);

	/* print it out */
	readfile(FILE1,stale_file1);
	
	
	//Sequential Updates
	reads = 0;
	writes = 0;
	erases = 0;
	if ((fd=PF_OpenFile(FILE1))<0){
		PF_PrintError("open file");
		exit(1);
	}
	pagenum = -1;
	int temp = 0;
	printf("\noutput Sequential Updates\n");
	while (temp<nowrites && (error=PF_GetNextPage(fd,&pagenum,&buf))== PFE_OK)
	//for(i=0; i<nowrites; i++)
	{
		i = pagenum;
		temp++;
		if ((error=PF_UnfixPage(fd,pagenum,FALSE))!= PFE_OK){
			PF_PrintError("unfix");
			exit(1);
		}
		updatefile(FILE1,stale_file1,i);
	}
	printf("reads - %d\n", reads);
	printf("erases - %d\n", erases);
	printf("writes - %d\n", writes);
	garbagecollect(FILE1,stale_file1);
	printf("output file size - %d\n", filesize);
	printf("output Number of updates - %d\n", nowrites);
	printf("output Number of pages in block - %d\n", PAGEINBLOCK);
	printf("output After garbage collection\n");
	printf("output reads - %d\n", reads);
	printf("output erases - %d\n", erases);
	printf("output writes - %d\n", writes);
	
	if ((error=PF_CloseFile(fd))!= PFE_OK){
		PF_PrintError("close file");
		exit(1);
	}
	readfile(FILE1, stale_file1);

	/* write to file2 */
	writefile(FILE2,stale_file2);

	/* print it out */
	readfile(FILE2,stale_file2);
		

	//Random Updates 
	reads = 0;
	writes = 0;
	erases = 0;
	if ((fd=PF_OpenFile(FILE2))<0){
		PF_PrintError("open file");
		exit(1);
	}
	pagenum = -1;
	temp = 0;
	printf("\noutput Random Updates\n");
	while (temp<nowrites && ((error=PF_GetNextPage(fd,&pagenum,&buf))== PFE_OK || error == PFE_EOF) )
	//for(i=0; i<nowrites; i++)
	{
		if(error == PFE_EOF)
		{
			printf("output EOF reached\n");
			pagenum = -1;
			continue;
		}
		i = pagenum;
		if ((error=PF_UnfixPage(fd,pagenum,FALSE))!= PFE_OK){
			printf("outputerror - %d\n", error);
			PF_PrintError("unfix");
			exit(1);
		}
		int j = rand();
		if(j%2 == 0) continue;
		temp++;
		//printf("output i : %d", i);
		//int j = rand() % filesize;
		updatefile(FILE2,stale_file2,i);
		if(temp==nowrites/2) 
		{
			printf("output Before garbage collection1:-\n");
			printf("output reads - %d\n", reads);
			printf("output erases - %d\n", erases);
			printf("output writes - %d\n", writes);
			numpages(FILE2,stale_file2);
			printf("output total number of pages - %d, stale pages is - %d\n", numtotal,numstale);
			garbagecollect(FILE2,stale_file2);
			printf("output After garbage collection1:-\n");
			printf("output reads - %d\n", reads);
			printf("output erases - %d\n", erases);
			printf("output writes - %d\n", writes);
			numpages(FILE2,stale_file2);
			printf("output total number of pages - %d, stale pages is - %d\n", numtotal,numstale);
		}
	}
	if(error != PFE_OK) 
	{
		PF_PrintError("output");
	}
	printf("output Before garbage collection2\n");
	printf("output reads - %d\n", reads);
	printf("output erases - %d\n", erases);
	printf("output writes - %d\n", writes);
	numpages(FILE2,stale_file2);
	printf("output total number of pages - %d, stale pages is - %d\n", numtotal,numstale);
	garbagecollect(FILE2,stale_file2);
	printf("output file size - %d\n", filesize);
	printf("output Number of updates - %d, %d\n", temp, nowrites);
	printf("output Number of pages in block - %d\n", PAGEINBLOCK);
	printf("output After garbage collection2\n");
	printf("output reads - %d\n", reads);
	printf("output erases - %d\n", erases);
	printf("output writes - %d\n", writes);
	numpages(FILE2,stale_file2);
	printf("output total number of pages - %d, stale pages is - %d\n", numtotal,numstale);
		
	if ((error=PF_CloseFile(fd))!= PFE_OK){
		PF_PrintError("close file");
		exit(1);
	}
	readfile(FILE2, stale_file2);

	//readfile(FILE1,stale_file1);
	
	//garbagecollect(FILE1,stale_file1);
	
	//readfile(FILE1,stale_file1);
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
					erases++;
				}
				else {
					if((error=PF_GetThisPage(fd,pagenum,&buf))== PFE_OK){
						reads++;
						printf("Got the page to be moved\n");
						if ((error=PF_AllocPage(fd,&pagenum2,&buf2))!= PFE_OK){
							PF_PrintError("first buffer alloc\n");
							exit(1);
						}
						*((int *)buf2) = (*buf);
						writes++;
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
						erases++;
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
	reads++;
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
	writes++;
	stale[pagenum2] = 0;
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

numpages(fname,stale)
char* fname;
int stale[];
{
int error;
int *buf;
int pagenum;
int fd;
numtotal = 0;
numstale = 0;

	printf("opening %s\n",fname);
	if ((fd=PF_OpenFile(fname))<0){
		PF_PrintError("open file");
		exit(1);
	}
	printf("reading file\n");
	pagenum = -1;
	while ((error=PF_GetNextPage(fd,&pagenum,&buf))== PFE_OK){
		numtotal++;
		if(stale[pagenum] == 1) numstale++;
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
