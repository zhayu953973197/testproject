/*******************************************************************************
* Copyright @ Huawei Technologies Co., Ltd. 1998-2014. All rights reserved.
* File name: securec.h
* Decription:
*             the user of this secure c library should include this header file
*             in you source code. This header file declare all supported API
*             prototype of the library, such as memcpy_s, strcpy_s, wcscpy_s,
*             strcat_s, strncat_s, sprintf_s, scanf_s, and so on.
* History:
*     1. Date:
*         Author:
*         Modification:
********************************************************************************
*/

#ifndef __SECUREC_H__5D13A042_DC3F_4ED9_A8D1_882811274C27
#define __SECUREC_H__5D13A042_DC3F_4ED9_A8D1_882811274C27

/* If you need high performance, enable the WITH_PERFORMANCE_ADDONS macro! */
#define WITH_PERFORMANCE_ADDONS

#include "securectype.h"    /*lint !e537*/
#include <stdarg.h>
#include "errno.h"

/* If stack size on some embedded platform is limited, you can define the following macro
*  which will put some variables on heap instead of stack.
*/
#define STACK_SIZE_LESS_THAN_1K


/* for performance consideration, the following macro will call the corresponding API
* of libC for memcpy, memmove and memset
*/
#define CALL_LIBC_COR_API

/* codes should run under the macro COMPATIBLE_LINUX_FORMAT in unknow system on default,
   and strtold. The function
   strtold is referenced first at ISO9899:1999(C99), and some old compilers can
   not support these functions. Here provides a macro to open these functions:

       SECUREC_SUPPORT_STRTOLD  -- if defined, strtold will   be used
*/
#define SECUREC_SUPPORT_STRTOLD

/*define error code*/
#ifndef errno_t
typedef int errno_t;
#endif

/* success */
#define EOK (0)

/* invalid parameter */
#define EINVAL_AND_RESET (22 | 0X80)
/* invalid parameter range */
#define ERANGE_AND_RESET  (34 | 0X80)

#ifdef EOVERLAP_AND_RESET
#undef EOVERLAP_AND_RESET
#endif
/*Once the buffer overlap is detected, the dest buffer must be reseted! */
#define EOVERLAP_AND_RESET (54 | 0X80)

/*if you need export the function of this library in Win32 dll, use __declspec(dllexport) */

#ifdef __cplusplus
extern "C"
{
#endif

   /**
    * @Description:The getHwSecureCVersion function get SecureC Version string and version number .
    * @param verStr -   address to store verison string
    * @param bufSize -The maximum length of dest
    * @param verNumber - to store version number
    * @return  EOK if there was no runtime-constraint violation
    */
    void getHwSecureCVersion(char* verStr, int bufSize, unsigned short* verNumber);

    /**
    * @Description:The memmove_s function copies n characters from the object pointed to by src into the object pointed to by dest.
    * @param dest - destination  address
    * @param destMax -The maximum length of destination buffer
    * @param src -source  address
    * @param count -copies count wide characters from the  src
    * @return  EOK if there was no runtime-constraint violation
    */
    errno_t memmove_s(void* dest, size_t destMax, const void* src, size_t count);

    /**
    * @Description: The  strtok_s  function parses a string into a sequence of tokens,On the first call to strtok_s the string to be parsed should be specified in strToken.
    *                       In each subsequent call that should parse the same string, strToken should be NULL
    * @param strToken - the string to be delimited
    * @param strDelimit -specifies a set of characters that delimit the tokens in the parsed string
    * @param context -is a pointer to a char * variable that is used internally by strtok_s function
    * @return:returns a pointer to the first character of a token, or a null pointer if there is no token or there is a runtime-constraint violation.
    */
    char* strtok_s(char* strToken, const char* strDelimit, char** context);

    /**
    * @Description: The sprintf_s function is equivalent to the sprintf function except for the parameter destMax and the explicit runtime-constraints violation
    * @param strDest -  produce output according to a format ,write to the character string strDest
    * @param destMax - The maximum length of destination buffer(including the terminating null byte ('\0'))
    * @param format - fromat string
    * @return:success the number of characters printed(not including the terminating null byte ('\0')), If an error occurred return -1.
    */
    int sprintf_s(char* strDest, size_t destMax, const char* format, ...) SECUREC_ATTRIBUTE(3,4);

    /**
    * @Description: The vsprintf_s function is equivalent to the vsprintf function except for the parameter destMax and the explicit runtime-constraints violation
    * @param strDest -  produce output according to a format ,write to the character string strDest
    * @param destMax - The maximum length of destination buffer(including the terminating null wide characte)
    * @param format - fromat string
    * @param argptr - instead of  a variable  number of arguments
    * @return:return the number of characters printed(not including the terminating null byte ('\0')), If an error occurred return -1.
    */
    int vsprintf_s(char* strDest, size_t destMax, const char* format, va_list argptr) SECUREC_ATTRIBUTE(3,0);

    /**
    * @Description: The vsnprintf_s function is equivalent to the vsnprintf function except for the parameter destMax/count and the explicit runtime-constraints violation
    * @param strDest -  produce output according to a format ,write to the character string strDest
    * @param destMax - The maximum length of destination buffer(including the terminating null  byte ('\0'))
    * @param count - do not write more than count bytes to strDest(not including the terminating null  byte ('\0'))
    * @param format - fromat string
    * @param argptr - instead of  a variable  number of arguments
    * @return:return the number of characters printed(not including the terminating null byte ('\0')), If an error occurred return -1.
    */
    int vsnprintf_s(char* strDest, size_t destMax, size_t count, const char* format, va_list arglist) SECUREC_ATTRIBUTE(4,0);

    /**
    * @Description: The snprintf_s function is equivalent to the snprintf function except for the parameter destMax/count and the explicit runtime-constraints violation
    * @param strDest -  produce output according to a format ,write to the character string strDest
    * @param destMax - The maximum length of destination buffer(including the terminating null  byte ('\0'))
    * @param count - do not write more than count bytes to strDest(not including the terminating null  byte ('\0'))
    * @param format - fromat string
    * @param argptr - instead of  a variable  number of arguments
    * @return:return the number of characters printed(not including the terminating null byte ('\0')), If an error occurred return -1.
    */
    int snprintf_s(char* strDest, size_t destMax, size_t count, const char* format, ...) SECUREC_ATTRIBUTE(4,5);

     /**
    * @Description:The memset_s function copies the value of c (converted to an unsigned char) into each of the first count characters of the object pointed to by dest.
    * @param dest - destination  address
    * @param destMax -The maximum length of destination buffer
    * @param c - the value to be copied
    * @param count -copies fisrt count characters of  dest
    * @return  EOK if there was no runtime-constraint violation
    */
    errno_t memset_s(void* dest, size_t destMax, int c, size_t count);

    /**
    * @Description:The memcpy_s function copies n characters from the object pointed to by src into the object pointed to by dest.
    * @param dest - destination  address
    * @param destMax -The maximum length of destination buffer
    * @param src -source  address
    * @param count -copies count  characters from the  src
    * @return  EOK if there was no runtime-constraint violation
    */
    errno_t memcpy_s(void* dest, size_t destMax, const void* src, size_t count);

    /**
    * @Description:The strcpy_s function copies the string pointed to by strSrc (including the terminating null character) into the array pointed to by strDest
    * @param strDest - destination  address
    * @param destMax -The maximum length of destination buffer(including the terminating null character)
    * @param strSrc -source  address
    * @return  EOK if there was no runtime-constraint violation
    */
    errno_t strcpy_s(char* strDest, size_t destMax, const char* strSrc);

    /**
    * @Description:The strncpy_s function copies not more than n successive characters (not including the terminating null character)
    *                     from the array pointed to by strSrc to the array pointed to by strDest
    * @param strDest - destination  address
    * @param destMax -The maximum length of destination buffer(including the terminating null character)
    * @param strSrc -source  address
    * @param count -copies count  characters from the  src
    * @return  EOK if there was no runtime-constraint violation
    */
    errno_t strncpy_s(char* strDest, size_t destMax, const char* strSrc, size_t count);

    /**
    * @Description:The strcat_s function appends a copy of the  string pointed to by strSrc (including the terminating null  character)
    *                     to the end of the  string pointed to by strDest
    * @param strDest - destination  address
    * @param destMax -The maximum length of destination buffer(including the terminating null wide character)
    * @param strSrc -source  address
    * @return  EOK if there was no runtime-constraint violation
    */
    errno_t strcat_s(char* strDest, size_t destMax, const char* strSrc);


    /**
    * @Description:The strncat_s function appends not more than n successive  characters (not including the terminating null  character)
    *                       from the array pointed to by strSrc to the end of the  string pointed to by strDest.
    * @param strDest - destination  address
    * @param destMax -The maximum length of destination buffer(including the terminating null character)
    * @param strSrc -source  address
    * @param count -copies count  characters from the  src
    * @return  EOK if there was no runtime-constraint violation
    */
    errno_t strncat_s(char* strDest, size_t destMax, const char* strSrc, size_t count);

   /* those functions are used by macro */
    errno_t strncpy_error(char* strDest, size_t destMax, const char* strSrc, size_t count);
    errno_t strcpy_error(char* strDest, size_t destMax, const char* strSrc);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif/* __SECUREC_H__5D13A042_DC3F_4ED9_A8D1_882811274C27 */


