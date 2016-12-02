// Tomer Elmalem
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include "../libutility/libutility.h"
#include "../libmp4dir/mp4info.h"
#include <errno.h>
#include <time.h>


int aastreamout(void *data)
{
	printf("%s finish \n",(char *)data);
	return 0;
}

int spslen = 16;
int ppslen = 4;

unsigned char spsdata[16] = {0x67,0x42,0x00,0x2a,0x9d,0xa8,0x1e,0x00,0x89,0xf9,0x66,0xe0,0x20,0x20,0x20,0x40};
unsigned char ppsdata[4] = {0x68,0xce,0x3c,0x80};

int mp4_finish(int a)
{
	//printf("%s %d\n",__FUNCTION__,__LINE__);
	return 0;
}

int mp4_cutfinish(void* a)
{
	printf("%s %d\n",__FUNCTION__,__LINE__);
	return 0;
}


int main(void) 
{
	cdr_init_mp4dir(MP4DIRPATH);	

	//demo of read rec stream.
	//create mp4 async
#if 1	
	char fileName[256];
	getchar();
	srand((unsigned)time(0));
	int nSize;
	int nIFrame;
	int handle;
	char *pData = malloc(1920*1080);
	MP4_FRAME *pDataFrame = malloc(sizeof(MP4_FRAME));
	int nCreateFile = 10;
	while(nCreateFile)
	{
		//int totalFrame = 1000+rand() % 300;	
		Mp4Context *pMp4Context = malloc(sizeof(Mp4Context));
		sprintf(fileName,"/home/ll/work2016/study/cdr_mp4/out%d.mp4",nCreateFile--);
		strcpy(pMp4Context->sName,fileName);
		pMp4Context->nlen = 5 + rand() % 10;
		pMp4Context->nNeedAudio= 0;
		pMp4Context->oVideoCfg.timeScale = 90000;
		pMp4Context->oVideoCfg.fps = 30;
		pMp4Context->oVideoCfg.width = 1920;
		pMp4Context->oVideoCfg.height = 1080;
		pMp4Context->spslen = spslen;
		memcpy(pMp4Context->sps,spsdata,spslen);
		pMp4Context->ppslen = ppslen;
		memcpy(pMp4Context->pps,ppsdata,ppslen);
		pMp4Context->outcb = mp4_finish;
		handle = cdr_mp4_create(pMp4Context);	
		free(pMp4Context);
		pMp4Context = NULL;
		while(cdr_mp4_checklen(handle))
		{
			cdr_mp4ex_read_vframe(&pData,&nSize,&nIFrame);
			memset(pDataFrame,0x00,sizeof(MP4_FRAME));
			pDataFrame->streamType = MP4STREAM_VIDEO;
			pDataFrame->nFrameType = nIFrame;
			pDataFrame->uPTS = 0x00;
			pDataFrame->nlen = nSize;	
			pDataFrame->pData= (unsigned char*)pData;		
			cdr_mp4_write_frame(handle,pDataFrame);	
		}	
		cdr_mp4_close(handle);
		handle = -1;
		//getchar();
	}

	free(pDataFrame);
	pDataFrame = NULL;
	free(pData);
	pData = NULL;
	getchar();
#endif	

	//demo of cut mp4.
	cdr_read_mp4_ex("19700113063129",10,(stream_out_cb)mp4_cutfinish);
	cdr_read_mp4_ex("19700113063129",15,(stream_out_cb)mp4_cutfinish);
	cdr_read_mp4_ex("19700113063139",15,(stream_out_cb)mp4_cutfinish);
	getchar();

	cdr_mp4dirlist_free();		

	return 0;
}
