/*
 * gcc -Wall -Wextra -Wconversion -c src/cwpx_resphandler.c -I"include" -o lib/
 * cwpx_resphandler.o
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <time.h>
#ifdef _WIN32
  #include <io.h>
  #include <fcntl.h>
#endif
#include <cwpx_resphandler.h>
#include <cwpx_misc.h>
#include <cwpx_config.h>

struct Cwpx_RequestHandler *dmyreqh; /* dummy ptr' */

extern int cwpx_forwclude(struct Cwpx_Context **cwpx_context, 
  const char *scriptpath, enum cwpx_forwclude_options forwclude);

void cwpx_init_responsehandler(Cwpx_ResponseHandler **_this){
  
  Cwpx_ResponseHandler *tryMalloc = malloc(sizeof(Cwpx_ResponseHandler));
  if(!tryMalloc){ /* free everything ... */ exit(1); }
  *_this = tryMalloc;
  Cwpx_ResponseHandler *_thisPtr = *_this;
  
  _thisPtr->headersqueue = NULL;
  cwpx_init_nodequeue( &_thisPtr->headersqueue );
  _thisPtr->is_header_sent = 0;
  _thisPtr->content_length_sent = 0;
  
  /* methods for header */
  _thisPtr->header = cwpx_respheader;
  _thisPtr->cookie = cwpx_respcookie;
  /* methods for body */
  _thisPtr->write = cwpx_write_format;
  _thisPtr->print = _thisPtr->write;
  _thisPtr->write_b = _thisPtr->print_b = cwpx_write_bytes;
  /* other methods */
  _thisPtr->include = cwpx_include;
  
  
  /* in Windows, the response write functions worked fine in textmode (it seems
  that it's also STDOUT default mode), but, when trying to diplay an image from
  a cwpx script it failed, due to Windows newlines problems. Being binarymode 
  what we need to write, in Windows we must reopen STDOUT for binary mode using 
  the instructions 'freopen(NULL, "wb", stdin); setmode(1, O_BINARY);', and the 
  writing part is left as it was 'fwrite(buffer, 1, BUFSIZ, stdout)'... */
  #ifdef _WIN32
  freopen(NULL, "wb", stdout);
  setmode(1, O_BINARY);
  #endif
}

/* methods for header */
int cwpx_respheader(Cwpx_ResponseHandler **_this, 
  const char *key, const char *value){
  /* https://stackoverflow.com/questions/2085302/printing-all-environment-varia
  bles-in-c-c */

  if(*_this == NULL || key == NULL || value == NULL) return -1;
  Cwpx_ResponseHandler *_thisPtr = *_this;
  
  char trimmedKey[CWPX_MAX_KEYLEN];
  cwpx_strtrim((char *)key, trimmedKey);
  if(strlen(trimmedKey) == 0) return -1;
  
  Cwpx_Node *it = _thisPtr->headersqueue->head;
  int replaceValue = 0;
  while(it != NULL) {
    if(strcmp(it->key, key) == 0){
      if(strcmp(key, "Status") == 0) {
        /* https://stackoverflow.com/questions/15656514/return-http-error-code-
		from-cgi-c-module */
		/*printf("Status: 400 Bad Request\n");*/
        replaceValue = 1;
        break;
      }
      if(strcmp(key, "Content-Length") == 0) {
        replaceValue = 1;
        break;
      }
      if(strcmp(key, "Content-Type") == 0) {
        replaceValue = 1;
        break;
      }
      if(strcmp(key, "Set-Cookie") == 0) {
        /* find session cookie and replace it */
        if(CWPX_SESSID != NULL){
		  char cookiePattern[strlen(CWPX_SESSID) + 2];
		  sprintf(cookiePattern, "%s=", CWPX_SESSID);
		  if(strncmp(value, cookiePattern, strlen(cookiePattern)) == 0){
		    replaceValue = 1;
		    break;
		  }
		}
        /*if(cwpx_session_name() != NULL){
          char cookiePattern[strlen(cwpx_session_name()) + 2];
          sprintf(cookiePattern, "%s=", cwpx_session_name());
          if(strncmp(value, cookiePattern, strlen(cookiePattern)) == 0){
            replace_value = 1;
            break;
          }
        }*/
      }
    }
    if(it->next == NULL) {
	  /* this 'if' is for keeping the last node stored in 'it' for next step,
	  if it wasn't found in the replaceable keys */
      break;
    }
    it = it->next;
  }

  if(replaceValue){
    size_t oldValueLen = it->valueLen;
    size_t newValueLen = strlen(value);
    if(newValueLen > oldValueLen){
	  char *tryRealloc = realloc(it->value, newValueLen + 1);
      if(!tryRealloc){ /* free everything ... */ exit(1); }
      it->value = tryRealloc;
    }
    strcpy(it->value, value);
    it->valueLen = newValueLen;
  }
  else{
	size_t valueLen = strlen(value);
    if(valueLen == 0) return -1;
    
    Cwpx_Node *tryMalloc = malloc(sizeof(Cwpx_Node));
    if(!tryMalloc){ /* free everything ... */ exit(1); }          
    Cwpx_Node *newNodeObject = tryMalloc;
    newNodeObject->key = NULL;
    newNodeObject->value = NULL;
    newNodeObject->keyLen = 0;
    newNodeObject->valueLen = 0;
    newNodeObject->next = NULL;
    
    size_t keyLen = strlen(key);
    char *tryStrMalloc = malloc(keyLen + 1);
    if(!tryStrMalloc){ /* free everything ... */ exit(1); }
    newNodeObject->key = tryStrMalloc;
    strcpy(newNodeObject->key, key);
    newNodeObject->keyLen = keyLen;
    
        
    tryStrMalloc = malloc(valueLen + 1);
    if(!tryStrMalloc){ /* free everything ... */ exit(1); }
    newNodeObject->value = tryStrMalloc;
    strcpy(newNodeObject->value, value);
    newNodeObject->valueLen = valueLen;
    
	_thisPtr->headersqueue->push(&_thisPtr->headersqueue, NULL, 
	  newNodeObject, 0);
  }

  return 0;
}


int cwpx_respcookie(Cwpx_ResponseHandler **_this, 
  const char *key, const char *value, long expires, long maxage, 
  const char *path, const char *domain, int secure, int httponly, 
  const char *attrformat, va_list args){

  if(*_this == NULL || key == NULL || value == NULL) return -1;
  Cwpx_ResponseHandler *_thisPtr = *_this;
  
  size_t keyLen = strlen(key);
  if(keyLen == 0) return -1;
	
  size_t valueLen = 0;
  size_t expiresLen = 0;
  size_t maxageLen = 0;
  size_t pathLen = 0;
  size_t domainLen = 0;
  size_t secureLen = 0;
  size_t httponlyLen = 0;
  size_t attrformatLen = 0;
	
  int finalSemicolon = 0;	
  if(value != NULL)
    valueLen = strlen(CWPX_EQUAL) + strlen(value);
  /*else
    value_length = strlen("=") + strlen("");*/
  if(expires != 0) expiresLen = strlen("; expires=") + 30;
  if(maxage != 0) maxageLen = strlen("; max-age=") + 30;
  if(path != NULL) pathLen = strlen("; path=") + strlen(path);
  if(domain != NULL) domainLen = strlen("; domain=") + strlen(domain);
  if(secure != 0) secureLen = strlen("; secure");
  if(httponly != 0) httponlyLen = strlen("; httponly");
  if(attrformat != NULL){
    attrformatLen = strlen(attrformat);
    if(attrformatLen > 0){
      /*va_list args;*/
      /* this first va_start is for counting the dynamic length */
      /* va_start(args, attrformat);*/ /* counting the dynamic length */
      /*the function vsnprintf modifies args,giving undefined results in Linux:
  https://stackoverflow.com/questions/41855571/why-does-the-same-vsnprintf-code
  -output-differently-on-windows-msvc-and-unix */
      va_list argscopy;
      va_copy(argscopy, args);

      size_t temp_attrformatLen=(size_t)vsnprintf(NULL,0,attrformat, argscopy);
      va_end(argscopy);/*va_end(args);*/
      if(temp_attrformatLen > 0) attrformatLen = temp_attrformatLen;
	}
  }
  
  size_t cookieLen = keyLen + valueLen + expiresLen + maxageLen + 
    pathLen + domainLen + secureLen + httponlyLen + attrformatLen + 
	strlen(CWPX_COOKIEBOUNDARY); /* ';' */
	
  char *tryMalloc = malloc(cookieLen + 1);
  if(tryMalloc == NULL){ /* free everything */ exit(1); }
  char *newCookie = tryMalloc;
  strcpy(newCookie, key);
  strcat(newCookie, "=");
  strcat(newCookie, value);
	
  if(expires != 0){
    time_t expires_seconds = time(NULL) + expires;
    struct tm *expires_date = gmtime(&expires_seconds);
    char gmt_date[30];
	strftime(gmt_date, 30, "%a, %d %b %Y %H:%M:%S GMT", expires_date);
	strcat(newCookie, "; expires=");
	strcat(newCookie, gmt_date);
	finalSemicolon = 1;
  }
  if(maxage != 0){
    if(maxage < 0) maxage = 0;
    char maxageStr[30];
    sprintf(maxageStr, "%ld", maxage);
    strcat(newCookie, "; max-age=");
    strcat(newCookie, maxageStr);
    finalSemicolon = 1;
  }
  if(path != NULL){
    strcat(newCookie, "; path=");
    strcat(newCookie, path);
    finalSemicolon = 1;
  }
  if(domain != NULL){
    strcat(newCookie, "; domain=");
    strcat(newCookie, domain);
    finalSemicolon = 1;
  }
  if(secure != 0){
    strcat(newCookie, "; secure");
    finalSemicolon = 1;
  }
  if(httponly != 0){
    strcat(newCookie, "; httponly");
    finalSemicolon = 1;
  }
  if(attrformat != NULL){
    tryMalloc = malloc(attrformatLen + 2);
    if(tryMalloc == NULL){ /* free everything */ exit(1); }
	char* buffer = tryMalloc;
	/*va_list args;*/
	/* this second va_start is for writing the buffer */
    /*va_start(args, attrformat);*/
    /* vsnprintf includes the null terminator */
    vsnprintf(buffer, attrformatLen + 1, attrformat, args);
    va_end(args);

    strcat(newCookie, buffer);
    free(buffer);
    
    finalSemicolon = 1;
  }
	
  if(finalSemicolon){
    /*strcat(newCookie, ";");*/
  }
  
  return cwpx_respheader(&_thisPtr, "Set-Cookie", newCookie);
}

/* methods for body */
int cwpx_write_format(Cwpx_ResponseHandler **_this, 
  const char *format, va_list args){
  
  if(*_this == NULL || format == NULL) return -1;
  Cwpx_ResponseHandler *_thisPtr = *_this;

  size_t format_length = strlen(format);
  if(format_length <= 0) return 0;

  if(!_thisPtr->is_header_sent) cwpx_send_respheader(&_thisPtr);

  /*va_list args;*/
  /* this first va_start is for counting the dynamic length */
  /*va_start(args, format);*/
  /* the function vsnprintf modifies args, giving undefined results in Linux:
  https://stackoverflow.com/questions/41855571/why-does-the-same-vsnprintf-code
  -output-differently-on-windows-msvc-and-unix */
  va_list argscopy;
  va_copy(argscopy, args);

  int pf_ret = vsnprintf(NULL, 0, format, argscopy);
  va_end(argscopy);
  if(pf_ret <= 0) return pf_ret;

  size_t buffer_length = (size_t)pf_ret + 1;
  char* buffer = malloc(buffer_length);
  if(buffer == NULL) return -1;

  /* this second va_start is for writing the buffer */
  /*va_start(args, format);*/

  /* vsnprintf includes the null terminator */
  pf_ret = vsnprintf(buffer, buffer_length, format, args);
  va_end(args);

  fprintf(stdout, "%s", buffer);
  /* fflush(stdout) */

  free(buffer);
  
  _thisPtr->content_length_sent += pf_ret > 0 ? (unsigned)pf_ret : 0;

  /* returns >= 0 on success*/
  return pf_ret;
}


long cwpx_write_bytes(Cwpx_ResponseHandler **_this, 
  char *bytes, unsigned long byteslen){
  
  if(*_this == NULL || bytes == NULL || byteslen == 0) return -1;
  Cwpx_ResponseHandler *_thisPtr = *_this;
  
  if(!_thisPtr->is_header_sent) cwpx_send_respheader(&_thisPtr);

  unsigned long bytes_to_write = byteslen;

  long bytes_written = 0;
  unsigned long bytes_remaining = bytes_to_write;

  while((unsigned)bytes_written < bytes_to_write){
    size_t new_bytes_written = 
	  fwrite(bytes + bytes_written, 1, bytes_remaining, stdout);
    if(new_bytes_written <= 0) { break; }
    bytes_written += (long)new_bytes_written;
    bytes_remaining = bytes_remaining - (unsigned long)bytes_written;
  }
  
  _thisPtr->content_length_sent += (unsigned)bytes_written;

  return bytes_written;
}


/* other methods */
int cwpx_include(struct Cwpx_Context **cwpx_context, const char *scriptpath){
  if(*cwpx_context == NULL || scriptpath == NULL) return -1;
  
  return cwpx_forwclude(&*cwpx_context, scriptpath, CWPX_INCLUDE_SCRIPT);
}


void cwpx_destroy_responsehandler(Cwpx_ResponseHandler **_this){
  if(*_this == NULL) return;
  
  Cwpx_ResponseHandler *_thisPtr = *_this;
  
  cwpx_destroy_nodequeue( &_thisPtr->headersqueue );
  _thisPtr->is_header_sent = 0;
  _thisPtr->content_length_sent = 0;
    
  free(*_this);
  *_this = NULL;
}

/* local file functions */
int cwpx_send_respheader(Cwpx_ResponseHandler **_this){
  /* read https://datatracker.ietf.org/doc/html/rfc2616#section-4.2 about white
  spaces in headers */
  if(*_this == NULL) return -1;
  Cwpx_ResponseHandler *_thisPtr = *_this;
  
  Cwpx_Node *it = _thisPtr->headersqueue->head;
  if(it == NULL){
    /* at least one header must be sent */
    printf("%s%c%c\r\n", "Content-Type: text/html; charset=utf-8", 13, 10);
  }
  else{
    while(it != NULL){
      printf("%s: %s%c%c", it->key, it->value, 13, 10);
      it = it->next;
    }
    printf("%c%c", 13, 10);
  }
  
  _thisPtr->is_header_sent = 1;
  
  return 0;
}
