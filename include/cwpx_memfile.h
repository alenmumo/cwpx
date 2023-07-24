#ifndef _CWPX_MEMFILE_H_
#define _CWPX_MEMFILE_H_

#include <stdio.h>
#ifdef _WIN32
#include <windows.h>
#include <tchar.h>
#endif

#include <cwpx_globals.h>
#include <cwpx_config.h>

typedef struct Cwpx_ObjectNode{
  char *name;
  char *type;
  int complex;
  void *value;
  unsigned long size_of;
  struct Cwpx_ObjectNode *next;
}Cwpx_ObjectNode;

typedef struct Cwpx_ObjectQueue {
  Cwpx_ObjectNode *head;
  int (*push)(struct Cwpx_ObjectQueue **_this, Cwpx_ObjectNode *node);
} Cwpx_ObjectQueue;

typedef struct Cwpx_MemMappedFile {
  #ifdef _WIN32
  HANDLE file;
  LPCTSTR fileview;
  #else
  FILE *file;
  char *fileview;
  #endif
  
  char *filename;
  int fd;
  int memorymapped;
  
} Cwpx_MemMappedFile;

typedef struct Cwpx_MemFile{
  Cwpx_ObjectQueue *queue;
  Cwpx_MemMappedFile *file;
  int sessionstarted;
}Cwpx_MemFile;


void cwpx_init_objectnode(Cwpx_ObjectNode **_this);
void cwpx_init_objectqueue(Cwpx_ObjectQueue **_this);
void cwpx_init_memmappedfile(Cwpx_MemMappedFile **_this, const char* filename);
void cwpx_init_memfile(Cwpx_MemFile **_this, const char *filename, 
  int memorymapped);
int cwpx_push_object(Cwpx_ObjectQueue **_this, Cwpx_ObjectNode *node);
void cwpx_destroy_objectnode(Cwpx_ObjectNode **_this);
void cwpx_destroy_objectqueue(Cwpx_ObjectQueue **_this);
void cwpx_destroy_memmappedfile(Cwpx_MemMappedFile **_this);
void cwpx_destroy_memfile(Cwpx_MemFile **_this);

int _cwpx_set_object(Cwpx_ObjectQueue **oqueue, const char *name, void *object, 
  unsigned long size_of, char *vartype);
void *_cwpx_get_object(Cwpx_ObjectQueue **oqueue, const char *name, 
  unsigned long size_of, char *vartype);
int cwpx_setobjectsqueuetofile(Cwpx_ObjectQueue **oqueue, 
  Cwpx_MemMappedFile **mfile);
int cwpx_getobjectsqueuefromfile(Cwpx_ObjectQueue **oqueue, 
  Cwpx_MemMappedFile **mfile, const char *objectname, void *objectbuffer);

#endif
