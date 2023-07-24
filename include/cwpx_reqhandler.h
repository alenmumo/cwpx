#ifndef _CWPX_REQHANDLER_H_
#define _CWPX_REQHANDLER_H_

#include <cwpx_reqparser.h>
#include <cwpx_resphandler.h>

extern struct Cwpx_Context *cwpx_drpctxh; /* 'dummy ptr' */

typedef struct Cwpx_RequestHandler{
  Cwpx_NodeQueue *headersqueue;
  Cwpx_NodeQueue *getqueue;
  Cwpx_NodeQueue *cookiesqueue;
  Cwpx_NodeQueue *postqueue;
  Cwpx_FileNodeQueue *filesqueue;
  Cwpx_RequestParser *request_parser;
  
  int deletefiles; /* custom var to handle filesqueue file removal */
  
  /* methods for header */
  const char* (*header)(struct Cwpx_RequestHandler **_this, const char *key);
  const char* (*header_at)(struct Cwpx_RequestHandler **_this, const char *key,
    unsigned int index);
  const char* (*env)(struct Cwpx_RequestHandler **_this, const char *key);
  const char* (*env_at)(struct Cwpx_RequestHandler **_this, const char *key,
    unsigned int index);
  const char* (*cookie)(struct Cwpx_RequestHandler **_this, const char *key);
  const char* (*cookie_at)(struct Cwpx_RequestHandler **_this, const char *key,
    unsigned int index);
  const char* (*get)(struct Cwpx_RequestHandler **_this, const char *key);
  const char* (*get_at)(struct Cwpx_RequestHandler **_this, const char *key,
    unsigned int index);
  int (*isset_get)(struct Cwpx_RequestHandler **_this, const char *key);
  int (*isset_get_at)
    (struct Cwpx_RequestHandler **_this, const char *key, unsigned int index);
  /* methods for body */
  const char* (*post)(struct Cwpx_RequestHandler **_this, const char *key);
  const char* (*post_at)(struct Cwpx_RequestHandler **_this, const char *key,
    unsigned int index);
  int (*isset_post)(struct Cwpx_RequestHandler **_this, const char *key);
  int (*isset_post_at)
    (struct Cwpx_RequestHandler **_this, const char *key, unsigned int index);
  const char* (*files)(struct Cwpx_RequestHandler **_this,
    const char *key, const char *attr);
  const char* (*files_at)(struct Cwpx_RequestHandler **_this,
    const char *key, const char *attr, unsigned int index);
  const char* (*raw)(struct Cwpx_RequestHandler **_this, const char *attr);
  /* other methods */
  int (*forward)(struct Cwpx_Context **cwpx_context, const char *scriptpath);
  
}Cwpx_RequestHandler;

void cwpx_init_requesthandler(Cwpx_RequestHandler **_this);

/* methods for header */
const char *cwpx_reqheader(Cwpx_RequestHandler **_this, const char *key);
const char *cwpx_reqheader_at(Cwpx_RequestHandler **_this, const char *key,
  unsigned int index);
const char *cwpx_env(Cwpx_RequestHandler **_this, const char *key);
const char *cwpx_env_at(Cwpx_RequestHandler **_this, const char *key,
  unsigned int index);
const char *cwpx_reqcookie(Cwpx_RequestHandler **_this, const char *key);
const char *cwpx_reqcookie_at(Cwpx_RequestHandler **_this, const char *key,
  unsigned int index);
const char *cwpx_get(Cwpx_RequestHandler **_this, const char *key);
const char *cwpx_get_at(Cwpx_RequestHandler **_this, const char *key,
  unsigned int index);
int cwpx_issetget(Cwpx_RequestHandler **_this, const char *key);
int cwpx_issetget_at(Cwpx_RequestHandler **_this, const char *key,
  unsigned int index);
/* methods for body */
const char *cwpx_post(Cwpx_RequestHandler **_this, const char *key);
const char *cwpx_post_at(Cwpx_RequestHandler **_this, const char *key,
  unsigned int index);
int cwpx_issetpost(Cwpx_RequestHandler **_this, const char *key);
int cwpx_issetpost_at(Cwpx_RequestHandler **_this, const char *key,
  unsigned int index);
const char *cwpx_files(Cwpx_RequestHandler **_this,
  const char *key, const char *attr);
const char *cwpx_files_at(Cwpx_RequestHandler **_this,
  const char *key, const char *attr, unsigned int index);
const char *cwpx_raw(Cwpx_RequestHandler **_this, const char *attr);
/* other methods */
int cwpx_forward(struct Cwpx_Context **cwpx_context, const char *scriptpath);

void cwpx_destroy_requesthandler(Cwpx_RequestHandler **_this);

/* other methods */
enum cwpx_request_type {URLENCODED_QS, REQCOOKIE, 
  MULTIPART, MULTIPART_FILE, URLENCODED, RAW};
void cwpx_send_r(Cwpx_RequestHandler **reqhandler, 
  char *body, unsigned long bodylen, char *boundary, 
  enum cwpx_request_type rtype);

#endif
