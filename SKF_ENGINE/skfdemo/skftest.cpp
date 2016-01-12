#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SKFAPI.h"

void PrintError(char *FunName, ULONG ErrorCode, char *Buf=NULL);

ULONG GetDevInfo(DEVHANDLE *phDev);

int main(int argc, char* argv[]){
	DEVHANDLE hDev;
	HAPPLICATION happ;
	ULONG rv;

	rv = GetDevInfo(&hDev);
	if (rv){
		return 0;
	}
	return 0;
}

ULONG GetDevInfo(DEVHANDLE *phDev){
	ULONG rv = 0;
	char *pbDevList = 0;
	ULONG ulDevListLen = 0;

	rv = SKF_EnumDev(1, pbDevList, &ulDevListLen);
	if(rv != SAR_OK){
		PrintError("SKF_EnumDev", rv);
		return rv;
	}

	if(ulDevListLen <2){
		printf("No Device!\n");
		return -1;
	}
	
	pbDevList = (char *)malloc(ulDevListLen);
	if(pbDevList == NULL){
		printf("Memory Error!");
		return -1;
	}
	rv = SKF_EnumDev(1,pbDevList,&ulDevListLen);
	if(rv != SAR_OK){
		PrintError("SKF_EnumDev",rv,pbDevList);
		return rv;		
	}

    char *pp = pbDevList;
	while(pbDevList+ulDevListLen - pp){
		if(strlen(pp)){
			printf("find Device %s\n",pp);
			pp+=strlen(pp);
		}
		else{
			pp++;
		}
	}

	pp = 0;
	
	DEVHANDLE hDev;
	rv = SKF_ConnectDev(pbDevList,&hDev);
	if(rv){
		PrintError("SKF_ConnectDev",rv,pbDevList);
		return rv;	
	}
	printf("Connect Device %s\n",pbDevList);
    *phDev = hDev;

	if(pbDevList)
		free(pbDevList);

	return SAR_OK;
}

void PrintError(char *szFunName,ULONG dwErrorCode,char *Buf)
{

	printf("the Fun %s failed! the ErrorCode is 0x%0x\n",szFunName,dwErrorCode);
	if(Buf)
	{
		free(Buf);
		Buf = NULL;
	
	}
}