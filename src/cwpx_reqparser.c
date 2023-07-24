/*
 * gcc -Wall -Wextra -Wconversion -c src/cwpx_reqparser.c -I"include" -o lib/cw
 * px_reqparser.o
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cwpx_reqparser.h>
#include <cwpx_misc.h>

#define CWPX_PARSEBUFSIZE 1024
#define CWPX_BOUNDARYSIZE 75
#define CWPX_BOUNDARYEXTRA 5

void cwpx_init_requestparser(Cwpx_RequestParser **_this){
  
  Cwpx_RequestParser *tryMalloc = malloc(sizeof(Cwpx_RequestParser));
  if(!tryMalloc){ /* free everything ... */ exit(1); }
  *_this = tryMalloc;
  Cwpx_RequestParser *_thisPtr = *_this;
  
  _thisPtr->temp_node = NULL;
  _thisPtr->parse_state = CWPX_FILLINGKEY;
    
  /* methods for body ... */
  _thisPtr->parse_multipartformdata = cwpx_parse_multipartformdata;
  _thisPtr->parse_urlencoded = cwpx_parse_urlencoded;
  _thisPtr->parse_rawbody = cwpx_parse_rawbody;
}

/* methods for body */
void cwpx_parse_multipartformdata(Cwpx_RequestParser **_this, 
  char *chunk, size_t chunklength, 
  Cwpx_NodeQueue **nodequeue, Cwpx_FileNodeQueue **filenodequeue, 
  char *boundary){
		
  if(*_this == NULL || chunk == NULL || 
    *nodequeue == NULL || boundary == NULL) return;
  
  Cwpx_RequestParser *_thisPtr = *_this;
  Cwpx_NodeQueue *nodequeuePtr = *nodequeue;
  Cwpx_FileNodeQueue *filenodequeuePtr = *filenodequeue;

  const char *TWOHYPHENS = "--";
  char boundaryPattern[CWPX_BOUNDARYSIZE];
  sprintf(boundaryPattern, 
    "%s%.*s", TWOHYPHENS, (CWPX_BOUNDARYSIZE - CWPX_BOUNDARYEXTRA), boundary);
  boundaryPattern[CWPX_BOUNDARYSIZE - CWPX_BOUNDARYEXTRA + 1] = 0;
  char *chunkPtr = chunk;
  char *chunkEnd = chunkPtr + chunklength;
  size_t remainingLength = chunklength;
  
  while(chunkPtr < chunkEnd){
    if(_thisPtr->parse_state == CWPX_FILLINGKEY){
      /* init the current node */
      if(!_thisPtr->temp_node){
          Cwpx_Node *tryMalloc = malloc(sizeof(Cwpx_Node));
          if(!tryMalloc){
            /* free everything ... */
            exit(1);
          }          
          Cwpx_Node *newNodeObject = tryMalloc;
          newNodeObject->key = NULL;
          newNodeObject->value = NULL;
          newNodeObject->keyLen = 0;
          newNodeObject->valueLen = 0;
          newNodeObject->next = NULL;
          _thisPtr->temp_node = newNodeObject;
      }
      
      /* if we're filling a key, let's look for double newline. 
	  If no double newline, just continue filling the Node key. 
	  If there is a double newline, 
	  finish the current key and start filling the value */
      
      size_t temporaryKeyLength = remainingLength;
      
      char *emptyLine = NULL;
      unsigned traslapped = 0;
	  unsigned long keyLenLast = 0, keyLenFirst = 0, 
	    lastTraslappedLen = 0, firstTraslappedLen = 0;
      
      /* in case that double newline were 'traslapped', 
      check it by joining the last 3 chars of current forming key
      and the first 3 chars of current reading buffer */
      if(_thisPtr->temp_node->key != NULL){
		/*
		\r|\n\r\n
		\r\n|\r\n
		\r\n\r|\n
		*/
		unsigned SMSIZE = 3; /* max 3 chars on each traslapping side */
        char joinedBuffer[SMSIZE * 2 + 1];
        memset(joinedBuffer, 0, SMSIZE * 2 + 1);
        unsigned long keyPos = _thisPtr->temp_node->keyLen > SMSIZE ?
          _thisPtr->temp_node->keyLen - SMSIZE : 0;
        keyLenLast = keyPos == 0 ? _thisPtr->temp_node->keyLen : SMSIZE;
        memcpy(joinedBuffer, _thisPtr->temp_node->key + keyPos, keyLenLast);
        keyLenFirst = temporaryKeyLength > SMSIZE ?
        SMSIZE : temporaryKeyLength;
        memcpy(joinedBuffer + keyLenLast, chunkPtr, keyLenFirst);
        
        emptyLine = strstr(joinedBuffer, CWPX_DOUBLECRLF);
        if(emptyLine){
          /* determine how many bytes from current emptyLine were traslapped */
          char *traslappedPtr = joinedBuffer + keyLenLast;
          lastTraslappedLen = (unsigned)(traslappedPtr - emptyLine);
          firstTraslappedLen = strlen(CWPX_DOUBLECRLF) - lastTraslappedLen;
          
          /* recalculate key length, again */
          temporaryKeyLength = 0;
          /* reset 'emptyLine' for mapping it to chunkPtr, 
		  not to joinedBuffer */
          emptyLine = chunkPtr;
          traslapped = 1;
        }
	  }
	
	  if(!traslapped){
		emptyLine = strstr(chunkPtr, CWPX_DOUBLECRLF);
		int validatePosition = emptyLine < chunkPtr + temporaryKeyLength - 1;
		if(!validatePosition){
		  emptyLine = NULL;
		}
        if(emptyLine){
          /* recalculate key length*/
          temporaryKeyLength = (unsigned)(emptyLine - chunkPtr);
        }
	  }
      
      
      char *tryMalloc = _thisPtr->temp_node->key != NULL ? 
        realloc(_thisPtr->temp_node->key, 
          _thisPtr->temp_node->keyLen + temporaryKeyLength + 1) : 
        malloc(temporaryKeyLength + 1);
      if(!tryMalloc){ /* free everything ... */ exit(1); }
      _thisPtr->temp_node->key = tryMalloc;
      memcpy(_thisPtr->temp_node->key + _thisPtr->temp_node->keyLen, 
	    chunkPtr, temporaryKeyLength);
      _thisPtr->temp_node->keyLen += temporaryKeyLength;
      _thisPtr->temp_node->key[_thisPtr->temp_node->keyLen] = 0;
      
      
      if(emptyLine){
        /* reset buffer pointer and length to start from the value */
        chunkPtr = !traslapped ? 
		  emptyLine + strlen(CWPX_DOUBLECRLF) : emptyLine + firstTraslappedLen;
        remainingLength -= !traslapped ? 
          temporaryKeyLength + strlen(CWPX_DOUBLECRLF) :
          temporaryKeyLength + firstTraslappedLen;
        
        /* if key ends with \r */
        if(traslapped && 
		  _thisPtr->temp_node->key[_thisPtr->temp_node->keyLen - 1] == 13){
          _thisPtr->temp_node->key[_thisPtr->temp_node->keyLen - 1] = 0;
          _thisPtr->temp_node->keyLen--;
		}
          
        /* set state to "FILLINGVALUE" */
        _thisPtr->parse_state = CWPX_FILLINGVALUE;
      }
      else{
        chunkPtr += remainingLength;
	  }
    }
  
    if(_thisPtr->parse_state == CWPX_FILLINGVALUE){
        
      /* almost the same process of 'fillingKey', 
      but finding boundary instead of newline.
	  the node was already initialized in FILLINGKEY */
      
      /* if we're filling a value, let's look for the boundary. If no boundary,
      just continue filling the Node vale. If there is a boundary, finish the 
      current value and start filling a new key (and a new node) */
      
      size_t temporaryValueLength = remainingLength;
      
      char *boundaryPtr = NULL;
      unsigned traslapped = 0;
	  unsigned long valueLenLast = 0, valueLenFirst = 0, 
	    lastTraslappedLen = 0, firstTraslappedLen = 0;
	    
      /* in case that boundary were 'traslapped', 
      check it by joining the last 75 chars of current forming value
      and the first 75 chars of current reading buffer */
      if(_thisPtr->temp_node->value != NULL && 
	    _thisPtr->temp_node->valueLen > 0){
			
        char joinedBuffer[CWPX_BOUNDARYSIZE * 2];
        unsigned long valuePos = 
		  _thisPtr->temp_node->valueLen > CWPX_BOUNDARYSIZE ?
          _thisPtr->temp_node->valueLen - CWPX_BOUNDARYSIZE : 0;
        valueLenLast = valuePos == 0 ? 
		  _thisPtr->temp_node->valueLen : CWPX_BOUNDARYSIZE;
        memcpy(joinedBuffer,_thisPtr->temp_node->value+valuePos,valueLenLast);
        valueLenFirst = temporaryValueLength > CWPX_BOUNDARYSIZE ?
        CWPX_BOUNDARYSIZE : temporaryValueLength;
        memcpy(joinedBuffer + valueLenLast, chunkPtr, valueLenFirst);
        char *joinedBufferPtr = joinedBuffer;
        char *joinedBufferEnd = joinedBufferPtr + (valueLenLast+valueLenFirst);
        boundaryPtr = 
		  cwpx_memstr(&joinedBufferPtr, boundaryPattern, &joinedBufferEnd);
        
        /* THERE IS A POSSIBLE BUG HERE, BUT CAN'T FIND (BY NOW) WHETHER IT
		STILL PERSISTS. IT MIGHT HAPPEN THAT THE BOUNDARY IS FOUND IN THE 
		JOINED BUFFER, BUT IT'S NOT THE ONE THAT WAS 'JOINED', 
		BUT ONE THAT COMES IN THE FIRST 75 CHARS OF THE CURRENT CHUNK, 
		WHICH FITS IN THE JOINED BUFFER AND THUS, 
		IT'S TAKEN AS THE FOUND BOUNDARY.
		ONE FIX SOLUTION TO THIS WAS TO CHECK IF THE CURRENT NODE'S VALUE 
		ACTUALLY HAS SOMETHING (temporaryNode->valueLen > 0). THIS CHECKING IS
		AT THE CURRENT 'IF' STATEMENT:
		  if(temporaryNode->value != NULL && temporaryNode->valueLen > 0)
        */
        
        if(boundaryPtr){
          /* determine how many bytes from current boundary were traslapped */
          char *traslappedPtr = joinedBuffer + valueLenLast;
          lastTraslappedLen = (unsigned)(traslappedPtr - boundaryPtr);
          firstTraslappedLen = strlen(boundaryPattern) - lastTraslappedLen;
            
          /* recalculate value length, again */
          temporaryValueLength = 0;
          /* reset 'boundaryPtr' for mapping it with chunkPtr, 
		  not to joinedBuffer */
          boundaryPtr = chunkPtr;
          traslapped = 1;
        }
      }
      
      if(!traslapped){
	    boundaryPtr = cwpx_memstr(&chunkPtr, boundaryPattern, &chunkEnd);
        if(boundaryPtr){
          /* recalculate value length */
          temporaryValueLength = (unsigned)(boundaryPtr - chunkPtr);
        }
	  }
      
      /*char *tryMalloc = _thisPtr->temp_node->value != NULL ? 
        realloc(_thisPtr->temp_node->value, 
          _thisPtr->temp_node->valueLen + temporaryValueLength + 1) : 
        malloc(temporaryValueLength + 1);*/
      /* 3/7/22: THE INSTRUCTION COMMENTED ABOVE WAS CHANGED BY THE FOLLOWING,
	  because in case that temporaryValueLength is 0, we don't need to realloc,
	  saving time and resources */
	  char *tryMalloc = _thisPtr->temp_node->value != NULL ? 
        temporaryValueLength > 0 ? realloc(_thisPtr->temp_node->value, 
          _thisPtr->temp_node->valueLen + temporaryValueLength + 1) : 
		  _thisPtr->temp_node->value :
        malloc(temporaryValueLength + 1);
      /* 3/7/22: In addition to that change, WE FOUND A BUG THAT WE THOUGHT HAD
	  BEEN ALREADY FIXED AND TESTED, but not. It happens when it's a traslapped
	  boundary. Everything is alright (the previous calculations in this very 
	  function are good), but the current evaluated value has a piece of the 
	  traslapped boundary concatenated on it, an it seems that we forgot to 
	  remove that piece and thus we're not getting the actual value, but 
	  'value+pieceofboundary'. The next line fixes this until future tests */
	  if(traslapped && lastTraslappedLen > 0)
	    _thisPtr->temp_node->valueLen -= lastTraslappedLen;
	  
        
      if(!tryMalloc){ /* free everything ... */ exit(1); }
      _thisPtr->temp_node->value = tryMalloc;
      memcpy(_thisPtr->temp_node->value + _thisPtr->temp_node->valueLen, 
	    chunkPtr, temporaryValueLength);
      _thisPtr->temp_node->valueLen += temporaryValueLength;
      _thisPtr->temp_node->value[_thisPtr->temp_node->valueLen] = 0;
      
      if(boundaryPtr){
        /* reset buffer pointer and length to start from the key */
        chunkPtr = !traslapped ? 
		  boundaryPtr + strlen(boundaryPattern) : 
		  boundaryPtr + firstTraslappedLen;
        remainingLength -= !traslapped ? 
          temporaryValueLength + strlen(boundaryPattern) :
          temporaryValueLength + firstTraslappedLen;
          
        /* when a boundary was found, 
		the newline (\r\n) before that boundary 
		comes in the value, so we have to remove it */
		_thisPtr->temp_node->valueLen -= 2;
		_thisPtr->temp_node->value[_thisPtr->temp_node->valueLen] = 0;
          
        /* set state to "fillingKey" */
        _thisPtr->parse_state = CWPX_FILLINGKEY;
        
        /* finish the current node */
        //pushNode(&*queue, &*fqueue, temporaryNode);
       nodequeuePtr->push(&nodequeuePtr, &filenodequeuePtr, 
	     _thisPtr->temp_node, 1);
        _thisPtr->temp_node = NULL; /* must be NULL for next node creation */
      }
      else{
        chunkPtr += remainingLength;
	  }
    }
  }
}


void cwpx_parse_urlencoded(Cwpx_RequestParser **_this, 
  char *chunk, size_t chunklength, Cwpx_NodeQueue **nodequeue, 
  char *boundary){
  
  if(*_this == NULL || chunk == NULL || *nodequeue == NULL) return;
  
  Cwpx_RequestParser *_thisPtr = *_this;
  Cwpx_NodeQueue *nodequeuePtr = *nodequeue;
  
  char *chunkPtr = chunk;
  char *chunkEnd = chunkPtr + chunklength;
  size_t remainingLength = chunklength;
  
  while(chunkPtr < chunkEnd){
    if(_thisPtr->parse_state == CWPX_FILLINGKEY){
      /* init the current node */
      if(!_thisPtr->temp_node){
          Cwpx_Node *tryMalloc = malloc(sizeof(Cwpx_Node));
          if(!tryMalloc){ /* free everything ... */ exit(1); }          
          Cwpx_Node *newNodeObject = tryMalloc;
          newNodeObject->key = NULL;
          newNodeObject->value = NULL;
          newNodeObject->keyLen = 0;
          newNodeObject->valueLen = 0;
          newNodeObject->next = NULL;
          _thisPtr->temp_node = newNodeObject;
      }
      
      /* Let's treat this different from multipart.
	  In this case, if we're filling a key, let's look for the whole key=value 
	  pair, that means, let's look for the ampersand ('&') symbol.
	  If no ampersand symbol, just continue filling the Node key. 
	  If there is an ampersand symbol, 
	  finish the current key and start filling the value (splits the current 
	  key) */
      
      size_t temporaryKeyLength = remainingLength;
      
      char *boundaryPtr = cwpx_memstr(&chunkPtr, boundary, &chunkEnd);
	  unsigned validatePosition = 
	    boundaryPtr < chunkPtr + temporaryKeyLength - 1;
	  if(!validatePosition){
	    /*ampersandSymbol = NULL;*/
	  }
      if(boundaryPtr){
        /* recalculate key length */
        temporaryKeyLength = (unsigned)(boundaryPtr - chunkPtr);
	  }
      
      char *tryMalloc = _thisPtr->temp_node->key != NULL ? 
        realloc(_thisPtr->temp_node->key, 
          _thisPtr->temp_node->keyLen + temporaryKeyLength + 1) : 
        malloc(temporaryKeyLength + 1);
      if(!tryMalloc){ /* free everything ... */ exit(1); }
      _thisPtr->temp_node->key = tryMalloc;
      memcpy(_thisPtr->temp_node->key + _thisPtr->temp_node->keyLen, 
	    chunkPtr, temporaryKeyLength);
      _thisPtr->temp_node->keyLen += temporaryKeyLength;
      _thisPtr->temp_node->key[_thisPtr->temp_node->keyLen] = 0;
      
      
      if(boundaryPtr){
        /* reset buffer pointer and length to start from the value */
        chunkPtr = boundaryPtr + strlen(boundary);
        remainingLength -= temporaryKeyLength + strlen(boundary);
          
        /* set state to "fillingValue" */
        _thisPtr->parse_state = CWPX_FILLINGVALUE;
      }
      else{
        chunkPtr += remainingLength;
	  }
    }
  
    if(_thisPtr->parse_state == CWPX_FILLINGVALUE){
        
      /* the current key has both the key and the value.
      the node was already initialized in fillingKey */
      
      /* if we're filling a value, let's look for the equal symbol. 
	  If no equal symbol,
      just continue filling the Node vale. If there is a boundary, finish the 
      current value and start filling a new key (and a new node) */
      
      int hasValue = 0;
      size_t temporaryValueLength = 0;
      
      char *equalPtr = strstr(_thisPtr->temp_node->key, CWPX_EQUAL);
      if(equalPtr){
        /* check real key length */
        size_t keyLen = (unsigned)(equalPtr - _thisPtr->temp_node->key);
        /* recalculate value length */
        temporaryValueLength = 
		  _thisPtr->temp_node->keyLen - keyLen - strlen(CWPX_EQUAL);
        /* recalculate key length */
        _thisPtr->temp_node->keyLen = keyLen;
        
        hasValue = 1;
	  }
	  
	  if(hasValue){
		char *tryMalloc = malloc(temporaryValueLength + 1);
        if(!tryMalloc){ /* free everything ... */ exit(1); }
        _thisPtr->temp_node->value = tryMalloc;
        memcpy(_thisPtr->temp_node->value, 
	     _thisPtr->temp_node->key + _thisPtr->temp_node->keyLen + 
		 strlen(CWPX_EQUAL), temporaryValueLength);
        _thisPtr->temp_node->valueLen = temporaryValueLength;
        _thisPtr->temp_node->value[_thisPtr->temp_node->valueLen] = 0;
        
        /* decode the value, update the length */
        cwpx_decode_x_www_form_urlencoded(
		  _thisPtr->temp_node->value, _thisPtr->temp_node->value);
        _thisPtr->temp_node->valueLen = strlen(_thisPtr->temp_node->value);
			
        char *tryRealloc = 
		  realloc(_thisPtr->temp_node->key, _thisPtr->temp_node->keyLen + 1);
        if(!tryRealloc){ /* free everything ... */ exit(1); }
        _thisPtr->temp_node->key = tryRealloc;
        _thisPtr->temp_node->key[_thisPtr->temp_node->keyLen] = 0;
	  }
      
      /* set state to "fillingKey" */
      _thisPtr->parse_state = CWPX_FILLINGKEY;
        
      /* finish the current node */
      nodequeuePtr->push(&nodequeuePtr, NULL, 
	    _thisPtr->temp_node, 0);
      _thisPtr->temp_node = NULL; /* needs to be NULL for next node creation */
    }
  }
}


void cwpx_parse_rawbody(Cwpx_RequestParser **_this, 
  char *chunk, size_t chunklength, Cwpx_FileNodeQueue **filenodequeue){
		
  if(*_this == NULL || chunk == NULL || *filenodequeue == NULL) return;
  
  Cwpx_RequestParser *_thisPtr = *_this;
  _thisPtr = _thisPtr; /* avoid 'unused' warning */
  Cwpx_FileNodeQueue *filenodequeuePtr = *filenodequeue;

  char *chunkPtr = chunk;
  char *chunkEnd = chunkPtr + chunklength;
  size_t remainingLength = chunklength;
  
  while(chunkPtr < chunkEnd){
    /* init the current and only one node */
    if(filenodequeuePtr->head == NULL){
      Cwpx_FileNode *tryMalloc = malloc(sizeof(Cwpx_FileNode));
      if(!tryMalloc){ /* free everything ... */ exit(1); }
      tryMalloc->key = NULL;
      tryMalloc->filename = NULL;
      tryMalloc->temp_filename = NULL;
      tryMalloc->content_type = NULL;
      tryMalloc->content_length = NULL;
      tryMalloc->fileLen = 0;
      tryMalloc->next = NULL;
      
      // init a fix-sized content_length buffer (will be constantly updated)
      char *mallocLen = malloc(CWPX_MIN_BUFLEN);
      if(!mallocLen){ /* free everything ... */ exit(1); }
      tryMalloc->content_length = mallocLen;
      
      // the last instructions 'push' the only one node to the queue
      filenodequeuePtr->head = tryMalloc;
    }
    
    Cwpx_FileNode *currentNode = filenodequeuePtr->head;
      
    /* Different from Multipart & UrlEncoded, here we don't have key=value
    pairs. We'll have just one Node in the queue, the node's key will be the
	actual value. We only have to worry about the value and don't have to worry 
	about where or when it finishes (the caller must handle this) */
      
    size_t temporaryValueLength = remainingLength;
    
    char *tryMalloc = currentNode->key != NULL ? 
      realloc(currentNode->key, 
        currentNode->fileLen + temporaryValueLength + 1) : 
      malloc(temporaryValueLength + 1);
    if(!tryMalloc){ /* free everything ... */ exit(1); }
    currentNode->key = tryMalloc;
    memcpy(currentNode->key + currentNode->fileLen, 
      chunkPtr, temporaryValueLength);
    currentNode->fileLen += temporaryValueLength;
    currentNode->key[currentNode->fileLen] = 0;
    
    /* OJO this function was changed because "argument to ‘sizeof’ in 
	‘snprintf’ call is the same expression as the destination; did you mean to 
	provide an explicit length?" */
	
    /*snprintf(currentNode->content_length, sizeof currentNode->content_length,
	  "%lu", (long unsigned)currentNode->fileLen);*/
	  
	/* was changed to this: */
	snprintf(currentNode->content_length, sizeof(currentNode->fileLen),
	  "%lu", (long unsigned)currentNode->fileLen);
    
    chunkPtr += remainingLength;
  }
}


void cwpx_destroy_requestparser(Cwpx_RequestParser **_this){
  if(*_this == NULL) return;
  
  Cwpx_RequestParser *_thisPtr = *_this;
  
  if(_thisPtr->temp_node != NULL){
    cwpx_destroy_node(&_thisPtr->temp_node);
  }
  
  _thisPtr->parse_state = 0;
  
  free(*_this);
  *_this = NULL;
}



