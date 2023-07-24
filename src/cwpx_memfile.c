/*
 * gcc -Wall -Wextra -Wconversion -c src/cwpx_memfile.c -I"include" -o lib/cwpx
 * _memfile.o
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>
#include <fcntl.h> /* open */
#include <unistd.h> /* close */
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#include <errno.h>
#endif

#include <cwpx_memfile.h>

#ifdef _WIN32
#define cwpx_memtofile(fv, buf, buflen) CopyMemory((PVOID)fv, buf, buflen)
#else
#define cwpx_memtofile(fv, buf, buflen) memcpy(fv, buf, buflen)
#endif

int cwpx_determine_vartype(const char *vartypepattern, char *destination);
size_t cwpx_determine_typesize(const char *vartype);

int _cwpx_set_object(Cwpx_ObjectQueue **oqueue, const char *name, void *object, 
  unsigned long size_of, char *vartype){
	
  if(*oqueue == NULL || name == NULL || object == NULL) return 0;

  char determinedType[50];
  int complex = cwpx_determine_vartype(vartype, determinedType);
  
  Cwpx_ObjectQueue *oqueuePtr = *oqueue;
  
  /* check if the node already exists, in which case, replace it */
  Cwpx_ObjectNode *it = oqueuePtr->head;
  while(it != NULL){
	if(strcmp(it->name, name) == 0){
	  char *tryRealloc = realloc(it->type, strlen(determinedType) + 1);
      if(tryRealloc == NULL){ /* free everything */ exit(1); }
      it->type = tryRealloc;
      strcpy(it->type, determinedType);
  
      it->complex = complex;
      
      void *tryReallocObj = realloc(it->value, size_of + 1);
      if(tryReallocObj == NULL){ /* free everything */ exit(1); }
      it->value = tryReallocObj;
      memcpy(it->value, object, size_of);
      memset(it->value + size_of, 0, 1); /* add final 'terminator' */
      it->size_of = size_of;
      
	  return 0;
	}
    it = it->next;
  }
  
  /* continue if it's a brand new node */
  
  Cwpx_ObjectNode *newObject = NULL;
  cwpx_init_objectnode(&newObject);
  
  char *tryMalloc = malloc(strlen(name) + 1);
  if(tryMalloc == NULL){ /* free everything */ exit(1); }
  newObject->name = tryMalloc;
  strcpy(newObject->name, name);
  
  tryMalloc = malloc(strlen(determinedType) + 1);
  if(tryMalloc == NULL){ /* free everything */ exit(1); }
  newObject->type = tryMalloc;
  strcpy(newObject->type, determinedType);
  
  newObject->complex = complex;
  
  void *tryMallocObj = malloc(size_of + 1);
  if(tryMallocObj == NULL){ /* free everything */ exit(1); }
  newObject->value = tryMallocObj;
  memcpy(newObject->value, object, size_of);
  memset(newObject->value + size_of, 0, 1); /* add final 'terminator' */
  newObject->size_of = size_of;
  
  oqueuePtr->push(&oqueuePtr, newObject);
  
  return 0;  
}


void *_cwpx_get_object(Cwpx_ObjectQueue **oqueue, const char *name, 
  unsigned long size_of, char *vartype){
	
  if(*oqueue == NULL || name == NULL) return 0;

  char determinedType[50];
  int complex = cwpx_determine_vartype(vartype, determinedType);
  complex = complex; /* for avoiding unused warning */
  size_of = size_of; /* for avoiding unused warning */
  
  Cwpx_ObjectQueue *oqueuePtr = *oqueue;
  Cwpx_ObjectNode *it = oqueuePtr->head;
  while(it != NULL){
	if(strcmp(it->name, name) == 0){
	  return it->value;
	}
    it = it->next;
  }
  
  return NULL;  
}


int cwpx_setobjectsqueuetofile(Cwpx_ObjectQueue **oqueue, 
  Cwpx_MemMappedFile **mfile){

  if(*oqueue == NULL || *mfile == NULL) return -1;
  Cwpx_ObjectQueue *oqueuePtr = *oqueue;
  Cwpx_MemMappedFile *mfilePtr = *mfile;
  
  if(mfilePtr->filename == NULL) return -1;
  
  size_t CWPX_CRLF_LEN = strlen(CWPX_CRLF);
  unsigned long filePosition = 0;

  if(mfilePtr->memorymapped){
	/* FILE opening */
	#ifdef _WIN32
	if(mfilePtr->file == NULL){
      mfilePtr->file = 
        CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 
	      CWPX_BUF_SIZE, mfilePtr->filename);
	
	  if(mfilePtr->file == NULL){
        /* free evertyhing */
        /* log the error 'GetLastError()' */
        exit(-1);
      }
    }
	#else
	if(mfilePtr->file == NULL){
	  mfilePtr->fd = 
	    open(mfilePtr->filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
	  if(mfilePtr->fd < 0){
        /* free evertyhing */
        /* log the error 'GetLastError()' */
        exit(-1);
      }
      /* the open function will return a zero-length file in case that the file 
	  doesn't exist and thus, be created. because of that 'zero-length', the 
	  mmap function will not work as expected, it'll return a zero-length 
	  buffer, causing 'Bus error (core dumped)' when trying to write to the 
	  buffer. one solution for this is using the ftruncate function for 
	  creating the file with to a specified length before mmapping the file or
	  ...*/
	  
	  /*lseek(mfilePtr->fd, CWPX_BUF_SIZE + 1, SEEK_SET);*//*original working,
	  the next (cast) was chosen over this for avoiding sign warnings */
	  lseek(mfilePtr->fd, (long)(CWPX_BUF_SIZE + 1), SEEK_SET);
      write(mfilePtr->fd, "", 1);
      lseek(mfilePtr->fd, 0, SEEK_SET);
      
      /* because in Linux we don't use the FILE* mfilePtr->file for the file
	  mapping, but a file descriptor, and we close the file descriptor right 
	  after establishing the mapping, we cannot rely upon the mfilePtr->fd 
	  being -1. to handle this uniformly with the Windows implementation,
	  we FAKE a FILE* for mfilePtr->file to keep track if the mapping is 
	  set or not */
	  mfilePtr->file = fmemopen(NULL, 0, "w"); /* fake FILE* initialization */
      if (mfilePtr->file == NULL){
	    close(mfilePtr->fd);
	    mfilePtr->fd = -1;
        /* free evertyhing */
        /* log the error 'GetLastError()' */
        exit(-1);
	  }
    }
	#endif
	
    /* 'memory opening */
    #ifdef _WIN32
    mfilePtr->fileview = 
      (LPTSTR)MapViewOfFile(mfilePtr->file, FILE_MAP_ALL_ACCESS, 0, 0, 
	    CWPX_BUF_SIZE);
    if(mfilePtr->fileview == NULL){
	  CloseHandle(mfilePtr->file);
	  mfilePtr->file = NULL;
      /* free evertyhing */
      /* log the error 'GetLastError()' */
      exit(-1);
    }
    #else    
    mfilePtr->fileview = 
	  mmap(0, CWPX_BUF_SIZE, PROT_WRITE, MAP_SHARED, mfilePtr->fd, 0);
	if(mfilePtr->fileview == NULL){
	  close(mfilePtr->fd);
	  mfilePtr->fd = -1;
	  fclose(mfilePtr->file);
	  mfilePtr->file = NULL;
      /* free evertyhing */
      /* log the error 'GetLastError()' */
      exit(-1);
    }
    /* once the mapping is done, we close the associated file */
    close(mfilePtr->fd);
    mfilePtr->fd = -1;
    #endif

    cwpx_memtofile(mfilePtr->fileview + filePosition, 
	  CWPX_CONTEXTQUEUE_ID, strlen(CWPX_CONTEXTQUEUE_ID));
    filePosition += strlen(CWPX_CONTEXTQUEUE_ID);
    cwpx_memtofile(mfilePtr->fileview + filePosition, 
	  CWPX_CRLF, CWPX_CRLF_LEN);
    filePosition += CWPX_CRLF_LEN;
  }
  else{
    mfilePtr->file = fopen(mfilePtr->filename, "wb");
    if(mfilePtr->file == NULL){
      /* free evertyhing */
      /* log the error 'GetLastError()' */
      exit(-1);
    }
    
    fwrite(CWPX_CONTEXTQUEUE_ID,1,strlen(CWPX_CONTEXTQUEUE_ID),mfilePtr->file);
    filePosition += strlen(CWPX_CONTEXTQUEUE_ID);
    fwrite(CWPX_CRLF, 1, CWPX_CRLF_LEN, mfilePtr->file);
    filePosition += CWPX_CRLF_LEN;
  }
  
  Cwpx_ObjectNode *it = oqueuePtr->head;
  char sizeToStr[CWPX_MAX_KEYLEN + 50]; /* max key name*/
  while(it != NULL){
    sprintf(sizeToStr, "%lu,%s,%s,%s", 
	  it->size_of, it->name, it->type, CWPX_CRLF);
    
    if(mfilePtr->memorymapped){
      cwpx_memtofile(mfilePtr->fileview + filePosition, 
	    sizeToStr, strlen(sizeToStr));
      filePosition += strlen(sizeToStr);
      cwpx_memtofile(mfilePtr->fileview + filePosition, 
	    it->value, it->size_of);
      filePosition += it->size_of;
      cwpx_memtofile(mfilePtr->fileview + filePosition, 
	    CWPX_CRLF, CWPX_CRLF_LEN);
      filePosition += CWPX_CRLF_LEN;
    }
    else{
	  fwrite(sizeToStr, 1, strlen(sizeToStr), mfilePtr->file);
      filePosition += strlen(sizeToStr);
	  fwrite(it->value, 1, it->size_of, mfilePtr->file);
      filePosition += it->size_of;
	  fwrite(CWPX_CRLF, 1, CWPX_CRLF_LEN, mfilePtr->file);
      filePosition += CWPX_CRLF_LEN;
	}
	
    it = it->next;
  }
  
  /* the other side needs something for checking the end of the queue. we can 
  send another crlf */
  if(mfilePtr->memorymapped){
    cwpx_memtofile(mfilePtr->fileview + filePosition, 
      CWPX_CRLF, CWPX_CRLF_LEN);
    filePosition += CWPX_CRLF_LEN;
  }
  else{
    fwrite(CWPX_CRLF, 1, CWPX_CRLF_LEN, mfilePtr->file);
    filePosition += CWPX_CRLF_LEN;
  }
  
  if(mfilePtr->memorymapped){
    #ifndef _WIN32
    msync(mfilePtr->fileview, CWPX_BUF_SIZE, MS_SYNC);
    #endif
  }
  else{
    fflush(mfilePtr->file);
    fclose(mfilePtr->file);
    mfilePtr->file = NULL;
  }
  
  return 0;
}

int cwpx_getobjectsqueuefromfile(Cwpx_ObjectQueue **oqueue, 
  Cwpx_MemMappedFile **mfile, const char *objectname, void *objectbuffer){
  
  if(*oqueue == NULL || *mfile == NULL) return -1;
  Cwpx_ObjectQueue *oqueuePtr = *oqueue;
  Cwpx_MemMappedFile *mfilePtr = *mfile;
  
  objectname = objectname; /* for avoiding unused warning */
  objectbuffer = objectbuffer; /* for avoiding unused warning */
  
  if(mfilePtr->filename == NULL) return -1;
  
  size_t CWPX_CRLF_LEN = strlen(CWPX_CRLF);
  
  /* the actual pointer (or view) to the memory-mapped file (or session file)*/
  char *viewPtr = NULL;
  
  if(mfilePtr->memorymapped){
    /* when reading the file that the caller created, we don't mean to exit if 
	the file doesn't exist, but just returning an error */
    #ifdef _WIN32
    mfilePtr->file = 
	  OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, mfilePtr->filename);
	if(mfilePtr->file == NULL){
	  return -1;
	}
	#else
	mfilePtr->fd = open(mfilePtr->filename, O_RDWR, S_IRUSR | S_IWUSR);
	if(mfilePtr->fd < 0){
	  return -1;
    }
	#endif
	
	#ifdef _WIN32
	mfilePtr->fileview = (LPTSTR) MapViewOfFile(mfilePtr->file, 
	  FILE_MAP_ALL_ACCESS, 0, 0, CWPX_BUF_SIZE);
	  
    if (mfilePtr->fileview == NULL){
	  CloseHandle(mfilePtr->file);
	  mfilePtr->file = NULL;
	  /* free evertyhing */
	  exit(1);
    }
    #else
    /* create memory mapping */
    mfilePtr->fileview = 
	mmap(0, CWPX_BUF_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mfilePtr->fd,0);
	if(mfilePtr->fileview == NULL){
	  close(mfilePtr->fd);
	  mfilePtr->fd = -1;
      /* free evertyhing */
      exit(-1);
    }
    /* once the mapping is done, we close the associated file */
    close(mfilePtr->fd);
    mfilePtr->fd = -1;
    #endif
    
    /* the actual pointer (or view) to the memory-mapped file */
	viewPtr = (char*)mfilePtr->fileview;
    
  }
  else{
    /* if it's not a memorymappedfile, then read the file into a buffer and 
	create something similar as the view. when reading the file, we don't mean 
	to exit if the file doesn't exist, but just returning an error */
	
	mfilePtr->file = fopen(mfilePtr->filename, "rb");/*"rb");ab+*/
    if(mfilePtr->file == NULL){
      /* free evertyhing */
      /* log the error 'GetLastError()' */
      return -1;
    }
    fflush(mfilePtr->file);
    
	char *tryMalloc = malloc(CWPX_BUF_SIZE);
	if(tryMalloc == NULL){
	  fclose(mfilePtr->file);
	  mfilePtr->file = NULL;
      /* free everything */ 
      exit(1); 
    }
	viewPtr = tryMalloc;
    
    int tries = 3;
    unsigned long totalread = 0;
    unsigned long remaining = CWPX_BUF_SIZE;
    
    while(totalread < CWPX_BUF_SIZE){
	  size_t read = 
	    fread(viewPtr + totalread, 1, remaining, mfilePtr->file);
	  
	  if(read <= 0){
		fflush(mfilePtr->file);
	    if(tries-- <= 0) break;
	    else continue;
	  }
	  
	  totalread += read;
	  remaining -= read;
	}
    fclose(mfilePtr->file);
    mfilePtr->file = NULL;
  }
	
  /* the actual pointer (or view) to the memory-mapped file */
  /*char *viewPtr = (char*)mfilePtr->fileview;*/
	
  /* skip the queue's name and then read the key-value pairs to fill the 
  actual queue */
  if(strstr(viewPtr, CWPX_CONTEXTQUEUE_ID) != NULL){
    viewPtr += strlen(CWPX_CONTEXTQUEUE_ID) + strlen(CWPX_CRLF);
		
    int fillingQueue = 1;
    const char *SPLITTER = ",";
		
    while(fillingQueue){
      /* read the current key. the current line should have the following
      pattern:
        varsize,varname,vartype,crlf
        Ex.: 4,intVar,int,\r\n */
			
      /* check if the current read is just a crlf that indicates the end */
      if(strncmp(viewPtr, CWPX_CRLF, CWPX_CRLF_LEN) == 0){
        /* printf("finishing reading the queue\n"); */
        break;
      }
		  
      unsigned long varsize = (unsigned long)atol(viewPtr);
      /* find and skip the ',' */
      char *varnamePtr = strstr(viewPtr, SPLITTER) + 1;
      char *varnamePtrEnd = strstr(varnamePtr, SPLITTER);
      size_t varnameLen = (unsigned long)(varnamePtrEnd - varnamePtr);
		  
      char *vartypePtr = varnamePtrEnd + 1;
      char *vartypePtrEnd = strstr(vartypePtr, SPLITTER);
      size_t vartypeLen = (unsigned long)(vartypePtrEnd - vartypePtr);
      char varType[50];
      strncpy(varType, vartypePtr, vartypeLen);
      varType[vartypeLen] = 0;
      char determinedType[50];
      int complex = cwpx_determine_vartype(varType, determinedType);
		  
      /* start creating the node using the above data */
      Cwpx_ObjectNode *newObject = NULL;
      cwpx_init_objectnode(&newObject);
  
      char *tryMalloc = malloc(varnameLen + 1);
      if(tryMalloc == NULL){ /* free everything */ exit(1); }
      newObject->name = tryMalloc;
      strncpy(newObject->name, varnamePtr, varnameLen);
      newObject->name[varnameLen] = 0;
  
      tryMalloc = malloc(strlen(determinedType) + 1);
      if(tryMalloc == NULL){ /* free everything */ exit(1); }
      newObject->type = tryMalloc;
      strcpy(newObject->type, determinedType);
          
      newObject->complex = complex;
          
      /* once the key is ready, move the pointer to the value */
      void *valuePtr = strstr(vartypePtrEnd, CWPX_CRLF) + CWPX_CRLF_LEN;
          
      /* buffer for storing the object */
      void *tryMallocObj = malloc(varsize + 1);
      if(tryMallocObj == NULL){ /* free everything */ exit(1); }
      newObject->value = tryMallocObj;
      memset(newObject->value, 0, varsize + 1);
          
      /* read the data */
      if(strcmp(newObject->type, "byte") == 0 || newObject->complex){
        memcpy(newObject->value, valuePtr, varsize);
        memset(newObject->value + varsize, 0, 1);/*add final 'terminator'*/
        newObject->size_of = varsize;
      }
      else{
        memcpy(newObject->value, valuePtr, varsize);
        newObject->size_of = varsize;
      }
  
      oqueuePtr->push(&oqueuePtr, newObject);
		  
      /* move towards the next key */
      viewPtr = valuePtr + varsize + CWPX_CRLF_LEN;
		  
    }
  }
  
  
  if(mfilePtr->memorymapped){
    #ifdef _WIN32
    if (mfilePtr->fileview != NULL){
      UnmapViewOfFile(mfilePtr->fileview);
      mfilePtr->fileview = NULL;
	}
	if (mfilePtr->file != NULL){
      CloseHandle(mfilePtr->file);
      mfilePtr->file = NULL;
	}
	#else
	if (mfilePtr->fileview != NULL){
	  munmap(mfilePtr->fileview, CWPX_BUF_SIZE);
	  mfilePtr->fileview = NULL;
	}
	if (mfilePtr->fd > -1){
      close(mfilePtr->fd);
      mfilePtr->fd = -1;
    }
    #endif
  }
  else{
	/* it was already closed, but, for following the uniformity in code...*/
	if(mfilePtr->file != NULL){
      fclose(mfilePtr->file);
      mfilePtr->file = NULL;
    }
  }
  
  return 0;
}

void cwpx_init_objectnode(Cwpx_ObjectNode **_this){
  
  Cwpx_ObjectNode *tryMalloc = malloc(sizeof(Cwpx_ObjectNode));
  if(!tryMalloc){ /* free everything ... */ exit(1); }
  *_this = tryMalloc;
  Cwpx_ObjectNode *_thisPtr = *_this;
  
  _thisPtr->name = NULL;
  _thisPtr->type = NULL;
  _thisPtr->complex = 0;
  _thisPtr->value = NULL;
  _thisPtr->size_of = 0;
  
  _thisPtr->next = NULL;
}

void cwpx_init_objectqueue(Cwpx_ObjectQueue **_this){
  
  Cwpx_ObjectQueue *tryMalloc = malloc(sizeof(Cwpx_ObjectQueue));
  if(!tryMalloc){ /* free everything ... */ exit(1); }
  *_this = tryMalloc;
  Cwpx_ObjectQueue *_thisPtr = *_this;
  
  _thisPtr->head = NULL;
  
  _thisPtr->push = cwpx_push_object;
}


void cwpx_init_memmappedfile(Cwpx_MemMappedFile **_this, const char *filename){
  Cwpx_MemMappedFile *tryMalloc = malloc(sizeof(Cwpx_MemMappedFile));
  if(!tryMalloc){ /* free everything ... */ exit(1); }
  *_this = tryMalloc;
  Cwpx_MemMappedFile *_thisPtr = *_this;
  
  _thisPtr->file = NULL;
  _thisPtr->fileview = NULL;
  
  _thisPtr->filename = NULL;
  if(filename != NULL){
    char *tryMallocStr = malloc(strlen(filename) + 1);
    if(!tryMallocStr){ /* free everything ... */ exit(1); }
    _thisPtr->filename = tryMallocStr;
    strcpy(_thisPtr->filename, filename);
  }
  
  _thisPtr->fd = -1;
  
  _thisPtr->memorymapped = 0;
}

void cwpx_init_memfile(Cwpx_MemFile **_this, const char *filename, 
  int memorymapped){
  if(filename == NULL) return;
  
  Cwpx_MemFile *tryMalloc = malloc(sizeof(Cwpx_MemFile));
  if(!tryMalloc){ /* free everything ... */ exit(1); }
  *_this = tryMalloc;
  Cwpx_MemFile *_thisPtr = *_this;
  
  cwpx_init_objectqueue(&_thisPtr->queue);
  cwpx_init_memmappedfile(&_thisPtr->file, filename);
  /* cwpx_init_memmappedfile defaults 0 for memorymapped attribute, so we have
  to assign the parameter after initialization */
  _thisPtr->file->memorymapped = memorymapped;
  
  _thisPtr->sessionstarted = 0;
}


int cwpx_push_object(Cwpx_ObjectQueue **_this, Cwpx_ObjectNode *node){

  if(*_this == NULL || node == NULL) return -1;
  
  Cwpx_ObjectQueue *_thisPtr = *_this;
	  
  if( _thisPtr->head == NULL){
    _thisPtr->head = node;
    node->next = NULL;
  }
  else{
    Cwpx_ObjectNode *it = _thisPtr->head;
    while(it->next)
      it = it->next;
    it->next = node;
    node->next = NULL;
  }
  
  return 0;
}

void cwpx_destroy_objectnode(Cwpx_ObjectNode **_this){
  if(*_this == NULL) return;
  
  Cwpx_ObjectNode *_thisPtr = *_this;
  
  if(_thisPtr->name != NULL){
    free(_thisPtr->name);
    _thisPtr->name = NULL;
  }
  if(_thisPtr->type != NULL){
    free(_thisPtr->type);
    _thisPtr->type = NULL;
  }
  if(_thisPtr->value != NULL){
    free(_thisPtr->value);
    _thisPtr->value = NULL;
  }
  
  _thisPtr->size_of = 0;
  _thisPtr->next = NULL;
  
  free(*_this); 
  *_this = NULL;
}


void cwpx_destroy_objectqueue(Cwpx_ObjectQueue **_this){
  if(*_this == NULL) return;
  
  Cwpx_ObjectQueue *_thisPtr = *_this;
  
  Cwpx_ObjectNode *it = _thisPtr->head;
  while(it != NULL){
    Cwpx_ObjectNode *nodeToDelete = it;
    it = it->next;
    cwpx_destroy_objectnode(&nodeToDelete);
  }
  
  free(*_this); 
  *_this = NULL;
}


void cwpx_destroy_memmappedfile(Cwpx_MemMappedFile **_this){
  if(*_this == NULL) return;
  
  Cwpx_MemMappedFile *_thisPtr = *_this;
  
  if(_thisPtr->memorymapped){
    #ifdef _WIN32
    if(_thisPtr->fileview != NULL){
      UnmapViewOfFile(_thisPtr->fileview);
      _thisPtr->fileview = NULL;
    }
    if(_thisPtr->file != NULL){
      CloseHandle(_thisPtr->file);
      _thisPtr->file = NULL;
    }
    #else
    if(_thisPtr->fileview != NULL){
      munmap(_thisPtr->fileview, CWPX_BUF_SIZE);
      _thisPtr->fileview = NULL;
    }
    if(_thisPtr->fd > -1){
      close(_thisPtr->fd);
      _thisPtr->fd = -1;
    }
    if(_thisPtr->file != NULL){
      fclose(_thisPtr->file);
      _thisPtr->file = NULL;
    }
    #endif
  }
  else{
    if(_thisPtr->file != NULL){
      fclose(_thisPtr->file);
      _thisPtr->file = NULL;
    }
  }
  
  if(_thisPtr->filename != NULL){
    free(_thisPtr->filename);
    _thisPtr->filename = NULL;
  }
  
  _thisPtr->memorymapped = 0;
  
  free(*_this); 
  *_this = NULL;
}

void cwpx_destroy_memfile(Cwpx_MemFile **_this){
  if(*_this == NULL) return;
	
  Cwpx_MemFile *_thisPtr = *_this;
  
  cwpx_destroy_objectqueue(&_thisPtr->queue);
  cwpx_destroy_memmappedfile(&_thisPtr->file);
}

int cwpx_determine_vartype(const char *vartypepattern, char *destination){
  
  if(vartypepattern == NULL || destination == NULL) return -1;
  
  char *vartype = (char *)vartypepattern;
  
  /* the first time this function is called by a particular entity, the vartype
  pattern is supposed to come in any of these forms:
		'sizeof(type)', 'strlen(char *)' or 'just_a_valid_number'
  if it's a number, there is no way to determine the vartype from that, so the
  determination for that case is 'byte' and the function returns 0. the same
  happens in the case of 'strlen' pattern. however, when the pattern is sizeof,
  the type might be determined from that ...
  subsequent calls to this function by the same entity would be made for
  retrieving the type it previously set, so, if the above patterns are not 
  present, the we know the call is a subsequent one */
  
  int isComplex = 0;
  
  if(strstr(vartype, "strlen(") != NULL){
    sprintf(destination, "%s", "byte"); /* "char *" */
    return isComplex;
  }
  
  /* check if it's only a number (valid size_t), no letters */
  cwpx_seterror(0);
  char *eptr;
  long vartypeToLong = (long)strtoul(vartype, &eptr, 10);
  if (*eptr != '\0') { /* invalid number syntax error */}
  else if (eptr == vartype) { /* invalid number '' empty string */ }
  else if (cwpx_geterror()) { /* GetLastError()(errno) */ }
  else if (vartypeToLong == LONG_MIN || vartypeToLong == LONG_MAX){
    /* not checking errno == ERANGE (not present in Windows), so just don't 
    count in the  limits */
  }
  else {
    /* number is valid, no letters ... */
    sprintf(destination, "%s", "byte"); /* "char *" */
    return isComplex;
  }
  
  char filteredVartype[50];
  strcpy(filteredVartype, ""); /* default value if sscanf fails */
  
  int allowedSpaces = 0;
  while(allowedSpaces <= 15){
    char regexPattern[100];
    sprintf(regexPattern, "sizeof(%*s", allowedSpaces, "");
    strcat(regexPattern, "%[ _a-zA-Z0-9]\\w*$)]");
    sscanf(vartype, regexPattern, filteredVartype);
    allowedSpaces++;
  }
  char *rtrimPtr = filteredVartype + strlen(filteredVartype) - 1;
  while(*rtrimPtr == ' '){ *rtrimPtr = 0; rtrimPtr--; }
  
  /* validate that variable name doesn't start with a number */
  if(isdigit(filteredVartype[0])){
	/* this case theorically will never happen because the compiler won't let 
	compile if it encounters variable names starting with numbers. this is here
	for the sake of the algorithm */
    sprintf(destination, "%s", "undetermined");
    return -1;
  }
  
  /* if filteredVartype is empty, the pattern doesn't contain 'sizeof' and the
  filter finishes there. the next thing is to know the type ... */
  if(strcmp(filteredVartype, "") != 0)
    vartype = (char *)filteredVartype;
  
  char *CWPX_ASTERISK = "*";
  //char *CWPX_SPACE = " ";
  //char *CWPX_EMPTY = "";
  char determinedType[50];
  
  if(strstr(vartype, "char")){
	sprintf(determinedType, "%s%s%s", 
      strstr(vartype, "unsigned")? "unsigned " : 
	    strstr(vartype, "signed")? "signed ": "",
      "char ",
      strstr(vartype, CWPX_ASTERISK)? "* " : "");
    determinedType[strlen(determinedType)-1] = 0;
	/* 'char *' would not make any sense in this context */
  }
  
  else if(strstr(vartype, "short")){
    sprintf(determinedType, "%s%s%s%s", 
      strstr(vartype, "unsigned")? "unsigned " : 
	    strstr(vartype, "signed")? "signed " : "",
      "short ",
      strstr(vartype, "int")? "int " : "",
      strstr(vartype, CWPX_ASTERISK)? "* " : "");
    determinedType[strlen(determinedType)-1] = 0;
  }
  
  else if(strstr(vartype, "long")){
    char *longLong = strstr(vartype, "long") + strlen("long");
    longLong = strstr(longLong, "long");
    char longType[15];
    strcpy(longType, "long ");
    if(longLong) strcat(longType, "long ");
    sprintf(determinedType, "%s%s%s%s", 
      strstr(vartype, "unsigned")? "unsigned " : 
	    strstr(vartype, "signed")? "signed " : "",
      longType,
      strstr(vartype, "int")? "int ": strstr(vartype, "double")? "double ": "",
      strstr(vartype, CWPX_ASTERISK)? "* " : "");
    determinedType[strlen(determinedType)-1] = 0;
  }
  
  else if(strstr(vartype, "signed")){
    char *unsignedT = strstr(vartype, "unsigned");
    char signType[10];
    strcpy(signType, " signed");
    if(unsignedT) strcpy(signType, " unsigned");
    sprintf(determinedType, "%s%s%s", 
      signType,
      strstr(vartype, "int")? "int " : "",
      strstr(vartype, CWPX_ASTERISK)? "* " : "");
    determinedType[strlen(determinedType)-1] = 0;
  }
  
  else if(strstr(vartype, "int")){
    sprintf(determinedType, "%s%s", 
      "int ",
      strstr(vartype, CWPX_ASTERISK)? "* " : "");
    determinedType[strlen(determinedType)-1] = 0;
  }
  
  else if(strstr(vartype, "float")){
    sprintf(determinedType, "%s%s", 
      "float ",
      strstr(vartype, CWPX_ASTERISK)? "* " : "");
    determinedType[strlen(determinedType)-1] = 0;
  }
  
  else if(strstr(vartype, "double")){
    sprintf(determinedType, "%s%s", 
      "double ",
      strstr(vartype, CWPX_ASTERISK)? "* " : "");
    determinedType[strlen(determinedType)-1] = 0;
  }
  
  else{
    sprintf(determinedType, "%s", vartype);
	isComplex = 1;
  }
  
  vartype = (char *)vartypepattern;
  
  /* comparing directly, as this: 
		if(strstr(vartype, "[") != NULL && strstr(vartype, "]") != NULL)
  provokes appcrash, therefore, we do it one by one */
  int containsBrackets = 0;
  if(strstr(vartypepattern, "[") != NULL ) containsBrackets = 1;
  if(strstr(vartypepattern, "]") == NULL ) containsBrackets = 0;
  
  int isBidimenMatrix = 0;
  char *matrixPtr = strstr(vartypepattern, "["); /* find first [ */ 
  matrixPtr = matrixPtr != NULL ? strstr(matrixPtr + 1, "["):
    NULL; /* find second [ */
  if(matrixPtr != NULL){ /* we've found 2 [ brackets, now find 2 ] brackets*/
    matrixPtr = strstr(vartypepattern, "]"); /* first ] */ 
    matrixPtr = matrixPtr != NULL ? strstr(matrixPtr + 1, "]"):
      NULL; /* second ] */
    if(matrixPtr != NULL) isBidimenMatrix = 1;
  }
  
  if(containsBrackets){
	/* there are two possibilities. the first time the type is determined by a 
	particular entity, the pattern comes like 'vartype[size_t]' (specifying the
	size of the array between the []). on subsequent calls from the same entity 
	the pattern will come like 'vartype[]' (without any size). that '[]' is the
	one we add right here when it is the first call */
	
	if(!isBidimenMatrix){
	  int subsequentArray = strstr(vartypepattern, "[]") != NULL;
	  if(subsequentArray && strstr(determinedType, "[]") == NULL){
        strcat(determinedType, "[]");
	  }
	  else if(!subsequentArray){
	    /* it's the first time determining the array type */
	    strcat(determinedType, "[]");
	  }
    }
    else if(isBidimenMatrix){
	  int subsequentMatrix = strstr(vartypepattern, "[][]") != NULL;
	  if(subsequentMatrix && strstr(determinedType, "[][]") == NULL){
        strcat(determinedType, "[][]");
	  }
	  else if(!subsequentMatrix){
	    /* it's the first time determining the matrix type */
	    strcat(determinedType, "[][]");
	  }
	}
  }
  
  strcpy(destination, determinedType);
  
  return isComplex;
}
