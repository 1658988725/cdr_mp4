#ifndef _MP4INFO_H
#define _MP4INFO_H

// Tomer Elmalem
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include "../libutility/libutility.h"
#include <mp4v2/mp4v2.h>

//#define MP4DIRPATH "/home/ll/work2016/study/mp4dirapp/res"
//#define MP4CUTOUTPATH "/home/ll/work2016/study/cdr_app"

#define MP4DIRPATH "/mnt/mmc/VIDEO"
#define MP4CUTOUTPATH "/mnt/mmc/tmp"

typedef int (*stream_out_cb)(const void*);

typedef struct _mp4Info 
{
	char oFileName[256];	// 文件名字	
	unsigned long  oStreamDuration;//视频文件长度.-ms
	time_t	tmStart;//begin time of mp4.
	time_t  tmEnd;  //endtime of mp4.		
	unsigned char pps[64];
	unsigned char sps[64];
	int ppslen;
	int spslen;
	int nframerate;	
	int videoindex;
	int audioindex;	
	unsigned short oFrameCount;	
}Mp4Info;

typedef struct _mp4ReadInfo
{
	char 	chName[256];
	time_t	tmStart;
	time_t	tmEnd;
	int nTotalLen;
	int VReadIndex;
	int AReadIndex;
	stream_out_cb pStreamcb;
	MP4FileHandle oMp4File;
	Mp4Info *oMp4Info;
}Mp4ReadInfo;

typedef struct streamout
{
	int streamType;
	int type_ex;
	unsigned char *pData;
	unsigned int size;	
}StreamOut;



#define VIDEO_TYPE_SPS 	7
#define VIDEO_TYPE_PPS 	8
#define VIDEO_TYPE_IF 	5
#define VIDEO_TYPE_PF 	1

#define STREAM_TYPE_V 	0
#define STREAM_TYPE_A 	1
#define STREAM_TYPE_END 2
#define STREAM_TYPE_START 3


int demuxer_mp4_info(const char *in_filename,Mp4Info *pMp4InfoOut);
int cdr_parsemp4dir(char *path);
int cdr_read_mp4_ex(char *pTm,int len,stream_out_cb pstreamout);
int cdr_mp4dirlist_free(void);

int cdr_init_mp4dir(char *path);



#define MP4STREAM_VIDEO 0
#define MP4STREAM_AUDIO 1

typedef struct mp4_frame
{
	int streamType;
	int nFrameType;
	//int nIFrame;
	unsigned long long uPTS;
	int nlen;	
	unsigned char* pData;
}MP4_FRAME;


#define CDR_H264_NALU_PSLICE 1
#define CDR_H264_NALU_ISLICE 5
#define CDR_H264_NALU_SEI 6
#define CDR_H264_NALU_SPS 7
#define CDR_H264_NALU_PPS 8
#define CDR_H264_NALU_IPSLICE 9


typedef int (*newmp4_out_cb)(int);

typedef struct _Mp4v2_AConfig
{
	unsigned long nSampleRate;     	//音频采样数
	unsigned int nChannels;         //音频声道数
	unsigned int nPCMBitSize;       //音频采样精度
	unsigned long nInputSamples;    //每次调用编码时所应接收的原始数据长度
	unsigned long nMaxOutputBytes;  //每次调用编码时生成的AAC数据的最大长度
	unsigned char* pbAACBuffer;     //aac数据
}MP4V2_ACONFIG;

typedef struct _Mp4v2_VConfig
{
	unsigned int timeScale;        //视频每秒的ticks数,如90000
	unsigned int fps;              //视频帧率
	unsigned short width;          //视频宽
	unsigned short height;         //视频高
}MP4V2_VCONFIG;

typedef struct mp4Context
{
	int nIndex;
	char sName[256];		
	int  nlen;
	int nCutFlag;
	int closeFlag;
	int  nNeedAudio;
	unsigned char sps[64];//sps with out nal
	unsigned char pps[64];
	int spslen;
	int ppslen;
	list *pFramelist;
	MP4FileHandle hFile;          //mp4文件描述符
	MP4TrackId video;              //视频轨道标志符
	MP4TrackId audio;              //音频轨道标志符
	
	MP4V2_ACONFIG oAudioCfg;
	MP4V2_VCONFIG oVideoCfg;	
	newmp4_out_cb outcb;
	pthread_t tfid;
}Mp4Context;

int cdr_mp4_create(Mp4Context *pMp4Context);
int cdr_mp4_write_frame(int handle,MP4_FRAME *pData);
int cdr_mp4_close(int handle);



int cdr_mp4_create_ex(Mp4Context *pMp4Context);
int cdr_mp4_write_frame_ex(int handle,MP4_FRAME *pData);
int cdr_mp4_close_ex(int handle);
int cdr_mp4ex_read_vframe(char **pFrameData,int *nLen,int *IFrame);


typedef struct _mp4_dir_info
{
	list *fileList; //file list.
	list *cutList;  //file cut list.
	int nThreadStart;
	char chDir[256];
	pthread_t tCutfid;
	pthread_t tDemuxerfid;	
}mp4_dir_info;


#endif
