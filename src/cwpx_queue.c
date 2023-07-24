/*
 * gcc -Wall -Wextra -Wconversion -c src/cwpx_queue.c -I"include" -o lib/cwpx_q
 * ueue.o
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cwpx_queue.h>
#include <cwpx_misc.h>


void cwpx_init_node(Cwpx_Node **_this){
  
  Cwpx_Node *tryMalloc = malloc(sizeof(Cwpx_Node));
  if(!tryMalloc){ /* free everything ... */ exit(1); }
  *_this = tryMalloc;
  Cwpx_Node *_thisPtr = *_this;
  
  _thisPtr->key = NULL;
  _thisPtr->keyLen = 0;
  _thisPtr->value = NULL;
  _thisPtr->valueLen = 0;
  
  _thisPtr->next = NULL;

}

void cwpx_init_filenode(Cwpx_FileNode **_this){
  
  Cwpx_FileNode *tryMalloc = malloc(sizeof(Cwpx_FileNode));
  if(!tryMalloc){ /* free everything ... */ exit(1); }
  *_this = tryMalloc;
  Cwpx_FileNode *_thisPtr = *_this;
  
  _thisPtr->key = NULL;
  _thisPtr->filename = NULL;
  _thisPtr->temp_filename = NULL;
  _thisPtr->content_type = NULL;
  _thisPtr->content_length = NULL;
  _thisPtr->fileLen = 0;
  
  _thisPtr->next = NULL;

}


void cwpx_init_nodequeue(Cwpx_NodeQueue **_this){
  
  Cwpx_NodeQueue *tryMalloc = malloc(sizeof(Cwpx_NodeQueue));
  if(!tryMalloc){ /* free everything ... */ exit(1); }
  *_this = tryMalloc;
  Cwpx_NodeQueue *_thisPtr = *_this;
  
  _thisPtr->head = NULL;
  _thisPtr->count = 0;
  _thisPtr->content_type = CWPX_RAWBODY;
  
  _thisPtr->push = cwpx_push_node;
}

void cwpx_init_filenodequeue(Cwpx_FileNodeQueue **_this){
  
  Cwpx_FileNodeQueue *tryMalloc = malloc(sizeof(Cwpx_FileNodeQueue));
  if(!tryMalloc){ /* free everything ... */ exit(1); }
  *_this = tryMalloc;
  Cwpx_FileNodeQueue *_thisPtr = *_this;
  
  _thisPtr->head = NULL;
  _thisPtr->count = 0;
  
  _thisPtr->push = cwpx_push_filenode;
}


int cwpx_push_node(Cwpx_NodeQueue **_this, 
  Cwpx_FileNodeQueue **filenodequeue, Cwpx_Node *node, int pushfilenode){

  if(*_this == NULL || node == NULL) return -1;
  /* although '_this' isn't supposed to ever be NULL */
  
  Cwpx_NodeQueue *_thisPtr = *_this;
  Cwpx_FileNodeQueue *filenodequeuePtr = 
    filenodequeue != NULL ? *filenodequeue : NULL;
	  
  char *keyPtr = node->key;
  char *filenamePattern = "filename=\"";
  if(strstr(keyPtr, filenamePattern) != NULL){
    /* it may be a file, so validate that the current queue is multipart */
    if(_thisPtr->content_type == CWPX_MULTIPARTFORMDATA){
      if(pushfilenode)
        filenodequeuePtr->push(&filenodequeuePtr, node);
    }
  }
	  
  if( _thisPtr->head == NULL){
    _thisPtr->head = node;
    node->next = NULL;
  }
  else{
    Cwpx_Node *it = _thisPtr->head;
    while(it->next)
      it = it->next;
    it->next = node;
    node->next = NULL;
  }
  
  return 0;
}


int cwpx_push_filenode(Cwpx_FileNodeQueue **_this, Cwpx_Node *node){
	
  if(*_this == NULL || node == NULL) return -1;
	
  Cwpx_FileNodeQueue *_thisPtr = *_this;
  
  char *keyPtr = node->key;
  char *namePattern = "name=\"";
  char *namePatternEnd = "\"";
  char *filenamePattern = "filename=\"";
  char *filenamePatternEnd = namePatternEnd;
  char *namePtr = strstr(keyPtr, namePattern);
  char *filenamePtr = strstr(keyPtr, filenamePattern);

  if(namePtr == NULL || filenamePtr == NULL) return 0;
  if(filenamePtr + 4 == namePtr) return 0;
  /* namePtr is the same as filename, therefore +4 skips 'file' and just search
  for 'name' */
	
  Cwpx_FileNode *tryMalloc = malloc(sizeof(Cwpx_FileNode));
  if(!tryMalloc){ /* free everything ... */ exit(1); }          
  Cwpx_FileNode *newNodeObject = tryMalloc;
  newNodeObject->key = NULL;
  newNodeObject->filename = NULL;
  newNodeObject->temp_filename = NULL;
  newNodeObject->content_type = NULL;
  newNodeObject->content_length = NULL;
  newNodeObject->fileLen = 0;
  newNodeObject->next = NULL;
  
  namePtr += strlen(namePattern);
  char *namePtrEnd = strstr(namePtr, namePatternEnd);
  size_t nameLen = namePtrEnd != NULL ? (size_t)(namePtrEnd - namePtr) : 0;
  if(nameLen > 0){
    char *tryStrMalloc = malloc(nameLen + 1);
    if(!tryStrMalloc){ /* free everything ... */ exit(1); }
    newNodeObject->key = tryStrMalloc;
    strncpy(newNodeObject->key, namePtr, nameLen);
    newNodeObject->key[nameLen] = 0;
  }
  
  filenamePtr += strlen(filenamePattern);
  char *filenamePtrEnd = strstr(filenamePtr, filenamePatternEnd);
  size_t filenameLen = 
    filenamePtrEnd != NULL ? (size_t)(filenamePtrEnd - filenamePtr) : 0;
  if(filenameLen > 0){
    char *tryStrMalloc = malloc(filenameLen + 1);
    if(!tryStrMalloc){ /* free everything ... */ exit(1); }
    newNodeObject->filename = tryStrMalloc;
    strncpy(newNodeObject->filename, filenamePtr, filenameLen);
    newNodeObject->filename[filenameLen] = 0;
  }
  
  newNodeObject->fileLen = node->valueLen;
  char contentLength[CWPX_MIN_BUFLEN]; // snprintf() adds the 0-terminator.
  snprintf(contentLength, sizeof contentLength, "%lu", 
    (unsigned long)newNodeObject->fileLen);
  char *tryStrMalloc = malloc(strlen(contentLength) + 1);
  if(!tryStrMalloc){ /* free everything ... */ exit(1); }
  newNodeObject->content_length = tryStrMalloc;
  strncpy(newNodeObject->content_length, contentLength, strlen(contentLength));
  newNodeObject->content_length[strlen(contentLength)] = 0;
  
  char uuidStr[CWPX_MIN_BUFLEN] = "";
  cwpx_generate_uuid(uuidStr);
  
  size_t tempFilenameLen = strlen(uuidStr) + 1 + filenameLen + 1;
  tryStrMalloc = malloc(tempFilenameLen + 1);
  if(!tryStrMalloc){ /* free everything ... */ exit(1); }
  newNodeObject->temp_filename = tryStrMalloc;
  sprintf(newNodeObject->temp_filename, "%s.%s", 
    uuidStr, newNodeObject->filename);
  newNodeObject->temp_filename[tempFilenameLen] = 0;
  
  char *contentTypePattern = "Content-Type: ";
  char *contentTypePtr = strstr(keyPtr, contentTypePattern);
  if(contentTypePtr != NULL){
    contentTypePtr += strlen(contentTypePattern);
    char *contentTypePtrEnd = strstr(contentTypePtr, ";");
    contentTypePtrEnd = contentTypePtrEnd == NULL ? 
	  strstr(contentTypePtr, "\r") : contentTypePtrEnd;
    contentTypePtrEnd = contentTypePtrEnd == NULL ? 
	  strstr(contentTypePtr, "\n") : contentTypePtrEnd;
    contentTypePtrEnd = contentTypePtrEnd == NULL ? 
	  contentTypePtr + strlen(contentTypePtr) : contentTypePtrEnd;
    size_t contentTypeLen = contentTypePtrEnd != NULL ? 
	  (size_t)(contentTypePtrEnd - contentTypePtr) : 0;
	
    if(contentTypeLen > 0){
      char *tryStrMalloc = malloc(contentTypeLen + 1);
      if(!tryStrMalloc){ /* free everything ... */ exit(1); }
      newNodeObject->content_type = tryStrMalloc;
      strncpy(newNodeObject->content_type, contentTypePtr, contentTypeLen);
      newNodeObject->content_type[contentTypeLen] = 0;
    }
  }
  
  FILE *tmpFile = fopen(newNodeObject->temp_filename, "wb");/*remembertmpdir*/
  if(tmpFile == NULL){ /* free everything ... */ exit(1); }
  
  size_t remaining = node->valueLen;
  char *valuePtr = node->value;
  int tryError = 0;
  while(remaining > 0){
    size_t written = fwrite(valuePtr, 1, remaining, tmpFile);
    
    tryError += written == 0 ? 1 : 0;
    if(tryError == 3) break;
    
    remaining -= written;
    valuePtr += written;
  }
  fclose(tmpFile);
  /* SHOULD WE FREE node->value ?*/
  
  /* REMEMBER TO REMOVE ALL FILES AFTER THE SCRIPT HAS FINISHED EXECUTION */
	
  if( _thisPtr->head == NULL){
    _thisPtr->head = newNodeObject;
    newNodeObject->next = NULL;
  }
  else{
    Cwpx_FileNode *it = _thisPtr->head;
    while(it->next)
      it = it->next;
    it->next = newNodeObject;
    newNodeObject->next = NULL;
  }
  
  return 0;
}


void cwpx_destroy_node(Cwpx_Node **_this){

  if(*_this == NULL) return;
  
  Cwpx_Node *_thisPtr = *_this;
  
  if(_thisPtr->key != NULL){
    free(_thisPtr->key);
    _thisPtr->key = NULL;
  }
  if(_thisPtr->value != NULL){
    free(_thisPtr->value);
    _thisPtr->value = NULL;
  }
  _thisPtr->keyLen = 0;
  _thisPtr->valueLen = 0;
  _thisPtr->next = NULL;
  
  free(*_this);
  *_this = NULL;
}


void cwpx_destroy_filenode(Cwpx_FileNode **_this, int deletefile){

  if(*_this == NULL) return;
  
  Cwpx_FileNode *_thisPtr = *_this;
  
  if(_thisPtr->key != NULL){
    free(_thisPtr->key);
    _thisPtr->key = NULL;
  }
  if(_thisPtr->filename != NULL){
    free(_thisPtr->filename);
    _thisPtr->filename = NULL;
  }
  if(_thisPtr->temp_filename != NULL){
	if(deletefile) remove(_thisPtr->temp_filename);
    free(_thisPtr->temp_filename);
    _thisPtr->temp_filename = NULL;
  }
  if(_thisPtr->content_type != NULL){
    free(_thisPtr->content_type);
    _thisPtr->content_type = NULL;
  }
  if(_thisPtr->content_length != NULL){
    free(_thisPtr->content_length);
    _thisPtr->content_length = NULL;
  }
  _thisPtr->fileLen = 0;
  _thisPtr->next = NULL;
  
  free(*_this);  
  *_this = NULL;
}


void cwpx_destroy_nodequeue(Cwpx_NodeQueue **_this){

  if(*_this == NULL) return;
  
  Cwpx_NodeQueue *_thisPtr = *_this;
  
  Cwpx_Node *it = _thisPtr->head;
  while(it != NULL){
    Cwpx_Node *nodeToDelete = it;
    it = it->next;
    cwpx_destroy_node(&nodeToDelete);
  }
  
  free(*_this); 
  *_this = NULL;
}


void cwpx_destroy_filenodequeue(Cwpx_FileNodeQueue **_this, int deletefiles){

  if(*_this == NULL) return;
  
  Cwpx_FileNodeQueue *_thisPtr = *_this;
  
  Cwpx_FileNode *it = _thisPtr->head;
  while(it != NULL){
    Cwpx_FileNode *nodeToDelete = it;
    it = it->next;
    cwpx_destroy_filenode(&nodeToDelete, deletefiles);
  }
  
  free(*_this); 
  *_this = NULL;
}
