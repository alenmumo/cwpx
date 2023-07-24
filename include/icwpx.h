#ifndef _ICWPX_H_
#define _ICWPX_H_

#include <string.h>

#ifdef _WIN32
  #define ICWPX_DLL __declspec(dllexport)
#else
  #define ICWPX_DLL 
#endif

#include <cwpx_context.h>

Request request;
Response response;
Config icwpx_config;
void icwpx_do_http();

#define _maproute(X,Y) \
  if(icwpx_maproute(X, (long)Y , request, response, config) != -1) \
    {icwpx_freethings(); return;} icwpx_freethings();


#define _http() do_http(Request req, Response resp){\
  request = req; response = resp; icwpx_do_http(); return; } \
    void icwpx_do_http()

#define _content response.print
#define _bcontent response.printb
#define _method (const char*)request.env("REQUEST_METHOD")

#define _p(X) icwpx_getparam(X, atParams)
#define _(at, vartype, varname) {vartype varname; (strstr(#vartype,"*"))?\
  icwpx_addat(at, #vartype , #varname , (void*)varname):\
  icwpx_addat(at, #vartype , #varname , &varname);}

#define _get icwpx_addat("method", "char *" , "get" , (void*)"get");
#define _post icwpx_addat("method", "char *" , "post" , (void*)"post");
#define _put icwpx_addat("method", "char *" , "put" , (void*)"put");
#define _delete icwpx_addat("method", "char *" , "delete" , (void*)"delete");

#define HEADER_MACRO(_1, _2, NAME,...) NAME
#define _header(...) \
  HEADER_MACRO(__VA_ARGS__, response.header, request.header)(__VA_ARGS__)

char icwpx_status[30];
#define _status(X, Y) sprintf(icwpx_status, "%d %s", X, Y); \
  response.header("Status", icwpx_status);

#define _view(X) icwpx_getview(X, request, response)


#define CAST_ARG_1(...) PRIVATE_CAST_ARG_1(0, ## __VA_ARGS__, 3, 2, 1, 0)
#define PRIVATE_CAST_ARG_1(_0, _1_, _2_, _3_, count3, ...) _1_
#define CAST_ARG_2(...) PRIVATE_CAST_ARG_2(0, ## __VA_ARGS__, 3, 2, 1, 0)
#define PRIVATE_CAST_ARG_2(_0, _1_, _2_, _3_, count3, ...) _2_
#define CAST_ARG_3(...) PRIVATE_CAST_ARG_3(0, ## __VA_ARGS__, 3, 2, 1, 0)
#define PRIVATE_CAST_ARG_3(_0, _1_, _2_, _3_, count3, ...) _3_
#define CAST_ARG_2STR(...) PRIVATE_CAST_ARG_2STR(0, ## __VA_ARGS__, 3, 2, 1, 0)
#define PRIVATE_CAST_ARG_2STR(_0, _1_, _2_, _3_, count3, ...) #_2_
#define CAST_ARG_3STR(...) PRIVATE_CAST_ARG_3STR(0, ## __VA_ARGS__, 3, 2, 1, 0)
#define PRIVATE_CAST_ARG_3STR(_0, _1_, _2_, _3_, count3, ...) #_3_
#define GET_ARG_COUNT(...) \
  INTERNAL_GET_ARG_COUNT_PRIVATE(0, ## __VA_ARGS__, 3, 2, 1, 0)
#define INTERNAL_GET_ARG_COUNT_PRIVATE(_0, _1_, _2_, _3_, count, ...) count
#define _context(...) (GET_ARG_COUNT(__VA_ARGS__) == 3) ? \
  (cwpx_args.set(CAST_ARG_1(__VA_ARGS__), CAST_ARG_2(__VA_ARGS__), \
  CAST_ARG_3(__VA_ARGS__), CAST_ARG_3STR(__VA_ARGS__))) : \
  (cwpx_args.get(CAST_ARG_1(__VA_ARGS__), CAST_ARG_2(__VA_ARGS__), \
  CAST_ARG_2STR(__VA_ARGS__)))

#define _setvar(...) set_context(__VA_ARGS__)
#define _getvar(...) get_context(__VA_ARGS__)

#define COOKIE_MACRO(_1, _2, _3, _4, _5, _6, _7, _8, _9, NAME,...) NAME
#define _cookies(...) \
  COOKIE_MACRO(__VA_ARGS__, response.cookies, request.cookies, \
  request.cookies, request.cookies, request.cookies, request.cookies, \
  request.cookies, request.cookies, request.cookies)(__VA_ARGS__)

#define _session(...) (GET_ARG_COUNT(__VA_ARGS__) == 3) ? \
  (session.set(CAST_ARG_1(__VA_ARGS__), CAST_ARG_2(__VA_ARGS__), \
  CAST_ARG_3(__VA_ARGS__), CAST_ARG_3STR(__VA_ARGS__))) : \
  (session.get(CAST_ARG_1(__VA_ARGS__), CAST_ARG_2(__VA_ARGS__), \
  CAST_ARG_2STR(__VA_ARGS__)))

#define _setsess(...) set_session(__VA_ARGS__)
#define _getsess(...) get_session(__VA_ARGS__)

#define _redirect(X) response.header("Status", "302 Found"); \
  response.header("Location", X); return;
#define _forward(X) request.forward(X); return;
#define _include(X) response.include(X);

#define CAST_ARG_DIR(...) PRIVATE_CAST_ARG_DIR(0, ## __VA_ARGS__, 1, 0)
#define PRIVATE_CAST_ARG_DIR(_0, _1_, count3, ...) (char*)_1_
#define CAST_ARG_DEFAULTS(...) \
  PRIVATE_CAST_ARG_DEFAULTS(0, ## __VA_ARGS__, 2, 1, 0)
#define PRIVATE_CAST_ARG_DEFAULTS(_0, _1_, _2_, count3, ...) (char*)_2_
#define CAST_ARG_LISTDIR(...) \
  PRIVATE_CAST_ARG_LISTDIR(0, ## __VA_ARGS__, 3, 2, 1, 0)
#define PRIVATE_CAST_ARG_LISTDIR(_0, _1_, _2_, _3_, count3, ...) (int)_3_

#define _static(...) \
  if(GET_ARG_COUNT(__VA_ARGS__) == 0){\
    if(icwpx_mapstatic("", "", 0, request, response, config) != -1){ \
	  icwpx_freethings(); return;\
	}\
  }\
  else if(GET_ARG_COUNT(__VA_ARGS__) == 1){\
    if(icwpx_mapstatic(CAST_ARG_DIR(__VA_ARGS__), "", 0, request, response, \
	  config) != -1){\
        icwpx_freethings(); return;\
    }\
  }\
  else if(GET_ARG_COUNT(__VA_ARGS__) == 2){\
    if(icwpx_mapstatic(CAST_ARG_DIR(__VA_ARGS__), \
	  CAST_ARG_DEFAULTS(__VA_ARGS__), 0, request, response, config) != -1){ \
	    icwpx_freethings(); return;\
    }\
  }\
  else if(GET_ARG_COUNT(__VA_ARGS__) == 3){\
    if(icwpx_mapstatic(CAST_ARG_DIR(__VA_ARGS__), \
	  CAST_ARG_DEFAULTS(__VA_ARGS__), CAST_ARG_LISTDIR(__VA_ARGS__), \
	    request, response, config) != -1){ \
	    icwpx_freethings(); return; \
    }\
  }\
  icwpx_freethings();

/*#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
#pragma GCC diagnostic ignored "-Wpointer-to-int-cast"*/

#define ICWPX_MAXPARAMNAMELEN 30
#define ICWPX_MAXPARAMSINFUNCTION 15

typedef struct iCwpx_AnnotationQueue{
	char at[ICWPX_MAXPARAMNAMELEN];
	char name[ICWPX_MAXPARAMNAMELEN];
	char type[ICWPX_MAXPARAMNAMELEN];
	int index;
	void* variable;
	struct iCwpx_AnnotationQueue *next;
}Annotation;

typedef struct FormFile{
	 char *name;
	 char *filename;
	 char *temp_filename;
	 char *content_type;
	 char *content_encoding;
	 unsigned long length;
}FormFile;


int ICWPX_DLL icwpx_maproute(const char *, long function, Request, Response, 
  Config);
int ICWPX_DLL icwpx_addat(const char *at, const char *vartype, 
  const char *varname, void *value);
int ICWPX_DLL icwpx_getview(const char *, Request, Response);
int ICWPX_DLL icwpx_mapstatic(const char *directory, const char *defaults, 
  int listdir, Request, Response, Config);
void ICWPX_DLL icwpx_initglobals(Request _request, Response _response);
void ICWPX_DLL icwpx_freethings();

struct Cwpx_ConfigHandler *dmych;

const char *icwpx_mimetypes[] = {  
  "text/plain: [.txt]", 
  "text/html: [.xhtml .html .htm]", 
  "image/jpeg: [.jpeg .jpg]",
  "image/png: [.png]",
  "audio/mpeg: [.mp3]",
  "video/mp4: [.mp4]",
  "application/json: [.json]",
  "application/xml: [.xml]",
  "text/javascript: [.js]",
  "text/css: [.css]"
};

#endif
