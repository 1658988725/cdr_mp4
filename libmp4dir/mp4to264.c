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


unsigned char sps[64],pps[64];
int spslen = 0,ppslen = 0;

int GetH264(MP4FileHandle oMp4File,int VTrackId,int totalFrame)
{
	if(!oMp4File) return -1;

	char NAL[5] = {0x00,0x00,0x00,0x01};	

	unsigned char *pData = NULL;

	unsigned int nSize = 0;
	MP4Timestamp pStartTime;
	MP4Duration pDuration;
	MP4Duration pRenderingOffset;
	bool pIsSyncSample = 0;

	int nReadIndex = 0;
	FILE *pFile = NULL;
	pFile = fopen("out.h264","wb");	

	while(nReadIndex < totalFrame)
	{	
		nReadIndex ++;
		//printf("nReadIndex:%d\n",nReadIndex);
		MP4ReadSample(oMp4File,VTrackId,nReadIndex,&pData,&nSize,&pStartTime,&pDuration,&pRenderingOffset,&pIsSyncSample);

		//IDR帧.I帧之前写入sps pps信息.
		if(pIsSyncSample)
		{
			fwrite(NAL,4,1,pFile);
			fwrite(sps,spslen,1,pFile);
			
			fwrite(NAL,4,1,pFile);
			fwrite(pps,ppslen,1,pFile);
												
		}
		//写入frame数据.
		if(pData && nSize > 4)
		{
			//标准的264数据，前面是个字节代表一个frame的长度,去掉0x00 0x00 0x00 0x01
			pData[0] = 0x00;
			pData[1] = 0x00;
			pData[2] = 0x00;
			pData[3] = 0x01;

			fwrite(pData,nSize,1,pFile);
		}
		
		free(pData);
		pData = NULL;
	}		
	fflush(pFile);
	fclose(pFile);	
	
	return 0;
}
int GetMp4info(char *sMp4file)
{
	MP4FileHandle oMp4File;
	int i;

	unsigned int oStreamDuration,oFrameCount;
	oMp4File = MP4Read(sMp4file);
	int videoindex = -1,audioindex = -1;
	uint32_t numSamples;
	uint32_t timescale;
	uint64_t duration;		
	
	if (!oMp4File)
	{
		printf("Read error....%s\r\n",sMp4file);
		return -1;
	}

	MP4TrackId trackId = MP4_INVALID_TRACK_ID;
	uint32_t numTracks = MP4GetNumberOfTracks(oMp4File,NULL,0);
	printf("numTracks:%d\n",numTracks);
	
	for (i = 0; i < numTracks; i++)
	{
		trackId = MP4FindTrackId(oMp4File, i,NULL,0);
		const char* trackType = MP4GetTrackType(oMp4File, trackId);
		if (MP4_IS_VIDEO_TRACK_TYPE(trackType))
		{
			//printf("[%s %d] trackId:%d\r\n",__FUNCTION__,__LINE__,trackId);
			videoindex= trackId;
	
			duration = MP4GetTrackDuration(oMp4File, trackId );
			numSamples = MP4GetTrackNumberOfSamples(oMp4File, trackId);
			timescale = MP4GetTrackTimeScale(oMp4File, trackId);			
			oStreamDuration = duration/(timescale/1000);			
			oFrameCount = numSamples;
		
			// read sps/pps 
			uint8_t **seqheader;
			uint8_t **pictheader;
			uint32_t *pictheadersize;
			uint32_t *seqheadersize;
			uint32_t ix;
			MP4GetTrackH264SeqPictHeaders(oMp4File, trackId, &seqheader, &seqheadersize, &pictheader, &pictheadersize);

			for (ix = 0; seqheadersize[ix] != 0; ix++)
			{
				memcpy(sps, seqheader[ix], seqheadersize[ix]);
				spslen = seqheadersize[ix];
				free(seqheader[ix]);
			}
			free(seqheader);
			free(seqheadersize);

			for (ix = 0; pictheadersize[ix] != 0; ix++)
			{
				memcpy(pps, pictheader[ix], pictheadersize[ix]);
				ppslen = pictheadersize[ix];
				free(pictheader[ix]);
			}
		}
		else if (MP4_IS_AUDIO_TRACK_TYPE(trackType))
		{
			audioindex = trackId;
		}	
	}

	MP4Close(oMp4File,0);	

	return 0;
}


int main()
{
	GetMp4info();
	return 0;
}

