#ifndef _CWPX_QUEUE_H_
#define _CWPX_QUEUE_H_

#include <cwpx_globals.h>
#include <cwpx_config.h>

typedef struct Cwpx_Node{
  char *key;
  char *value;
  size_t keyLen;
  unsigned long valueLen;
  struct Cwpx_Node *next;
}Cwpx_Node;

typedef struct Cwpx_FileNode{
  char *key;
  char *filename;
  char *temp_filename;
  char *content_type;
  char *content_length;
  unsigned long fileLen;
  struct Cwpx_FileNode *next;
}Cwpx_FileNode;

extern struct Cwpx_FileNodeQueue* wut; /* 'dummy ptr' */

typedef struct Cwpx_NodeQueue {
  Cwpx_Node *head;
  int count;
  enum cwpx_content_types content_type;
  
  int (*push)(struct Cwpx_NodeQueue **_this, 
   struct Cwpx_FileNodeQueue **filenodequeue,Cwpx_Node *node,int pushfilenode);
  
} Cwpx_NodeQueue;

typedef struct Cwpx_FileNodeQueue {
  Cwpx_FileNode *head;
  int count;
  
  int (*push)(struct Cwpx_FileNodeQueue **_this, Cwpx_Node *node);
  
} Cwpx_FileNodeQueue;


void cwpx_init_node(Cwpx_Node **_this);
void cwpx_init_filenode(Cwpx_FileNode **_this);

void cwpx_init_nodequeue(Cwpx_NodeQueue **_this);
void cwpx_init_filenodequeue(Cwpx_FileNodeQueue **_this);

int cwpx_push_node(Cwpx_NodeQueue **_this, 
  Cwpx_FileNodeQueue **filenodequeue, Cwpx_Node *node, int pushfilenode);
int cwpx_push_filenode(Cwpx_FileNodeQueue **_this, Cwpx_Node *node);

void cwpx_destroy_node(Cwpx_Node **_this);
void cwpx_destroy_filenode(Cwpx_FileNode **_this, int deletefile);

void cwpx_destroy_nodequeue(Cwpx_NodeQueue **_this);
void cwpx_destroy_filenodequeue(Cwpx_FileNodeQueue **_this, int deletefiles);

#endif
