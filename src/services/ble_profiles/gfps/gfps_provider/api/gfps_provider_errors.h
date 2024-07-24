#ifndef _GFPS_PROVIDER_ERRORS_H_
#define _GFPS_PROVIDER_ERRORS_H_

enum{
GFPS_SUCCESS,
GFPS_ERROR_EXEC_FAIL,
GFPS_ERROR_NULL,
GFPS_ERROR_INVALID_PARAM,
GFPS_ERROR_DATA_SIZE    
};
#define Gfps_CheckParm(code, exp) if (!(exp)) return (code)

#endif
