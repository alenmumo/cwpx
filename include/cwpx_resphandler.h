#ifndef _CWPX_RESPHANDLER_H_
#define _CWPX_RESPHANDLER_H_

#include <cwpx_queue.h>
#include <stdarg.h>

extern struct Cwpx_Context *cwpx_drspctxh; /* 'dummy ptr' */

typedef struct Cwpx_ResponseHandler{
  Cwpx_NodeQueue *headersqueue;
  int is_header_sent;
  unsigned long content_length_sent;
  
  /* methods for header */
  int (*header)(struct Cwpx_ResponseHandler **_this, 
    const char *key, const char *value);
  int (*cookie)(struct Cwpx_ResponseHandler **_this, 
    const char *key, const char *value, long expires, long maxage, 
	const char *path, const char *domain, int secure, int httponly, 
	const char *attrformat, va_list args);
  /* methods for body */
  int (*write)(struct Cwpx_ResponseHandler **_this,
    const char *format, va_list args);
  /* write & print point to the same function */
  int (*print)(struct Cwpx_ResponseHandler **_this,
    const char *format, va_list args);
  long (*write_b)(struct Cwpx_ResponseHandler **_this, 
    char *bytes, unsigned long byteslen);
  long (*print_b)(struct Cwpx_ResponseHandler **_this, 
    char *bytes, unsigned long byteslen);
  int (*include)(struct Cwpx_Context **cwpx_context, const char *scriptpath);
  
}Cwpx_ResponseHandler;

void cwpx_init_responsehandler(Cwpx_ResponseHandler **_this);

/* methods for header */
int cwpx_send_respheader(Cwpx_ResponseHandler **_this);
int cwpx_respheader(Cwpx_ResponseHandler **_this, 
  const char *key, const char *value);
int cwpx_respcookie(Cwpx_ResponseHandler **_this, 
  const char *key, const char *value, long expires, long maxage, 
  const char *path, const char *domain, int secure, int httponly, 
  const char *attrformat, va_list args);

/* methods for body */
int cwpx_write_format(Cwpx_ResponseHandler **_this,
  const char *format, va_list args);
long cwpx_write_bytes(Cwpx_ResponseHandler **_this, 
  char *bytes, unsigned long byteslen);
/* other methods */
int cwpx_include(struct Cwpx_Context **cwpx_context, const char *scriptpath);

void cwpx_destroy_responsehandler(Cwpx_ResponseHandler **_this);

#endif
