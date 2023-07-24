/*
 * gcc -c -DB_CWPX_CONTEXT src/cwpx_context.c -Iinclude -fPIC -o lib/cwpx_conte
 * xt.o
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <sys/stat.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#ifdef _WIN32
  #include <io.h>
  #include <fcntl.h>
#endif
#include <cwpx_context.h>
#include <cwpx_misc.h>

/* global local variable for context management */
Cwpx_Context *cwpx_context = NULL;
/* the cwpx 'brigde' function pointer */
void (*do_http_ptr)(Request request, Response response);

/* local functions */
/* functions for request interface (depend on cwpx_context variable) */
const char *cwpx_i_reqheader(const char *key);
const char *cwpx_i_env(const char *key);
const char *cwpx_i_reqcookies(const char *key);
const char *cwpx_i_get(const char *key);
const char *cwpx_i_getat(const char *key, unsigned int index);
int cwpx_i_issetget(const char *key);
int cwpx_i_issetgetat(const char *key, unsigned int index);
const char *cwpx_i_post(const char *key);
const char *cwpx_i_postat(const char *key, unsigned int index);
int cwpx_i_issetpost(const char *key);
int cwpx_i_issetpostat(const char *key, unsigned int index);
const char *cwpx_i_files(const char *key, const char *attr);
const char *cwpx_i_raw(const char *attr);
int cwpx_i_forward(const char *scriptpath);
/* functions for response interface (depend on cwpx_context variable) */
int cwpx_i_write(const char *, ...);
int cwpx_i_print(const char *, ...); /* write & print point to the same */
int cwpx_i_respheader(const char *key, const char *value);
int cwpx_i_respcookies(const char *key, const char *value, long expires, 
  long maxage, const char *path, const char *domain, int secure, int httponly, 
  const char *attrformat, ...);
long cwpx_i_writeb(char *bytes, unsigned long byteslen);
long cwpx_i_printb(char *bytes, unsigned long byteslen); /* point to the same*/
int cwpx_i_include(const char *scriptpath);
/* functions or cwpx_args, sessions and config */
int cwpx_i_cwpxargsset(const char *key, void *value, unsigned long size_of, 
  char *vartype);
void *cwpx_i_cwpxargsget(const char *key, unsigned long size_of,char *vartype);

const char *cwpx_i_getworkdir();
const char *cwpx_i_getrootdir();
const char *cwpx_i_gethostname();
const char *cwpx_i_getservip();
unsigned int cwpx_i_getservpt();
const char *cwpx_i_getcurrentdir();
const char *cwpx_i_gettempdir();
const char *cwpx_i_getsessdir();
const char *cwpx_i_getsessid();
int cwpx_i_setsessid(const char *id);
unsigned long cwpx_i_getsessdur();
const char *cwpx_i_getlogfile();
unsigned long cwpx_i_getbuf_size();
unsigned int cwpx_i_getmaxqueueelems();
unsigned short cwpx_i_getallow_post();
unsigned long cwpx_i_getmax_post();

int cwpx_i_sessionstart();
int cwpx_i_sessionset(const char *key, void *value, unsigned long size_of, 
  char *vartype);
void *cwpx_i_sessionget(const char *key, unsigned long size_of,char *vartype);
const char *cwpx_i_getsessidvalue();
int cwpx_forwclude(Cwpx_Context **cwpx_context, 
  const char *scriptpath, enum cwpx_forwclude_options forwclude);
/* functions for calculating the estimated queues sizes */
unsigned long cwpx_calcqueuestrlen(char *queueheader, Cwpx_NodeQueue *queue,
  Cwpx_ResponseHandler *resphandler);
unsigned long cwpx_calcfilequeuestrlen(Cwpx_FileNodeQueue *queue, 
  enum cwpx_content_types content_type);
unsigned long cwpx_calcqueueslen(Cwpx_RequestHandler *reqhandler,
  struct Cwpx_ResponseHandler *resphandler);
/* functions for getting a stream out the queues */
unsigned long cwpx_getlenandqueuetostr(Cwpx_NodeQueue *queue, 
  Cwpx_ResponseHandler *resphandler, char *queueheader, 
  char *queuestdin); /* queueStdin: out param */
unsigned long cwpx_getlenandfilequeuetostr(Cwpx_FileNodeQueue *queue, 
  char *queueheader, enum cwpx_content_types content_type, 
  char *queuestdin); /* queueStdin: out param */
unsigned long cwpx_setqueuestostdin(Cwpx_RequestHandler *reqhandler,
  Cwpx_ResponseHandler *resphandler, 
  char *queuestdin); /* queueStdin: out param */
/* functions for getting queues out a stream */
int cwpx_getqueuesfromstdin(char *queuestdin, unsigned long queuestdinLen,
  Cwpx_RequestHandler **reqhandler, 
  Cwpx_ResponseHandler **resphandler,
  char *queueid); /*reqhandler & resphandler: out params*/
/* general functions */
int cwpx_readandfillrequest(Cwpx_RequestHandler **reqhandler);
void cwpx_signal_handler(int signum);

/* other vars */
Response *responseNULL = NULL; /* dummy ptr for initializing static memory 
allocated for Response with a NULL value */

int inherited = 0; /* for testing only */

Cwpx_Args cwpx_args;
Config config;
Session session;

/* DEFAULT VALUES FOR CONFIGURATION */
/* auto-detectable if empty (""), NULL  or 0 &| custom config (cwpx_config.c)*/
char *CWPX_WORKDIR = NULL;
char *CWPX_ROOTDIR = NULL;
char *CWPX_HOSTNAME = NULL;
char *CWPX_SERVIP = NULL;
unsigned int CWPX_SERVPT = 80;
char *CWPX_TEMPDIR = NULL;
char *CWPX_SESSDIR = NULL;
char *CWPX_SESSID = NULL;
unsigned long CWPX_SESSDUR = 1500; /* 25 minutes (php24asp20jsp30!djo2w)*/
char *CWPX_LOGFILE = NULL; /* _uuid._pid_.yyyy-mm-dd.cwpxlog"; */
unsigned long CWPX_BUF_SIZE = (1024 * 1024);
unsigned int CWPX_MAX_QUEUEELEMS = 50;
unsigned short CWPX_ALLOW_POST = 0;
unsigned long CWPX_MAX_POST = ((1024 * 1024) * 5); /* 5Mb*/
/* if cwpx_context.h detects values in cwpxconf,it will malloc the char* vars*/


/* only for local file */
typedef struct Cwpx_ConfigHandler{
  /* must be analogous with cwpx_config */
  char *workdir;
  char *rootdir;
  
  char *hostname;
  char *servip;
  unsigned int servpt;
  char *tempdir;
  char *sessdir;
  char *sessid;
  unsigned long sessdur;
  char *currentdir;
  char *logfile;
  
  unsigned long buf_size;
  unsigned int max_queueelems;
  unsigned short allow_post;
  unsigned long max_post; 
  
}Cwpx_ConfigHandler;

void cwpx_init_confighandler(Cwpx_ConfigHandler **_this);
void cwpx_destroy_confighandler(Cwpx_ConfigHandler **_this);


int cwpx_main(int argc, char *argv[], void *func_ptr){

  do_http_ptr = func_ptr;
	
  /* as this is a CGI implementation, after the initialization of the context, 
  the first thing a script must do is to check if it was called from another
  script by either 'forward' or 'include' redirection. In this implementation
  we use special flags in the command line to determine when the current script
  call is for a forward or include request from another cwpx cgi script */
  if(argc > 1 && strcmp(argv[1], CWPX_FORWARD_ARG) == 0){
    inherited = 1;
    cwpx_init_inherited(&cwpx_context, argc, argv);
    
    Request *requestPtr = &cwpx_context->request;
    Response *responsePtr = &cwpx_context->response;
    
    cwpx_args = cwpx_context->cwpx_args;
    
    config = cwpx_context->config;
    
    session = cwpx_context->session;
    
    /* execution of the script */
    do_http_ptr(*requestPtr, *responsePtr);
    
    
    if(!cwpx_context->resphandler->is_header_sent){
      cwpx_send_respheader(&cwpx_context->resphandler);
    }
    
    /* after sending the actual response, a boundary indicates the beginning of 
	the queues data stream. the other side is expecting the pattern:
		CWPX_SEVENNINE_BOUNDARY + CWPX_CRLF + CWPX_RESPHEADERSQUEUE_ID
	the CWPX_RESPHEADERSQUEUE_ID is because it is the only one queue that isn't
	a read-only queue, the child can modify the response, but not the request.
	*/
    printf("%s%s", CWPX_SEVENNINE_BOUNDARY, CWPX_CRLF); /* DO NOT ERASE */
    
    /* calculate enough space to store the queues' raw data */
    /*unsigned long calcQueuesLen = 
    cwpx_calcqueueslen(cwpx_context->reqhandler, cwpx_context->resphandler);*/
    /* the above commented intruction calculates all the queues' length */
    unsigned long calcQueuesLen = 
      cwpx_calcqueuestrlen(CWPX_RESPHEADERSQUEUE_ID, 
	  cwpx_context->resphandler->headersqueue, cwpx_context->resphandler);
  
    /* store the queues' data in a buffer */
    char *queuesData = NULL;
    unsigned long queuesDataLen = 0;
    /* the calculated space for queues just gives us the compacted size of the
    queue. add CWPX_MIN_BUFLEN for extra space required for storing value 
	lengths and newlines */
    char *tryMalloc = malloc(calcQueuesLen + CWPX_MIN_BUFLEN); 
    if(!tryMalloc){ /* free everything ... */ exit(1); }
    queuesData = tryMalloc;
    queuesDataLen = 
      /*cwpx_setqueuestostdin(cwpx_context->reqhandler, 
	    cwpx_context->resphandler, queuesData);*/
      /* the above commented intruction sets all the queues to queuesData */
      cwpx_getlenandqueuetostr(cwpx_context->resphandler->headersqueue, 
	    cwpx_context->resphandler, CWPX_RESPHEADERSQUEUE_ID, queuesData);

    unsigned long queuesDataPos = 0;
    unsigned long remainingLen = queuesDataLen;
    unsigned long bytesSent = 0;
    
    while((bytesSent = 
	  fwrite(queuesData + queuesDataPos, 1, remainingLen, stdout)) > 0){

      queuesDataPos += bytesSent;
      remainingLen -= bytesSent;
    }
    
    /* send back cwpx_args (if any). ONLY FOR CHILDREN */
    Cwpx_MemFile *cwpxargshandlerPtr = cwpx_context->argshandler;
    cwpx_setobjectsqueuetofile(&cwpxargshandlerPtr->queue, 
	  &cwpxargshandlerPtr->file);
    
    /* if there are session vars, set them at disk before exiting */
    Cwpx_MemFile *sesshandlerPtr = cwpx_context->sesshandler;
    if(sesshandlerPtr->sessionstarted)
     cwpx_setobjectsqueuetofile(&sesshandlerPtr->queue, &sesshandlerPtr->file);
    
    
    cwpx_destroy_context(&cwpx_context);
    
    return 0;
  }


  cwpx_init_context(&cwpx_context, NULL, cwpx_getpid());
  
  Request *requestPtr = &cwpx_context->request;
  Response *responsePtr = &cwpx_context->response;
  
  cwpx_args = cwpx_context->cwpx_args;
  
  config = cwpx_context->config;
  
  session = cwpx_context->session;
  
  /* execution of the script */
  do_http_ptr(*requestPtr, *responsePtr);
  
  
  if(!cwpx_context->resphandler->is_header_sent){
    cwpx_send_respheader(&cwpx_context->resphandler);
  }
  
  /* if there are session vars, set them at disk before exiting */
  Cwpx_MemFile *sesshandlerPtr = cwpx_context->sesshandler;
  if(sesshandlerPtr->sessionstarted)
    cwpx_setobjectsqueuetofile(&sesshandlerPtr->queue, &sesshandlerPtr->file);
  
  
  cwpx_destroy_context(&cwpx_context);
  
  return 0;
}

void cwpx_init_context(Cwpx_Context **_this, char *guuid,
  unsigned int originpid){
  
  Cwpx_Context *tryMalloc = malloc(sizeof(Cwpx_Context));
  if(!tryMalloc){ /* free everything ... */ exit(1); }
  *_this = tryMalloc;
  Cwpx_Context *_thisPtr = *_this;
  
  _thisPtr->reqhandler = NULL;
  cwpx_init_requesthandler(&_thisPtr->reqhandler);
  _thisPtr->resphandler = NULL;
  cwpx_init_responsehandler(&_thisPtr->resphandler);
  
  /* set default content type header */
  _thisPtr->resphandler->header(&_thisPtr->resphandler, 
    "Content-Type", "text/html; charset=utf-8");
  
  /* init config right here, because we need some values before continuing */
  cwpx_init_confighandler(&_thisPtr->confighandler);
  
  
  /* ONLY FOR PARENT. once the requesthandler is ready, we can parse the actual
   request. in CGI, the request comes in two parts, the headers in environment
   variables and the body in the stdin */
  /* the parent is identified by checking if the guuid is null, not set yet */
  if(guuid == NULL){
    cwpx_readandfillrequest(&_thisPtr->reqhandler);
  }  
  
  /* when inherited, the parent sends its guuid and pid at args */
  _thisPtr->guuid = NULL;
  char newguuid[CWPX_MIN_BUFLEN]; strcpy(newguuid, CWPX_EMPTY);
  if(guuid == NULL) cwpx_generate_uuid(newguuid);
  else strcpy(newguuid, guuid);
  char *tryMallocStr = malloc(strlen(newguuid) + 1);
  if(!tryMallocStr){ /* free everything ... */ exit(1); }
  _thisPtr->guuid = tryMallocStr;
  strcpy(_thisPtr->guuid, newguuid);
  
  unsigned int pid = (guuid == NULL) ? 
    cwpx_getpid() : originpid;
  _thisPtr->pid = pid;


  char memmapfile[CWPX_MIN_BUFLEN];
  sprintf(memmapfile, "_%s._%d.cwpxctx", 
    _thisPtr->guuid, _thisPtr->pid);
  _thisPtr->argshandler = NULL;
  cwpx_init_memfile(&_thisPtr->argshandler, memmapfile, 1); /* 1: memoryfile */
  
  
  char sessionfile[CWPX_PATH_MAX + CWPX_NAME_MAX];
  int endsWithSlash = 
   _thisPtr->confighandler->sessdir[strlen(_thisPtr->confighandler->sessdir)-1]
   == '/';
  /* as the memmapfile needed a combination of guuid and pid for setting its 
  filename, a sessionfile also needs to obtain this name similarly. the first 
  place we have to look at for getting this filename is a cookie, or at least
  the URL that the clients sends to the script. in case it didn't exist, we 
  create a provisional name with the same combination of guuid+pid for a future
  use of session in the script. as opposite to the memmapfile, the sessionfile 
  needs not only the filename, but also the session's directory configured (if 
  any) at cwpx_conf.c or gotten from default OS temp folder. the session file
  will not be created until the first call to set_session */
  /*unsigned int pid = (guuid == NULL) ? 
    cwpx_getpid() : originpid;*/
    
  const char *sessid = _thisPtr->confighandler->sessid;
  
  const char *sessionCookie = 
    _thisPtr->reqhandler->cookie(&_thisPtr->reqhandler, sessid);
  char *sessionUrl = NULL;
  if(sessionCookie != NULL)
   sprintf(sessionfile, "%s%s%s.cwpxses", 
    _thisPtr->confighandler->sessdir, endsWithSlash ? "" : "/", sessionCookie);
    
  if(sessionCookie == NULL){
  sessionUrl = (char*)_thisPtr->reqhandler->get(&_thisPtr->reqhandler, sessid);
    if(sessionUrl != NULL)
      sprintf(sessionfile, "%s%s%s.cwpxses", 
       _thisPtr->confighandler->sessdir, endsWithSlash ? "" : "/", sessionUrl);
  }
  
  short issessionstarted = 0;
  if(sessionCookie == NULL && sessionUrl == NULL)
    sprintf(sessionfile, "%s%s_%s._%d.cwpxses", 
      _thisPtr->confighandler->sessdir, endsWithSlash ? "" : "/", 
      _thisPtr->guuid, _thisPtr->pid);
  else
    issessionstarted = 1;

  _thisPtr->sesshandler = NULL;
  cwpx_init_memfile(&_thisPtr->sesshandler, sessionfile, 0); /* 0: session */
  /* after initing the session object, its attribute 'sessionstarted' will tell
  later in the process if the file should be created or not (no session used)*/
  _thisPtr->sesshandler->sessionstarted = issessionstarted;
  
  if(_thisPtr->sesshandler->sessionstarted){
    /* if the session is started, initialize the objects (if any) */
    cwpx_getobjectsqueuefromfile(
	  &_thisPtr->sesshandler->queue, &_thisPtr->sesshandler->file, NULL, NULL);
  }  
  
  /* set the flag that tells if the current script was forwarded or included,
  useful for 'cwpx_destroy_context' which needs to do the distinction between
  origin and inherited scripts, since there are tasks that the origin script 
  does and are not allowed for inherited child scripts, such as the deletion
  of the 'physical' file used for mmap in Linux ... */
  _thisPtr->forwcluded = 0;
  if(guuid != NULL){ /* if there is a guuid as parameter, we are a forwcluded*/
    _thisPtr->forwcluded = 1;
  }
    
  /* interface functions */
  _thisPtr->request.header = cwpx_i_reqheader;
  _thisPtr->request.env = cwpx_i_env;
  _thisPtr->request.cookies = cwpx_i_reqcookies;
  _thisPtr->request.get = cwpx_i_get;
  _thisPtr->request.get_at = cwpx_i_getat;
  _thisPtr->request.isset_get = cwpx_i_issetget;
  _thisPtr->request.isset_get_at = cwpx_i_issetgetat;
  _thisPtr->request.post = cwpx_i_post;
  _thisPtr->request.post_at = cwpx_i_postat;
  _thisPtr->request.isset_post = cwpx_i_issetpost;
  _thisPtr->request.isset_post_at = cwpx_i_issetpostat;
  _thisPtr->request.files = cwpx_i_files;
  _thisPtr->request.raw = cwpx_i_raw;
  _thisPtr->request.forward = cwpx_i_forward;
  
  _thisPtr->response.write = cwpx_i_write;
  _thisPtr->response.print = _thisPtr->response.write;
  _thisPtr->response.header = cwpx_i_respheader;
  _thisPtr->response.cookies = cwpx_i_respcookies;
  _thisPtr->response.writeb = cwpx_i_writeb;
  _thisPtr->response.printb = _thisPtr->response.writeb;
  _thisPtr->response.include = cwpx_i_include;
  
  _thisPtr->cwpx_args.set = cwpx_i_cwpxargsset;
  _thisPtr->cwpx_args.get = cwpx_i_cwpxargsget;
  
  /* the initialization of _thisPtr->config.(functions) 
  is at cwpx_init_confighandler. Ex.: 
      _thisPtr->config.workdir = cwpx_i_getworkdir;
  */
  _thisPtr->config.workdir = cwpx_i_getworkdir;
  _thisPtr->config.rootdir = cwpx_i_getrootdir;
  _thisPtr->config.hostname = cwpx_i_gethostname;
  _thisPtr->config.servip = cwpx_i_getservip;
  _thisPtr->config.servpt = cwpx_i_getservpt;
  _thisPtr->config.currentdir = cwpx_i_getcurrentdir;
  _thisPtr->config.tempdir = cwpx_i_gettempdir;
  _thisPtr->config.sessdir = cwpx_i_getsessdir;
  _thisPtr->config.sessid = cwpx_i_getsessid;
  _thisPtr->config.set_sessid = cwpx_i_setsessid;
  _thisPtr->config.sessdur = cwpx_i_getsessdur;
  _thisPtr->config.logfile = cwpx_i_getlogfile;
  _thisPtr->config.buf_size = cwpx_i_getbuf_size;
  _thisPtr->config.maxqueueelems = cwpx_i_getmaxqueueelems;
  _thisPtr->config.allow_post = cwpx_i_getallow_post;
  _thisPtr->config.max_post = cwpx_i_getmax_post;  
  
  _thisPtr->session.start = cwpx_i_sessionstart;
  _thisPtr->session.set = cwpx_i_sessionset;
  _thisPtr->session.get = cwpx_i_sessionget;
  _thisPtr->session.id = cwpx_i_getsessidvalue;
  
  _thisPtr->log = NULL;
  
  cwpx_context = _thisPtr;
  
}

/*
 * 'cwpx_init_inherited' is a special function for the CGI implementation of 
 * CWPX, because, unlike the classic CWPX in which scripts are .dll or .so, 
 * making it flawlessly easy to pass the data as function arguments, 
 * in CGI we have to launch a new process and we cannot send data as arguments.
 * as we cannot send arguments as we'd do for functions, we send the data by 
 * other means: ENV_VARS, command line arguments and STDIN.
 * in STDIN we're going to send the whole context of the parent, that is, the
 * queues containing the data set by the parent. 
 * the data comes serialized, therefore, we'll need some sort of protocol to 
 * parse the information and recreate the queues in the child process.
 * once the child process has finished, it will have to serialized again the 
 * data of its own context and send it back to its parent by using the STDOUT.
 * the child only needs to send back the response's queues and not the 
 * request's ones, because the request is read-only
 */
int cwpx_init_inherited(Cwpx_Context **_this, int argc, char *argv[]){
		
  signal(SIGABRT, cwpx_signal_handler);
  signal(SIGFPE, cwpx_signal_handler);
  signal(SIGILL, cwpx_signal_handler);
  signal(SIGINT, cwpx_signal_handler);
  signal(SIGSEGV, cwpx_signal_handler);
  signal(SIGTERM, cwpx_signal_handler);

  if(argc < 2 || argv == NULL) return -1;
  
  char *guuid = argv[2];
  unsigned int originpid = (unsigned)atoi(argv[3]);

  cwpx_init_context(&*_this, guuid, originpid);
  
  Cwpx_Context *_thisPtr = *_this;
  Cwpx_RequestHandler *reqHandlerPtr = _thisPtr->reqhandler;
  Cwpx_ResponseHandler *respHandlerPtr = _thisPtr->resphandler;
  
  /* firstly, read the whole stdin for getting all the context that the calling 
  script sends us, the stdin comes hexbyte-encoded */
  char buffer[BUFSIZ];
  size_t bytesRead = 0;
  char *stdinPtr = NULL;
  unsigned long stdinLen = 0;
  
  while ((bytesRead = (unsigned)read(0, buffer, BUFSIZ)) > 0) {
    char *tryMalloc = stdinPtr == NULL ?
      malloc(bytesRead + 1) : realloc(stdinPtr, stdinLen + bytesRead + 1);
    if(!tryMalloc){ /* free everything ... */ exit(1); }
    stdinPtr = tryMalloc;
    memcpy(stdinPtr + stdinLen, buffer, bytesRead);
    stdinLen += bytesRead;
  }
  
  /* remove the final character that the popen command is adding (is it the 
  'ENTER' key emulation?) */
  stdinPtr[stdinLen - 1] = 0;
  stdinLen--;
  
  /* by now, stdinLen is counting in +2 chars to the actual stdinLen, 
  because it comes with leading and trailing quotes like "stdin";
  the quotes were needed to pass the stream to the pipe */
  
  /* decode the stdin bytestream back to 'raw' data */
  char *processStdin = NULL;
  unsigned long processStdinLen = 
    (unsigned long)ceil((double)strlen(stdinPtr) / 4) + 2;
  char *tryMalloc = malloc(processStdinLen + 1);
  if(!tryMalloc){ /* free everything ... */ exit(1); }
  processStdin = tryMalloc;
  
  /* IMPORTANT BUG THAT TOOK A LOT OF TIME TO SOLVE:
  in some version, the following instruction made the asumption that 'stdinPtr'
  always arrived with a leading '"' double-quote, like this:
    cwpx_hexbytestostr(stdinPtr + 1, processStdin); // +1 skip the leading "
  however, in further tests, we detected that this happened in Windows, but not
  in Linux, even though we thought everything was alright for both and the fact
  that we HAD MADE TESTS ON THIS BEFORE AND THEY WERE SUCCESSFUL, so we don't 
  know what really happend between some version and the other, there is still
  the possibility that those "previuos successful tests" weren't taking into 
  account all use cases */
  if(stdinPtr[0] == '"') stdinPtr += 1;/* +1 skip the leading " if any */
  cwpx_hexbytestostr(stdinPtr, processStdin); 
  
  /* processStdinLen is the space reserved, which is +2 chars than the 
  expected length, therefore consider this when using processStdinLen */
  
  /* Parse the data to recreate the queues */
  cwpx_getqueuesfromstdin(processStdin, processStdinLen, 
    &reqHandlerPtr, &respHandlerPtr, NULL);
  
  /* as this is is an iherited request, do not delete files from here */
  reqHandlerPtr->deletefiles = 0;
  
  
  /* receive cwpx_args (if any)*/
  Cwpx_MemFile *cwpxargshandlerPtr = _thisPtr->argshandler;
  cwpx_getobjectsqueuefromfile(
    &cwpxargshandlerPtr->queue, &cwpxargshandlerPtr->file, NULL, NULL);
  
  return 0;
}


int cwpx_forwclude(Cwpx_Context **cwpx_context, 
  const char *scriptpath, enum cwpx_forwclude_options forwclude){

  if(*cwpx_context == NULL || scriptpath == NULL) return -1;
  Cwpx_Context *ctxHandlerPtr = *cwpx_context;
  
  Cwpx_RequestHandler *reqHandlerPtr = ctxHandlerPtr->reqhandler;
  struct Cwpx_ResponseHandler *respHandlerPtr = ctxHandlerPtr->resphandler;
  
  /* check if the scriptpath exists */
  struct stat resourceInfo;
	
  if(stat(scriptpath, &resourceInfo) != 0){
    /*printf("404 Not Found\n");*/
    return -404;
  }
  else if(resourceInfo.st_mode & S_IFDIR) { /* Directory ... */
    /*printf("403 Forbidden\n");*/
    return -403;
  }
  /* else, it's "200 Ok" */
  
  /* send cwpx_args (if any)*/
  Cwpx_MemFile *cwpxargshandlerPtr = ctxHandlerPtr->argshandler;
  cwpx_setobjectsqueuetofile(
    &cwpxargshandlerPtr->queue, &cwpxargshandlerPtr->file);
  
  /* confirm session (if any)*/
  Cwpx_MemFile *sesshandlerPtr = ctxHandlerPtr->sesshandler;
  if(sesshandlerPtr->sessionstarted){
    cwpx_setobjectsqueuetofile(&sesshandlerPtr->queue, &sesshandlerPtr->file);
  }
  
  
  /* calculate enough space to store the queues' raw data */
  unsigned long calcQueuesLen = 
    cwpx_calcqueueslen(reqHandlerPtr, respHandlerPtr);
  
  /* store the queues' data in a buffer */
  char *queuesData = NULL;
  unsigned long queuesDataLen = 0;
  /* the calculated space for queues just gives us the compacted size of the
  queue. add CWPX_MIN_BUFLEN for extra space required for storing value lengths
  and newlines */
  char *tryMalloc = malloc(calcQueuesLen + CWPX_MIN_BUFLEN); 
  if(!tryMalloc){ /* free everything ... */ exit(1); }
  queuesData = tryMalloc;
  queuesDataLen = 
    cwpx_setqueuestostdin(reqHandlerPtr, respHandlerPtr, queuesData);
    
  /* convert the data to a bytestream for sendinig it to the stdin. 
  each char will be converted into 4 chars, ex.: 'A' = "\x41" */
  char *queuesDataBytes = NULL;
  unsigned long queuesDataBytesLen = queuesDataLen * 4;
  tryMalloc = malloc(queuesDataBytesLen + 1);
  if(!tryMalloc){ /* free everything ... */ exit(1); }
  queuesDataBytes = tryMalloc;
  cwpx_strtohexbytes(queuesData, queuesDataBytes);

  FILE *scriptProcess; /* the pipe for opening the other script */
  char *processStdin = queuesDataBytes;
  unsigned long processStdinLen = queuesDataBytesLen;
	
  char cmdArgsInput[(CWPX_MIN_BUFLEN * 2) - 50]; /* 50 space for the command*/
  sprintf(cmdArgsInput, "%s %s %d", 
    CWPX_FORWARD_ARG, ctxHandlerPtr->guuid, cwpx_getpid());
  
  const char *commandFormat = "echo \"%s\"|%s %s";
  unsigned long commandLength = strlen(commandFormat) + processStdinLen + 
    strlen(scriptpath) + strlen(cmdArgsInput) + 
	(unsigned long)50; /* better abundance than lackness */
	
  tryMalloc = malloc(commandLength + 1);
  if(!tryMalloc){ /* free everything ... */ exit(1); }
  char *command = tryMalloc;
  memset(command + commandLength - 50, 0, 50);
  sprintf(command, "echo \"%s\"|%s %s", 
    processStdin, scriptpath, cmdArgsInput);
  
  /*scriptProcess = popen(command, "r");*/
  /* the same issue with binary mode in Windows ... */
  #ifdef _WIN32
  scriptProcess = popen(command, "rb");
  #else
  scriptProcess = popen(command, "r");
  #endif
  if(scriptProcess == NULL){
    printf("Unable to open scriptProcess\n");
    return -1;
  }
    
  char buffer[BUFSIZ];
  char *processStdout = NULL;
  unsigned long stdoutLen = 0;
  unsigned long bytesRead = 0;
    
  while((bytesRead = fread(buffer, 1, BUFSIZ, scriptProcess)) > 0){
    char *tryMalloc = processStdout == NULL ?
     malloc(bytesRead + 1) : realloc(processStdout, stdoutLen + bytesRead + 1);
    if(!tryMalloc){ /* free everything ... */ exit(1); }
    processStdout = tryMalloc;
    memcpy(processStdout + stdoutLen, buffer, bytesRead);
    stdoutLen += bytesRead;
  }
  
  int scriptResult = pclose(scriptProcess);
  scriptResult = scriptResult; /* avoid unsed warning */
    
  /* now that we have the output from the forwarded script, let's extract the 
  queues part and then carry on creating the response.
  the queues part comes at the end, beginning from seventynine hyphens (-), 
  the CRLF ending and then the cwpx_respheaders info 
  (when we expect all the queues, cwpx_getqueue leads the queues' info */
  char boundaryPattern[CWPX_MIN_BUFLEN];
  sprintf(boundaryPattern, "%s%s%s", 
    CWPX_SEVENNINE_BOUNDARY, CWPX_CRLF, CWPX_RESPHEADERSQUEUE_ID);
    
  char *processStdoutPtr = processStdout;
  char *processStdoutPtrEnd = processStdoutPtr + stdoutLen;
  char *boundaryPtr = 
    cwpx_memstr(&processStdoutPtr, boundaryPattern, &processStdoutPtrEnd);

  if(boundaryPtr == NULL){ /* free everything ...*/ exit(1); }
  
  unsigned long actualResponseLen = 
    (unsigned long)(boundaryPtr - processStdoutPtr);
  
  /* position the pointer to the beginning of the queues stream */
  char * queuesStr = 
    boundaryPtr + strlen(boundaryPattern) - strlen(CWPX_RESPHEADERSQUEUE_ID);
  
  unsigned long queuesStrLen = 
    (unsigned long)(processStdoutPtrEnd - queuesStr);
  
  
  /* parse the queues stream to recreate the queues */
  cwpx_destroy_nodequeue(&respHandlerPtr->headersqueue);
  
  cwpx_init_nodequeue(&respHandlerPtr->headersqueue);
  
  /*cwpx_getqueuesfromstdin(queuesStr, queuesStrLen, 
    &reqHandlerPtr, &respHandlerPtr);*/
  /*the above code get all the queues */
  cwpx_getqueuesfromstdin(queuesStr, queuesStrLen, 
    &reqHandlerPtr, &respHandlerPtr, CWPX_RESPHEADERSQUEUE_ID);

  /* save the generated content_length somewhere because the content hasn't 
  been sent yet. that content will be sent by the next 'cwpx_write_bytes'
  call, and that function will sum bytes as if they weren't summed before, 
  creating a wrong final value for content_length */
  unsigned long originalontentLengthSent = respHandlerPtr->content_length_sent;
  
  /* as this is the original request, files deletion must be set true here */
  reqHandlerPtr->deletefiles = 1;  
  
  /* send the response generated from the child */
  cwpx_write_bytes(&respHandlerPtr, processStdoutPtr, actualResponseLen);  
  
  /* after sending the child response, put back the content_length to its 
  otiginal value */
  respHandlerPtr->content_length_sent = originalontentLengthSent;
  
  
  /* receive cwpx_args (if any) and recreate the objects queue */
  //Cwpx_MemFile *cwpxargshandlerPtr = ctxHandlerPtr->cwpxargshandler;
  cwpx_destroy_objectqueue(&cwpxargshandlerPtr->queue);
  cwpx_init_objectqueue(&cwpxargshandlerPtr->queue);
  cwpx_getobjectsqueuefromfile(
    &cwpxargshandlerPtr->queue, &cwpxargshandlerPtr->file, NULL, NULL);
    
  
  if(forwclude == CWPX_FORWARD_SCRIPT){
    /* forward means that the execution control is transfered to another script
	and the current script (the caller) does't have to do with the request 
	anymore. as the current script is the parent process, we waited until the 
	child script was done. now, we just destroy and free everything and exit 
	the program successfully from this point */
    cwpx_destroy_context(cwpx_context); /* cwpx_context parameter takes prior*/
  	exit(0);
  }
  else if(forwclude == CWPX_INCLUDE_SCRIPT){
    /* include means that the execution control is transfered to another script
	and the current script waits for it (and its response) to continue further
	processing of the request. when the child script finishes, we continue the 
	execution of the current script */
  }
  
  return 0;
}


unsigned long cwpx_calcqueuestrlen(char *queueheader, Cwpx_NodeQueue *queue,
  Cwpx_ResponseHandler *resphandler){

  if(queueheader == NULL || queue == NULL || resphandler == NULL) return 0;
  
  unsigned int CWPX_CRLF_LEN = strlen(CWPX_CRLF);
  unsigned long totalLen = 0;
  totalLen += strlen(queueheader) + CWPX_CRLF_LEN;
  
  char sizeToStr[50];
  
  if(strcmp(queueheader, CWPX_RESPHEADERSQUEUE_ID) == 0){
    sprintf(sizeToStr, "%d%s", resphandler->is_header_sent, CWPX_CRLF);
	totalLen += strlen(sizeToStr);
	
    sprintf(sizeToStr, "%lu%s", resphandler->content_length_sent, CWPX_CRLF);
	totalLen += strlen(sizeToStr);
  }
  
  totalLen += strlen("1") + CWPX_CRLF_LEN; /* 1 for content_type attr */
  
  Cwpx_Node *it = queue->head;
  
  while(it != NULL) {
	/* if the value of the current node is NULL, append "(null)" to the key 
	length for differentiating between a NULL value and an EMPTY value. the 
	other side is going to check the existence of "(null)" everytime it reads
	a length of zero */
	sprintf(sizeToStr, "%lu%s%s", 
	  (unsigned long)it->keyLen, 
	  it->value == NULL ? CWPX_NULLSTR : CWPX_EMPTY, CWPX_CRLF);
	totalLen += strlen(sizeToStr);
	totalLen += it->keyLen + CWPX_CRLF_LEN;
	
	sprintf(sizeToStr, "%lu%s", it->valueLen, CWPX_CRLF);
	totalLen += strlen(sizeToStr);
	totalLen += it->valueLen + CWPX_CRLF_LEN;
	
    it = it->next;
  }
  
  return totalLen;
}


unsigned long cwpx_calcfilequeuestrlen(Cwpx_FileNodeQueue *queue, 
  enum cwpx_content_types content_type){

  if(queue == NULL) return 0;
  
  unsigned int CWPX_CRLF_LEN = strlen(CWPX_CRLF);
  unsigned long totalLen = 0;
  totalLen += strlen(CWPX_FILESQUEUE_ID) + strlen(CWPX_CRLF);
  
  Cwpx_FileNode *fit = queue->head;
  char sizeToStr[50];
  int isNull;
  
  while(fit != NULL) {
    if(content_type != CWPX_RAWBODY){
        
      /* in multipart, the file is physically stored at temp_*/
    
      isNull = fit->key == NULL ? 1 : 0; 
      sprintf(sizeToStr, "%lu%s%s", 
	    isNull ? 0 : (unsigned long)strlen(fit->key), 
		isNull ? CWPX_NULLSTR : CWPX_EMPTY, 
		CWPX_CRLF);
	  totalLen += strlen(sizeToStr); /* keyLen */
	  totalLen += isNull ? 0 : strlen(fit->key) + CWPX_CRLF_LEN; /* keyValue */
	  
      isNull = fit->filename == NULL ? 1 : 0; 
      sprintf(sizeToStr, "%lu%s%s", 
	    isNull ? 0 : (unsigned long)strlen(fit->filename), 
		isNull ? CWPX_NULLSTR : CWPX_EMPTY, 
		CWPX_CRLF);
	  totalLen += strlen(sizeToStr); /* filenameLen */
	  totalLen += 
	    isNull ? 0 : strlen(fit->filename) + CWPX_CRLF_LEN; /* filenameValue */
	  
      isNull = fit->temp_filename == NULL ? 1 : 0; 
      sprintf(sizeToStr, "%lu%s%s", 
	    isNull ? 0 : (unsigned long)strlen(fit->temp_filename), 
		isNull ? CWPX_NULLSTR : CWPX_EMPTY, CWPX_CRLF);
	  totalLen += strlen(sizeToStr); /* temp_filenameLen, temp_filenameValue*/
	  totalLen += isNull ? 0 : strlen(fit->temp_filename) + CWPX_CRLF_LEN;
	  
      isNull = fit->content_type == NULL ? 1 : 0; 
      sprintf(sizeToStr, "%lu%s%s", 
	    isNull ? 0 : (unsigned long)strlen(fit->content_type), 
		isNull ? CWPX_NULLSTR : CWPX_EMPTY, CWPX_CRLF);
	  totalLen += strlen(sizeToStr); /* content_typeLen, content_typeValue*/
	  totalLen += isNull ? 0 : strlen(fit->content_type) + CWPX_CRLF_LEN;
	  
      isNull = fit->content_length == NULL ? 1 : 0; 
      sprintf(sizeToStr, "%lu%s%s", 
	    isNull ? 0 : (unsigned long)strlen(fit->content_length), 
		isNull ? CWPX_NULLSTR : CWPX_EMPTY, CWPX_CRLF);
	  totalLen += strlen(sizeToStr); /* content_lengthLen,content_lengthValue*/
	  totalLen += isNull ? 0 : strlen(fit->content_length) + CWPX_CRLF_LEN;
	  
	  totalLen += strlen("fileLen") + CWPX_CRLF_LEN; /* fileLen attr */
	  sprintf(sizeToStr, "%lu%s", fit->fileLen, CWPX_CRLF);
	  totalLen += strlen(sizeToStr); /* fileLenLen */
      
    }
    else{
      /* in raw, the key contains the value and content_length (or fileLen) 
	  contains its length */
      /*printf("raw value: %s(%u)\n", fit->key, fit->fileLen);*/
      
      totalLen += strlen(CWPX_RAWQUEUE_ID)+CWPX_CRLF_LEN; /*cwpx_rawqueue\r\n*/
      
	  isNull = fit->key == NULL ? 1 : 0; 
	  sprintf(sizeToStr, "%lu%s%s", 
	    fit->fileLen, isNull ? CWPX_NULLSTR: CWPX_EMPTY, CWPX_CRLF);
	  totalLen += strlen(sizeToStr); /* fileLenLen */
	  
      totalLen += fit->fileLen + CWPX_CRLF_LEN;
    }
	  
    fit = fit->next;
  }
  
  return totalLen;
}


unsigned long cwpx_calcqueueslen(Cwpx_RequestHandler *reqhandler,
  struct Cwpx_ResponseHandler *resphandler){

  if(reqhandler == NULL) return 0;
  
  unsigned long getqueueLen = 
    cwpx_calcqueuestrlen(CWPX_GETQUEUE_ID, reqhandler->getqueue, resphandler);
  unsigned long reqcookiesqueueLen = 
    cwpx_calcqueuestrlen(CWPX_REQCOOKIESQUEUE_ID, reqhandler->cookiesqueue,
	resphandler);
  unsigned long postqueueLen = 
    cwpx_calcqueuestrlen(CWPX_POSTQUEUE_ID, reqhandler->postqueue,resphandler);
  unsigned long filesqueueLen = 
    cwpx_calcfilequeuestrlen(reqhandler->filesqueue, 
    reqhandler->postqueue->content_type);
    
  unsigned long respheadersqueueLen = 
    cwpx_calcqueuestrlen(CWPX_RESPHEADERSQUEUE_ID, resphandler->headersqueue,
	resphandler);

  return getqueueLen + reqcookiesqueueLen + postqueueLen + filesqueueLen +
    respheadersqueueLen;
}


unsigned long cwpx_setqueuestostdin(Cwpx_RequestHandler *reqhandler, 
  Cwpx_ResponseHandler *resphandler,
  char *queuestdin){
		
  if(reqhandler == NULL || resphandler == NULL || queuestdin == NULL) return 0;
  
  char *QUEUE_IDS[] = { CWPX_GETQUEUE_ID, CWPX_REQCOOKIESQUEUE_ID,
    CWPX_POSTQUEUE_ID, CWPX_FILESQUEUE_ID, CWPX_RESPHEADERSQUEUE_ID };
  size_t QUEUE_IDS_LEN = sizeof(QUEUE_IDS)/sizeof(QUEUE_IDS[0]);

  unsigned long queuestdinLen = 0;
  
  size_t queueCounter;
  for(queueCounter = 0; queueCounter < QUEUE_IDS_LEN; queueCounter++){

    Cwpx_NodeQueue *queue = NULL;
    Cwpx_FileNodeQueue *fqueue = NULL;
	unsigned long calcQueueLen;
	
	if(strcmp(QUEUE_IDS[queueCounter], CWPX_GETQUEUE_ID) == 0){
	  queue = reqhandler->getqueue;
	}
	else if(strcmp(QUEUE_IDS[queueCounter], CWPX_REQCOOKIESQUEUE_ID) == 0){
	  queue = reqhandler->cookiesqueue;
	}
	else if(strcmp(QUEUE_IDS[queueCounter], CWPX_POSTQUEUE_ID) == 0){
	  queue = reqhandler->postqueue;
	}
	else if(strcmp(QUEUE_IDS[queueCounter], CWPX_FILESQUEUE_ID) == 0){
	  queue = reqhandler->postqueue;
	  fqueue = reqhandler->filesqueue;
	}
	else if(strcmp(QUEUE_IDS[queueCounter], CWPX_RESPHEADERSQUEUE_ID) == 0){
	  queue = resphandler->headersqueue;
	}
	
	if(strcmp(QUEUE_IDS[queueCounter], CWPX_FILESQUEUE_ID) == 0)
	  calcQueueLen = cwpx_calcfilequeuestrlen(fqueue,
	    queue->content_type);
	else
	  calcQueueLen = 
	    cwpx_calcqueuestrlen(QUEUE_IDS[queueCounter], queue, resphandler);
  
    /* store the queue's data in a buffer */
    char *queueData = NULL;
    unsigned long queueDataLen = 0;
    /* CWPX_MIN_BUFLEN is for extra space required for storing value lengths */
    char *tryMalloc = malloc(calcQueueLen + CWPX_MIN_BUFLEN); 
    if(!tryMalloc){ /* free everything ... */ exit(1); }
    queueData = tryMalloc;
    
    if(strcmp(QUEUE_IDS[queueCounter], CWPX_FILESQUEUE_ID) == 0)
	  queueDataLen = cwpx_getlenandfilequeuetostr(fqueue, 
	    QUEUE_IDS[queueCounter], queue->content_type, queueData);
	else
	  queueDataLen = cwpx_getlenandqueuetostr(queue, resphandler,
	    QUEUE_IDS[queueCounter], queueData);
    
    /* pass the queue data to the global stdin */
    memcpy(queuestdin + queuestdinLen, queueData, queueDataLen);
    queuestdinLen += queueDataLen;
    queuestdin[queuestdinLen] = 0;
    
    free(queueData);
  }
  
  return queuestdinLen;
}
  
unsigned long cwpx_getlenandqueuetostr(Cwpx_NodeQueue *queue, 
  Cwpx_ResponseHandler *resphandler, char *queueheader, char *queuestdin){

  if(queue == NULL || queueheader == NULL || queuestdin == NULL) return 0;
  
  unsigned long queueStdinLen = 0;
  
  char sizeToStr[CWPX_MIN_BUFLEN];
  int isNull;
  size_t CWPX_CRLF_LEN = strlen(CWPX_CRLF);

  /* add the queue header */
  sprintf(queuestdin + queueStdinLen, "%s%s", queueheader, CWPX_CRLF);
  queueStdinLen += strlen(queueheader) + CWPX_CRLF_LEN;
  
  if(strcmp(queueheader, CWPX_RESPHEADERSQUEUE_ID) == 0){
    /* add the response 'is_header_sent' attribute */
    sprintf(sizeToStr, "%lu%s", 
	  (long unsigned)resphandler->is_header_sent, CWPX_CRLF);
    size_t sizeToStrLen = strlen(sizeToStr);
    sprintf(queuestdin + queueStdinLen, "%s", sizeToStr);
    queueStdinLen += sizeToStrLen;
    
    /* add the response 'content_length_sent' attribute */
    sprintf(sizeToStr, "%lu%s", 
	  (long unsigned)resphandler->content_length_sent, CWPX_CRLF);
    sizeToStrLen = strlen(sizeToStr);
    sprintf(queuestdin + queueStdinLen, "%s", sizeToStr);
    queueStdinLen += sizeToStrLen;
  }
  
  /* add the queue content_type */
  sprintf(queuestdin + queueStdinLen, "%d%s", queue->content_type, CWPX_CRLF);
  queueStdinLen += strlen("1") + CWPX_CRLF_LEN;
  /* strlen("1") content_type is an int number lesser than 10 */
    
  Cwpx_Node *it = queue->head;
  while(it != NULL) {
    /* keyLen */
    isNull = it->value == NULL ? 1 : 0;
    sprintf(sizeToStr, "%lu%s%s", 
	  (long unsigned)it->keyLen, isNull ? CWPX_NULLSTR: CWPX_EMPTY, CWPX_CRLF);
    size_t sizeToStrLen = strlen(sizeToStr);
    memcpy(queuestdin + queueStdinLen, sizeToStr, sizeToStrLen);
    queueStdinLen += sizeToStrLen;
    /* keyVal */
    memcpy(queuestdin + queueStdinLen, it->key, it->keyLen);
    queueStdinLen += it->keyLen;
    memcpy(queuestdin + queueStdinLen, CWPX_CRLF, strlen(CWPX_CRLF));
    queueStdinLen += CWPX_CRLF_LEN;
      
    /* valueLen*/
    sprintf(sizeToStr, "%lu%s", (long unsigned)it->valueLen, CWPX_CRLF);
    sizeToStrLen = strlen(sizeToStr);
    memcpy(queuestdin + queueStdinLen, sizeToStr, sizeToStrLen);
    queueStdinLen += sizeToStrLen;
    /* valueVal */
    if(!isNull){
      memcpy(queuestdin + queueStdinLen, it->value, it->valueLen);
      queueStdinLen += it->valueLen;
	}
    memcpy(queuestdin + queueStdinLen, CWPX_CRLF, CWPX_CRLF_LEN);
    queueStdinLen += CWPX_CRLF_LEN;
    queuestdin[queueStdinLen] = 0;
    
    it = it->next;
  }
    
  return queueStdinLen;
}


unsigned long cwpx_getlenandfilequeuetostr(Cwpx_FileNodeQueue *queue, 
  char *queueheader, enum cwpx_content_types content_type, char *queuestdin){

  if(queue == NULL || queueheader == NULL || queuestdin == NULL) return 0;
  
  unsigned long queueStdinLen = 0;
  int isNull;
  size_t CWPX_CRLF_LEN = strlen(CWPX_CRLF);
    
  /* add the queue header  */
  sprintf(queuestdin + queueStdinLen, "%s%s", queueheader, CWPX_CRLF);
  queueStdinLen += strlen(queueheader) + strlen(CWPX_CRLF);
    
  Cwpx_FileNode *it = queue->head;
  while(it != NULL) {
    if(content_type != CWPX_RAWBODY){
      char sizeToStr[CWPX_MIN_BUFLEN];
      /* keyLen */
      isNull = it->key == NULL ? 1 : 0;
      long unsigned keyLen = !isNull ? strlen(it->key): 0;
      sprintf(sizeToStr, "%lu%s%s", 
	    keyLen, isNull ? CWPX_NULLSTR : CWPX_EMPTY, CWPX_CRLF);
      size_t sizeToStrLen = strlen(sizeToStr);
      memcpy(queuestdin + queueStdinLen, sizeToStr, sizeToStrLen);
      queueStdinLen += sizeToStrLen;
      /* keyVal */
      if(!isNull){
        memcpy(queuestdin + queueStdinLen, it->key, keyLen);
        queueStdinLen += keyLen;
	  }
      memcpy(queuestdin + queueStdinLen, CWPX_CRLF, CWPX_CRLF_LEN);
      queueStdinLen += CWPX_CRLF_LEN;
      
      /* filenameLen*/
      isNull = it->filename == NULL ? 1 : 0;
      long unsigned filenameLen = !isNull ? strlen(it->filename) : 0;
      sprintf(sizeToStr, "%lu%s%s", 
	    filenameLen, isNull ? CWPX_NULLSTR : CWPX_EMPTY, CWPX_CRLF);
      sizeToStrLen = strlen(sizeToStr);
      memcpy(queuestdin + queueStdinLen, sizeToStr, sizeToStrLen);
      queueStdinLen += sizeToStrLen;
      /* filenameVal */
      if(!isNull){
        memcpy(queuestdin + queueStdinLen, it->filename, filenameLen);
        queueStdinLen += filenameLen;
	  }
      memcpy(queuestdin + queueStdinLen, CWPX_CRLF, CWPX_CRLF_LEN);
      queueStdinLen += CWPX_CRLF_LEN;
      queuestdin[queueStdinLen] = 0;
    
      /* temp_filenameLen*/
      isNull = it->temp_filename == NULL ? 1 : 0;
      unsigned long temp_filenameLen = !isNull ? strlen(it->temp_filename): 0;
      sprintf(sizeToStr, "%lu%s%s", 
	    temp_filenameLen, isNull ? CWPX_NULLSTR : CWPX_EMPTY, CWPX_CRLF);
      sizeToStrLen = strlen(sizeToStr);
      memcpy(queuestdin + queueStdinLen, sizeToStr, sizeToStrLen);
      queueStdinLen += sizeToStrLen;
      /* temp_filenameVal */
      if(!isNull){
       memcpy(queuestdin + queueStdinLen, it->temp_filename, temp_filenameLen);
        queueStdinLen += temp_filenameLen;
	  }
      memcpy(queuestdin + queueStdinLen, CWPX_CRLF, CWPX_CRLF_LEN);
      queueStdinLen += CWPX_CRLF_LEN;
      queuestdin[queueStdinLen] = 0;
    
      /* content_typeLen*/
      isNull = it->content_type == NULL ? 1 : 0;
      unsigned long content_typeLen = !isNull ? strlen(it->content_type): 0;
      sprintf(sizeToStr, "%lu%s%s", 
	    content_typeLen, isNull ? CWPX_NULLSTR : CWPX_EMPTY, CWPX_CRLF);
      sizeToStrLen = strlen(sizeToStr);
      memcpy(queuestdin + queueStdinLen, sizeToStr, sizeToStrLen);
      queueStdinLen += sizeToStrLen;
      /* content_typeVal */
      if(!isNull){
        memcpy(queuestdin + queueStdinLen, it->content_type, content_typeLen);
        queueStdinLen += content_typeLen;
	  }
      memcpy(queuestdin + queueStdinLen, CWPX_CRLF, CWPX_CRLF_LEN);
      queueStdinLen += CWPX_CRLF_LEN;
      queuestdin[queueStdinLen] = 0;
    
      /* content_lengthLen*/
      isNull = it->content_length == NULL ? 1 : 0;
      unsigned long content_lengthLen = !isNull? strlen(it->content_length): 0;
      sprintf(sizeToStr, "%lu%s%s", 
	    content_lengthLen, isNull ? CWPX_NULLSTR : CWPX_EMPTY, CWPX_CRLF);
      sizeToStrLen = strlen(sizeToStr);
      memcpy(queuestdin + queueStdinLen, sizeToStr, sizeToStrLen);
      queueStdinLen += sizeToStrLen;
      /* content_lengthVal */
      if(!isNull){
      memcpy(queuestdin + queueStdinLen, it->content_length,content_lengthLen);
        queueStdinLen += content_lengthLen;
	  }
      memcpy(queuestdin + queueStdinLen, CWPX_CRLF, CWPX_CRLF_LEN);
      queueStdinLen += CWPX_CRLF_LEN;
      queuestdin[queueStdinLen] = 0;
    
      /* fileLenLen*/
      sprintf(queuestdin + queueStdinLen, "%s%s", "fileLen", CWPX_CRLF);
      queueStdinLen += strlen("fileLen") + CWPX_CRLF_LEN;
      /* fileLenVal */
      sprintf(sizeToStr, "%lu%s", it->fileLen, CWPX_CRLF);
      sizeToStrLen = strlen(sizeToStr);
      memcpy(queuestdin + queueStdinLen, sizeToStr, sizeToStrLen);
      queueStdinLen += sizeToStrLen;
    
    }
    else{
	  /* in raw there is only one node (head). the 'key' attr of the head holds
	  the raw data and the content_length attribute its length, as well as the 
	  fileLen attribute (we use the latter for easyness). 
	  we're sending only the fileLen and the raw content */
      /*printf("raw value: %s(%u)\n", 
        it->key, it->fileLen);*/
    
      sprintf(queuestdin + queueStdinLen, "%s%s", CWPX_RAWQUEUE_ID, CWPX_CRLF);
      queueStdinLen += strlen(CWPX_RAWQUEUE_ID) + CWPX_CRLF_LEN;
      
      /* rawLen */
      char sizeToStr[CWPX_MIN_BUFLEN];
      isNull = it->key == NULL ? 1 : 0;
      sprintf(sizeToStr, "%lu%s%s", 
	    it->fileLen, isNull ? CWPX_NULLSTR : CWPX_EMPTY, CWPX_CRLF);
      size_t sizeToStrLen = strlen(sizeToStr);
      memcpy(queuestdin + queueStdinLen, sizeToStr, sizeToStrLen);
      queueStdinLen += sizeToStrLen;
    
      /* rawVal */
      if(!isNull){
        memcpy(queuestdin + queueStdinLen, it->key, it->fileLen);
        queueStdinLen += it->fileLen;
	  }
      memcpy(queuestdin + queueStdinLen, CWPX_CRLF, CWPX_CRLF_LEN);
      queueStdinLen += CWPX_CRLF_LEN;
      
      queuestdin[queueStdinLen] = 0;
	  
    }
    
    it = it->next;
  }
    
  return queueStdinLen;
}


int cwpx_getqueuesfromstdin(char *queuestdin, unsigned long queuestdinlen,
  Cwpx_RequestHandler **reqhandler, Cwpx_ResponseHandler **resphandler,
  char *queueid){

  if(queuestdin == NULL || queuestdinlen == 0 || 
    *reqhandler == NULL || *resphandler == NULL) 
    return -1;
    
  char *QUEUE_IDS[] = { CWPX_GETQUEUE_ID, CWPX_REQCOOKIESQUEUE_ID, 
    CWPX_POSTQUEUE_ID, CWPX_FILESQUEUE_ID, CWPX_RESPHEADERSQUEUE_ID };
  size_t QUEUE_IDS_LEN = sizeof(QUEUE_IDS)/sizeof(QUEUE_IDS[0]);
  
  if(queueid != NULL){
    QUEUE_IDS[0] = queueid;
    QUEUE_IDS_LEN = 1;
  }

  Cwpx_RequestHandler *requestHandlerPtr = *reqhandler;
  Cwpx_ResponseHandler *responseHandlerPtr = *resphandler;
  
  size_t CWPX_CRLF_LEN = strlen(CWPX_CRLF);
  
  size_t queueCounter;
  for(queueCounter = 0; queueCounter < QUEUE_IDS_LEN; queueCounter++){

    /* first, read the queue header */
    char *queueStdinPtr = strstr(queuestdin, QUEUE_IDS[queueCounter]);
    if(queueStdinPtr != NULL){

	  queueStdinPtr += strlen(QUEUE_IDS[queueCounter]) + CWPX_CRLF_LEN;
	  
	  Cwpx_NodeQueue *queue = NULL;
      Cwpx_FileNodeQueue *fqueue = NULL;
	  int isFileQueue = 0;
	  int isRawPost = 0;
	  if(strcmp(QUEUE_IDS[queueCounter], CWPX_GETQUEUE_ID) == 0){
	    queue = requestHandlerPtr->getqueue;
	  }
	  else if(strcmp(QUEUE_IDS[queueCounter], CWPX_REQCOOKIESQUEUE_ID) == 0){
	    queue = requestHandlerPtr->cookiesqueue;
	  }
	  else if(strcmp(QUEUE_IDS[queueCounter], CWPX_POSTQUEUE_ID) == 0){
	    queue = requestHandlerPtr->postqueue;
	  }
	  else if(strcmp(QUEUE_IDS[queueCounter], CWPX_FILESQUEUE_ID) == 0){
	    fqueue = requestHandlerPtr->filesqueue;
	    isFileQueue = 1;
	  }
	  else if(strcmp(QUEUE_IDS[queueCounter], CWPX_RESPHEADERSQUEUE_ID) == 0){
	    queue = responseHandlerPtr->headersqueue;
	  }
	  
	  if(!isFileQueue){
			
		if(strcmp(QUEUE_IDS[queueCounter], CWPX_RESPHEADERSQUEUE_ID) == 0){
          /* get the response 'is_header_sent' attribute */
          int isHeaderSent = atoi(queueStdinPtr);
          responseHandlerPtr->is_header_sent = isHeaderSent;
          char *newlinePtr = strstr(queueStdinPtr, CWPX_CRLF);
          if(newlinePtr == NULL) return -1;
          newlinePtr += CWPX_CRLF_LEN;
          queueStdinPtr = newlinePtr;
          
          /* get the response 'content_length_sent' attribute */
          unsigned long contentLengthSent = (unsigned long)atol(queueStdinPtr);
          responseHandlerPtr->content_length_sent = contentLengthSent;
          newlinePtr = strstr(queueStdinPtr, CWPX_CRLF);
          if(newlinePtr == NULL) return -1;
          newlinePtr += CWPX_CRLF_LEN;
          queueStdinPtr = newlinePtr;
          
        }
			
	    /* the next thing that comes after the queue header (unless response
		headers queue) is the content_type attribute, it's just an integer */
	    unsigned int queue_content_type = (unsigned)atoi(queueStdinPtr);
      
        queue->content_type = queue_content_type;
        
        char *content_typeEnd = strstr(queueStdinPtr, CWPX_CRLF);
        if(content_typeEnd == NULL) return -1;
        queueStdinPtr = content_typeEnd + CWPX_CRLF_LEN;
        
	  }
	  else{
	    if(strncmp(queueStdinPtr, CWPX_RAWQUEUE_ID, strlen(CWPX_RAWQUEUE_ID))
		  == 0){
		  isRawPost = 1;
		  
		  char *rowqueueidEnd = strstr(queueStdinPtr, CWPX_CRLF);
          if(rowqueueidEnd == NULL) return -1;
          queueStdinPtr = rowqueueidEnd + CWPX_CRLF_LEN;
		}
	  }
	
	  /*  the next thing to read in the stdin are the key-value pairs */
	  unsigned int fillingQueue = 1;
	  while(fillingQueue){
			
		unsigned long keyLen = (unsigned)atoi(queueStdinPtr);
		/*int isNull = strstr(queueStdinPtr, CWPX_NULLSTR) != NULL ? 1 : 0;*/
		/* using strstr in the whole queueStdinPtr for checking isNull is wrong 
		because it may take a further coicidence as the a correct one, leading
		to misinterpretations. this requires to be tested on one single line, 
		not a chunk of text, or at least check only at the beginning: */
		char *textPart; unsigned long numericPart;
        numericPart = strtoul(queueStdinPtr, &textPart, 10);
        numericPart = numericPart; /* avoid unused warning */
		int isNull = strstr(textPart, CWPX_NULLSTR) == textPart ? 1 : 0;
		
		/* when it's not a file queue, isNull asks if the node value is null,
		but when it is a file queue, isNull asks if the node key is null */
      
        if(!keyLen) break;
      
        char *keyPtr = strstr(queueStdinPtr, CWPX_CRLF);
        if(keyPtr == NULL) return -1;
        keyPtr += strlen(CWPX_CRLF);
        
        queueStdinPtr = keyPtr + keyLen + CWPX_CRLF_LEN;
        
	    if(!isFileQueue){
          
	      unsigned long valueLen = (unsigned long)atol(queueStdinPtr);
          char *valuePtr = strstr(queueStdinPtr, CWPX_CRLF);
          if(valuePtr == NULL) return -1;
          valuePtr += CWPX_CRLF_LEN;
          
          queueStdinPtr = valuePtr + valueLen + CWPX_CRLF_LEN;
      
          Cwpx_Node *tryMalloc = malloc(sizeof(Cwpx_Node));
          if(!tryMalloc){ /* free everything ... */ exit(1); }
          Cwpx_Node *newNodeObject = tryMalloc;
          newNodeObject->key = NULL;
          newNodeObject->keyLen = 0;
          newNodeObject->value = NULL;
          newNodeObject->valueLen = 0;
          
          
          char *tryStrMalloc = malloc(keyLen + 1);
          if(!tryStrMalloc){ /* free everything ... */ exit(1); }
          newNodeObject->key = tryStrMalloc;
          memcpy(newNodeObject->key, keyPtr, keyLen);
          newNodeObject->key[keyLen] = 0;
          newNodeObject->keyLen = keyLen;
      
          if(!isNull){
            tryStrMalloc = malloc(valueLen + 1);
            if(!tryStrMalloc){ /* free everything ... */ exit(1); }
            newNodeObject->value = tryStrMalloc;
            memcpy(newNodeObject->value, valuePtr, valueLen);
            newNodeObject->value[valueLen] = 0;
            newNodeObject->valueLen = valueLen;
		  }
      
          newNodeObject->next = NULL;
      
          queue->push(&queue, NULL, newNodeObject, 0);
	    
	      if(fillingQueue++ > CWPX_MAX_QUEUEELEMS) 
	        break;
	      
	    }
	    else{ /* filequeue node */
		  unsigned long filenameLen, temp_filenameLen, content_typeLen, 
		    content_lengthLen;
		  char *filenamePtr; char *temp_filenamePtr; char *content_typePtr;
		    char *content_lengthPtr;
		  int isFilenameNull, isTempFilenameNull, isContentTypeNull, 
		    isContentLengthNull;
		  unsigned long fileLen;
		    
          if(!isRawPost){
	        filenameLen = (unsigned long)atol(queueStdinPtr);
		    isFilenameNull = strstr(queueStdinPtr, CWPX_NULLSTR) != NULL ? 1:0;
            filenamePtr = strstr(queueStdinPtr, CWPX_CRLF);
            if(filenamePtr == NULL) return -1;
            filenamePtr += CWPX_CRLF_LEN;
          
            queueStdinPtr = filenamePtr + filenameLen + CWPX_CRLF_LEN;
          
            temp_filenameLen = (unsigned long)atol(queueStdinPtr);
		    isTempFilenameNull = 
			  strstr(queueStdinPtr, CWPX_NULLSTR) != NULL ? 1 : 0;
            temp_filenamePtr = strstr(queueStdinPtr, CWPX_CRLF);
            if(temp_filenamePtr == NULL) return -1;
            temp_filenamePtr += strlen(CWPX_CRLF);
          
            queueStdinPtr = temp_filenamePtr + temp_filenameLen +CWPX_CRLF_LEN;
          
            content_typeLen = (unsigned long)atol(queueStdinPtr);
		    isContentTypeNull = strstr(queueStdinPtr, CWPX_NULLSTR)!= NULL?1:0;
            content_typePtr = strstr(queueStdinPtr, CWPX_CRLF);
            if(content_typePtr == NULL) return -1;
            content_typePtr += CWPX_CRLF_LEN;
          
            queueStdinPtr = content_typePtr + content_typeLen + CWPX_CRLF_LEN;
          
            content_lengthLen = (unsigned long)atol(queueStdinPtr);
		    isContentLengthNull = strstr(queueStdinPtr,CWPX_NULLSTR)!=NULL?1:0;
            content_lengthPtr = strstr(queueStdinPtr, CWPX_CRLF);
            if(content_lengthPtr == NULL) return -1;
            content_lengthPtr += CWPX_CRLF_LEN;
          
            queueStdinPtr = content_lengthPtr+content_lengthLen+CWPX_CRLF_LEN;
		  
          
            /* the final attribute is fileLen */
            queueStdinPtr += strlen("fileLen") + CWPX_CRLF_LEN;
            fileLen = (unsigned long)atol(queueStdinPtr);
            
            /* move the pointer to pass the fileLen value */
            char *passPtr = strstr(queueStdinPtr, CWPX_CRLF);
            queueStdinPtr = passPtr + CWPX_CRLF_LEN;
          }
          else{
		    fileLen = keyLen;
		  }
          
      
          Cwpx_FileNode *tryMalloc = malloc(sizeof(Cwpx_FileNode));
          if(!tryMalloc){ /* free everything ... */ exit(1); }
          Cwpx_FileNode *newFileNodeObject = tryMalloc;
          newFileNodeObject->key = NULL;
          newFileNodeObject->filename = NULL;
          newFileNodeObject->temp_filename = NULL;
          newFileNodeObject->content_type = NULL;
          newFileNodeObject->content_length = NULL;
          newFileNodeObject->fileLen = 0;
          
          char *tryStrMalloc = NULL;
          if(!isNull){
            tryStrMalloc = malloc(keyLen + 1);
            if(!tryStrMalloc){ /* free everything ... */ exit(1); }
            newFileNodeObject->key = tryStrMalloc;
            memcpy(newFileNodeObject->key, keyPtr, keyLen);
            newFileNodeObject->key[keyLen] = 0;
		  }
		  
          if(!isRawPost){
			if(!isFilenameNull){
              tryStrMalloc = malloc(filenameLen + 1);
              if(!tryStrMalloc){ /* free everything ... */ exit(1); }
              newFileNodeObject->filename = tryStrMalloc;
              memcpy(newFileNodeObject->filename, filenamePtr, filenameLen);
              newFileNodeObject->filename[filenameLen] = 0;
		    }
          
            if(!isTempFilenameNull){
              tryStrMalloc = malloc(temp_filenameLen + 1);
              if(!tryStrMalloc){ /* free everything ... */ exit(1); }
              newFileNodeObject->temp_filename = tryStrMalloc;
              memcpy(newFileNodeObject->temp_filename, 
			    temp_filenamePtr, temp_filenameLen);
              newFileNodeObject->temp_filename[temp_filenameLen] = 0;
		    }
          
            if(!isContentTypeNull){
              tryStrMalloc = malloc(content_typeLen + 1);
              if(!tryStrMalloc){ /* free everything ... */ exit(1); }
              newFileNodeObject->content_type = tryStrMalloc;
              memcpy(newFileNodeObject->content_type, 
			    content_typePtr, content_typeLen);
              newFileNodeObject->content_type[content_typeLen] = 0;
		    }
          
            if(!isContentLengthNull){
              tryStrMalloc = malloc(content_lengthLen + 1);
              if(!tryStrMalloc){ /* free everything ... */ exit(1); }
              newFileNodeObject->content_length = tryStrMalloc;
              memcpy(newFileNodeObject->content_length, 
			    content_lengthPtr, content_lengthLen);
              newFileNodeObject->content_length[content_lengthLen] = 0;
		    }
		  }
          
          newFileNodeObject->fileLen = fileLen;
      
          
          newFileNodeObject->next = NULL;
      
          /*fqueue->push(&fqueue, NULL, newFileNodeObject, 0);*/
          /* the 'push' function on Cwpx_FileNodeQueue struct is not meant to 
		  be called by anyone but the 'push' function on Cwpx_NodeQueue by the
		  very first script that handles the request, therefore, we have to 
		  manually push the filenode in Cwpx_FileNodeQueue */
		  if( fqueue->head == NULL){
		    fqueue->head = newFileNodeObject;
		    newFileNodeObject->next = NULL;
		  }
		  else{
		    Cwpx_FileNode *it = fqueue->head;
		    while(it->next)
		      it = it->next;
		    it->next = newFileNodeObject;
		    newFileNodeObject->next = NULL;
		  }
	    
	      if(fillingQueue++ > CWPX_MAX_QUEUEELEMS) 
	        break;
	      
	      if(isRawPost)
	        break;
	    }
	  }
    }
  }

  return 0;
}


void cwpx_destroy_context(Cwpx_Context **_this){

  if(*_this == NULL) return;
  
  Cwpx_Context *_thisPtr = *_this;
  
  cwpx_destroy_requesthandler(&_thisPtr->reqhandler);  
  cwpx_destroy_responsehandler(&_thisPtr->resphandler);
  
  #ifndef _WIN32
  /* the current implementation of mmap doesn't use MAP_ANONYMOUS, so we have
  to use a 'physical' file for working with mmap. that file is not removed 
  automatically when the program finishes, so it has to be removed manually. 
  since this 'cwpx_destroy_context' function is called by both parent and child 
  processes, a child process may remove the file, which is not allowed, only 
  the first parent (the origin script), being this the initial creator of the 
  mapping, so we need to check this before 'cwpx_destroy_memfile' call ... */
  if(!_thisPtr->forwcluded){
    if(_thisPtr->argshandler->file->filename != NULL){
      remove(_thisPtr->argshandler->file->filename);
    }
  }
  #endif
  cwpx_destroy_memfile(&_thisPtr->argshandler);
  
  cwpx_destroy_confighandler(&_thisPtr->confighandler);
  
  if(_thisPtr->guuid != NULL){
    free(_thisPtr->guuid);
    _thisPtr->guuid = NULL;
  }
  
  if(_thisPtr->log != NULL){
    fclose(_thisPtr->log);
    _thisPtr->log = NULL;
  }
  
  free(*_this);
  *_this = NULL;
}


/* local functions for request interface (depend on cwpx_context variable) */
const char *cwpx_i_reqheader(const char *key){
  if(key == NULL || cwpx_context == NULL) return NULL;
  return cwpx_context->reqhandler->header(&cwpx_context->reqhandler, key);
}

const char *cwpx_i_env(const char *key){
  if(key == NULL || cwpx_context == NULL) return NULL;
  return cwpx_context->reqhandler->env(&cwpx_context->reqhandler, key);
}

const char *cwpx_i_reqcookies(const char *key){
  if(key == NULL || cwpx_context == NULL) return NULL;
  return cwpx_context->reqhandler->cookie(&cwpx_context->reqhandler, key);
}

const char *cwpx_i_get(const char *key){
  if(key == NULL || cwpx_context == NULL) return NULL;
  return cwpx_context->reqhandler->get(&cwpx_context->reqhandler, key);
}

const char *cwpx_i_getat(const char *key, unsigned int index){
  if(key == NULL || cwpx_context == NULL) return NULL;
  return cwpx_context->reqhandler->get_at(&cwpx_context->reqhandler,key,index);
}

int cwpx_i_issetget(const char *key){
  if(key == NULL || cwpx_context == NULL) return -1;
  return cwpx_context->reqhandler->isset_get(&cwpx_context->reqhandler, key);
}

int cwpx_i_issetgetat(const char *key, unsigned int index){
  if(key == NULL || cwpx_context == NULL) return -1;
  return 
   cwpx_context->reqhandler->isset_get_at(&cwpx_context->reqhandler,key,index);
}

const char *cwpx_i_post(const char *key){
  if(key == NULL || cwpx_context == NULL) return NULL;
  return cwpx_context->reqhandler->post(&cwpx_context->reqhandler, key);
}

const char *cwpx_i_postat(const char *key, unsigned int index){
  if(key == NULL || cwpx_context == NULL) return NULL;
 return cwpx_context->reqhandler->post_at(&cwpx_context->reqhandler,key,index);
}

int cwpx_i_issetpost(const char *key){
  if(key == NULL || cwpx_context == NULL) return -1;
  return cwpx_context->reqhandler->isset_post(&cwpx_context->reqhandler, key);
}

int cwpx_i_issetpostat(const char *key, unsigned int index){
  if(key == NULL || cwpx_context == NULL) return -1;
  return 
  cwpx_context->reqhandler->isset_post_at(&cwpx_context->reqhandler,key,index);
}

const char *cwpx_i_files(const char *key, const char *attr){
  if(key == NULL || attr == NULL || cwpx_context == NULL) return NULL;
  return cwpx_context->reqhandler->files(&cwpx_context->reqhandler, key, attr);
}

const char *cwpx_i_raw(const char *attr){
  if(cwpx_context == NULL) return NULL;
  return cwpx_context->reqhandler->raw(&cwpx_context->reqhandler, attr);
}

int cwpx_i_forward(const char *scriptpath){
  if(cwpx_context == NULL || scriptpath == NULL) return -1;
  return cwpx_context->reqhandler->forward(&cwpx_context, scriptpath);
}

/* local functions for response interface (depend on cwpx_context variable) */
int cwpx_i_write(const char *format, ...){
  if(format == NULL || cwpx_context == NULL) return -1;
  va_list args;
  va_start(args, format);
  int ret = cwpx_context->resphandler->write(&cwpx_context->resphandler, 
    format, args);
  va_end(args);
  return ret;
}

int cwpx_i_respheader(const char *key, const char *value){
  if(key == NULL || value == NULL || cwpx_context == NULL) return -1;
  return cwpx_context->resphandler->header(&cwpx_context->resphandler, 
    key, value);
}

int cwpx_i_respcookies(const char *key, const char *value, long expires, 
  long maxage, const char *path, const char *domain, int secure, int httponly, 
  const char *attrformat, ...){

  if(key == NULL || value == NULL || cwpx_context == NULL) return -1;
  va_list args;
  va_start(args, attrformat);
  int ret = cwpx_context->resphandler->cookie(&cwpx_context->resphandler, key, 
    value, expires, maxage, path, domain, secure, httponly, attrformat, args);
  va_end(args);
  return ret;
}

long cwpx_i_writeb(char *bytes, unsigned long byteslen){
  if(bytes == NULL || cwpx_context == NULL || byteslen <= 0) return -1;
  return cwpx_context->resphandler->write_b(&cwpx_context->resphandler, 
    bytes, byteslen);
}

int cwpx_i_include(const char *scriptpath){
  if(cwpx_context == NULL || scriptpath == NULL) return -1;
  return cwpx_context->resphandler->include(&cwpx_context, scriptpath);
}


int cwpx_i_cwpxargsset(const char *key, void *value, unsigned long size_of, 
  char *vartype){
  
  return _cwpx_set_object(
    &cwpx_context->argshandler->queue, key, value, size_of, vartype
  );
}

void *cwpx_i_cwpxargsget(const char *key, unsigned long size_of,char *vartype){
  return _cwpx_get_object(
    &cwpx_context->argshandler->queue, key, size_of, vartype
  );
}

const char *cwpx_i_getworkdir(){
  return cwpx_context->confighandler->workdir;
}


const char *cwpx_i_getrootdir(){
  return cwpx_context->confighandler->rootdir;
}

const char *cwpx_i_gethostname(){
  return cwpx_context->confighandler->hostname;
}


const char *cwpx_i_getservip(){
  return cwpx_context->confighandler->servip;
}


unsigned int cwpx_i_getservpt(){
  return cwpx_context->confighandler->servpt;
}

const char *cwpx_i_getcurrentdir(){
  return cwpx_context->confighandler->currentdir;
}

const char *cwpx_i_gettempdir(){
  return cwpx_context->confighandler->tempdir;
}

const char *cwpx_i_getsessdir(){
  return cwpx_context->confighandler->sessdir;
}

const char *cwpx_i_getsessid(){
  return cwpx_context->confighandler->sessid;
}

int cwpx_i_setsessid(const char *id){
  
  if(id == NULL) return -1;
  size_t idLen = strlen(id);
  if(idLen >= CWPX_MAX_KEYLEN) return -1;
  
  char *tryMalloc = CWPX_SESSID == NULL ? 
    malloc(idLen + 1) : realloc(CWPX_SESSID, idLen + 1);
  if(tryMalloc == NULL){ /* free everything */ exit(1); }
  CWPX_SESSID = tryMalloc;
  strcpy(CWPX_SESSID, id);
  
  tryMalloc = cwpx_context->confighandler->sessid == NULL ?
    malloc(idLen + 1): realloc(cwpx_context->confighandler->sessid, idLen + 1);
  if(tryMalloc == NULL){ /* free everything */ exit(1); }
  cwpx_context->confighandler->sessid = tryMalloc;
  strcpy(cwpx_context->confighandler->sessid, id);
  
  return 0;
}

unsigned long cwpx_i_getsessdur(){
  return cwpx_context->confighandler->sessdur;
}

const char *cwpx_i_getlogfile(){
  return cwpx_context->confighandler->logfile;
}

unsigned long cwpx_i_getbuf_size(){
  return cwpx_context->confighandler->buf_size;
}

unsigned int cwpx_i_getmaxqueueelems(){
  return cwpx_context->confighandler->max_queueelems;
}

unsigned short cwpx_i_getallow_post(){
  return cwpx_context->confighandler->allow_post;
}

unsigned long cwpx_i_getmax_post(){
  return cwpx_context->confighandler->max_post;
}

int cwpx_i_sessionstart(){

  /* if a session was already started it means that the client sent us the 
  cookie (or URL param) and sesshandler.sessionstarted was set to 1 (true), as 
  well as the objects queue got loaded from file.
  if that's not the case, this means that the session is going to be started 
  right at this point, so we have to create a cookie (or some handy for that)*/
  
  /* check whether the session hasn't been started yet, in which case we have 
  to initialize the session from file before starting to work on the session */
  if(!cwpx_context->sesshandler->sessionstarted){
    int getfromfile = cwpx_getobjectsqueuefromfile(
	  &cwpx_context->sesshandler->queue, &cwpx_context->sesshandler->file, 
	  NULL, NULL);
	  getfromfile = getfromfile; /* avoid unused warning */
	/*if(getfromfile != 0) return -1;*/
	
	/* this method is in charge of setting the response cookie. by default, the 
	cookie is a session-scoped cookie, only available until the client 'closes 
	the window'. other attributes of the cookie such as 'expires' must be set 
	manually by calling the response.cookie function */
	
	char sessionvalue[CWPX_MIN_BUFLEN];
    sprintf(sessionvalue, "_%s._%d", cwpx_context->guuid, cwpx_context->pid);
    /* DON'T change the low hyphens, or we have to change it everywhere */
	
	
	int setRespCookie = 
	cwpx_context->resphandler->cookie(&cwpx_context->resphandler, CWPX_SESSID, 
	  sessionvalue, 0, 0, NULL, NULL, 0, 0, NULL, NULL);
	if(setRespCookie != 0) return -1;
	
	cwpx_context->sesshandler->sessionstarted = 1;
  }
  
  return 0;
}


int cwpx_i_sessionset(const char *key, void *value, unsigned long size_of, 
  char *vartype){

  if(!cwpx_context->sesshandler->sessionstarted){
    int getfromfile = cwpx_getobjectsqueuefromfile(
	  &cwpx_context->sesshandler->queue, &cwpx_context->sesshandler->file, 
	  NULL, NULL);
	getfromfile = getfromfile; /* avoid unused warning */
	/*if(getfromfile != 0) return -1;*/ /* IS THIS NECESSARY? */
  } 

  return _cwpx_set_object(
    &cwpx_context->sesshandler->queue, key, value, size_of, vartype
  );
}

void *cwpx_i_sessionget(const char *key, unsigned long size_of,char *vartype){
	
  /* check whether the session hasn't been started yet, in which case we have 
  to initialize the session from file before starting to work on the session */
  if(!cwpx_context->sesshandler->sessionstarted){
    int getfromfile = cwpx_getobjectsqueuefromfile(
	  &cwpx_context->sesshandler->queue, &cwpx_context->sesshandler->file, 
	  NULL, NULL);
	  getfromfile = getfromfile; /* avoid unused warning */
	/*if(getfromfile != 0) return NULL;*/ /* not sure if needed */
	/*cwpx_context->sesshandler->sessionstarted = 1;*/ /* not sure if needed */
  }
  
  return _cwpx_get_object(
    &cwpx_context->sesshandler->queue, key, size_of, vartype
  );
}

const char *cwpx_i_getsessidvalue(){
  if(cwpx_context->sesshandler == NULL) return NULL;
  
  if(!cwpx_context->sesshandler->sessionstarted) return NULL;
  
  const char *sessid = cwpx_context->confighandler->sessid;
  if(sessid == NULL) return NULL;
  
  const char *sessionCookie = 
    cwpx_context->reqhandler->cookie(&cwpx_context->reqhandler, sessid);
  if(sessionCookie != NULL) return sessionCookie;
    
  const char *sessionUrl = 
    cwpx_context->reqhandler->get(&cwpx_context->reqhandler, sessid);
  if(sessionUrl != NULL) return sessionUrl;
  
  if(cwpx_context->guuid == NULL) return NULL;
  static char configGuuid[CWPX_MIN_BUFLEN];
  sprintf(configGuuid, "_%s._%d", cwpx_context->guuid, cwpx_context->pid);
  return configGuuid;
}


int cwpx_log(const char *format, ...){
  
  if(format == NULL) return -1;

  size_t format_length = strlen(format);
  if(format_length <= 0) return -1;
  
  if(cwpx_context->confighandler->logfile == NULL) return -1;
  
  va_list args;
  va_start(args, format);
  
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
  
  if(cwpx_context->log == NULL){
    cwpx_context->log = fopen(cwpx_context->confighandler->logfile, "ab");
    if(cwpx_context->log == NULL){
	  free(buffer);
	  /* free everything */
	  exit(1);
	}
  }
  
  char timezonestr[50];
  memset(timezonestr, 0, 50);
  cwpx_getgmtdateandzone(timezonestr);
  
  fprintf(cwpx_context->log, "[%s] - %s%s", timezonestr, buffer, CWPX_CRLF);

  free(buffer);
  
  /* returns >= 0 on success*/
  return pf_ret;
}


void cwpx_init_confighandler(Cwpx_ConfigHandler **_this){

  Cwpx_ConfigHandler *tryMalloc = malloc(sizeof(Cwpx_ConfigHandler));
  if(!tryMalloc){ /* free everything ... */ exit(1); }
  *_this = tryMalloc;
  Cwpx_ConfigHandler *_thisPtr = *_this;
  
  _thisPtr->workdir = NULL;
  _thisPtr->rootdir = NULL;
  _thisPtr->hostname = NULL;
  _thisPtr->servip = NULL;
  /*_thisPtr->servpt;*/
  _thisPtr->tempdir = NULL;
  _thisPtr->sessdir = NULL;
  _thisPtr->sessid = NULL;
  _thisPtr->currentdir = NULL;
  _thisPtr->logfile = NULL;
  /*_thisPtr->buf_size;
  _thisPtr->max_keylen;
  _thisPtr->max_queueelems;
  _thisPtr->allow_post;
  _thisPtr->max_post; */
    

  char currentdir[CWPX_PATH_MAX];
  /* Sometimes using "./" or "~/" don't work as espected */
  if(getcwd(currentdir, sizeof(currentdir)) == NULL) {
    #ifdef _WIN32
    strcpy(currentdir, "./");
    #else
    strcpy(currentdir, "~/");
    #endif
  }

  char *tryMallocStr = malloc(strlen(currentdir) + 1);
  if(tryMallocStr == NULL){ /* free everything */ exit(1); }
  _thisPtr->currentdir = tryMallocStr;
  strcpy(_thisPtr->currentdir, currentdir);
    
  #ifdef _WIN32
  /*int i;
  for(i = 0; i < strlen(_thisPtr->currentdir); i++)
    if(_thisPtr->currentdir[i]=='\\')
      _thisPtr->currentdir[i] = '/';*/
  cwpx_replacebackslashes(_thisPtr->currentdir);
  #endif
  
  char workdir[CWPX_PATH_MAX];
  int setdir = CWPX_WORKDIR == NULL ? 1 : 0;
  if(CWPX_WORKDIR != NULL){
    strcpy(workdir, CWPX_EMPTY); 
    cwpx_strtrim(CWPX_WORKDIR, workdir);
    if(strcmp(workdir, CWPX_EMPTY) == 0) setdir = 1;
    else setdir = 0;
  }
  /* if no workdir provided ... */
  char *copydir = setdir ? _thisPtr->currentdir : workdir;
  tryMallocStr = malloc(strlen(copydir) + 1);
  if(tryMallocStr == NULL){ /* free everything */ exit(1); }
  _thisPtr->workdir = tryMallocStr;
  strcpy((char *)_thisPtr->workdir, copydir);
  #ifdef _WIN32
  cwpx_replacebackslashes(_thisPtr->workdir);
  #endif
  
  char rootdir[CWPX_PATH_MAX];
  setdir = CWPX_ROOTDIR == NULL ? 1 : 0;
  if(CWPX_ROOTDIR != NULL){
    strcpy(rootdir, CWPX_EMPTY); 
    cwpx_strtrim(CWPX_ROOTDIR, rootdir);
    if(strcmp(rootdir, CWPX_EMPTY) == 0) setdir = 1;
    else setdir = 0;
  }
  /* if no rootdir provided ... */
  copydir = setdir ? _thisPtr->currentdir : rootdir;
  tryMallocStr = malloc(strlen(copydir) + 1);
  if(tryMallocStr == NULL){ /* free everything */ exit(1); }
  _thisPtr->rootdir = tryMallocStr;
  strcpy((char *)_thisPtr->rootdir, copydir);
  #ifdef _WIN32
  cwpx_replacebackslashes(_thisPtr->rootdir);
  #endif
  
  char hostname[CWPX_MIN_BUFLEN] = CWPX_EMPTY;
  setdir = CWPX_HOSTNAME == NULL ? 1 : 0;
  if(CWPX_HOSTNAME != NULL){
    strcpy(hostname, CWPX_EMPTY); 
    cwpx_strtrim(CWPX_HOSTNAME, hostname);
    if(strcmp(hostname, CWPX_EMPTY) == 0) setdir = 1;
    else setdir = 0;
  }
  /* if no hostname provided ... */
  if(setdir) cwpx_gethostname(hostname);
  tryMallocStr = malloc(strlen(hostname) + 1);
  if(tryMallocStr == NULL){ /* free everything */ exit(1); }
  _thisPtr->hostname = tryMallocStr;
  strcpy(_thisPtr->hostname, hostname);
  
  char servip[CWPX_MIN_BUFLEN] = CWPX_EMPTY;
  setdir = CWPX_SERVIP == NULL ? 1 : 0;
  if(CWPX_SERVIP != NULL){
    strcpy(servip, CWPX_EMPTY); 
    cwpx_strtrim(CWPX_SERVIP, servip);
    if(strcmp(servip, CWPX_EMPTY) == 0) setdir = 1;
    else setdir = 0;
  }
  /* if no servip provided ... */
  if(setdir) cwpx_getipaddress(_thisPtr->hostname, servip);
  tryMallocStr = malloc(strlen(servip) + 1);
  if(tryMallocStr == NULL){ /* free everything */ exit(1); }
  _thisPtr->servip = tryMallocStr;
  strcpy(_thisPtr->servip, servip);
  
  _thisPtr->servpt = CWPX_SERVPT;
  
  char tempdir[CWPX_PATH_MAX];
  setdir = CWPX_TEMPDIR == NULL ? 1 : 0;
  if(CWPX_TEMPDIR != NULL){
    strcpy(tempdir, CWPX_EMPTY); 
    cwpx_strtrim(CWPX_TEMPDIR, tempdir);
    if(strcmp(tempdir, CWPX_EMPTY) == 0) setdir = 1;
    else setdir = 0;
  }
  /* if no tempdir provided ... */
  if(setdir) cwpx_gettempdir(tempdir);
  copydir = tempdir;
  tryMallocStr = malloc(strlen(copydir) + 1);
  if(tryMallocStr == NULL){ /* free everything */ exit(1); }
  _thisPtr->tempdir = tryMallocStr;
  strcpy((char *)_thisPtr->tempdir, copydir);
  #ifdef _WIN32
  cwpx_replacebackslashes(_thisPtr->tempdir);
  #endif
  
  char sessdir[CWPX_PATH_MAX];
  setdir = CWPX_SESSDIR == NULL ? 1 : 0;
  if(CWPX_SESSDIR != NULL){
    strcpy(sessdir, CWPX_EMPTY); 
    cwpx_strtrim(CWPX_SESSDIR, sessdir);
    if(strcmp(sessdir, CWPX_EMPTY) == 0) setdir = 1;
    else setdir = 0;
  }
  /* if no sessdir provided ... */
  copydir = setdir ? _thisPtr->tempdir : sessdir;
  tryMallocStr = malloc(strlen(copydir) + 1);
  if(tryMallocStr == NULL){ /* free everything */ exit(1); }
  _thisPtr->sessdir = tryMallocStr;
  strcpy((char *)_thisPtr->sessdir, copydir);
  #ifdef _WIN32
  cwpx_replacebackslashes(_thisPtr->sessdir);
  #endif
  
  char sessid[CWPX_MAX_KEYLEN];
  setdir = CWPX_SESSID == NULL ? 1 : 0;
  if(CWPX_SESSID != NULL){
    strcpy(sessid, CWPX_EMPTY); 
    cwpx_strtrim(CWPX_SESSID, sessid);
    if(strcmp(sessid, CWPX_EMPTY) == 0) setdir = 1;
    else setdir = 0;
  }
  /* if no sessid provided ... */
  copydir = setdir ? "cwpxsid" : sessid;
  tryMallocStr = malloc(strlen(copydir) + 1);
  if(tryMallocStr == NULL){ /* free everything */ exit(1); }
  _thisPtr->sessid = tryMallocStr;
  strcpy((char *)_thisPtr->sessid, copydir);
  
  /* new,  UPDATE ALSO CWPX_SESSID global */
  if(setdir){
    tryMallocStr = CWPX_SESSID == NULL ? 
	  malloc(strlen(copydir) + 1) : realloc(CWPX_SESSID, strlen(copydir) + 1);
    if(tryMallocStr == NULL){ /* free everything */ exit(1); }
    CWPX_SESSID = tryMallocStr;
    strcpy((char *)CWPX_SESSID, copydir);
  }
  
  _thisPtr->sessdur = CWPX_SESSDUR;
  
  char logfile[CWPX_PATH_MAX + CWPX_NAME_MAX];
  setdir = CWPX_LOGFILE == NULL ? 1 : 0;
  if(CWPX_LOGFILE != NULL){
    strcpy(logfile, CWPX_EMPTY); 
    cwpx_strtrim(CWPX_LOGFILE, logfile);
    if(strcmp(logfile, CWPX_EMPTY) == 0) setdir = 1;
    else setdir = 0;
  }
  /* if no logfile provided ... */
  if(setdir){
    time_t t = time(NULL);
    struct tm *tm = (struct tm *)localtime(&t);
    char date[30];
    strftime(date, sizeof(date), "%Y-%m-%d", tm);
	
	int endsWithSlash = 
	  _thisPtr->currentdir[strlen(_thisPtr->currentdir) - 1] == '/';
    
    /*Ex: c.2022-06-12.cwpxlog */
    sprintf(logfile, "%s%s%s.%s%s",  
	  _thisPtr->currentdir, endsWithSlash ? "" : "/", 
	  "c", date, ".cwpxlog");
  }
	  
  copydir = logfile;
  tryMallocStr = malloc(strlen(copydir) + 1);
  if(tryMallocStr == NULL){ /* free everything */ exit(1); }
  _thisPtr->logfile = tryMallocStr;
  strcpy((char *)_thisPtr->logfile, copydir);
  #ifdef _WIN32
  cwpx_replacebackslashes(_thisPtr->logfile);
  #endif
  
  _thisPtr->buf_size = CWPX_BUF_SIZE;
  
  _thisPtr->max_queueelems = CWPX_MAX_QUEUEELEMS;
  
  _thisPtr->allow_post = CWPX_ALLOW_POST;
  
  _thisPtr->max_post = CWPX_MAX_POST;

}


int cwpx_readandfillrequest(Cwpx_RequestHandler **reqhandler){

  if(*reqhandler == NULL) return -1;
  
  Cwpx_RequestHandler *reqhandlerPtr = *reqhandler;
  /* request headers: read the environment variables */
  /* in CGI, the request headers don't need to get stored in a buffer and then
  pushed into 'Cwpx_RequestHandler.headersqueue', because those headers are
  stored in environment variables and the corresponding functions that handle
  the work look at that place */
  
  /* as an 'exception' to the above paragraph,the 'cookiesqueue' is a special 
  case of header which items need to get stored and then queued, because one 
  cookie header may contain multiple 'key-value' pairs, and also there may be 
  more than one cookie header. the get (query_string) queue is also a special
  case.  */
  unsigned int index = 0;
  int fillingCookiesQueue = 1;
  while(fillingCookiesQueue){
    char *cookie = 
	  (char*)reqhandlerPtr->header_at(&reqhandlerPtr, "Cookie", index);
	if(cookie == NULL) break;
    reqhandlerPtr->cookiesqueue->content_type = CWPX_FORMURLENCODED;
    cwpx_send_r(&reqhandlerPtr, cookie, strlen(cookie), NULL, REQCOOKIE);
    index++;
  }
  
  char *reqURI = (char*)reqhandlerPtr->env(&reqhandlerPtr, "REQUEST_URI");
  if(reqURI != NULL){
    reqhandlerPtr->getqueue->content_type = CWPX_FORMURLENCODED;
    char *queryString = strstr(reqURI, "?");
    queryString = queryString != NULL ? queryString + 1 : ""; /*skip the '?' */  
    cwpx_send_r(&reqhandlerPtr, 
	  queryString, strlen(queryString), NULL, URLENCODED_QS);
  }
  
  char *reqMethod = 
    (char*)reqhandlerPtr->env(&reqhandlerPtr, "REQUEST_METHOD");
    
  int readBody= reqMethod != NULL && strcmp(reqMethod, "post") == 0;
  readBody = readBody ? readBody : 
    reqMethod != NULL && strcmp(reqMethod, "POST") == 0;
  readBody = readBody ? readBody : 
    reqMethod != NULL && strcmp(reqMethod, "put") == 0;
  readBody = readBody ? readBody : 
    reqMethod != NULL && strcmp(reqMethod, "PUT") == 0;
  
  if(!readBody){
    /* return method not allowed response ... */
    return 0;
  }
  
  
  char *contentLengthPtr = 
    (char*)reqhandlerPtr->env(&reqhandlerPtr, "CONTENT_LENGTH");
  contentLengthPtr = contentLengthPtr != NULL ? contentLengthPtr : 
    (char*)reqhandlerPtr->env(&reqhandlerPtr, "HTTP_CONTENT_LENGTH");
  unsigned long testContentLength = 0;
  if(contentLengthPtr != NULL){ /* validate max post */
    char *ptr;
    testContentLength = strtoul(contentLengthPtr, &ptr, 10);
    if(testContentLength > cwpx_context->confighandler->max_post){
	  /* return entty too large error ... */
	  return 0;
	}
  }
  
  char *contentType = 
    (char*)reqhandlerPtr->env(&reqhandlerPtr, "CONTENT_TYPE");
  contentType = contentType != NULL ? contentType : 
    (char*)reqhandlerPtr->env(&reqhandlerPtr, "HTTP_CONTENT_TYPE");
  contentType=contentType != NULL ? contentType : "application/octet-stream";
  
  reqhandlerPtr->postqueue->content_type = CWPX_RAWBODY;
  char *boundary = NULL;
  char boundaryBuffer[CWPX_MIN_BUFLEN];
  strcpy(boundaryBuffer, "");
  if(strstr(contentType, "application/x-www-form-urlencoded") != NULL){
    reqhandlerPtr->postqueue->content_type = CWPX_FORMURLENCODED;
  }
  else if(strstr(contentType, "multipart/form-data") != NULL){
    reqhandlerPtr->postqueue->content_type = CWPX_MULTIPARTFORMDATA;
    boundary = strstr(contentType, "boundary=");
    if(boundary == NULL){
	  /* return request not understood ... */
	  return 0;
	}
	boundary += strlen("boundary=");
	char *boundaryEnd = strstr(boundary, ";");
	boundaryEnd= boundaryEnd!= NULL? boundaryEnd : strstr(boundary, " ");
	boundaryEnd= boundaryEnd!= NULL? boundaryEnd : boundary + strlen(boundary);
	unsigned int boundaryLen = (unsigned)(boundaryEnd - boundary);
	strncpy(boundaryBuffer, boundary, boundaryLen);
	boundaryBuffer[boundaryLen] = 0;
  }
  
  enum cwpx_request_type postType = RAW;
  if(reqhandlerPtr->postqueue->content_type != CWPX_MULTIPARTFORMDATA){
  	boundary = NULL;
  	if(reqhandlerPtr->postqueue->content_type == CWPX_FORMURLENCODED)
  	  postType = URLENCODED;
  }
  else{
  	boundary = boundaryBuffer;
    postType = MULTIPART;
  }  
  
  /* request post: read the whole stdin */
  char buffer[BUFSIZ];
  size_t bytesRead = 0;
  char *stdinPtr = NULL;
  unsigned long stdinLen = 0;
  
  /* in Windows, the following commented instruction '(bytesRead = (unsigned)
  read(0, buffer, BUFSIZ)...)', worked in textmode (STDIN default mode) when, 
  from parent process 'popen' mode was only 'w' as in popen(script, "w"),
  without "wb" (and using fwrite). However, being binarymode what we need to 
  read, in Windows we must reopen STDIN for binary mode using the instructions
  'freopen(NULL, "rb", stdin); setmode(0, O_BINARY);', and the reading part is 
  made by 'fread(buffer, 1, BUFSIZ, stdin)', and from parent process we must 
  specify "wb" at open */

  /* change stdin mode 'cause it defaults to text mode */
  #ifdef _WIN32
  freopen(NULL, "rb", stdin);
  setmode(0, O_BINARY);
  #endif

  /*while ((bytesRead = (unsigned)read(0, buffer, BUFSIZ)) > 0) {*/
  while((bytesRead = fread(buffer, 1, BUFSIZ, stdin)) > 0){
	
	/* validate max post in case there isn't a content_length */
    if(contentLengthPtr == NULL && 
	  (stdinLen + bytesRead) > cwpx_context->confighandler->max_post){
	  /* return entty too large error ... */
      if(stdinPtr != NULL){
        free(stdinPtr);
        stdinPtr = NULL;
      }
	  return 0;
	}
	
    char *tryMalloc = stdinPtr == NULL ?
      malloc(bytesRead + 1) : realloc(stdinPtr, stdinLen + bytesRead + 1);
    if(!tryMalloc){
      /* free everything ... */
      if(stdinPtr != NULL){
        free(stdinPtr);
        stdinPtr = NULL;
      }
      exit(1);
    }
    stdinPtr = tryMalloc;
    memcpy(stdinPtr + stdinLen, buffer, bytesRead);
    /*stdinLen += bytesRead;*/ /* placed below */

    cwpx_send_r(&reqhandlerPtr,
	  stdinPtr+stdinLen, bytesRead, boundary, postType);
      
    stdinLen += bytesRead;

  }
  
  if(stdinPtr != NULL){
    free(stdinPtr);
    stdinPtr = NULL;
  }
  
  return 0;
}



void cwpx_destroy_confighandler(Cwpx_ConfigHandler **_this){
  if(*_this == NULL) return;
  
  Cwpx_ConfigHandler *_thisPtr = *_this;
  
  if(_thisPtr->workdir != NULL){
    free(_thisPtr->workdir);
    _thisPtr->workdir = NULL;
  }
  if(_thisPtr->rootdir != NULL){
    free(_thisPtr->rootdir);
    _thisPtr->rootdir = NULL;
  }
  if(_thisPtr->hostname != NULL){
    free(_thisPtr->hostname);
    _thisPtr->hostname = NULL;
  }
  if(_thisPtr->servip != NULL){
    free(_thisPtr->servip);
    _thisPtr->servip = NULL;
  }
  if(_thisPtr->tempdir != NULL){
    free(_thisPtr->tempdir);
    _thisPtr->tempdir = NULL;
  }
  if(_thisPtr->sessdir != NULL){
    free(_thisPtr->sessdir);
    _thisPtr->sessdir = NULL;
  }
  if(_thisPtr->sessid != NULL){
    free(_thisPtr->sessid);
    _thisPtr->sessid = NULL;
  }
  if(_thisPtr->currentdir != NULL){
    free(_thisPtr->currentdir);
    _thisPtr->currentdir = NULL;
  }
  if(_thisPtr->logfile != NULL){
    free(_thisPtr->logfile);
    _thisPtr->logfile = NULL;
  }
  
  if(CWPX_WORKDIR != NULL){
    free(CWPX_WORKDIR);
    CWPX_WORKDIR = NULL;
  }
  if(CWPX_ROOTDIR != NULL){
    free(CWPX_ROOTDIR);
    CWPX_ROOTDIR = NULL;
  }
  if(CWPX_HOSTNAME != NULL){
    free(CWPX_HOSTNAME);
    CWPX_HOSTNAME = NULL;
  }
  if(CWPX_SERVIP != NULL){
    free(CWPX_SERVIP);
    CWPX_SERVIP = NULL;
  }
  if(CWPX_TEMPDIR != NULL){
    free(CWPX_TEMPDIR);
    CWPX_TEMPDIR = NULL;
  }
  if(CWPX_SESSDIR != NULL){
    free(CWPX_SESSDIR);
    CWPX_SESSDIR = NULL;
  }
  if(CWPX_SESSID != NULL){
    free(CWPX_SESSID);
    CWPX_SESSID = NULL;
  }
  if(CWPX_SESSID != NULL){
    free(CWPX_SESSID);
    CWPX_SESSID = NULL;
  }
  if(CWPX_LOGFILE != NULL){
    free(CWPX_LOGFILE);
    CWPX_LOGFILE = NULL;
  }  
  
  free(*_this);
  *_this = NULL;
  
}


void cwpx_signal_handler(int signum) {
  /*SIGABRT(Signal Abort):
		Abnormal termination, such as is initiated by the function
    SIGFPE(Signal Floating-Point Exception):
		Erroneous arithmetic operation, such as zero divide or an operation 
		resulting in overflow (not necessarily with a floating-point operation)
    SIGILL(Signal Illegal Instruction):
		Invalid function image, such as an illegal instruction. This is 
		generally due to a corruption in the code or to an attempt to execute 
		data
	SIGINT(Signal Interrupt):
		Interactive attention signal. Generally generated by the application 
		user
	SIGSEGV(Signal Segmentation Violation):
		Invalid access to storage - When a program tries to read or write 
		outside the memory it is allocated for it
	SIGTERM(Signal Terminate):
		Termination request sent to program*/
	
  /*printf("cwpx_signal_handler: %d\n", signum);
  fflush(stderr);
  fflush(stdout);
  close(1);
  close(2);*/

  signum = signum; /* avoid unused warning */
  
  signal(SIGABRT, SIG_IGN);
  signal(SIGFPE, SIG_IGN);
  signal(SIGILL, SIG_IGN);
  signal(SIGINT, SIG_IGN);
  signal(SIGSEGV, SIG_IGN);
  signal(SIGTERM, SIG_IGN);
  
  /* free everything */
	
  exit(500);
}
