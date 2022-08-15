#include "cmsis_os.h"
#include "cmsis.h"
#include "cqueue.h"
#include "hal_trace.h"
#include <string.h>
#include "demo_lib.h"
#include "LibDemo.h"


int demo_example_init(bool on,void *param)
{
	TRACE(0,"demo_example_init");
	return 0;
}

int demo_example_start(bool on,void *param)
{
	TRACE(0,"demo_example_start");
	return 0;
}

int demo_example_stop(bool on,void *param)
{
	TRACE(0,"demo_example_stop");
	return 0;
}


#include "app_thirdparty.h"


THIRDPARTY_HANDLER_TAB(DEMO_LIB_NAME)
{
    {{THIRDPARTY_FUNC_DEMO,THIRDPARTY_ID_DEMO,THIRDPARTY_START},(APP_THIRDPARTY_HANDLE_CB_T)demo_example_start,true,NULL},
    {{THIRDPARTY_FUNC_DEMO,THIRDPARTY_ID_DEMO,THIRDPARTY_STOP},(APP_THIRDPARTY_HANDLE_CB_T)demo_example_stop,true,NULL},
    {{THIRDPARTY_FUNC_DEMO,THIRDPARTY_ID_DEMO,THIRDPARTY_INIT},(APP_THIRDPARTY_HANDLE_CB_T)demo_example_init,true,NULL},
};

THIRDPARTY_HANDLER_TAB_SIZE(DEMO_LIB_NAME)


