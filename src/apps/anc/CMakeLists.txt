set (sources "./src/anc_wnr.c"
             "./src/app_anc.c"
             "./src/anc_assist.c"
             "./src/peak_detector.c")

set (headers "./inc/anc_wnr.h"
             "./inc/app_anc.h"
             "./inc/anc_assist.h"
             "./inc/peak_detector.h")

add_library (apps_anc STATIC ${sources} ${headers})
target_link_libraries (apps_anc PUBLIC apps_common)
