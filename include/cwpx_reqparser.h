#ifndef _CWPX_REQPARSER_H_
#define _CWPX_REQPARSER_H_

#include <cwpx_queue.h>

typedef struct Cwpx_RequestParser{
  Cwpx_Node *temp_node;
  enum cwpx_parse_params_states parse_state;

  /* methods for body (parse_urlencoded is also for header's query_string) */
  void (*parse_multipartformdata)(struct Cwpx_RequestParser **_this,
    char *chunk, size_t chunklength,
    Cwpx_NodeQueue **nodequeue, Cwpx_FileNodeQueue **filenodequeue, 
    char *boundary);
  void (*parse_urlencoded)(struct Cwpx_RequestParser **_this, 
    char *chunk, size_t chunklength, Cwpx_NodeQueue **nodequeue, 
	char *boundary);
  void (*parse_rawbody)(struct Cwpx_RequestParser **_this,
    char *chunk, size_t chunklength, Cwpx_FileNodeQueue **filenodequeue);
  
}Cwpx_RequestParser;

void cwpx_init_requestparser(Cwpx_RequestParser **_this);

/* methods for body */
void cwpx_parse_multipartformdata(Cwpx_RequestParser **_this,
  char *chunk, size_t chunklength, 
  Cwpx_NodeQueue **nodequeue, Cwpx_FileNodeQueue **filenodequeue, 
  char *boundary);
void cwpx_parse_urlencoded(Cwpx_RequestParser **_this,
  char *chunk, size_t chunklength, Cwpx_NodeQueue **nodequeue,
  char *boundary);
void cwpx_parse_rawbody(Cwpx_RequestParser **_this,
  char *chunk, size_t chunklength, Cwpx_FileNodeQueue **filenodequeue);
  
void cwpx_destroy_requestparser(Cwpx_RequestParser **_this);

#endif
