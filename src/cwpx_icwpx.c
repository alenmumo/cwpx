/*
 * gcc -Wall -Wextra -Wconversion -c src/cwpx_icwpx.c -I"include" -o lib/cwpx_i
 * cwpx.o
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <dirent.h>
#include <sys/stat.h>

#include <icwpx.h>
#include <cwpx_context.h>
#include <cwpx_misc.h>

typedef struct iCwpx_RouteQueue{
  const char *pattern;
  long function; /* the address of function in hex */
  struct iCwpx_RouteQueue *next;
}Route;

typedef struct iCwpx_PathQueue{
  int index;
  char paramName[ICWPX_MAXPARAMNAMELEN];
  char *value; /* malloced */
  struct iCwpx_PathQueue *next;
}Path;

Route *routeshead = NULL;
Annotation *annotationshead = NULL;
Path *pathshead = NULL;

int icwpx_compareandgetpaths(const char *pattern, 
  Request request, Response response, Config config);
int icwpx_pushpath(int, const char *, char *);
void *icwpx_getparam(int index, Annotation *atP[]);
int icwpx_convertstringtotype(const char *type, const char *value, 
  void **variable);
int icwpx_pushannotation(const char *, const char *, const char *, int,void *);
int icwpx_getfilecontent(const char *uri, char **buffer, unsigned long size,
  char **mime);

int icwpx_maproute(const char *pattern, long function, Request _request, 
  Response _response, Config config){
		
  { extern Request request; request = _request; }
  { extern Response response; response = _response; }
  icwpx_config = config;

  int compareandgetpaths = 
    icwpx_compareandgetpaths(pattern, _request, response, config);
  if(compareandgetpaths == -1) return -1;
	
  Path *aux = pathshead;
  while(aux != NULL){
    /*printf("icwpx_maproute1.Path: index(%d), paramName(%s), value(%s)<br>", 
    aux->index, aux->paramName, aux->value);*/
    aux = aux->next;
  }
  Annotation *at = annotationshead;
	
  int paramCount = 0;
  /* MAX 15 params, more than that is a bad idea, 
  doing "char **paramNames" is also a bad idea */
  char *paramNames[ICWPX_MAXPARAMSINFUNCTION]; 
  /*memset(paramNames, NULL, 15);*/
  void *paramValues[ICWPX_MAXPARAMSINFUNCTION];
	
  Annotation *atParams[ICWPX_MAXPARAMSINFUNCTION];
	
  int methodCount = 0;
  char *methodNames[4]; /* only get, post, put, delete */

	
  while(at != NULL){
		
    if(strstr(at->at, "@path") || strstr(at->at, "@header") || 
	  strstr(at->at, "@query") || strstr(at->at, "@body") || 
	  strstr(at->at, "@raw") || strstr(at->at, "@files") || 
	  strstr(at->at, "@cookies")){
			
	  paramNames[paramCount] = at->name;
	  paramValues[paramCount] = at->variable;
	  atParams[paramCount] = at;
	  paramCount++;
	}
	else if(strstr(at->at, "method")){
	  		
	  methodNames[methodCount] = at->name;
	  methodCount++;
	}
		
	at = at->next;
  }

	
  char *method = "get";
  if(_method != NULL){
    if(strcmp(_method, "POST") == 0) method = "post";
    else if(strcmp(_method, "PUT")==0) method = "put";
    else if(strcmp(_method, "DELETE")==0) method = "delete";
  }

  int methodFound = 0;

  char allowedMethods[20];/* = "get|post|put|delete";*/
  strcpy(allowedMethods, "");

  if(methodCount > 0){
    int i;
    for(i = 0; i < methodCount; i++){
      if(strcmp(method, methodNames[i]) == 0){
	    methodFound = 1;
	  }
			
	  if(i!=0)strcat(allowedMethods, "|");
	    strcat(allowedMethods, methodNames[i]);
    }
  }
  else{
    methodFound = 1;
  }


  if(!methodFound){
    return -1;
  }

	
  if(paramCount == 1) ((void(*)())function)(_p(0));
  else if(paramCount == 2) ((void(*)())function)(_p(0),_p(1));
  else if(paramCount == 3) ((void(*)())function)(_p(0),_p(1),_p(2));
  else if(paramCount == 4) ((void(*)())function)(_p(0),_p(1),_p(2),_p(3));
  else if(paramCount == 5) ((void(*)())function)(_p(0),_p(1),_p(2),_p(3),
  _p(4));
  else if(paramCount == 6) ((void(*)())function)(_p(0),_p(1),_p(2),_p(3),
  _p(4),_p(5));
  else if(paramCount == 7) ((void(*)())function)(_p(0),_p(1),_p(2),_p(3),
  _p(4),_p(5),_p(6));
  else if(paramCount == 8) ((void(*)())function)(_p(0),_p(1),_p(2),_p(3),
  _p(4),_p(5),_p(6),_p(7));
  else if(paramCount == 9) ((void(*)())function)(_p(0),_p(1),_p(2),_p(3),
  _p(4),_p(5),_p(6),_p(7),_p(8));
  else if(paramCount == 10) ((void(*)())function)(_p(0),_p(1),_p(2),_p(3),
  _p(4),_p(5),_p(6),_p(7),_p(8),_p(9));
  else if(paramCount == 11) ((void(*)())function)(_p(0),_p(1),_p(2),_p(3),
  _p(4),_p(5),_p(6),_p(7),_p(8),_p(9),_p(10));
  else if(paramCount == 12) ((void(*)())function)(_p(0),_p(1),_p(2),_p(3),
  _p(4),_p(5),_p(6),_p(7),_p(8),_p(9),_p(10),_p(11));
  else if(paramCount == 13) ((void(*)())function)(_p(0),_p(1),_p(2),_p(3),
  _p(4),_p(5),_p(6),_p(7),_p(8),_p(9),_p(10),_p(11),_p(12));
  else if(paramCount == 14) ((void(*)())function)(_p(0),_p(1),_p(2),_p(3),
  _p(4),_p(5),_p(6),_p(7),_p(8),_p(9),_p(10),_p(11),_p(12),_p(13));
  else if(paramCount == 15) ((void(*)())function)(_p(0),_p(1),_p(2),_p(3),
  _p(4),_p(5),_p(6),_p(7),_p(8),_p(9),_p(10),_p(11),_p(12),_p(13),_p(14));
  else ((void(*)())function)();
	
	

  return 0;
}

int icwpx_compareandgetpaths(const char *pattern, 
  Request request, Response response, Config config){

  if(pattern == NULL) return -1;
  if(strlen(pattern) == 0) return -1;
  if(pattern[0] != '/') return -1;


  char *requestURI = (char *)request.env("REQUEST_URI");
  char *documentRoot = (char *)request.env("DOCUMENT_ROOT");
  
  char *scriptFilename = (char *)request.env("SCRIPT_FILENAME");
  char *scriptName = (char *)request.env("SCRIPT_NAME");
  
  /* the 'appPath' (path to the current executing cwpx script) can be obtained
  either by SCRIPT_FILENAME + strlen(DOCUMENT_ROOT) or by SCRIPT_NAME, then 
  doing strrchr to remove the filename and leave only the path */
	
  char *findAppPath = scriptFilename + strlen(documentRoot);
  char *findAppPathEnd = strrchr(findAppPath, '/');
  unsigned int appPathLen = findAppPathEnd - findAppPath;
  char appPath[appPathLen + 1];
  strncpy(appPath, findAppPath, appPathLen);
  appPath[appPathLen] = 0;
  
  char *resourcePath = strlen(appPath) == 0 ? "/" : appPath;
	
  char *inURI = requestURI + strlen(resourcePath);
  
  if(strcmp(resourcePath, "/") != 0){ /* redirection is not root */
    inURI = (strlen(resourcePath) >0 && resourcePath[0]=='/') ? 
	  inURI + 1 : inURI; /* +1 to skip the leading / */
  }

  if(strcmp(request.env("REQUEST_URI"), "/")==0){
    inURI = "";
  }
	

  char *inURIEnd = inURI + strlen(inURI);
  char *inPattern = (strlen(pattern) >0 && pattern[0] == '/') ? 
    (char *)pattern + 1 : (char *)pattern; /* +1 to skip the leading / */
  char *inPatternEnd = inPattern + strlen(inPattern);
	
  int pathIndex = 0;

  while(inURI != NULL){
		
    char *nextURIToken = strstr(inURI, "/");
	
	short foundQuery = 0;
	if(nextURIToken == NULL){
	  nextURIToken = strstr(inURI, "?");
	  foundQuery = nextURIToken == NULL ? 0 : 1;
	}
	
	short foundHashTag = 0;
	if(nextURIToken == NULL){
	  nextURIToken = strstr(inURI, "#");
	  foundHashTag = nextURIToken == NULL ? 0 : 1;
	}
	nextURIToken = (nextURIToken==NULL)?inURI+strlen(inURI):nextURIToken;
		
	int uriPathLenght = nextURIToken - inURI;
	char uriPath[uriPathLenght + 1];
	strncpy(uriPath, inURI, uriPathLenght);
	uriPath[uriPathLenght] = 0;
		
	char *nextPatternToken = strstr(inPattern, "/");
	short isFinalPatternToken = nextPatternToken == NULL ? 1 : 0;
	nextPatternToken = (nextPatternToken==NULL)?
	  strstr(inPattern, "?"):nextPatternToken;
	nextPatternToken = (nextPatternToken==NULL)?
	  strstr(inPattern, "#"):nextPatternToken;
	nextPatternToken = (nextPatternToken==NULL)?
	  inPattern+strlen(inPattern):nextPatternToken;
	
	int patternPathLenght = nextPatternToken - inPattern;
	char patternPath[patternPathLenght + 1];
	strncpy(patternPath, inPattern, patternPathLenght);
	patternPath[patternPathLenght] = 0;
	
	
	if(strcmp(uriPath, patternPath) != 0/* && strcmp(patternPath, "") != 0*/){
		
	  if(patternPath[0] == '{' && patternPath[patternPathLenght-1] == '}'){ 
        /* it's a path param */
	    char paramName[strlen(patternPath) - 1]; 
		/* it was -2 for substracting { and } but +1 for extra null space */
		strncpy(paramName, patternPath+1, strlen(patternPath)-2);
		paramName[strlen(patternPath)-2] = 0;
		
		icwpx_pushpath(pathIndex++, paramName, uriPath);
	  }
	  else if(strcmp(patternPath, "*") == 0){ 
        /* /example/*   all the rest of URLs fit ... */
	    char paramName[strlen("_ast_in_uri") + 1];
		strcpy(paramName, "_ast_in_uri");
		
		icwpx_pushpath(pathIndex++, paramName, inURI);
				
		break;
	  }
	  else{
		return -1;
	  }
	}
	else{
	  icwpx_pushpath(pathIndex++, "", uriPath);
	}
		
	
	if(nextURIToken >= inURIEnd){ /* if URI ends */
	  
	  if(nextPatternToken < inPatternEnd){ /* but pattern continues */
	  	return -1;
	  }
	  break;
	}
		
	inURI = nextURIToken + 1;
	inPattern = !isFinalPatternToken ? nextPatternToken + 1 : "";
		
	if(foundQuery || foundHashTag){
      break;
    }
  }/* while */
	
  
  return 0;
}


int icwpx_pushpath(int index, const char *paramName, char *value){
	
  Path *newPath = malloc(sizeof(Path));
  if(newPath == NULL) return -1;
    newPath->index = index;
	strncpy(newPath->paramName, paramName, 29);
	newPath->value = NULL;
	newPath->next = NULL;
	
	newPath->value = malloc(strlen(value) + 1);
	if(newPath->value == NULL){
		free(newPath);
		return -1;
	}
	strncpy(newPath->value, value, strlen(value));
	newPath->value[strlen(value)] = 0;
	
	
	int cont = 1;
	if(pathshead == NULL){
		pathshead = newPath;
	}
	else{
		Path *aux = pathshead;
		while(aux->next != NULL){
			aux = aux->next;
			cont++;
		}
		aux->next = newPath;
	}
	
	return cont;
}

void *icwpx_getparam(int index, Annotation *atP[]){
	
  if(strstr(atP[index]->at, "@path")){
		
    Path *aux = pathshead;
    while(aux!=NULL){
			
      if(strcmp(atP[index]->name, aux->paramName) == 0){
				
        icwpx_convertstringtotype(atP[index]->type, aux->value, 
		  &atP[index]->variable); /* look the reference & */
				
        /*in case of a 'string' in path it comes url-encoded */
        if(strstr(atP[index]->type, "char") != NULL &&
          strstr(atP[index]->type, "*") != NULL){
          
          cwpx_decode_x_www_form_urlencoded(
		            atP[index]->variable, atP[index]->variable);
        }
				
        break;
      }
      aux = aux->next;
    }
  }
  else if(strstr(atP[index]->at, "@header")){
		
    char hyphenedHeader[strlen(atP[index]->name) + 1];
    strcpy(hyphenedHeader, atP[index]->name);
    int cont;
    int hyphened = 0;
    int i;
    for(i = 0; hyphenedHeader[i]; i++){
      if(i == 0) 
	    hyphenedHeader[i] = toupper(hyphenedHeader[i]);/*uppercase first char*/
      if(hyphenedHeader[i] == '_') {
	    hyphenedHeader[i] = '-'; hyphened = 1;
		/* middle hyphen the low hyphen char */
	  }
      if(hyphened && hyphenedHeader[i-1] == '-'){
	    hyphenedHeader[i] = toupper(hyphenedHeader[i]);
		/* uppercase char after - */
	  }
    }
		
    const char *header_param = request.header(hyphenedHeader);
		
    icwpx_convertstringtotype(atP[index]->type, header_param, 
	  &atP[index]->variable);/* look the reference & */
  }
  else if(strstr(atP[index]->at, "@query")){
		
    const char *query_param = request.get(atP[index]->name);
		
    icwpx_convertstringtotype(atP[index]->type, query_param, 
	  &atP[index]->variable);/* look the reference & */
  }
  else if(strstr(atP[index]->at, "@body")){
		
    const char *post_param = request.post(atP[index]->name);
		
    icwpx_convertstringtotype(atP[index]->type, post_param, 
	  &atP[index]->variable);/* look the reference & */
  }
  else if(strstr(atP[index]->at, "@raw")){
    	
    const char *rawBody_param = request.raw(NULL);
    if(rawBody_param != NULL)
      icwpx_convertstringtotype(atP[index]->type, rawBody_param, 
	    &atP[index]->variable);/* look the reference & */
  }
  else if(strstr(atP[index]->at, "@files")){
		
    if(request.isset_post(atP[index]->name)){
			
      FormFile *ff = malloc(sizeof(FormFile));
			
      if(ff != NULL){
	
        ff->name = (char *)request.files(atP[index]->name, "name");
        ff->filename = (char *)request.files(atP[index]->name, "filename");
        ff->temp_filename = 
		  (char *)request.files(atP[index]->name, "temp_filename");
        ff->content_type = 
		  (char *)request.files(atP[index]->name, "content_type");
        ff->content_encoding = 
		  (char *)request.files(atP[index]->name, "content_encoding");
        char *contentLenStr = 
		  (char *)request.files(atP[index]->name, "content_length");
        unsigned long ret; char *strPtr;
        ret = strtoul(contentLenStr, &strPtr, 10);
        ff->length = ret;
      }
			
      atP[index]->variable = ff;
    }
  }
  else if(strstr(atP[index]->at, "@cookies")){
		
    const char *cookie_param = request.cookies(atP[index]->name);
		
    icwpx_convertstringtotype(atP[index]->type, cookie_param, 
	  &atP[index]->variable);/* look the reference & */
  }
	
  return atP[index]->variable;
}



int icwpx_convertstringtotype(const char *type, const char *value, 
  void **variable){

  if(strstr(type, "char") && strstr(type, "*")){
    *variable = (char *)value;
  }
  else{
    /* https://www.techonthenet.com/c_language/standard_library_functions/stdli
	b_h/strtol.php
    https://stackoverflow.com/questions/27260304/equivalent-of-atoi-for-unsigne
	d-integers */
    char *eptr;
		
    if(strcmp(type, "char") == 0) { *variable = (char)value[0]; }
    else if(strcmp(type, "signed char") == 0) { 
	  *variable = (signed char)value[0]; }
    else if(strcmp(type, "unsigned char") == 0) { 
	  *variable = (unsigned char)value[0]; }
		
    else if(strcmp(type, "short") == 0) { *variable = (short)atoi(value);}
    else if(strcmp(type, "short *") == 0 || strcmp(type, "short*") == 0) { 
	  *variable = (short)atoi(value); }
    else if(strcmp(type, "short int") == 0) { 
	  *variable = (short int)atoi(value); }
    else if(strcmp(type, "short int *") == 0 || strcmp(type, "short int*")== 0) 
	  { *variable = (short int)atoi(value); }
    else if(strcmp(type, "signed short") == 0) { 
	  *variable = (signed short)atoi(value); }
    else if(strcmp(type, "signed short *") == 0 || 
	  strcmp(type, "signed short*") == 0) { 
	  *variable = (signed short)atoi(value); }
    else if(strcmp(type, "signed short int") == 0) { 
	  *variable = (signed short int)atoi(value); }
    else if(strcmp(type, "signed short int *") == 0 || 
	  strcmp(type, "signed short int*") == 0) { 
	  *variable = (signed short int)atoi(value); }
    else if(strcmp(type, "unsigned short") == 0) { 
	  *variable = (unsigned short)strtoul(value, &eptr, 10); }
    else if(strcmp(type, "unsigned short *") == 0 || 
	  strcmp(type, "unsigned short*") == 0) { 
	  *variable = (unsigned short)strtoul(value, &eptr, 10); }
    else if(strcmp(type, "unsigned short int") == 0) { 
	  *variable = (unsigned short int)strtoul(value, &eptr, 10); }
    else if(strcmp(type, "unsigned short int *") == 0 || 
	  strcmp(type, "unsigned short int*") == 0) { 
	  *variable = (unsigned short int)strtoul(value, &eptr, 10); }
		
    /* because of some unknown reason (macro?), the pointers can't be evaluated 
	in the same if as the non-poniter, regardless they're wil do the same */
    else if(strcmp(type, "int") == 0) { *variable = (int)atoi(value); }
    else if(strcmp(type, "int *") == 0 || strcmp(type, "int*") == 0) { 
	  *variable = (int)atoi(value); }
		
    else if(strcmp(type, "signed") == 0) { *variable = (signed)atoi(value); }
    else if(strcmp(type, "signed *") == 0 || strcmp(type, "signed*") == 0) { 
	  *variable = (signed)atoi(value); }
    else if(strcmp(type, "signed int") == 0) { 
	  *variable = (signed int)atoi(value); }
    else if(strcmp(type, "signed int *") == 0 || 
      strcmp(type, "signed int*") == 0) { *variable = (signed int)atoi(value);}
    else if(strcmp(type, "unsigned") == 0) { 
	  *variable = (unsigned)strtoul(value, &eptr, 10); }
    else if(strcmp(type, "unsigned *") == 0 || strcmp(type, "unsigned*") == 0) 
	  { *variable = (unsigned)strtoul(value, &eptr, 10); }
    else if(strcmp(type, "unsigned int") == 0) { 
	  *variable = (unsigned int)strtoul(value, &eptr, 10); }
    else if(strcmp(type, "unsigned int *") == 0 || 
	  strcmp(type, "unsigned int*") == 0) { 
	  *variable = (unsigned int)strtoul(value, &eptr, 10); }
		
    else if(strcmp(type, "long") == 0) { 
	  *variable = (long)strtol(value, &eptr, 10); }
    else if(strcmp(type, "long *") == 0 || strcmp(type, "long*") == 0) { 
	  *variable = (long)strtol(value, &eptr, 10); }
    else if(strcmp(type, "long int") == 0) { 
	  *variable = (long int)strtol(value, &eptr, 10); }
    else if(strcmp(type, "long int *") == 0 || strcmp(type, "long int*") == 0){ 
	  *variable = (long int)strtol(value, &eptr, 10); }
    else if(strcmp(type, "signed long") == 0) { 
	  *variable = (signed long)strtol(value, &eptr, 10); }
    else if(strcmp(type, "signed long *") == 0 || 
	  strcmp(type, "signed long*") == 0) { 
	  *variable = (signed long)strtol(value, &eptr, 10); }
    else if(strcmp(type, "signed long int") == 0) { 
	  *variable = (signed long int)strtol(value, &eptr, 10); }
    else if(strcmp(type, "signed long int *") == 0 || 
	  strcmp(type, "signed long int*") == 0) { 
	  *variable = (signed long int)strtol(value, &eptr, 10); }
    else if(strcmp(type, "unsigned long") == 0) { 
	  *variable = (unsigned long)strtoul(value, &eptr, 10); }
    else if(strcmp(type, "unsigned long *") == 0 || 
	  strcmp(type, "unsigned long*") == 0) { 
	  *variable = (unsigned long)strtoul(value, &eptr, 10); }
    else if(strcmp(type, "unsigned long int") == 0) { 
	  *variable = (unsigned long int)strtoul(value, &eptr, 10); }
    else if(strcmp(type, "unsigned long int *") == 0 || 
	  strcmp(type, "unsigned long int*") == 0) { 
	  *variable = (unsigned long int)strtoul(value, &eptr, 10); }
		
    else if(strcmp(type, "long long") == 0) { 
	  *variable = (long long)strtoll(value, &eptr, 10); }
    else if(strcmp(type, "long long *") == 0 ||strcmp(type, "long long*") == 0)
	  { *variable = (long long)strtoll(value, &eptr, 10); }
    else if(strcmp(type, "long long int") == 0) { 
	  *variable = (long long int)strtoll(value, &eptr, 10); }
    else if(strcmp(type, "long long int *") == 0 || 
	  strcmp(type, "long long int*") == 0) { 
	  *variable = (long long int)strtoll(value, &eptr, 10); }
    else if(strcmp(type, "signed long long") == 0) { 
	  *variable = (signed long long)strtoll(value, &eptr, 10); }
    else if(strcmp(type, "signed long long *") == 0 || 
	  strcmp(type, "signed long long*") == 0) { 
	  *variable = (signed long long)strtoll(value, &eptr, 10); }
    else if(strcmp(type, "signed long long int") == 0) { 
	  *variable = (signed long long int)strtoll(value, &eptr, 10); }
    else if(strcmp(type, "signed long long int *") == 0 || 
	  strcmp(type, "signed long long int*") == 0) { 
	  *variable = (signed long long int)strtoll(value, &eptr, 10); }
    else if(strcmp(type, "unsigned long long") == 0) { 
	  *variable = (unsigned long long)strtoull(value, &eptr, 10); }
    else if(strcmp(type, "unsigned long long *") == 0 || 
      strcmp(type, "unsigned long long*") == 0) { 
	  *variable = (unsigned long long)strtoull(value, &eptr, 10); }
    else if(strcmp(type, "unsigned long long int") == 0) { 
	  *variable = (unsigned long long int)strtoull(value, &eptr, 10); }
    else if(strcmp(type, "unsigned long long int *") == 0 || 
	  strcmp(type, "unsigned long long int*") == 0) { 
	  *variable = (unsigned long long int)strtoull(value, &eptr, 10); }
		
	else if(strcmp(type, "float *") == 0 || strcmp(type, "float*") == 0) { 
	  float f = atof(value); *variable = &f; }
	else if(strcmp(type, "double *") == 0 || strcmp(type, "double*") == 0) { 
	  double d = strtod(value, &eptr); *variable = &d; }
	else if(strcmp(type, "long double *") == 0 || 
	  strcmp(type, "long double*") == 0) { 
	  long double ld = strtold(value, &eptr); *variable = &ld; }
		
	else *variable = value;
  }
	
  return 0;
}

int icwpx_addat(const char *at, const char *vartype, const char *varname, 
  void *value){
	
  char *eptr;
  if(strstr(vartype, "float")!=NULL && strstr(vartype, "*")!=NULL){
    float f = atof("0"); value = &f;}
  else if(strstr(vartype, "double")!=NULL && strstr(vartype, "*")!=NULL){
    double d = strtod("0", &eptr); value = &d;}
  else if(strstr(vartype, "long double")!=NULL && strstr(vartype, "*")!=NULL){
    long double ld = strtold("0", &eptr); value = &ld;}
  else if(strstr(vartype, "formfile")!=NULL && strstr(vartype, "*")!=NULL){
    FormFile *ff = NULL; value = NULL;}
  else if(strstr(vartype, "*")!=NULL){value = NULL;} /* LOOK OUT THIS */
  else value = 0;
	
  icwpx_pushannotation(at, varname, vartype, -1, value);
	
  return 0;
}


int icwpx_pushannotation(const char *at, const char *name, const char *type, 
  int index, void *variable){
	
  Annotation *newAnnotation = malloc(sizeof(Annotation));
  if(newAnnotation == NULL) return -1;
  strncpy(newAnnotation->at, at, 14);
  strncpy(newAnnotation->name, name, 29);
  strncpy(newAnnotation->type, type, 29);
  newAnnotation->index = index;
  newAnnotation->variable = variable;
  newAnnotation->next = NULL;
	
  int cont = 1;
  if(annotationshead == NULL){
    annotationshead = newAnnotation;
  }
  else{
    Annotation *aux = annotationshead;
    while(aux->next != NULL){
      aux = aux->next;
      cont++;
    }
    aux->next = newAnnotation;
  }
	
  return cont;
}


int icwpx_getview(const char *uri, Request request, Response response){

  char *requestURI = (char *)request.env("REQUEST_URI");
  char *rootDir = (char *)request.env("DOCUMENT_ROOT");
  
  char *findAppPath = requestURI;
  char *findAppPathEnd = strrchr(findAppPath, '/');
  unsigned int appPathLen = findAppPathEnd - findAppPath;
  char appPath[appPathLen + 1];
  strncpy(appPath, findAppPath, appPathLen);
  appPath[appPathLen] = 0;
	
  char viewURI[strlen(uri) + strlen(".cwpx") + 1];
  sprintf(viewURI, "%s.cwp", uri);

  int success = response.include(viewURI);
  if(success == -404){/* try '.cwpx' */
    sprintf(viewURI, "%s.cwpx", uri);
    success = request.forward(viewURI);
  }
	
  return success;
}

int icwpx_mapstatic(const char *directory, const char *defaults, int listdir, 
  Request _request, Response _response, Config config){

  { extern Request request; request = _request; }
  { extern Response response; response = _response; }
  icwpx_config = config;
  
  char *scriptFilename = (char *)request.env("SCRIPT_FILENAME");
  if(scriptFilename == NULL) return -1;
	
  if(strstr(scriptFilename, ".") != NULL){
    char *lastSlash = strrchr(scriptFilename, '/');
    if(lastSlash != NULL){
      unsigned int cwdLen = lastSlash - scriptFilename;
      char cwd[PATH_MAX];
      strncpy(cwd, scriptFilename, cwdLen);
      cwd[cwdLen] = 0;
	    
      chdir(cwd);
    }
  }

  char *requestURI = (char *)request.env("REQUEST_URI");
  char *rootDir = (char *)request.env("DOCUMENT_ROOT");
  
  char *findAppPath = requestURI;
  char *findAppPathEnd = strrchr(findAppPath, '/');
  unsigned int appPathLen = findAppPathEnd - findAppPath;
  char appPath[appPathLen + 1];
  strncpy(appPath, findAppPath, appPathLen);
  appPath[appPathLen] = 0;
  	
  char *inURI = requestURI + strlen(appPath);
	
  int redirectionIsRoot = strcmp(requestURI, "/") == 0 ? 1 : 0;
	
  if(!redirectionIsRoot){
    /*inURI = (strlen(resourcePath) > 0 && resourcePath[0] == '/') ? 
    inURI + 1 : inURI; // +1 to skip the leading /*/
  }
  else
	inURI = "";
	
  int inPatternLength = 
    strlen(appPath) + 1 + strlen(directory) + 1 + strlen(inURI) + 1;

  char inPattern[inPatternLength + 1];
  
  if(strcmp(directory, "")!=0){
	if(appPath[strlen(appPath)-1] == '/') /* if appPath ends with / */
      sprintf(inPattern, "%s%s/%s", appPath, directory, inURI);
	else
      sprintf(inPattern, "%s/%s/%s", appPath, directory, inURI);
  }
  else{
	if(appPath[strlen(appPath)-1] == '/') /* if appPath ends with / */
      sprintf(inPattern, "%s%s", appPath, inURI);
	else
      sprintf(inPattern, "%s/%s", appPath, inURI);
  }
		
  inPattern[inPatternLength - 1] = 0;
  
  char *cleanPattern = strstr(inPattern, "//");
  while(cleanPattern != NULL){
    int from = cleanPattern - inPattern;
    int to = strlen(inPattern);
    cleanPattern = cleanPattern + 1; /* +1 to skip the leading / */
    int patternCount = 0;
    for(from=from; from < to; from++){
      inPattern[from] = cleanPattern[patternCount];
      patternCount++;
    }
    cleanPattern = strstr(inPattern, "//");
  }
		
  struct stat st;
	
  char *relativePath = inPattern + strlen(appPath);
	
  short isCurrent = strcmp(relativePath, "/") == 0 ? 1 : 0;
	
  if(relativePath[strlen(relativePath)-1] == '/') 
    relativePath[strlen(relativePath)-1] = 0;
  if(relativePath[0] == '/' && strlen(relativePath)>2) 
    relativePath = relativePath + 1;
  
  if(isCurrent) strcpy(relativePath, ".");
  
  
  int result = 0;
  if(stat(relativePath, &st) != 0){
    response.header("Status", "404 Not Found");
    return -1;
  }
  else if(st.st_mode & S_IFDIR) { result = 403; }
  else{ result = 200; }
	
  if(result == 403){
    
	int foundDefault = 0;
		
    if(strcmp(defaults, "") != 0){
      char *inDefaults = (char*)defaults;
      char *defaultsEnd = (char*)defaults + strlen(defaults);
			
      while(inDefaults < defaultsEnd){
        char *nextDefault = strchr(inDefaults, ',');
        if(nextDefault == NULL) nextDefault = defaultsEnd;
				
        char defaultFile[nextDefault - inDefaults + 1];
        strncpy(defaultFile, inDefaults, (nextDefault - inDefaults));
        defaultFile[nextDefault - inDefaults] = 0;
				
        char newRelativeAppPath[strlen(relativePath)+1+strlen(defaultFile)+1]; 
		/* +1 if we had to add an extra slash ... */
        strcpy(newRelativeAppPath, relativePath);
        if(strlen(relativePath) > 0 && 
		  relativePath[strlen(relativePath)-1] != '/') 
		  strcat(newRelativeAppPath, "/");
        strcat(newRelativeAppPath, defaultFile);
				
        struct stat stDefault;				
        if(stat(newRelativeAppPath, &stDefault) != 0){ }
        else if(stDefault.st_mode & S_IFDIR) { }
        else{
          char *newResourceUrl = newRelativeAppPath;
					
          int rewrite_result = 1;
					
          char *contentType = "application/octet-stream";
					
          response.header("Content-Type", contentType);
          rewrite_result = 1;
					
          char newInPattern[strlen(inPattern) + 1 + strlen(defaultFile) + 1];
          strcpy(newInPattern, inPattern);
          if(inPattern[strlen(inPattern) - 1]!='/')
            strcat(newInPattern, "/");
          strcat(newInPattern, defaultFile);
					
          char *buffer = malloc(stDefault.st_size);
          if(buffer == NULL){
	        /* free everything */
	        exit(500);
	      }
	      char mimeType[50];
	      char *mimePtr= (char *)mimeType;
	  
	      if(icwpx_getfilecontent(
		    newRelativeAppPath, &buffer, stDefault.st_size, &mimePtr) == 0) {

		    response.header("Content-Type", mimeType);
		
	        char contentLength[50]; 
			sprintf(contentLength, "%lu", stDefault.st_size);
		    response.header("Content-Length", contentLength);
			
	        response.printb(buffer, stDefault.st_size);
	      }
	  
          free(buffer);
					
          foundDefault = 1;
					
          return 0;
        }
				
        inDefaults = nextDefault + 1;
				
        while(inDefaults[0] == ' '){
          inDefaults = inDefaults + 1;
        }
				
      }
    }
		
    if(!foundDefault && listdir){
      int listing_output_length = 2;
      char *listing_output = malloc(listing_output_length + 1);
      if(listing_output == NULL) return -1;
      strcpy(listing_output, "{ ");

      char *listing_output_ptr = listing_output;
			
      DIR *directory_path; struct dirent *dp; struct stat directory_info;
			
      if(strlen(relativePath) == 0) relativePath = ".";
			
      if ((directory_path = opendir(relativePath)) != NULL){
        char listing_title[strlen("\"") + strlen(relativePath) + 
		  strlen("\": [ ") + 1];
        sprintf(listing_title, "\"%s\": [ ", relativePath);
        listing_output_ptr = 
		  realloc(listing_output, listing_output_length + 
		    strlen(listing_title) + 1);
				
        if(listing_output_ptr == NULL){ free(listing_output); return -1; }
          listing_output = listing_output_ptr;
				
          strcat(listing_output + listing_output_length, listing_title);
         listing_output_length = listing_output_length + strlen(listing_title);

          int elementCount = 0;
				
          do{
            if ((dp = readdir(directory_path)) != NULL) {
              if(strcmp(dp->d_name,".")!=0 && strcmp(dp->d_name,"..")){
							
                char abs_path[strlen(relativePath) + strlen("/") + 
				  strlen(dp->d_name) + 1];
                sprintf(abs_path, "%s/%s", relativePath, dp->d_name);
							
                stat(abs_path, &directory_info);
							
                char *thePattern;
                char *thePatternStart;
                char *thePatternEnd;
							
                if (directory_info.st_mode & S_IFDIR){
                  thePattern = "{\"folder\": \"\"}";
                  thePatternStart = "{\"folder\": \"";
                  thePatternEnd = "\"}";
                }
                else{
                  thePattern = "{\"file\": \"\"}";
                  thePatternStart = "{\"file\": \"";
                  thePatternEnd = "\"}";
                }
							
                int thePatternLength = (elementCount == 0) ?
				  strlen(thePattern):strlen(thePattern)+2;/* 2 for  [, ] */
							
                listing_output_ptr = 
				  realloc(listing_output, listing_output_length + 
				  thePatternLength + strlen(dp->d_name) + 1);
                if(listing_output_ptr == NULL){ 
				  free(listing_output); return -1; }
                listing_output = listing_output_ptr;
							
                if(elementCount != 0) { 
                  strcat(listing_output + listing_output_length, ", ");
                  listing_output_length = listing_output_length + 2; 
				  /* strlen(", "); */
                }
							
                strcat(listing_output + listing_output_length,thePatternStart);
                listing_output_length = 
				  listing_output_length + strlen(thePatternStart);
                
				strcat(listing_output + listing_output_length, dp->d_name);
                listing_output_length = 
				  listing_output_length + strlen(dp->d_name);
                strcat(listing_output + listing_output_length, thePatternEnd);
                listing_output_length = 
				  listing_output_length + strlen(thePatternEnd);

                elementCount = 1;
              }
            }
          } while (dp != NULL);
				
          listing_output_ptr = 
		    realloc(listing_output, listing_output_length + strlen(" ]") + 1);
          if(listing_output_ptr == NULL){ free(listing_output); return -1; }
          listing_output = listing_output_ptr;
				
          strcat(listing_output + listing_output_length, " ]");
          listing_output_length = listing_output_length + strlen(" ]");

        }
        else{
          response.header("Status", "404 Not Found");
          free(listing_output);
          return -1;
        }
			
        listing_output_ptr = 
		  realloc(listing_output, listing_output_length + strlen(" }") + 1);
        if(listing_output_ptr == NULL){ free(listing_output); return -1; }
        listing_output = listing_output_ptr;
        strcat(listing_output + listing_output_length, " }");
        listing_output_length = listing_output_length + strlen(" }");

        char contentLength[20];
        sprintf(contentLength, "%d", listing_output_length);

        response.header("Content-Length", contentLength);
        response.header("Content-Type", "application/json");
        response.write(listing_output);
			
        free(listing_output);
      }
      else if(!foundDefault && !listdir){
        response.header("Status", "403 Forbidden");
        return -1;
      }
      else{
        return -1;
      }
    }/* end 403 */
    else{

      char *newResourceUrl = relativePath;
	
	  int rewrite_result = 1;
			
      char *newInPattern = inPattern;

      char *buffer = malloc(st.st_size);
      if(buffer == NULL){
	    /* free everything */
	    exit(500);
	  }
	  char mimeType[50];
	  char *mimePtr= (char *)mimeType;
	  
	  if(icwpx_getfilecontent(
	    relativePath, &buffer, st.st_size, &mimePtr) == 0) {

		response.header("Content-Type", mimeType);
		
	    char contentLength[50]; sprintf(contentLength, "%lu", st.st_size);
		response.header("Content-Length", contentLength);
			
	    response.printb(buffer, st.st_size);
	  }
	  
      free(buffer);
    }

  return 0;
}


int icwpx_getfilecontent(const char *uri, char **buffer, unsigned long size,
  char **mime){
	
  FILE *file = fopen(uri, "rb");
  if(file == NULL) return -1;
  
  int tries = 3;
  unsigned long totalread = 0;
  unsigned long remaining = size;
  
  while(totalread < size){
    unsigned long read =
	  fread(*buffer + totalread, 1, remaining, file);
	  
    if(read <= 0){
      fflush(file);
      if(tries-- <= 0) break;
      else continue;
    }
	  
    totalread += read;
    remaining -= read;
  }
  
  fclose(file);
  
  if(tries < 0) return -1;
  
  strcpy(*mime, "application/octet-stream");
  char *fileExtension = strrchr(uri, '.');
	    
  if(fileExtension != NULL){
    int mimesLen = sizeof(icwpx_mimetypes) / sizeof(icwpx_mimetypes[0]); 
    int i;
    for(i = 0; i < mimesLen; i++){
      char mimeKey[50]; memset(mimeKey, 0, 50);
      char mimeValues[50]; memset(mimeValues, 0, 50);
		    
      char *text = (char*)icwpx_mimetypes[i];
      sscanf( text, "%[^:]", mimeKey);
      text = strstr(text, "[");
      sscanf( text, "[%99[^]]]", mimeValues );
		    
      /* Extract the first token */
      char * token = strtok(mimeValues, " ");
      /* loop through the string to extract all other tokens */
      while( token != NULL ) {
        /* printf( " %s\n", token ); */
        if(strcmp(fileExtension, token) == 0){
          strcpy(*mime, mimeKey);
          break;
        }
        token = strtok(NULL, " ");
      }
    }
  }
  
  return 0;
}



void icwpx_initglobals(Request _request, Response _response){

  { extern Request request; request = _request; }
  { extern Response response; response = _response; }

}

void icwpx_freethings(){
  Annotation *at = annotationshead;
	
  while(at != NULL){
    Annotation *deleteAt = at;
    at = at->next;

    if(strcmp(deleteAt->at, "@files") == 0 && deleteAt->variable != NULL){
      free(deleteAt->variable);
    }

    free(deleteAt);
    deleteAt = NULL;
  }
	
  annotationshead = NULL;
	
  Path *_path = pathshead;
	
  while(_path != NULL){
    Path *deletePath = _path;
    _path = _path->next;
    if(deletePath->value != NULL) free(deletePath->value);
    free(deletePath);
    deletePath = NULL;
  }
	
  pathshead = NULL;
}
