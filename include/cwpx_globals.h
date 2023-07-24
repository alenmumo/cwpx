#ifndef _CWPX_GLOBALS_H_
#define _CWPX_GLOBALS_H_

#include <stdlib.h> /* _MAX_PATH, _MAX_FNAME in Windows */
#include <limits.h> /* PATH_MAX, NAME_MAX in Linux and Mac */

#ifndef _WIN32
#include <sys/mman.h>
#include <errno.h>
#endif

#define CWPX_CRLF "\r\n"
#define CWPX_DOUBLECRLF "\r\n\r\n"
#define CWPX_AMPERSAND "&"
#define CWPX_EQUAL "="
#define CWPX_COOKIEBOUNDARY ";"
#define CWPX_FORWARD_ARG "cwpx_forward"
#define CWPX_INCLUDE_ARG "cwpx_include"
#define CWPX_GETQUEUE_ID "cwpx_getqueue"
#define CWPX_REQCOOKIESQUEUE_ID "cwpx_reqcookiequeue"
#define CWPX_POSTQUEUE_ID "cwpx_postqueue"
#define CWPX_FILESQUEUE_ID "cwpx_filesqueue"
#define CWPX_RAWQUEUE_ID "cwpx_rawqueue"
#define CWPX_RESPHEADERSQUEUE_ID "cwpx_respheadersqueue"
#define CWPX_CONTEXTQUEUE_ID "cwpx_contextqueue"
#define CWPX_MAX_KEYLEN 256
#define CWPX_MIN_BUFLEN 256
#if _WIN32
  #define CWPX_PATH_MAX _MAX_PATH
  #define CWPX_NAME_MAX _MAX_FNAME
  #define CWPX_VERSION "WINCGI"
#else
  #define CWPX_PATH_MAX PATH_MAX
  #define CWPX_NAME_MAX NAME_MAX
  #define CWPX_VERSION "LINCGI"
#endif
#define CWPX_MAX_SCRIPTPATH (CWPX_PATH_MAX + CWPX_NAME_MAX)
#define CWPX_NULLSTR "(null)"
#define CWPX_EMPTY ""
#define CWPX_SEVENNINE_BOUNDARY "\
-------------------------------------------------------------------------------"

#ifdef _WIN32
#define cwpx_geterror() GetLastError()
#define cwpx_seterror(i) SetLastError(i)
#else
#define cwpx_geterror() errno
#define cwpx_seterror(i) errno = i
#endif

enum cwpx_content_types{ CWPX_FORMURLENCODED, CWPX_MULTIPARTFORMDATA,
  CWPX_JSON, CWPX_RAWBODY };
  
enum cwpx_parse_params_states{ CWPX_FILLINGKEY, CWPX_FILLINGVALUE };

enum cwpx_forwclude_options{ CWPX_FORWARD_SCRIPT, CWPX_INCLUDE_SCRIPT };

extern const char *cgi_vars[]; /* @ cwpx_reqhandler.c */

#endif
