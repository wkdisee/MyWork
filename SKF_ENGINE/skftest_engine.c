/* study with the openssl engine source file engine/e_chil.c
*/
#include <stdio.h>
#include <string.h>
#include <openssl/crypto.h>
#include <openssl/engine.h>
#include "SKFAPI.h"

static const char *engine_hwskf_id = "skfdev";
static const char *engine_hwskf_name = "hardware using skf API";

int hwskf_enum(){
	ULONG rv = 0;
	char *pbDevList = 0;
	ULONG ulDevListLen = 0;

	rv = SKF_EnumDev(1, pbDevList, &ulDevListLen);
	if(rv != SAR_OK){
		fprintf(stderr, "SKF_EnumDev error with code:%d", rv);
		return rv;
	}
	if (ulDevListLen < 2){
		printf("No Device!\n");
		return -1;
	}
	pbDevList = (char *)malloc(ulDevListLen);
	rv = SKF_EnumDev(1, pbDevList, &ulDevListLen);
	char *pp = pbDevList;
	while (pbDevList+ulDevListLen - pp){
		if(strlen(pp)){
			printf("find Device %s\n", pp);
			pp += strlen(pp);
		}else{
			pp ++;
		}
	}
	return rv;
}

#define HWSKF_CMD_ENUM  (ENGINE_CMD_BASE)
static const ENGINE_CMD_DEFN hwskf_cmd_defns[] = {
	{HWSKF_CMD_ENUM,
		"ENUM",
		"enum the all skf devices connected",
		ENGINE_CMD_FLAG_NO_INPUT}, /* enumerate the devices of skf */
	{0, NULL, NULL, 0}
	};

static int hwskf_ctrl(ENGINE *e, int cmd, long i, void *p, void (*f)(void))
{
	int to_return = 1;

	switch(cmd) {
	case HWSKF_CMD_ENUM:
        fprintf(stderr, "arrive at HWSKF_CMD_ENUM.\n");
        hwskf_enum();
        break;
	/* The command isn't understood by this engine */
	default:
		to_return = 0;
		break;
	}

	return to_return;
}

static int bind_helper(ENGINE *e)
{
    fprintf(stderr, "arrive at bind_helper\n");

	if(!ENGINE_set_id(e, engine_hwskf_id) ||
			!ENGINE_set_name(e, engine_hwskf_name) ||
			!ENGINE_set_ctrl_function(e, hwskf_ctrl) ||
			!ENGINE_set_cmd_defns(e, hwskf_cmd_defns))
		return 0;

	return 1;
}
static int bind_fn(ENGINE *e, const char *id)
{
    fprintf(stderr, "arrive at bind_fn\n");
	/*if(id && (strcmp(id, engine_hwskf_id) != 0) &&
			(strcmp(id, engine_hwskf_id_alt) != 0))
		return 0;*/
	if(!bind_helper(e))
		return 0;
	return 1;
}       
IMPLEMENT_DYNAMIC_CHECK_FN()
IMPLEMENT_DYNAMIC_BIND_FN(bind_fn)