/*
 * gcc -Wall -Wextra -Wconversion -c src/cwpx_reqhandler.c -I"include" -o lib/c
 * wpx_reqhandler.o
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <math.h>
#include <cwpx_reqhandler.h>
#include <cwpx_misc.h>
#include <cwpx_globals.h>
#include <cwpx_config.h>
#ifdef _WIN32
  #define environ _environ
#else
  extern char **environ;
#endif

const char *cgi_vars[] = {  
  "AUTH_TYPE", 
  "CONTENT_LENGTH", 
  "CONTENT_TYPE", 
  "DOCUMENT_ROOT", 
  "GATEWAY_INTERFACE", 
  "HTTPS",
  "PATH_INFO", 
  "PATH_TRANSLATED", 
  "QUERY_STRING", 
  "REMOTE_ADDR", 
  "REMOTE_HOST", 
  "REMOTE_PORT", 
  "REMOTE_IDENT", 
  "REMOTE_USER", 
  "REQUEST_METHOD", 
  /* REQUEST_URI: the whole URI from server path, including query_string,
  ex.: "/cgi-bin/script.cwx/bar?var1=value1&var2=with%20percent%20encoding" */
  "REQUEST_URI", 
  /* SCRIPT_NAME: the path to the file from server path,
  ex.: "/cgi-bin/script.cwx" */
  "SCRIPT_NAME", 
  /* SCRIPT_FILENAME: the absolute system path to the file, 
  ex.: "C:/Program Files (x86)/Apache2.2/cgi-bin/script.cwx" */
  "SCRIPT_FILENAME",
  "SERVER_NAME", 
  "SERVER_PORT", 
  "SERVER_PROTOCOL", 
  "SERVER_SOFTWARE"
};
/* number of elements = sizeof(cgi_vars) / sizeof(cgi_vars[0]); */

extern int cwpx_forwclude(struct Cwpx_Context **cwpx_context, 
  const char *scriptpath, enum cwpx_forwclude_options forwclude);

// local functions
int cwpx_forward(struct Cwpx_Context **cwpx_context, const char *scriptpath);
const char *_cwpx_reqheader(Cwpx_RequestHandler **_this, const char *key,
  unsigned int index);
const char *_cwpx_env(Cwpx_RequestHandler **_this, const char *key,
  unsigned int index);
const char *_cwpx_reqcookie(Cwpx_RequestHandler **_this, const char *key,
  unsigned int index);
const char *_cwpx_get(Cwpx_RequestHandler **_this, const char *key,
  unsigned int index, int *issettransfer);
const char *_cwpx_post(Cwpx_RequestHandler **_this, const char *key, 
  unsigned int index, int *issettransfer);
const char *_cwpx_files(Cwpx_RequestHandler **_this,
  const char *key, const char *attr, unsigned int index);


void cwpx_init_requesthandler(Cwpx_RequestHandler **_this){
  
  Cwpx_RequestHandler *tryMalloc = malloc(sizeof(Cwpx_RequestHandler));
  if(!tryMalloc){ /* free everything ... */ exit(1); }
  *_this = tryMalloc;
  Cwpx_RequestHandler *_thisPtr = *_this;
  
  _thisPtr->headersqueue = NULL;
  _thisPtr->getqueue = NULL;
  _thisPtr->cookiesqueue = NULL;
  _thisPtr->postqueue = NULL;
  _thisPtr->filesqueue = NULL;
  _thisPtr->request_parser = NULL;
  cwpx_init_nodequeue( &_thisPtr->headersqueue ); /* not necessary .. */
  cwpx_init_nodequeue( &_thisPtr->getqueue );
  cwpx_init_nodequeue( &_thisPtr->cookiesqueue );
  cwpx_init_nodequeue( &_thisPtr->postqueue );
  cwpx_init_filenodequeue( &_thisPtr->filesqueue );
  cwpx_init_requestparser( &_thisPtr->request_parser );
  
  _thisPtr->deletefiles = 1; /* default behavior is removing files */
  
  /* methods for header */
  _thisPtr->header = cwpx_reqheader;
  _thisPtr->header_at = cwpx_reqheader_at;
  _thisPtr->env = cwpx_env;
  _thisPtr->env_at = cwpx_env_at;
  _thisPtr->cookie = cwpx_reqcookie;
  _thisPtr->cookie_at = cwpx_reqcookie_at;
  _thisPtr->get = cwpx_get;
  _thisPtr->get_at = cwpx_get_at;
  _thisPtr->isset_get = cwpx_issetget;
  _thisPtr->isset_get_at = cwpx_issetget_at;
  /* methods for body */
  _thisPtr->post = cwpx_post;
  _thisPtr->post_at = cwpx_post_at;
  _thisPtr->isset_post = cwpx_issetpost;
  _thisPtr->isset_post_at = cwpx_issetpost_at;
  _thisPtr->files = cwpx_files;
  _thisPtr->files_at = cwpx_files_at;
  _thisPtr->raw = cwpx_raw;
  /* other methods */
  _thisPtr->forward = cwpx_forward;
  
}

/* methods for header */
const char *cwpx_reqheader(Cwpx_RequestHandler **_this, const char *key){
  Cwpx_RequestHandler *_thisPtr = *_this;
  return _cwpx_reqheader(&_thisPtr, key, 0);
}
const char *cwpx_reqheader_at(Cwpx_RequestHandler **_this, const char *key,
  unsigned int index){
  return _cwpx_reqheader(&*_this, key, index);
}
const char *_cwpx_reqheader(Cwpx_RequestHandler **_this, const char *key,
  unsigned int index){
  /* https://stackoverflow.com/questions/2085302/printing-all-environment-varia
  bles-in-c-c */

  if(*_this == NULL || key == NULL) return NULL;
  Cwpx_RequestHandler *_thisPtr = *_this; _thisPtr = _thisPtr;/*avoid unused */

  char http_header[CWPX_MIN_BUFLEN];
   /* room for "HTTP_" and "=" ... at least 7 */
  size_t key_length = strlen(key) < (CWPX_MIN_BUFLEN - 7) ? 
    strlen(key) : (CWPX_MIN_BUFLEN - 7);
  strcpy(http_header, "HTTP_");
  strncat(http_header, key, key_length);
  http_header[strlen("HTTP_") + key_length] = 0;
  strcat(http_header, "=");
  http_header[CWPX_MIN_BUFLEN-1] = 0;
  size_t http_header_length = strlen(http_header);
  int i = 0;
  while(http_header[i]) {
    http_header[i] = (char)toupper(http_header[i]);
    if(http_header[i] == '-') http_header[i] = '_';
    i++;
  }

  char **env_var = environ;
  unsigned int indexFinder = 0;
  for (; *env_var; env_var++) {
    char *env_varPtr = *env_var;
    /* startsWith */
    if(strncmp(env_varPtr, http_header, http_header_length) == 0){
	  if(indexFinder == index){
        env_varPtr += http_header_length;
        return env_varPtr;
	  }
	  indexFinder++;
    }
    else{
      /* some of the request headers are 'specific' for CGI,
      so they don't come with the 'HTTP_' prefix, 
      such as 'Content-Type' & 'Content-Length' */
      char *typeHeader = "CONTENT_TYPE";
      char *lengthHeader = "CONTENT_LENGTH";
      
      unsigned short isContentType = 
	    strcmp(key, "CONTENT_TYPE") == 0 || strcmp(key, "Content_Type") == 0 ||
	    strcmp(key, "CONTENT-TYPE") == 0 || strcmp(key, "Content-Type") == 0 ||
	    strcmp(key, "content_type") == 0 || strcmp(key, "content-type") == 0;
	    
      unsigned short isContentLength = 
	   strcmp(key,"CONTENT_LENGTH")== 0 || strcmp(key,"Content_Length") == 0 ||
	   strcmp(key,"CONTENT-LENGTH")== 0 || strcmp(key,"Content-Length") == 0 ||
	   strcmp(key,"content_length")== 0 || strcmp(key,"content-length") == 0;
      
      if(strncmp(env_varPtr, typeHeader, strlen(typeHeader)) == 0 &&
	    isContentType){
		if(indexFinder == index){
          env_varPtr += strlen(typeHeader) + 1; // skip the '='
          return env_varPtr;
		}
	    indexFinder++;
      }
      else if(strncmp(env_varPtr, lengthHeader, strlen(lengthHeader)) == 0 &&
	    isContentLength){
		if(indexFinder == index){
          env_varPtr += strlen(lengthHeader) + 1; // skip the '='
          return env_varPtr;
	    }
	    indexFinder++;
      }
    }
  }

  return NULL;
}


const char *cwpx_env(Cwpx_RequestHandler **_this, const char *key){
  return _cwpx_env(&*_this, key, 0);
}
const char *cwpx_env_at(Cwpx_RequestHandler **_this, const char *key, 
  unsigned int index){
  return _cwpx_env(&*_this, key, index);
}
const char *_cwpx_env(Cwpx_RequestHandler **_this, const char *key,
  unsigned int index){

  if(*_this == NULL || key == NULL) return NULL;
  Cwpx_RequestHandler *_thisPtr = *_this; _thisPtr = _thisPtr;/*avoid unused */

  char cgi_header[CWPX_MIN_BUFLEN];
  /* room for "HTTP_" and "=" ... at least 7 */
  size_t key_length = strlen(key) < (CWPX_MIN_BUFLEN - 7) ? 
    strlen(key) : (CWPX_MIN_BUFLEN - 7);
  strncpy(cgi_header, key, key_length);
  cgi_header[key_length] = 0;
  strcat(cgi_header, "=");
  cgi_header[CWPX_MIN_BUFLEN-1] = 0;
  size_t cgi_header_length = strlen(cgi_header);
  int i = 0;
  while(cgi_header[i]) {
    cgi_header[i] = (char)toupper(cgi_header[i]);
    if(cgi_header[i] == '-') cgi_header[i] = '_';
    i++;
  }

  char **env_var = environ;
  unsigned int indexFinder = 0;
  int cgi_vars_length = sizeof(cgi_vars) / sizeof(cgi_vars[0]);
  for (; *env_var; env_var++) {
    char *env_varPtr = *env_var;
    /* startsWith */
    if(strncmp(env_varPtr, cgi_header, cgi_header_length) == 0){
      i = 0;
      while(i < cgi_vars_length){
        size_t cgi_var_length = strlen(cgi_vars[i]);
        if(strncmp(cgi_header, cgi_vars[i], cgi_var_length) == 0){
		  if(indexFinder == index){
            env_varPtr += cgi_header_length;
            return env_varPtr;
		  }
		  indexFinder++;
        }
        i++;
      }
    }
  }

  const char *http_header = cwpx_reqheader(&_thisPtr, key);
  if(http_header != NULL) return http_header;

  return NULL;
}


const char *cwpx_get(Cwpx_RequestHandler **_this, const char *key){
  int issettransfer = 0;
  return _cwpx_get(&*_this, key, 0, &issettransfer);
}
const char *cwpx_get_at(Cwpx_RequestHandler **_this, const char *key,
  unsigned int index){
  int issettransfer = 0;
  return _cwpx_get(&*_this, key, index, &issettransfer);
}
int cwpx_issetget(Cwpx_RequestHandler **_this, const char *key){
  int issettransfer = 0;
  _cwpx_get(&*_this, key, 0, &issettransfer);
  return issettransfer;
}
int cwpx_issetget_at(Cwpx_RequestHandler **_this, const char *key, 
  unsigned int index){
  int issettransfer = 0;
  _cwpx_get(&*_this, key, index, &issettransfer);
  return issettransfer;
}
const char *_cwpx_get(Cwpx_RequestHandler **_this, const char *key,
  unsigned int index, int *issettransfer){
  
  if(*_this == NULL || key == NULL) return NULL;
  Cwpx_RequestHandler *_thisPtr = *_this;

  Cwpx_Node *it = _thisPtr->getqueue->head;
  unsigned int indexFinder = 0;
  *issettransfer = 0;
  while(it != NULL){
    if(strcmp(it->key, key) == 0){
	  if(indexFinder == index){
		*issettransfer = 1;
	    return it->value;
	  }
	  indexFinder++;
	}
    it = it->next;
  }
  
  return NULL;
}

const char *cwpx_reqcookie(Cwpx_RequestHandler **_this, const char *key){
  return _cwpx_reqcookie(&*_this, key, 0);
}
const char *cwpx_reqcookie_at(Cwpx_RequestHandler **_this, const char *key,
  unsigned int index){
  return _cwpx_reqcookie(&*_this, key, index);
}
const char *_cwpx_reqcookie(Cwpx_RequestHandler **_this, const char *key,
  unsigned int index){

  /* https://stackoverflow.com/questions/1969232/what-are-allowed-characters-in
  -cookies/1969339#1969339 */
  
  if(*_this == NULL || key == NULL) return NULL;
  Cwpx_RequestHandler *_thisPtr = *_this;

  Cwpx_Node *it = _thisPtr->cookiesqueue->head;
  unsigned int indexFinder = 0;
  while(it != NULL){
		
	/* cookies' keys were formed sometimes with a prefixing whitespace, 
	so we need to 'trim' the key */
	char trimmedCookie[CWPX_MAX_KEYLEN];
	strcpy(trimmedCookie, "");
	cwpx_strltrim(it->key, trimmedCookie);
	
    if(strcmp(key, trimmedCookie) == 0){
      if(indexFinder == index) return it->value;
      indexFinder++;
	}
    it = it->next;
  }
  
  return NULL;
}

/* methods for body */
const char *cwpx_post(Cwpx_RequestHandler **_this, const char *key){
  int issettransfer = 0;
  return _cwpx_post(&*_this, key, 0, &issettransfer);
}
const char *cwpx_post_at(Cwpx_RequestHandler **_this, const char *key, 
  unsigned int index){
  int issettransfer = 0;
  return _cwpx_post(&*_this, key, index, &issettransfer);
}
int cwpx_issetpost(Cwpx_RequestHandler **_this, const char *key){
  int issettransfer = 0;
  _cwpx_post(&*_this, key, 0, &issettransfer);
  return issettransfer;
}
int cwpx_issetpost_at(Cwpx_RequestHandler **_this, const char *key, 
  unsigned int index){
  int issettransfer = 0;
  _cwpx_post(&*_this, key, index, &issettransfer);
  return issettransfer;
}
const char *_cwpx_post(Cwpx_RequestHandler **_this, const char *key, 
  unsigned int index, int *issettransfer){

  if(*_this == NULL || key == NULL){ *issettransfer = 0; return NULL; }
  Cwpx_RequestHandler *_thisPtr = *_this;
  
  char *namePattern = "name=\"";
  char *namePatternEnd = "\"";
  Cwpx_Node *it = _thisPtr->postqueue->head;
  unsigned int indexFinder = 0;
  *issettransfer = 0;
  
  while(it != NULL){
	
	if(_thisPtr->postqueue->content_type == CWPX_RAWBODY){
	  return it->value;
	}
	
	if(_thisPtr->postqueue->content_type == CWPX_MULTIPARTFORMDATA){
	  char *namePtr = strstr(it->key, namePattern);
	  if(namePtr){
	    namePtr += strlen(namePattern);
	    char *namePtrEnd = strstr(namePtr, namePatternEnd);
	    if(namePtrEnd){
		  unsigned nameLen = (unsigned)(namePtrEnd - namePtr);
		  if(strncmp(key, namePtr, nameLen) == 0){
		    if(indexFinder == index){
			  *issettransfer = 1;
			  return it->value;
			}
		    indexFinder++;
		  }
		}
	  }
    }
    else if(_thisPtr->postqueue->content_type == CWPX_FORMURLENCODED){
	  if(strcmp(key, it->key) == 0){
	    if(indexFinder == index){
		  *issettransfer = 1;
		  return it->value;
	    }
	    indexFinder++;
	  }
	}
	
    it = it->next;
  }
  
  return NULL;
}


const char *cwpx_files(Cwpx_RequestHandler **_this,
  const char *key, const char *attr){
  return _cwpx_files(&*_this, key, attr, 0);
}
const char *cwpx_files_at(Cwpx_RequestHandler **_this,
  const char *key, const char *attr, unsigned int index){
  return _cwpx_files(&*_this, key, attr, index);
}
const char *_cwpx_files(Cwpx_RequestHandler **_this,
  const char *key, const char *attr, unsigned int index){
  
  if(*_this == NULL || key == NULL) return NULL;
  Cwpx_RequestHandler *_thisPtr = *_this;
		
  Cwpx_FileNode *it = _thisPtr->filesqueue->head;
  unsigned int indexFinder = 0;
  while(it != NULL){
    	
	if(strcmp(key, it->key) == 0){
	  if(indexFinder == index){
        if(strcmp(attr, "key") == 0 || strcmp(attr, "name") == 0)
		  return it->key;
		
	    if(strcmp(attr, "filename") == 0)
		  return it->filename;
		
	    if(strcmp(attr, "temp_filename") == 0)
		  return it->temp_filename;
		
	    if(strcmp(attr, "content_type") == 0)
		  return it->content_type;
		
	    /*if(strcmp(attr, "content_encoding") == 0)
		  return it->content_encoding;*/
		
	    if(strcmp(attr, "content_length") == 0)
		  return it->content_length;
	  }
	  indexFinder++;
	}
	
    it = it->next;
  }
  return NULL;
}


const char *cwpx_raw(Cwpx_RequestHandler **_this, const char *attr){

  if(*_this == NULL) return NULL;
  Cwpx_RequestHandler *_thisPtr = *_this;
  
  if(_thisPtr->filesqueue->head == NULL) return NULL;
  
  if( attr != NULL && 
    (strcmp(attr, "content_length") == 0 || 
	  strcmp(attr, "length") == 0 ||
	  strcmp(attr, "len") == 0) ){
    return _thisPtr->filesqueue->head->content_length;
  }
  
  return _thisPtr->filesqueue->head->key;
}

/* other methods */
int cwpx_forward(struct Cwpx_Context **cwpx_context, const char *scriptpath){
  if(*cwpx_context == NULL || scriptpath == NULL) return -1;

  return cwpx_forwclude(&*cwpx_context, scriptpath, CWPX_FORWARD_SCRIPT);
}


void cwpx_destroy_requesthandler(Cwpx_RequestHandler **_this){

  if(*_this == NULL) return;
  
  Cwpx_RequestHandler *_thisPtr = *_this;
  
  cwpx_destroy_nodequeue(&_thisPtr->headersqueue);
  cwpx_destroy_nodequeue(&_thisPtr->getqueue);
  cwpx_destroy_nodequeue(&_thisPtr->cookiesqueue);
  cwpx_destroy_nodequeue(&_thisPtr->postqueue);
  cwpx_destroy_filenodequeue(&_thisPtr->filesqueue, _thisPtr->deletefiles);
  
  cwpx_destroy_requestparser(&_thisPtr->request_parser);
  
  free(*_this);
  *_this = NULL;
}


void cwpx_send_r(Cwpx_RequestHandler **reqhandler, 
  char *body, unsigned long bodylen, char *boundary, 
  enum cwpx_request_type rtype){
  Cwpx_RequestHandler *requestHandler = *reqhandler;
  
  unsigned long chunkSize = bodylen;
  unsigned long remaining = bodylen;
  unsigned long sent = 0;
  while(remaining > 0){
	if(sent + chunkSize > bodylen) chunkSize = remaining;
	
	if(rtype == MULTIPART || rtype == MULTIPART_FILE){
      requestHandler->request_parser->parse_multipartformdata(
        &requestHandler->request_parser,
	    body + sent, chunkSize, 
	    &requestHandler->postqueue, &requestHandler->filesqueue,
        boundary);
	}
	else if(rtype == URLENCODED){
	  requestHandler->request_parser->parse_urlencoded(
	    &requestHandler->request_parser,
		body + sent, chunkSize, &requestHandler->postqueue, CWPX_AMPERSAND);
	}
	else if(rtype == RAW){
	  requestHandler->request_parser->parse_rawbody(
	    &requestHandler->request_parser,
		body + sent, chunkSize, &requestHandler->filesqueue);
	}
	else if(rtype == URLENCODED_QS){
	  requestHandler->request_parser->parse_urlencoded(
	    &requestHandler->request_parser,
		body + sent, chunkSize, &requestHandler->getqueue, CWPX_AMPERSAND);
	}
	else if(rtype == REQCOOKIE){
	  requestHandler->request_parser->parse_urlencoded(
	    &requestHandler->request_parser,
		body + sent, chunkSize, 
		&requestHandler->cookiesqueue, CWPX_COOKIEBOUNDARY);
	}
	else{
	}
	
    sent += chunkSize;
    remaining -= chunkSize;
  }
  
  /* we need to send a final '&' if processing urlencoded */
  if(rtype == URLENCODED){
    requestHandler->request_parser->parse_urlencoded(
	    &requestHandler->request_parser,
		"&", 1, &requestHandler->postqueue, CWPX_AMPERSAND);
  }
  else if(rtype == URLENCODED_QS){
    requestHandler->request_parser->parse_urlencoded(
	    &requestHandler->request_parser,
		"&", 1, &requestHandler->getqueue, CWPX_AMPERSAND);
  }
  else if(rtype == REQCOOKIE){
    requestHandler->request_parser->parse_urlencoded(
	    &requestHandler->request_parser,
		"; ", 1, &requestHandler->cookiesqueue, CWPX_COOKIEBOUNDARY);
  }
}

