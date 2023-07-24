#ifndef _CWPX_H_
#define _CWPX_H_

#include <cwpx_context.h>

#define INIT_CWPX_CONTEXT \
  X(main) 
#define X(NAME) \
  int main(int argc,char *argv[]){ cwpx_main(argc, argv, &do_http); return 0; }

void do_http(Request request, Response response);

#ifndef B_CWPX_CONTEXT_DLL
extern int cwpx_main(int argc, char *argv[], void *func_ptr);
extern Cwpx_Args cwpx_args;
extern Config config;
extern Session session;
extern int cwpx_log(const char *format, ...);
#endif

INIT_CWPX_CONTEXT

#endif
