// Tomer Elmalem
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include "../libutility/libutility.h"
#include "../libmp4dir/mp4info.h"


int aastreamout(void *data)
{
	printf("%s finish \n",(char *)data);
	return 0;
}

int main(void) 
{
	cdr_init_mp4dir(MP4DIRPATH);	
	getchar();
	getchar();
	while(1)
	{
		cdr_mp4ex_read_vframe(NULL,NULL,NULL);
		usleep(10000);
	}	
	cdr_mp4dirlist_free();			
	return 0;
}
