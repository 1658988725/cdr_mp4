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



static int nIndex = 0;

list* g_mp4_list = NULL;

void* write_frame_to_mp4_pro(void *pParam);


/*************************************************aac encode******************************************/
int InitAccEncoder(Mp4Context *p)
{
	if(!p) return -1;

	p->oAudioCfg.nSampleRate = 16000;
	p->oAudioCfg.nChannels = 2;
	p->oAudioCfg.nPCMBitSize = 16;
	p->oAudioCfg.nInputSamples = 1024;
	p->oAudioCfg.nMaxOutputBytes = 0;

	return 0;
}


static void make_dsi( unsigned int sampling_frequency_index, unsigned int channel_configuration, unsigned   char* dsi )
{
    unsigned int object_type = 2;
    dsi[0] = (object_type<<3) | (sampling_frequency_index>>1);
    dsi[1] = ((sampling_frequency_index&1)<<7) | (channel_configuration<<3);
}

static int get_sr_index(unsigned int sampling_frequency)
{
    switch (sampling_frequency) 
    {
	    case 96000: return 0;
	    case 88200: return 1;
	    case 64000: return 2;
	    case 48000: return 3;
	    case 44100: return 4;
	    case 32000: return 5;
	    case 24000: return 6;
	    case 22050: return 7;
	    case 16000: return 8;
	    case 12000: return 9;
	    case 11025: return 10;
	    case 8000:  return 11;
	    case 7350:  return 12;
	    default:    return 0;
    }
}


/***********************************************************mp4 encode***********************************************/
int InitMp4Encoder(Mp4Context *p)
{	
	if(!p) return -1;
		
	p->video = MP4_INVALID_TRACK_ID;
	p->audio = MP4_INVALID_TRACK_ID;
	p->hFile = NULL;

	/*file handle*/
	p->hFile = MP4Create(p->sName, 0);
	if (p->hFile == MP4_INVALID_FILE_HANDLE)
	{
		printf("open file fialed.\n");
		return -1;
	}
	MP4SetTimeScale(p->hFile, p->oVideoCfg.timeScale);    //timeScale

	p->video = MP4AddH264VideoTrack(p->hFile,p->oVideoCfg.timeScale,(p->oVideoCfg.timeScale / p->oVideoCfg.fps), \
				p->oVideoCfg.width,p->oVideoCfg.height,\
				p->sps[1],p->sps[2],p->sps[3],3); // sps[1] AVCProfileIndication  sps[2] profile_compat sps[3] AVCLevelIndication  
	if (p->video == MP4_INVALID_TRACK_ID)
	{
		//printf("add video track failed.\n");
		//printf("-%d--add video track failed.\n",__LINE__);
		return -1;
	}

	MP4SetVideoProfileLevel(p->hFile, 0x7F); //  Simple Profile @ Level 3
	MP4AddH264SequenceParameterSet(p->hFile, p->video, p->sps,p->spslen);
	MP4AddH264PictureParameterSet(p->hFile, p->video,p->pps,p->ppslen);

	if(p->nNeedAudio )
	{
		/*audio track*/
		p->audio = MP4AddAudioTrack(p->hFile, \
		p->oAudioCfg.nSampleRate, \
		p->oAudioCfg.nInputSamples,\
		MP4_MPEG4_AUDIO_TYPE);
		if (p->audio == MP4_INVALID_TRACK_ID)
		{
			printf("add audio track failed.\n");
			return -1;
		}

		MP4SetAudioProfileLevel(p->hFile, 0x2);

		unsigned char p_config[2] = {0x15, 0x88};
		make_dsi(get_sr_index(p->oAudioCfg.nSampleRate), p->oAudioCfg.nChannels, p_config);
		printf("%X %X\n",p_config[0],p_config[1]);
		MP4SetTrackESConfiguration(p->hFile, p->audio, p_config, 2);

	}		
	return 0;
}



int mp4list_eq(const void* a, const void* b)
{
	const int *s1 = a;
	const Mp4Context *s2 = b;

	if(*s1 == s2->nIndex) return 1;  
	
	return 0;
	
}

void mp4list_print(void* a)
{
	Mp4Context *p = a;
	printf("%d\n",p->nIndex);	
	//	return  0;	
}


//function:create a mp4 header
int cdr_mp4_create(Mp4Context *pMp4Context)
{	
	//create the list.
	if(!g_mp4_list) g_mp4_list = create_list();

	Mp4Context *p = malloc(sizeof(Mp4Context));
	memcpy(p,pMp4Context,sizeof(Mp4Context));
	//p->pFramelist = create_list();
	p->pFrameHead = NULL;
	p->pFrameTail = NULL;
	if(-1 == InitMp4Encoder(p))
	{
		free(p);
		p = NULL;
		return -1;
	}
	p->nIndex = nIndex ++;

	pthread_create(&p->tfid, NULL,&write_frame_to_mp4_pro, p);
	pthread_detach(p->tfid);	
	
	push_front(g_mp4_list,p);	
	
	return p->nIndex;
}

//set the mp4 len.
int cdr_mp4_setlen(int handle,int len)
{
	Mp4Context *p = get_if(g_mp4_list,&handle,mp4list_eq);
	if(p)
	{
		p->nlen = (p->nlen % p->oVideoCfg.fps) + (len * (p->oVideoCfg.fps));
		p->nCutFlag = 1;
	}
	return 0;
}

//H.264 frame format:nal + data(sps pps i p sei)
//AAC   Famer format:adts + data.
int cdr_mp4_write_frame(int handle,MP4_FRAME *pData)
{	
	int nIndex = handle;
	
	//if(size(g_mp4_list) <= 0) return -1;
		
	Mp4Context *p = get_if(g_mp4_list,&nIndex,mp4list_eq);

	if(!p) 
	{
		printf("handle = %d,p is null\n",handle);
		traverse(g_mp4_list,mp4list_print);
		return -1;
	}

	if(p->closeFlag == 1) return -1;

	MP4_FRAME *pDataNode = malloc(sizeof(MP4_FRAME));
	memcpy(pDataNode,pData,sizeof(MP4_FRAME));
	pDataNode->pData = malloc(pData->nlen);
	memcpy(pDataNode->pData,pData->pData,pDataNode->nlen);
	framenode *pNode = malloc(sizeof(framenode));
	pNode->next = NULL;
	pNode->data = (void*)pDataNode;
	
    //若 g_mp4_new[handle].pVideoListLast不为null则写入g_mp4_new[handle].pVideoListLast->next 
    //反之则写入g_mp4_new[handle].pVideoListLast
	if(p->pFrameTail != NULL)
	{
		p->pFrameTail->next = pNode;
	}		
	p->pFrameTail = pNode;


	if(p->pFrameHead == NULL)
	{
		p->pFrameHead = pNode;
	}
	
	return 0;
}

int cdr_mp4_close(int handle)
{
	Mp4Context *p = get_if(g_mp4_list,&handle,mp4list_eq);
	if(p)
	{
		p->closeFlag = 1;
		return 0;
	}
	return -1;
}


//------------------------------------------------------------------------------------------------- Mp4Encode说明
// 【h264编码出的NALU规律】
// 第一帧 SPS【0 0 0 1 0x67】 PPS【0 0 0 1 0x68】 SEI【0 0 0 1 0x6】 IDR【0 0 0 1 0x65】
// p帧      P【0 0 0 1 0x61】
// I帧    SPS【0 0 0 1 0x67】 PPS【0 0 0 1 0x68】 IDR【0 0 0 1 0x65】
// 【mp4v2封装函数MP4WriteSample】
// 此函数接收I/P nalu,该nalu需要用4字节的数据大小头替换原有的起始头，并且数据大小为big-endian格式
//-------------------------------------------------------------------------------------------------
int _Mp4VEncode(Mp4Context* pFd, unsigned char* naluData, int naluSize,int nalutype)
{	
	//printf("%s nalutype:%d,naluSize:%d\r\n",__FUNCTION__,nalutype,naluSize);	
	if (pFd->video == MP4_INVALID_TRACK_ID)
	{
		//有sps的时候.写入数据...创建video strack.
		printf("-%d--add video track failed.\n",__LINE__);
		return -1;
	}

	switch (nalutype)
	{	
	case CDR_H264_NALU_SPS:
		MP4AddH264SequenceParameterSet(pFd->hFile, pFd->video, naluData + 4, naluSize - 4);
		break;
	case CDR_H264_NALU_PPS:
		MP4AddH264PictureParameterSet(pFd->hFile, pFd->video, naluData + 4, naluSize - 4);
		break;
	case CDR_H264_NALU_SEI:
		//SEI帧暂时去掉.没太多用途.
		//printf("sei %d\r\n",naluSize);
		//MP4AddH264PictureParameterSet(pFd->hFile, pFd->video, naluData + 4, naluSize - 4);
		break;
	case CDR_H264_NALU_ISLICE:
	{
		naluData[0] = (naluSize - 4) >> 24;
		naluData[1] = (naluSize - 4) >> 16;
		naluData[2] = (naluSize - 4) >> 8;
		naluData[3] = (naluSize - 4) & 0xff;
	   if (!MP4WriteSample(pFd->hFile, pFd->video, naluData, naluSize, MP4_INVALID_DURATION, 0, 1))
	   {
	   	   printf("%s MP4WriteSample CDR_H264_NALU_ISLICE error\n",__FUNCTION__);
		   return -1;
	   }
	   break;
	}
	case CDR_H264_NALU_PSLICE:
	{
		   naluData[0] = (naluSize - 4) >> 24;
		   naluData[1] = (naluSize - 4) >> 16;
		   naluData[2] = (naluSize - 4) >> 8;
		   naluData[3] = (naluSize - 4) & 0xff;
		   if (!MP4WriteSample(pFd->hFile, pFd->video, naluData, naluSize, MP4_INVALID_DURATION, 0, 0))
		   {
		   	   printf("%s MP4WriteSample CDR_H264_NALU_PSLICE error\n",__FUNCTION__);
			   return -1;
		   }
		   break;
	}
	default:
		break;
	}
	return 0;
}


int _Mp4AEncode(Mp4Context* pFd, unsigned char* aacData, int aacSize)
{
	if (pFd->video == MP4_INVALID_TRACK_ID && pFd->nNeedAudio == 0)
	{
		return -1;
	}
	//printf("_Mp4AEncode :aac size:%d\r\n",aacSize);	
	MP4WriteSample(pFd->hFile, pFd->audio, aacData, aacSize, MP4_INVALID_DURATION, 0, 1);
	return 0;
}


int _write_frame_to_mp4(Mp4Context *pfd,MP4_FRAME *pFrame)
{
	//printf("%s \n",__FUNCTION__);
	if(!pFrame)
		printf("%s  pFrame is null\n",__FUNCTION__);
	
	if(pFrame->streamType == MP4STREAM_VIDEO)
	{
		//MP4 video编码
		return _Mp4VEncode(pfd,pFrame->pData,pFrame->nlen,pFrame->nFrameType);
	}
	else if(pFrame->streamType == MP4STREAM_AUDIO)
	{	
		//MP4 audio编码
		return _Mp4AEncode(pfd,pFrame->pData,pFrame->nlen);
	}
	return -1;
}

int get_mp4_len(MP4FileHandle hFile)
{
	int numSamples = 0;
	if(hFile == NULL)
		return -1;
	
	MP4TrackId trackId = MP4_INVALID_TRACK_ID;
	uint32_t numTracks = MP4GetNumberOfTracks(hFile,NULL,0);
	int i = 0;
	for (i = 0; i < numTracks; i++)
	{

		trackId = MP4FindTrackId(hFile, i,NULL,0);
		const char* trackType = MP4GetTrackType(hFile, trackId);
		if (MP4_IS_VIDEO_TRACK_TYPE(trackType))
		{
			numSamples = MP4GetTrackNumberOfSamples(hFile, trackId);	
		}
	}
	//printf("%s %d\r\n",__FUNCTION__,numSamples);
	return numSamples;
}


void CloseMp4Encoder(Mp4Context *pTmp)
{
	int numSamples = 0;
	if(pTmp->hFile)
	{
		numSamples = get_mp4_len(pTmp->hFile);
		MP4Close(pTmp->hFile, 0);
		pTmp->hFile = NULL;
		char fileName_last[256] = {0x00};
		char fileName_tmp[256] = {0x00};
		memset(fileName_last,0,sizeof(fileName_last));	
		sscanf(pTmp->sName, "%[^.]", fileName_tmp);		
		sprintf(fileName_last, "%s_%03d.MP4",fileName_tmp,numSamples/pTmp->oVideoCfg.fps);
		rename(pTmp->sName,fileName_last);
		printf("[%s]rename %s %s\r\n",__FUNCTION__,pTmp->sName,fileName_last);
		sync();		
	}
}

void FrameFree(void *data)
{
	MP4_FRAME *p = data;
	free(p->pData);
	free(p);
}


void Mp4ItemFree(void *data)
{
	Mp4Context *p = data;
	//empty_list(p->pFramelist,FrameFree);
	//free(p->pFramelist);
	free(p);
}

void* write_frame_to_mp4_pro(void *pParam)
{
	Mp4Context *pTmp = (Mp4Context *)pParam;
	framenode *pVCurrent = NULL;
	MP4_FRAME *pFrame = NULL;
	while(1)
	{
		usleep(10);
		if(pTmp->pFrameHead != NULL)
		{
			pVCurrent = pTmp->pFrameHead;
			pFrame = (MP4_FRAME*)pVCurrent->data;
			_write_frame_to_mp4(pTmp,pFrame);

			pTmp->pFrameHead = pVCurrent->next;

			if(pVCurrent == pTmp->pFrameTail)
			{
				pTmp->pFrameTail = NULL;
			}

			free(pFrame->pData);
			free(pVCurrent->data);
			free(pVCurrent);
			pFrame = NULL;
			pVCurrent = NULL;
		}

		 //找到链表的末端
		if(pTmp->pFrameTail == NULL && pTmp->closeFlag == 1)
		{		
			//这个是为截取mp4 ack响应做的处理.
			//最后需要用回调函数来解决.20160804.LL
			CloseMp4Encoder(pTmp);
			pTmp->outcb(0);
			break;
		}
	}

	return NULL;
}


//以下三个接口用于截取一段MP4
//不需要用新的线程异步写入数据，防止在嵌入式设备上内存爆掉.
int cdr_mp4_create_ex(Mp4Context *pMp4Context)
{
	//create the list.
	if(!g_mp4_list) g_mp4_list = create_list();

	Mp4Context *p = malloc(sizeof(Mp4Context));
	memcpy(p,pMp4Context,sizeof(Mp4Context));
	
	p->oVideoCfg.timeScale = 90000;
	p->oVideoCfg.fps = 30;
	p->oVideoCfg.width = 1920;
	p->oVideoCfg.height = 1080;
	
	if(-1 == InitMp4Encoder(p))
	{
		free(p);
		p = NULL;
		return -1;
	}
	p->nIndex = nIndex ++;	

	push_front(g_mp4_list,p);	
	
	return p->nIndex;
}


//H.264 frame format:nal + data(sps pps i p sei)
//AAC   Famer format:adts + data.
int cdr_mp4_write_frame_ex(int handle,MP4_FRAME *pData)
{	
	int nIndex = handle;
	
	if(size(g_mp4_list) <= 0) return -1;
	
	Mp4Context *p = get_if(g_mp4_list,&nIndex,mp4list_eq);
	
	if(!p) return -1;
	_write_frame_to_mp4(p,pData);
		
	return 0;
}

int cdr_mp4_close_ex(int handle)
{
	Mp4Context *p = get_if(g_mp4_list,&handle,mp4list_eq);
	if(p)
	{
		MP4Close(p->hFile, 0);
		return 0;
	}
	return -1;
}

