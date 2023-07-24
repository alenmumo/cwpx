#ifndef _CWPX_CONFIG_H_
#define _CWPX_CONFIG_H_

extern char *CWPX_WORKDIR;
extern char *CWPX_ROOTDIR;

extern char *CWPX_HOSTNAME;
extern char *CWPX_SERVIP;
extern unsigned int CWPX_SERVPT;
extern char *CWPX_TEMPDIR;
extern char *CWPX_SESSDIR;
extern char *CWPX_SESSID;
extern unsigned long CWPX_SESSDUR;
extern char *CWPX_LOGFILE;

extern unsigned long CWPX_BUF_SIZE;
extern unsigned int CWPX_MAX_QUEUEELEMS;
extern unsigned short CWPX_ALLOW_POST;
extern unsigned long CWPX_MAX_POST;

#ifdef _WIN32
  #include <windows.h>
  #define CWPXCFG __declspec(dllexport)
#else
  #include <dlfcn.h>
  #define CWPXCFG /**/
#endif

#endif
