#ifndef _CWPX_CONTEXT_H_
#define _CWPX_CONTEXT_H_

#ifdef B_CWPX_CONTEXT
  #ifdef _WIN32
    #define CWPX_CONTEXT_DLL __declspec(dllexport)
  #else
    #define CWPX_CONTEXT_DLL 
  #endif
#else
  #define CWPX_CONTEXT_DLL 
#endif

#if defined(_CWPX_INTERFACE_H_)
/* define printf response.write */
#endif

#include <cwpx_reqhandler.h>
#include <cwpx_resphandler.h>
#include <cwpx_memfile.h>

#define set_context(n, v, sov) cwpx_args.set(n, v, sov, #sov)
#define get_context(n, sov) cwpx_args.get(n, sov, #sov)
#define set_session(n, v, sov) session.set(n, v, sov, #sov)
#define get_session(n, sov) session.get(n, sov, #sov)
#define session_start() session.start()

typedef struct Request{
  const char* (*header)(const char *key);
  const char* (*env)(const char *key);
  const char* (*cookies)(const char *key);
  const char* (*get)(const char *key);
  const char* (*get_at)(const char *key, unsigned int index);
  int (*isset_get)(const char *key);
  int (*isset_get_at)(const char *key, unsigned int index);
  const char* (*post)(const char *key);
  const char* (*post_at)(const char *key, unsigned int index);
  int (*isset_post)(const char *key);
  int (*isset_post_at)(const char *key, unsigned int index);
  const char* (*files)(const char *key, const char *attr);
  const char* (*raw)(const char *attr);
  int (*forward)(const char *scriptpath);
} Request;


typedef struct Cwpx_Args{
  int (*set)(const char *key, void *value,unsigned long size_of,char *vartype);
  void *(*get)(const char *key, unsigned long size_of, char *vartype);
} Cwpx_Args;


typedef struct Response{
  int (*write)(const char *, ...);
  int (*print)(const char *, ...); /* write&print point to the same function*/
  int (*header)(const char *key, const char *value);
  int (*cookies)(const char *key, const char *value, long expires, long maxage, 
	const char *path, const char *domain, int secure, int httponly, 
	const char *attrformat, ...);
  long (*writeb)(char *bytes, unsigned long byteslen);
  long (*printb)(char *bytes, unsigned long byteslen);
  int (*include)(const char *scriptpath);
} Response;


typedef struct Config{
  const char* (*workdir)();
  const char* (*rootdir)();
  const char* (*hostname)();
  const char* (*servip)();
  unsigned int (*servpt)();
  const char* (*currentdir)();
  
  const char* (*tempdir)();
  int (*set_tempdir)(const char *dir);
  const char* (*sessdir)();
  int (*set_sessdir)(const char *dir);
  const char* (*sessid)();
  int (*set_sessid)(const char *id);
  unsigned long (*sessdur)();
  int (*set_sessdur)(unsigned long sessdur);
  const char* (*logfile)();
  int (*set_logfile)(const char *logfile);
  
  unsigned long (*buf_size)();
  void (*set_buf_size)(unsigned long buf_size);
  unsigned int (*maxqueueelems)();
  void (*set_maxqueueelems)(unsigned int maxqueueelems);
  unsigned short (*allow_post)();
  void (*set_allow_post)(unsigned short allowpost);
  unsigned long (*max_post)(); 
  void (*set_max_post)(unsigned long maxpost); 
  
}Config;

struct Cwpx_ConfigHandler *dmych;


typedef struct Session{
  int (*start)();
  int (*set)(const char *key, void *value,unsigned long size_of,char *vartype);
  void *(*get)(const char *key, unsigned long size_of, char *vartype);
  
  const char* (*id)();
} Session;


typedef struct Cwpx_Context{
  Cwpx_RequestHandler *reqhandler;
  Cwpx_ResponseHandler *resphandler;
  Cwpx_MemFile *argshandler;
  struct Cwpx_ConfigHandler *confighandler;
  Cwpx_MemFile *sesshandler;
  
  Request request;
  Response response;
  Cwpx_Args cwpx_args;
  Config config;
  Session session;
  
  char *guuid;
  unsigned int pid;
  unsigned short forwcluded;
  
  FILE *log;
}Cwpx_Context;

extern Cwpx_Context *cwpx_context; /* @ cwpx_context.c */ 

void CWPX_CONTEXT_DLL cwpx_init_context(Cwpx_Context **_this, 
  char *guuid, unsigned int originpid);
int CWPX_CONTEXT_DLL cwpx_init_inherited(Cwpx_Context **_this, 
  int argc, char *argv[]);
void CWPX_CONTEXT_DLL cwpx_destroy_context(Cwpx_Context **_this);
#ifdef B_CWPX_CONTEXT
int CWPX_CONTEXT_DLL cwpx_main(int argc, char *argv[], void *func_ptr);
Cwpx_Args CWPX_CONTEXT_DLL cwpx_args;
Config CWPX_CONTEXT_DLL config;
Session CWPX_CONTEXT_DLL session;
int CWPX_CONTEXT_DLL cwpx_log(const char *format, ...);


__attribute__((constructor)) void thecontruct(void) {
  #ifdef _WIN32
  HINSTANCE hinstDLLSO = LoadLibrary("cwpxconf.dll");
  #else
  void *hinstDLLSO = dlopen ("cwpxconf.so", RTLD_LAZY);
  #endif

  if(hinstDLLSO != NULL){
    cwpx_seterror(0);
    #ifdef _WIN32
    char **cwpx_workdir = (char **) GetProcAddress(hinstDLLSO, "cwpx_workdir");
	#else
    char **cwpx_workdir = dlsym(hinstDLLSO, "cwpx_workdir");
    #endif
    if(cwpx_workdir != NULL && cwpx_geterror() == 0){
	  char *tryMalloc = malloc(strlen(*cwpx_workdir) + 1);
	  if(tryMalloc == NULL) {/*free everything*/ exit(1);}
	  CWPX_WORKDIR = tryMalloc;
	  strcpy(CWPX_WORKDIR, *cwpx_workdir);
	}
	cwpx_seterror(0);
	#ifdef _WIN32
    char **cwpx_rootdir = (char **) GetProcAddress(hinstDLLSO, "cwpx_rootdir");
	#else
    char **cwpx_rootdir = dlsym(hinstDLLSO, "cwpx_rootdir");
    #endif
    if(cwpx_rootdir != NULL && cwpx_geterror() == 0){
	  char *tryMalloc = malloc(strlen(*cwpx_rootdir) + 1);
	  if(tryMalloc == NULL) {/*free everything*/ exit(1);}
	  CWPX_ROOTDIR = tryMalloc;
	  strcpy(CWPX_ROOTDIR, *cwpx_rootdir);
	}
	cwpx_seterror(0);
	#ifdef _WIN32
    char **cwpx_hostname = (char **)GetProcAddress(hinstDLLSO,"cwpx_hostname");
	#else
    char **cwpx_hostname = dlsym(hinstDLLSO, "cwpx_hostname");
    #endif
    if(cwpx_hostname != NULL && cwpx_geterror() == 0){
	  char *tryMalloc = malloc(strlen(*cwpx_hostname) + 1);
	  if(tryMalloc == NULL) {/*free everything*/ exit(1);}
	  CWPX_HOSTNAME = tryMalloc;
	  strcpy(CWPX_HOSTNAME, *cwpx_hostname);
	}
	cwpx_seterror(0);
	#ifdef _WIN32
    char **cwpx_servip = (char **) GetProcAddress(hinstDLLSO, "cwpx_servip");
	#else
    char **cwpx_servip = dlsym(hinstDLLSO, "cwpx_servip");
    #endif
    if(cwpx_servip != NULL && cwpx_geterror() == 0){
	  char *tryMalloc = malloc(strlen(*cwpx_servip) + 1);
	  if(tryMalloc == NULL) {/*free everything*/ exit(1);}
	  CWPX_SERVIP = tryMalloc;
	  strcpy(CWPX_SERVIP, *cwpx_servip);
	}
	cwpx_seterror(0);
	#ifdef _WIN32
    unsigned int *cwpx_servpt = 
	  (unsigned int *) GetProcAddress(hinstDLLSO, "cwpx_servpt");
	#else
    unsigned int *cwpx_servpt = dlsym(hinstDLLSO, "cwpx_servpt");
    #endif
    if(cwpx_servpt != NULL && cwpx_geterror() == 0){
	  CWPX_SERVPT = *cwpx_servpt;
	}
	cwpx_seterror(0);
	#ifdef _WIN32
    char **cwpx_tempdir = (char **) GetProcAddress(hinstDLLSO, "cwpx_tempdir");
	#else
    char **cwpx_tempdir = dlsym(hinstDLLSO, "cwpx_tempdir");
    #endif
    if(cwpx_tempdir != NULL && cwpx_geterror() == 0){
	  char *tryMalloc = malloc(strlen(*cwpx_tempdir) + 1);
	  if(tryMalloc == NULL) {/*free everything*/ exit(1);}
	  CWPX_TEMPDIR = tryMalloc;
	  strcpy(CWPX_TEMPDIR, *cwpx_tempdir);
	}
	cwpx_seterror(0);
	#ifdef _WIN32
    char **cwpx_sessdir = (char **) GetProcAddress(hinstDLLSO, "cwpx_sessdir");
	#else
    char **cwpx_sessdir = dlsym(hinstDLLSO, "cwpx_sessdir");
    #endif
    if(cwpx_sessdir != NULL && cwpx_geterror() == 0){
	  char *tryMalloc = malloc(strlen(*cwpx_sessdir) + 1);
	  if(tryMalloc == NULL) {/*free everything*/ exit(1);}
	  CWPX_SESSDIR = tryMalloc;
	  strcpy(CWPX_SESSDIR, *cwpx_sessdir);
	}
	cwpx_seterror(0);
	#ifdef _WIN32
    char **cwpx_sessid = (char **) GetProcAddress(hinstDLLSO, "cwpx_sessid");
	#else
    char **cwpx_sessid = dlsym(hinstDLLSO, "cwpx_sessid");
    #endif
    if(cwpx_sessid != NULL && cwpx_geterror() == 0){
	  char *tryMalloc = malloc(strlen(*cwpx_sessid) + 1);
	  if(tryMalloc == NULL) {/*free everything*/ exit(1);}
	  CWPX_SESSID = tryMalloc;
	  strcpy(CWPX_SESSID, *cwpx_sessid);
	}
	cwpx_seterror(0);
	#ifdef _WIN32
    unsigned long *cwpx_sessdur = 
	  (unsigned long *) GetProcAddress(hinstDLLSO, "cwpx_sessdur");
	#else
    unsigned long *cwpx_sessdur = dlsym(hinstDLLSO, "cwpx_sessdur");
    #endif
    if(cwpx_sessdur != NULL && cwpx_geterror() == 0){
	  CWPX_SESSDUR = *cwpx_sessdur;
	}
	cwpx_seterror(0);
	#ifdef _WIN32
    char **cwpx_logfile = (char **) GetProcAddress(hinstDLLSO, "cwpx_logfile");
	#else
    char **cwpx_logfile = dlsym(hinstDLLSO, "cwpx_logfile");
    #endif
    if(cwpx_logfile != NULL && cwpx_geterror() == 0){
	  char *tryMalloc = malloc(strlen(*cwpx_logfile) + 1);
	  if(tryMalloc == NULL) {/*free everything*/ exit(1);}
	  CWPX_LOGFILE = tryMalloc;
	  strcpy(CWPX_LOGFILE, *cwpx_logfile);
	}
	cwpx_seterror(0);
	#ifdef _WIN32
    unsigned long *cwpx_buf_size = 
	  (unsigned long *) GetProcAddress(hinstDLLSO, "cwpx_buf_size");
	#else
    unsigned long *cwpx_buf_size = dlsym(hinstDLLSO, "cwpx_buf_size");
    #endif
    if(cwpx_buf_size != NULL && cwpx_geterror() == 0){
	  CWPX_BUF_SIZE = *cwpx_buf_size;
	}
	cwpx_seterror(0);
	#ifdef _WIN32
    unsigned int *cwpx_max_queueelems = 
	  (unsigned int *) GetProcAddress(hinstDLLSO, "cwpx_max_queueelems");
	#else
    unsigned int *cwpx_max_queueelems= dlsym(hinstDLLSO,"cwpx_max_queueelems");
    #endif
    if(cwpx_max_queueelems != NULL && cwpx_geterror() == 0){
	  CWPX_MAX_QUEUEELEMS = *cwpx_max_queueelems;
	}
	cwpx_seterror(0);
	#ifdef _WIN32
    unsigned short *cwpx_allow_post = 
	  (unsigned short *) GetProcAddress(hinstDLLSO, "cwpx_allow_post");
	#else
    unsigned short *cwpx_allow_post = dlsym(hinstDLLSO, "cwpx_allow_post");
    #endif
    if(cwpx_allow_post != NULL && cwpx_geterror() == 0){
	  CWPX_ALLOW_POST = *cwpx_allow_post;
	}
	cwpx_seterror(0);
	#ifdef _WIN32
    unsigned long *cwpx_max_post = 
	  (unsigned long *) GetProcAddress(hinstDLLSO, "cwpx_max_post");
	#else
    unsigned long *cwpx_max_post = dlsym(hinstDLLSO, "cwpx_max_post");
    #endif
    if(cwpx_max_post != NULL && cwpx_geterror() == 0){
	  CWPX_MAX_POST = *cwpx_max_post;
	}

    #ifdef WIN32
    FreeLibrary(hinstDLLSO);
    #else
    dlclose(hinstDLLSO);
    #endif
    hinstDLLSO = NULL;
  }
  
}
#endif


#endif
