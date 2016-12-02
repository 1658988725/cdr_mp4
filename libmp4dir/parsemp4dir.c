// Tomer Elmalem
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <time.h> 
#include <sys/time.h>
#include "mp4info.h"
#include <mp4v2/mp4v2.h>

//#include "sps_pps.h"


mp4_dir_info *pMp4dirinfo = NULL;
Mp4ReadInfo *g_pRrecStream = NULL;

/* For these functions below you should never be passed in NULL */
/* You will always get valid data here */
void mp4Item_print(void* data)
{
	Mp4Info *s = data;
	int i = 0;
	//printf("strName: %s \n", s->strName);
	printf("%s\n", s->oFileName);

#if 1	
	printf("spslen %d\n", s->spslen);
	for(i=0;i<s->spslen;i++)
	{
		printf("0x%02x,",(unsigned char)s->sps[i]);
	}
	printf("\n");

	printf("ppslen %d\n", s->ppslen);
	for(i=0;i<s->ppslen;i++)
	{
		printf("0x%02x,",(unsigned char)s->pps[i]);
	}
	printf("\n");
#endif	
}


void mp4Item_free(void* data)
{
	free(data);
	data = NULL;
}

int mp4info_nameless(const void* a, const void* b)
{
	const Mp4Info *s1 = a;
	const Mp4Info *s2 = b;
	
	//printf("%s,%s",s1->strName,s2->strName);
	
	if(memcmp(s1->oFileName,s2->oFileName, 256) < 0) return 1;  
	return 0;
}


int mp4info_nameeq(const void* a, const void* b)
{
	const Mp4Info *s1 = a;
	const Mp4Info *s2 = b;
	
	//printf("%s,%s",s1->strName,s2->strName);
	
	if(memcmp(s1->oFileName,s2->oFileName, 256) == 0) return 1;  
	return 0;
}


int mp4info_timeeq(const void* a, const void* b)
{
	const Mp4ReadInfo *s1 = a;
	const Mp4Info *s2 = b;
	
	if(s1->tmStart >= s2->tmStart && s1->tmStart < s2->tmEnd)
		return 1;
		
	return 0;
}


//free list.
int cdr_mp4dirlist_free(void)
{	
	pMp4dirinfo->nThreadStart = 0;
	sleep(1);

	empty_list(pMp4dirinfo->fileList,mp4Item_free);
	empty_list(pMp4dirinfo->cutList,mp4Item_free);
	free(pMp4dirinfo->fileList);
	free(pMp4dirinfo->cutList);
	pMp4dirinfo->fileList = NULL;
	pMp4dirinfo->cutList = NULL;
	free(pMp4dirinfo);
	pMp4dirinfo = NULL;
	
	if(g_pRrecStream)
	{
		if(g_pRrecStream->oMp4File)
			MP4Close(g_pRrecStream->oMp4File,0);
		
		g_pRrecStream->oMp4File = NULL;	
		free(g_pRrecStream);
	}
	g_pRrecStream = NULL;
	
	return 0;
}

void *mp4dirstreamcut(void* pAgs)
{
	Mp4ReadInfo *p = (Mp4ReadInfo *)pAgs;
	//MP4FileHandle oMp4File;

	Mp4Context *pMp4Context;
	MP4_FRAME *pFrame;

	int totalFrame = ((p->nTotalLen) * (p->oMp4Info->nframerate));
	if(p->pStreamcb == NULL) 
	{
		return NULL;
	}

	//printf("fileName:%s \n",p->oMp4Info->oFileName);	
	
	unsigned short nReadIndex = 0;

	//printf("p->tmStart:%ld\n",p->tmStart);
	//printf("p->oMp4Info->tmStart:%ld\n",p->oMp4Info->tmStart);
	
	nReadIndex = (p->tmStart - p->oMp4Info->tmStart)*(p->oMp4Info->nframerate);	
	//printf("1 nReadIndex:%d\n",nReadIndex);
	
	unsigned char *pData = NULL;
	
	unsigned int nSize = 0;
	MP4Timestamp pStartTime;
	MP4Duration pDuration;
	MP4Duration pRenderingOffset;
	bool pIsSyncSample = 0;
	pMp4Context = malloc(sizeof(Mp4Context));
	memset(pMp4Context,0x00,sizeof(Mp4Context));
	strcpy(pMp4Context->sName,p->chName);
	pMp4Context->nNeedAudio = 0;
	memcpy(pMp4Context->sps,p->oMp4Info->sps,p->oMp4Info->spslen);
	memcpy(pMp4Context->pps,p->oMp4Info->pps,p->oMp4Info->ppslen);
	pMp4Context->spslen = p->oMp4Info->spslen;
	pMp4Context->ppslen = p->oMp4Info->ppslen;	
	int mp4Handle = cdr_mp4_create_ex(pMp4Context);		
	pFrame = malloc(sizeof(MP4_FRAME));
	pFrame->pData = malloc(1920*1024);

	p->oMp4File = MP4Read(p->oMp4Info->oFileName);	

	//first frame must I Frame
	//otherwise mp4 will screen black first second.
	while(pIsSyncSample==0 && nReadIndex <= p->oMp4Info->oFrameCount)
	{	
		nReadIndex ++;
		MP4ReadSample(p->oMp4File,p->oMp4Info->videoindex,nReadIndex,&pData,&nSize,&pStartTime,&pDuration,&pRenderingOffset,&pIsSyncSample);
		free(pData);
		pData = NULL;
	}		

	while(totalFrame)
	{	
		if(nReadIndex > p->oMp4Info->oFrameCount)
		{
			MP4Close(p->oMp4File,0);			
			p->oMp4Info = get_next(pMp4dirinfo->fileList,p->oMp4Info, mp4info_nameeq);
			//printf("get next mp4:%s\n",p->oMp4Info->oFileName);
			
			p->oMp4File = MP4Read(p->oMp4Info->oFileName);	
			nReadIndex = 1;
		}		
		pFrame->streamType = MP4STREAM_VIDEO;		
		MP4ReadSample(p->oMp4File,p->oMp4Info->videoindex,nReadIndex,&pData,&nSize,&pStartTime,&pDuration,&pRenderingOffset,&pIsSyncSample);
		nReadIndex++;	
		totalFrame --;
		//Send sps pps
		if(pIsSyncSample == 1)
		{			
			pFrame->nFrameType = VIDEO_TYPE_IF;
		}
		else
		{
			pFrame->nFrameType = VIDEO_TYPE_PF;
		}	
		
		memcpy(pFrame->pData,pData,nSize);
		pFrame->nlen = nSize;
		pFrame->pData[0] = 0x00;
		pFrame->pData[1] = 0x00;
		pFrame->pData[2] = 0x00;
		pFrame->pData[3] = 0x01;

		cdr_mp4_write_frame_ex(mp4Handle,pFrame);
		free(pData);
		pData = NULL;		
		usleep(10);
			
	}
	MP4Close(p->oMp4File,0);	
	cdr_mp4_close_ex(mp4Handle);

	free(pFrame->pData);
	free(pFrame);
	free(pMp4Context);
	return NULL;	
}


//在list里面截取一段视频.
//Param:pTm格式.20161110150100
//Param:len 时间长度.
//Param:pstreamout h264 stream and aac output.
//Output:如果seek在里在list里面.则返回0.如果不在list里面.返回-1
int cdr_read_mp4_ex(char *pTm,int len,stream_out_cb pstreamout)
{
	struct tm tmTMP;	

	if(!pTm || len <= 0) return -1;
	printf("%s %d\n",__FUNCTION__,__LINE__);
	Mp4ReadInfo *mp4ReadInfo = malloc(sizeof(Mp4ReadInfo));
	strptime(pTm, "%Y%m%d%H%M%S", &tmTMP); 
	tmTMP.tm_isdst = -1;	  
	mp4ReadInfo->tmStart  = mktime(&tmTMP);  
	mp4ReadInfo->tmEnd = mp4ReadInfo->tmStart + len;
	mp4ReadInfo->nTotalLen = len;
	Mp4Info *s = get_if(pMp4dirinfo->fileList,mp4ReadInfo, mp4info_timeeq);
	if(s)
	{
		printf("%s %d\n",__FUNCTION__,__LINE__);
		mp4ReadInfo->oMp4Info = s;
		mp4ReadInfo->pStreamcb = pstreamout;
		sprintf(mp4ReadInfo->chName,"%s/%s_%03d.mp4",MP4CUTOUTPATH,pTm,len);
		push_front(pMp4dirinfo->cutList,mp4ReadInfo);
	}
	else
	{
		printf("%s not in list...\n",pTm);
		free(mp4ReadInfo);
		mp4ReadInfo = NULL;
		return -1;
	}
	return 0;
}

//Mp4 demuxer thrad callback.
void *mp4dirdemuxercb(void* pAgs)
{
	mp4_dir_info *p = (mp4_dir_info *)pAgs;

	Mp4Info *pMp4Item;
	DIR *dir;
	struct dirent *ptr = NULL;
	static char chName[256];

	while(p->nThreadStart)
	{
		dir = opendir(p->chDir);
		if(dir == NULL)
		{
			printf("%s cann't open dir: %s \n",__FUNCTION__,p->chDir);
			sleep(3);
			continue;
			//return NULL;
		}

		while ((ptr = readdir(dir)) != NULL)
		{
			if (ptr->d_type != 8)
				continue;
			if(NULL == strchr(ptr->d_name,'_'))
				continue;
			
			memset(chName, 0, sizeof(chName));
			snprintf(chName, 256, "%s/%s", p->chDir, ptr->d_name);    					
			pMp4Item = malloc(sizeof(Mp4Info));
			memset(pMp4Item,0x00,sizeof(Mp4Info));
			strncpy(pMp4Item->oFileName, chName, strlen(chName)+1);


			if(find_occurrence(p->fileList,pMp4Item,mp4info_nameeq))
			{
				mp4Item_free(pMp4Item);
				pMp4Item = NULL;
				continue;
			}

			if(demuxer_mp4_info(chName,pMp4Item) == 0)
			{
				push_if(p->fileList,pMp4Item,mp4info_nameless);
			}
			else
			{
				mp4Item_free(pMp4Item);
				pMp4Item = NULL;
			}

		}
		closedir(dir);
		usleep(3);
		static int flag = 0;
		if(flag == 0)
		{
			traverse(p->fileList,mp4Item_print);
			flag = 1;
		}
	}

	printf("%s thread finish..\n",__FUNCTION__);
	return NULL;

}

//Mp4 cut thread callback.
void *mp4dircutcb(void* pAgs)
{
	mp4_dir_info *p = (mp4_dir_info *)pAgs;
	Mp4ReadInfo *mp4CutInfo = NULL;
	
	while(p->nThreadStart)
	{
		mp4CutInfo = NULL;
		if(size(p->cutList) == 0)
		{
			usleep(10000);
			continue;
		}		
		mp4CutInfo = front(p->cutList);
		mp4dirstreamcut(mp4CutInfo);		
		mp4CutInfo->pStreamcb(mp4CutInfo->chName);
		remove_front(p->cutList,mp4Item_free);
		sleep(1);
	}
	printf("%s thread finish..\n",__FUNCTION__);
	return NULL;
	
}


//Create parse mp4 thread and cut mp4 thread.
int cdr_init_mp4dir(char *path)
{
	if(!path) return -1;
	
	pMp4dirinfo = malloc(sizeof(mp4_dir_info));
	memset(pMp4dirinfo,0x00,sizeof(mp4_dir_info));
	strncpy(pMp4dirinfo->chDir,path,strlen(path));
	
	pMp4dirinfo->fileList = create_list();  
	pMp4dirinfo->cutList = create_list();
	pMp4dirinfo->nThreadStart = 1;
	
	int ret = 0;
	ret = pthread_create(&pMp4dirinfo->tDemuxerfid, NULL,&mp4dirdemuxercb, pMp4dirinfo);
	if (ret != 0)
	{
		printf("%s create thread failed..\n",__FUNCTION__);
		return -1;
	}
	pthread_detach(pMp4dirinfo->tDemuxerfid);	

	ret = pthread_create(&pMp4dirinfo->tCutfid, NULL,&mp4dircutcb, pMp4dirinfo);
	if (ret != 0)
	{
		printf("%s create thread failed..\n",__FUNCTION__);
		return -1;
	}
	pthread_detach(pMp4dirinfo->tCutfid);

	return 0;
	
}


int cdr_mp4ex_seek(char *pTm)
{
	if(!pTm) return -1;
	struct tm tmTMP;	
	Mp4ReadInfo *mp4ReadInfo = malloc(sizeof(Mp4ReadInfo));
	strptime(pTm, "%Y%m%d%H%M%S", &tmTMP); 
	tmTMP.tm_isdst = -1;	  
	mp4ReadInfo->tmStart  = mktime(&tmTMP);  
	Mp4Info *s = get_if(pMp4dirinfo->fileList,mp4ReadInfo, mp4info_timeeq);
	if(s)
	{
		mp4ReadInfo->oMp4Info = s;
		mp4ReadInfo->VReadIndex = (mp4ReadInfo->tmStart - s->tmStart)*(s->nframerate);
		if(g_pRrecStream)
		{
			free(g_pRrecStream);
		}
		g_pRrecStream = mp4ReadInfo;		
	}
	else
	{
		printf("%s not in list...\n",pTm);
		free(mp4ReadInfo);
		mp4ReadInfo = NULL;
		return -1;
	}
	return 0;
}


//读取视频帧.
int cdr_mp4ex_read_vframe(char **pFrameData,int *nLen,int *IFrame)
{
	//如果第一次进入，初始化一个新的指针
	if(!g_pRrecStream)
	{
		g_pRrecStream = malloc(sizeof(Mp4ReadInfo));
		g_pRrecStream->oMp4Info = front(pMp4dirinfo->fileList);
		g_pRrecStream->VReadIndex = 1;
		g_pRrecStream->AReadIndex = 1;
		g_pRrecStream->oMp4File = NULL;
		printf("Rec Get First MP4 File\n");
	}
	//如果已经读取到文件末尾，就打开下一个文件
	if(g_pRrecStream->VReadIndex > g_pRrecStream->oMp4Info->oFrameCount)
	{
		g_pRrecStream->VReadIndex = 1;
		g_pRrecStream->AReadIndex = 1;
		
		g_pRrecStream->oMp4Info = get_next(pMp4dirinfo->fileList,g_pRrecStream->oMp4Info, mp4info_nameeq);

		if(g_pRrecStream->oMp4File)	MP4Close(g_pRrecStream->oMp4File,0);
		
		g_pRrecStream->oMp4File = NULL;

		printf("Rec Get next MP4 File:%s\n",g_pRrecStream->oMp4Info->oFileName);
	}

	if(g_pRrecStream->oMp4File == NULL)
		g_pRrecStream->oMp4File = MP4Read(g_pRrecStream->oMp4Info->oFileName);
	
	unsigned char *pData = NULL;
	
	unsigned int nSize = 0;
	MP4Timestamp pStartTime;
	MP4Duration pDuration;
	MP4Duration pRenderingOffset;
	bool pIsSyncSample = 0;

	//printf("g_pRrecStream->VReadIndex :%d\n",g_pRrecStream->VReadIndex );
	
	MP4ReadSample(g_pRrecStream->oMp4File,g_pRrecStream->oMp4Info->videoindex,g_pRrecStream->VReadIndex,
		&pData,&nSize,&pStartTime,&pDuration,&pRenderingOffset,&pIsSyncSample);	

	//sample frame to 264.
	if(nSize > 4)
	{
		pData[0] = 0x00;
		pData[1] = 0x00;
		pData[2] = 0x00;
		pData[3] = 0x01;
	}

	if(*pFrameData)
		memcpy(*pFrameData,pData,nSize);
	if(nLen)
		*nLen = nSize;
	if(IFrame)
	{
		if(pIsSyncSample)
		*IFrame = CDR_H264_NALU_ISLICE;
		else
		*IFrame = CDR_H264_NALU_PSLICE;
	}

	free(pData);
	pData = NULL;
	
	g_pRrecStream->VReadIndex = g_pRrecStream->VReadIndex+1;
	
	return 0;
}
