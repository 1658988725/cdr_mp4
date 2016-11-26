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

int demuxer_mp4_info(const char *in_filename,Mp4Info *pMp4InfoOut)
{
	int i = 0;
	MP4FileHandle oMp4File;

	if (in_filename == NULL || pMp4InfoOut == NULL)
	{
		printf("%s,%d  in_filename == NULL \r\n", __FUNCTION__, __LINE__);
		return -1;
	}

	memset(pMp4InfoOut, 0, sizeof(Mp4Info));
	memcpy(pMp4InfoOut->oFileName, in_filename, strlen(in_filename));
	
	oMp4File = MP4Read(pMp4InfoOut->oFileName);

	if (!oMp4File)
	{
		printf("Read error....%s\r\n",in_filename);
		return -1;
	}

	MP4TrackId trackId = MP4_INVALID_TRACK_ID;
	uint32_t numTracks = MP4GetNumberOfTracks(oMp4File,NULL,0);
	//printf("numTracks:%d\n",numTracks);
	
	for (i = 0; i < numTracks; i++)
	{

		trackId = MP4FindTrackId(oMp4File, i,NULL,0);
		const char* trackType = MP4GetTrackType(oMp4File, trackId);
		if (MP4_IS_VIDEO_TRACK_TYPE(trackType))
		{
			//printf("[%s %d] trackId:%d\r\n",__FUNCTION__,__LINE__,trackId);
			pMp4InfoOut->videoindex = trackId;

			uint32_t numSamples;
			//MP4SampleId sid;
			//MP4Duration time;
			uint32_t timescale;
			uint64_t duration;			
			duration = MP4GetTrackDuration(oMp4File, trackId );
			numSamples = MP4GetTrackNumberOfSamples(oMp4File, trackId);
			timescale = MP4GetTrackTimeScale(oMp4File, trackId);			
			pMp4InfoOut->oStreamDuration = duration/(timescale/1000);			
			pMp4InfoOut->oFrameCount = numSamples;
			//printf("%s mp4file %s, track %d, samples %d, timescale %d,oStreamDuration:%d\n",__FUNCTION__,in_filename, trackId, numSamples, timescale,orec_file.oStreamDuration);

			// read sps/pps 
			uint8_t **seqheader;
			uint8_t **pictheader;
			uint32_t *pictheadersize;
			uint32_t *seqheadersize;
			uint32_t ix;
			MP4GetTrackH264SeqPictHeaders(oMp4File, trackId, &seqheader, &seqheadersize, &pictheader, &pictheadersize);

			for (ix = 0; seqheadersize[ix] != 0; ix++)
			{
				memcpy(pMp4InfoOut->sps, seqheader[ix], seqheadersize[ix]);
				pMp4InfoOut->spslen = seqheadersize[ix];
				free(seqheader[ix]);
			}
			free(seqheader);
			free(seqheadersize);

			for (ix = 0; pictheadersize[ix] != 0; ix++)
			{
				memcpy(pMp4InfoOut->pps, pictheader[ix], pictheadersize[ix]);
				pMp4InfoOut->ppslen = pictheadersize[ix];
				free(pictheader[ix]);
			}
			free(pictheader);
			free(pictheadersize);
		}
		else if (MP4_IS_AUDIO_TRACK_TYPE(trackType))
		{
			pMp4InfoOut->audioindex = trackId;
		}	
	}
		

	pMp4InfoOut->nframerate = 30;
	MP4Close(oMp4File,0);
	oMp4File = NULL;

	char time[20];
	char *p = strrchr(pMp4InfoOut->oFileName,'/');
	p++;
	
	sscanf(p, "%[0-9]", time); 
	struct tm tm_;    
    strptime(time, "%Y%m%d%H%M%S", &tm_); 
	tm_.tm_isdst = -1;    
    pMp4InfoOut->tmStart  = mktime(&tm_);  	
	pMp4InfoOut->tmEnd = pMp4InfoOut->tmStart + pMp4InfoOut->oStreamDuration/1000;

	return 0;
}
