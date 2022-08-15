#include "cmsis_os.h"
#include "hal_trace.h"
#include "app_ai_if_gsound.h"


#ifdef BISTO_ENABLED
#include "gsound_custom_service.h"
#endif

void app_ai_if_gsound_service_enable_switch(bool onOff)
{
#ifdef BISTO_ENABLED
    gsound_service_enable_switch(onOff);
#endif
}

