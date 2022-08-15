
/* -----------------------------------------------------------------------------------------------------------
Software License for The Third-Party Modified Version of the Fraunhofer FDK AAC Codec Library for Android

© Copyright  1995 - 2013 Fraunhofer-Gesellschaft zur Förderung der angewandten Forschung e.V.
  All rights reserved.

 1.    INTRODUCTION
The Third-Party Modified Version of the Fraunhofer FDK AAC Codec Library for Android ("FDK AAC Codec") is software that implements
the MPEG Advanced Audio Coding ("AAC") encoding and decoding scheme for digital audio.
This FDK AAC Codec software is intended to be used on a wide variety of Android devices.

AAC's HE-AAC and HE-AAC v2 versions are regarded as today's most efficient general perceptual
audio codecs. AAC-ELD is considered the best-performing full-bandwidth communications codec by
independent studies and is widely deployed. AAC has been standardized by ISO and IEC as part
of the MPEG specifications.

Patent licenses for necessary patent claims for the FDK AAC Codec (including those of Fraunhofer)
may be obtained through Via Licensing (www.vialicensing.com) or through the respective patent owners
individually for the purpose of encoding or decoding bit streams in products that are compliant with
the ISO/IEC MPEG audio standards. Please note that most manufacturers of Android devices already license
these patent claims through Via Licensing or directly from the patent owners, and therefore FDK AAC Codec
software may already be covered under those patent licenses when it is used for those licensed purposes only.

Commercially-licensed AAC software libraries, including floating-point versions with enhanced sound quality,
are also available from Fraunhofer. Users are encouraged to check the Fraunhofer website for additional
applications information and documentation.

2.    COPYRIGHT LICENSE

Redistribution and use in source and binary forms, with or without modification, are permitted without
payment of copyright license fees provided that you satisfy the following conditions:

You must retain the complete text of this software license in redistributions of the FDK AAC Codec or
your modifications thereto in source code form.

You must retain the complete text of this software license in the documentation and/or other materials
provided with redistributions of the FDK AAC Codec or your modifications thereto in binary form.
You must make available free of charge copies of the complete source code of the FDK AAC Codec and your
modifications thereto to recipients of copies in binary form.

The name of Fraunhofer may not be used to endorse or promote products derived from this library without
prior written permission.

You may not charge copyright license fees for anyone to use, copy or distribute the FDK AAC Codec
software or your modifications thereto.

Your modified versions of the FDK AAC Codec must carry prominent notices stating that you changed the software
and the date of any change. For modified versions of the FDK AAC Codec, the term
"Fraunhofer FDK AAC Codec Library for Android" must be replaced by the term
"Third-Party Modified Version of the Fraunhofer FDK AAC Codec Library for Android."

3.    NO PATENT LICENSE

NO EXPRESS OR IMPLIED LICENSES TO ANY PATENT CLAIMS, including without limitation the patents of Fraunhofer,
ARE GRANTED BY THIS SOFTWARE LICENSE. Fraunhofer provides no warranty of patent non-infringement with
respect to this software.

You may use this FDK AAC Codec software or modifications thereto only for purposes that are authorized
by appropriate patent licenses.

4.    DISCLAIMER

This FDK AAC Codec software is provided by Fraunhofer on behalf of the copyright holders and contributors
"AS IS" and WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES, including but not limited to the implied warranties
of merchantability and fitness for a particular purpose. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
CONTRIBUTORS BE LIABLE for any direct, indirect, incidental, special, exemplary, or consequential damages,
including but not limited to procurement of substitute goods or services; loss of use, data, or profits,
or business interruption, however caused and on any theory of liability, whether in contract, strict
liability, or tort (including negligence), arising in any way out of the use of this software, even if
advised of the possibility of such damage.

5.    CONTACT INFORMATION

Fraunhofer Institute for Integrated Circuits IIS
Attention: Audio and Multimedia Departments - FDK AAC LL
Am Wolfsmantel 33
91058 Erlangen, Germany

www.iis.fraunhofer.de/amm
amm-info@iis.fraunhofer.de
----------------------------------------------------------------------------------------------------------- */

/**************************  Fraunhofer IIS FDK SysLib  **********************

   Author(s):

******************************************************************************/

/** \file   genericStds.h
    \brief  Generic Run-Time Support function wrappers and heap allocation monitoring.
 */

#if !defined(__GENERICSTDS_H__)
#define __GENERICSTDS_H__

#include "machine_type.h"


/* Always increase verbosity of memory allocation in case of a debug built. DEBUG is defined globally in that case. */
#if defined(DEBUG) || defined(FDK_DEBUG)
//#define MEMORY_MEASUREMENT
#endif

#ifndef M_PI
  #define M_PI   3.14159265358979323846  /*! Pi. Only used in example projects. */
#endif


/* #define _CRT_SECURE_NO_DEPRECATE */


/**
 * Identifiers for various memory locations. They are used along with memory allocation
 * functions like FDKcalloc_L() to specify the requested memory's location.
 */
typedef enum {
  /* Internal */
  SECT_DATA_L1 = 0x2000,
  SECT_DATA_L2,
  SECT_DATA_L1_A,
  SECT_DATA_L1_B,
  SECT_CONSTDATA_L1,

  /* External */
  SECT_DATA_EXTERN = 0x4000,
  SECT_CONSTDATA_EXTERN

} MEMORY_SECTION;


/**
 * The H_ prefix indicates header file version, the C_* prefix indicates the corresponding
 * object version.
 *
 * Declaring memory areas requires to specify a unique name and a data type. Use the H_ macro
 * for this purpose inside a header file.
 *
 * For defining a memory area your require additionally one or two sizes, depending if the
 * memory should be organized into one or two dimensions.
 *
 * The macros containing the keyword AALLOC instead of ALLOC also do take care of returning
 * aligned memory addresses (beyond the natural alignment of its type). The preprocesor macro
 * ::ALIGNMENT_DEFAULT indicates the aligment to be used (this is hardware specific).
 *
 * The _L suffix indicates that the memory will be located in a specific section. This is
 * useful to allocate critical memory section into fast internal SRAM for example.
 *
 */

#define H_ALLOC_MEM(name,type)          type * Get ## name(int n=0); void Free ## name(type** p); \
                                        UINT GetRequiredMem ## name(void);

/** See #H_ALLOC_MEM for description. */
#define H_ALLOC_MEM_OVERLAY(name,type)  type * Get ## name(int n=0); void Free ## name(type** p); \
                                        UINT GetRequiredMem ## name(void);


  /** See #H_ALLOC_MEM for description. */
  #define C_ALLOC_MEM(name,type,num) \
    type * Get ## name(int n) { FDK_ASSERT((n) == 0); return ((type*)FDKcalloc(num, sizeof(type))); } \
    void Free ## name(type** p) { if (p != NULL) { FDKfree(*p); *p=NULL; } } \
    UINT GetRequiredMem ## name(void) { return ALGN_SIZE_EXTRES((num) * sizeof(type)); }

  /** See #H_ALLOC_MEM for description. */
  #define C_ALLOC_MEM_STATIC(name,type,num) \
    static type * Get ## name(int n) { FDK_ASSERT((n) == 0); return ((type*)FDKcalloc(num, sizeof(type))); } \
    static void Free ## name(type** p) { if (p != NULL) { FDKfree(*p); *p=NULL; } } \
    static UINT GetRequiredMem ## name(void) { return ALGN_SIZE_EXTRES((num) * sizeof(type)); }

  /** See #H_ALLOC_MEM for description. */
  #define C_ALLOC_MEM2(name,type,n1,n2) \
    type * Get ## name (int n) { FDK_ASSERT((n) < (n2)); return ((type*)FDKcalloc(n1, sizeof(type))); } \
    void Free ## name(type** p) { if (p != NULL) { FDKfree(*p); *p=NULL; } } \
    UINT GetRequiredMem ## name(void) { return ALGN_SIZE_EXTRES((n1) * sizeof(type)) * (n2); }

  /** See #H_ALLOC_MEM for description. */
  #define C_AALLOC_MEM(name,type,num) \
    type * Get ## name(int n) { FDK_ASSERT((n) == 0); return ((type*)FDKaalloc((num)*sizeof(type), ALIGNMENT_DEFAULT)); } \
    void Free ## name(type** p) { if (p != NULL) { FDKafree(*p); *p=NULL; } } \
    UINT GetRequiredMem ## name(void) { return ALGN_SIZE_EXTRES((num) * sizeof(type) + ALIGNMENT_DEFAULT + sizeof(void *)); }

  /** See #H_ALLOC_MEM for description. */
  #define C_AALLOC_MEM2(name,type,n1,n2) \
    type * Get ## name (int n) { FDK_ASSERT((n) < (n2)); return ((type*)FDKaalloc((n1)*sizeof(type), ALIGNMENT_DEFAULT)); } \
    void Free ## name(type** p) { if (p != NULL) { FDKafree(*p); *p=NULL; } } \
    UINT GetRequiredMem ## name(void) { return ALGN_SIZE_EXTRES((n1) * sizeof(type) + ALIGNMENT_DEFAULT + sizeof(void *)) * (n2); }

  /** See #H_ALLOC_MEM for description. */
  #define C_ALLOC_MEM_L(name,type,num,s) \
    type * Get ## name(int n) { FDK_ASSERT((n) == 0); return ((type*)FDKcalloc_L(num, sizeof(type), s)); } \
    void Free ## name(type** p) { if (p != NULL) { FDKfree_L(*p); *p=NULL; } } \
    UINT GetRequiredMem ## name(void) { return ALGN_SIZE_EXTRES((num) * sizeof(type)); }

  /** See #H_ALLOC_MEM for description. */
  #define C_ALLOC_MEM2_L(name,type,n1,n2,s) \
    type * Get ## name (int n) { FDK_ASSERT((n) < (n2)); return (type*)FDKcalloc_L(n1, sizeof(type), s); } \
    void Free ## name(type** p) { if (p != NULL) { FDKfree_L(*p); *p=NULL; } } \
    UINT GetRequiredMem ## name(void) { return ALGN_SIZE_EXTRES((n1) * sizeof(type)) * (n2); }

  /** See #H_ALLOC_MEM for description. */
  #define C_AALLOC_MEM_L(name,type,num,s) \
    type * Get ## name(int n) { FDK_ASSERT((n) == 0); return ((type*)FDKaalloc_L((num)*sizeof(type), ALIGNMENT_DEFAULT, s)); } \
    void Free ## name(type** p) { if (p != NULL) { FDKafree_L(*p); *p=NULL; } } \
    UINT GetRequiredMem ## name(void) { return ALGN_SIZE_EXTRES((num) * sizeof(type) + ALIGNMENT_DEFAULT + sizeof(void *)); }

  /** See #H_ALLOC_MEM for description. */
  #define C_AALLOC_MEM2_L(name,type,n1,n2,s) \
    type * Get ## name (int n) { FDK_ASSERT((n) < (n2)); return ((type*)FDKaalloc_L((n1)*sizeof(type), ALIGNMENT_DEFAULT, s)); } \
    void Free ## name(type** p) { if (p != NULL) { FDKafree_L(*p); *p=NULL; } } \
    UINT GetRequiredMem ## name(void) { return ALGN_SIZE_EXTRES((n1) * sizeof(type) + ALIGNMENT_DEFAULT + sizeof(void *)) * (n2); }

/** See #H_ALLOC_MEM_OVERLAY for description. */


  #define C_ALLOC_MEM_OVERLAY(name,type,num,sect,tag) C_AALLOC_MEM_L(name,type,num,sect)


   #define C_AALLOC_SCRATCH_START(name,type,n) \
     type _ ## name[(n)+(ALIGNMENT_DEFAULT+sizeof(type)-1)]; \
     type * name = (type*)ALIGN_PTR(_ ## name); \

   #define C_ALLOC_SCRATCH_START(name,type,n) \
     type name[n];

   #define C_AALLOC_SCRATCH_END(name,type,n)
   #define C_ALLOC_SCRATCH_END(name,type,n)


/*--------------------------------------------
 * Runtime support declarations
 *---------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif

#define USE_BUILTIN_STRING_FUNCTIONS
#include "hal_trace.h"

#define FDKprintf(num,fmt, ...) hal_trace_printf(num,fmt,##__VA_ARGS__)
#define FDKprintfErr(num,fmt,...) hal_trace_printf(num,fmt,##__VA_ARGS__)
#define FDKfprintf(stream, fmt, ...) hal_trace_printf(0,fmt, ##__VA_ARGS__)
#define FDKsprintf(str, fmt, ...) hal_trace_printf(0,fmt, ##__VA_ARGS__)
#if 0
/** printf() using stdout. If ::ARCH_WA_FLUSH_CONSOLE defined, a flush is  done additionally after printf(). */
void FDKprintf    ( const char* szFmt, ...);

/** printf() using stderr. If ::ARCH_WA_FLUSH_CONSOLE defined, a flush is done additionally after printf(). */
void FDKprintfErr ( const char* szFmt, ...);

/** Wrapper for <stdio.h>'s getchar(). */
int FDKgetchar(void);

INT  FDKfprintf(void  *stream,  const  char *format, ...);
INT  FDKsprintf(char *str, const char *format, ...);
#endif


const char *FDKstrchr(const char *s, INT c);
const char *FDKstrstr(const char *haystack, const char *needle);
char *FDKstrcpy(char *dest, const char *src);
char *FDKstrncpy(char *dest, const char *src, const UINT n);

#define FDK_MAX_OVERLAYS 8   /**< Maximum number of memory overlays. */


void *FDKcalloc (const UINT n, const UINT size);
void *FDKmalloc (const UINT size);
void  FDKfree   (void *ptr);

/**
 *  Allocate and clear an aligned memory area. Use FDKafree() instead of FDKfree() for these memory areas.
 *
 * \param size       Size of requested memory in bytes.
 * \param alignment  Alignment of requested memory in bytes.
 * \return           Pointer to allocated memory.
 */
void *FDKaalloc (const UINT size, const UINT alignment);

/**
 *  Free an aligned memory area.
 *
 * \param ptr  Pointer to be freed.
 * \return     void
 */
void FDKafree (void *ptr);


/**
 *  Allocate memory in a specific memory section.
 *  Requests can be made for internal or external memory. If internal memory is
 *  requested, FDKcalloc_L() first tries to use L1 memory, which sizes are defined
 *  by ::DATA_L1_A_SIZE and ::DATA_L1_B_SIZE. If no L1 memory is available, then
 *  FDKcalloc_L() tries to use L2 memory. If that fails as well, the requested
 *  memory is allocated at an extern location using the fallback FDKcalloc().
 *
 * \param n     See MSDN documentation on calloc().
 * \param size  See MSDN documentation on calloc().
 * \param s     Memory section.
 * \return      See MSDN documentation on calloc().
 */
void *FDKcalloc_L(const UINT n, const UINT size, MEMORY_SECTION s);

/**
 *  Allocate aligned memory in a specific memory section.
 *  See FDKcalloc_L() description for details - same applies here.
 */
void *FDKaalloc_L(const UINT size, const UINT alignment, MEMORY_SECTION s);

/**
 *  Free memory that was allocated in a specific memory section.
 */
void  FDKfree_L(void *ptr);

/**
 *  Free aligned memory that was allocated in a specific memory section.
 */
void  FDKafree_L(void *ptr);

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

/*---------------------------------------------------------------------------------------
 * FUNCTION:    FDKmemcpy
 * DESCRIPTION: - copies memory from "src" to "dst" with length "size" bytes
 *              - compiled with FDK_DEBUG will give you warnings
 *---------------------------------------------------------------------------------------*/
inline void FDKmemcpy(void *dst, const void *src, const UINT size)
{

  /* do the copy */
  memcpy(dst, src, size);
}

inline void FDKmemmove(void *dst, const void *src, const UINT size)     { memmove(dst, src, size); }
inline void FDKmemset(void *memPtr, const INT value, const UINT size)   { memset(memPtr, value, size); }
inline void FDKmemclear(void *memPtr, const UINT size)                  { FDKmemset(memPtr,0,size); }
inline UINT FDKstrlen(const char *s)                                    { return (UINT)strlen(s); }

/* Compare function wrappers */
inline INT FDKmemcmp(const void *s1, const void *s2, const UINT size)  { return memcmp(s1, s2, size); }
inline INT FDKstrcmp(const char *s1, const char *s2)                   { return strcmp(s1, s2); }
inline INT FDKstrncmp(const char *s1, const char *s2, const UINT size) { return strncmp(s1, s2, size); }


/* Math function wrappers. Only intended for compatibility, not to be highly optimized. */

inline INT FDKabs(INT j) { return abs(j); }
inline double FDKfabs(double x) { return fabs(x); }
inline double FDKpow(double x, double y) { return pow(x,y); }
inline double FDKsqrt(double x) { return sqrt(x); }
inline double FDKatan(double x) { return atan(x); }
inline double FDKlog(double x) { return log(x); }
inline double FDKsin(double x) { return sin(x); }
inline double FDKcos(double x) { return cos(x); }
inline double FDKexp(double x) { return exp(x); }
inline double FDKatan2(double y, double x) { return atan2(y, x); }
inline double FDKacos(double x) { return acos(x); }
inline double FDKtan(double x) { return tan(x); }
inline double FDKfloor(double x) { return floor(x); }
inline double FDKceil(double x) { return ceil(x); }

inline INT   FDKatoi(const char *nptr) { return atoi(nptr); }
inline long  FDKatol(const char *nptr) { return atol(nptr); }
inline float FDKatof(const char *nptr) { return (float)atof(nptr); }


#define FDKmax(a,b) ( (a) > (b) ? (a):(b))
#define FDKmin(a,b) ( (a) < (b) ? (a):(b))

#define FDK_INT_MAX ((INT)0x7FFFFFFF)
#define FDK_INT_MIN ((INT)0x80000000)

/* Math function wrappers. Only intended for compatibility, not to be highly optimized. */
/* Used for debugging, dev code .. */

#define FDKlog2(a) (FDKlog(a)*1.442695041) /* log(2.0) = 1.442695041 */
#define FDKlog10(a) (FDKlog(a)*0.434294482) /* 1.0/log(10.0) = 0.434294482 */
/* LONG LONG FDKatoll(const char *nptr); */
/* LONG LONG FDKatoq(const char *nptr); */




/* FILE I/O */

/*!
 *  Check platform for endianess.
 *
 * \return  1 if platform is little endian, non-1 if platform is big endian.
 */
#ifdef __cplusplus
inline
#else
static
#endif
int IS_LITTLE_ENDIAN(void) {
  int __dummy = 1;
  return ( *( (UCHAR*)(&(__dummy) ) ) );
}

/*!
 *  Convert input value to little endian format.
 *
 * \param val  Value to be converted. It may be in both big or little endian.
 * \return     Value in little endian format.
 */
#define TO_LITTLE_ENDIAN(val) \
  ( (IS_LITTLE_ENDIAN()) ? \
     (val) \
   : ( (((val) & 0xff) << 24) || (((val) & 0xff00)<< 8) || (((val) & 0xff0000)>>8) || (((val) & 0xff000000) >> 24) ) )


/*!
 * \fn     FDKFILE *FDKfopen(const char *filename, const char *mode);
 *         Standard fopen() wrapper.
 * \fn     INT FDKfclose(FDKFILE *FP);
 *         Standard fclose() wrapper.
 * \fn     INT FDKfseek(FDKFILE *FP, LONG OFFSET, int WHENCE);
 *         Standard fseek() wrapper.
 * \fn     INT FDKftell(FDKFILE *FP);
 *         Standard ftell() wrapper.
 * \fn     INT FDKfflush(FDKFILE *fp);
 *         Standard fflush() wrapper.
 * \fn     UINT FDKfwrite(void *ptrf, INT size, UINT nmemb, FDKFILE *fp);
 *         Standard fwrite() wrapper.
 * \fn     UINT FDKfread(void *dst, INT size, UINT nmemb, FDKFILE *fp);
 *         Standard fread() wrapper.
 */
typedef void FDKFILE;
extern const INT FDKSEEK_SET, FDKSEEK_CUR, FDKSEEK_END;

FDKFILE *FDKfopen(const char *filename, const char *mode);
INT FDKfclose(FDKFILE *FP);
INT FDKfseek(FDKFILE *FP, LONG OFFSET, int WHENCE);
INT FDKftell(FDKFILE *FP);
INT FDKfflush(FDKFILE *fp);
UINT FDKfwrite(void *ptrf, INT size, UINT nmemb, FDKFILE *fp);
UINT FDKfread(void *dst, INT size, UINT nmemb, FDKFILE *fp);
char* FDKfgets(void *dst, INT size, FDKFILE *fp);
void FDKrewind(FDKFILE *fp);
INT FDKfeof(FDKFILE *fp);

/**
 * \brief        Write each member in little endian order. Convert automatically to host endianess.
 * \param ptrf   Pointer to memory where to read data from.
 * \param size   Size of each item to be written.
 * \param nmemb  Number of items to be written.
 * \param fp     File pointer of type FDKFILE.
 * \return       Number of items read on success and fread() error on failure.
 */
UINT FDKfwrite_EL(void *ptrf, INT size, UINT nmemb, FDKFILE *fp);

/**
 * \brief        Read variable of size "size" as little endian. Convert automatically to host endianess.
 *                4-byte alignment is enforced for 24 bit data, at 32 bit full scale.
 * \param dst    Pointer to memory where to store data into.
 * \param size   Size of each item to be read.
 * \param nmemb  Number of items to be read.
 * \param fp     File pointer of type FDKFILE.
 * \return       Number of items read on success and fread() error on failure.
 */
UINT FDKfread_EL(void *dst, INT size, UINT nmemb, FDKFILE *fp);


/**
 * \brief  Print FDK software disclaimer.
 */
void FDKprintDisclaimer(void);

#ifdef __cplusplus
}
#endif

#endif /* __GENERICSTDS_H__ */
