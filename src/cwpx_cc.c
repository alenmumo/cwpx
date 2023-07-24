/*
 * cwpx_cc.c: simple program to parse an html document with embedded C code 
 * written on it by using the tags '<%' and '%>'
 *
 * gcc -Wall -Wextra -Wconversion src/cwpx_cc.c -I"include" -o src/cwpxcc
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <limits.h>
#include <sys/stat.h>
#include <cwpx_globals.h>
#ifdef _WIN32
  #include <io.h>
  #include <fcntl.h>
#endif

#define MAX_CODE_TAGS 1024

typedef struct Cwpx_MappedLine{
  unsigned int lineMapping[2]; /* [originalLine, mappedLine] */
  struct Cwpx_MappedLine *next;
}Cwpx_MappedLine;

typedef struct Cwpx_CPageSettings{
  
  char *infilename;
  char *tmpfilename;
  
  char *command;
  
  Cwpx_MappedLine *mappedlinenode;
  
  char *infilenameArgvPtr;/* pointer-only*/
}Cwpx_CPageSettings;

void printhelp();
int init_cwpxcpagesettings(Cwpx_CPageSettings **cpagesettings, int argc,
  char **argv);
int parsehtml(Cwpx_CPageSettings **cpagesettings);
int pushline(Cwpx_CPageSettings **cpagesettings, Cwpx_MappedLine *newline);
int pairlines(Cwpx_CPageSettings **cpagesettings, char *infilebuffer);
int createcwpx(Cwpx_CPageSettings **cpagesettings);

int main(int argc, char **argv){
  
  if(argc < 2){
    printhelp();
  }
  else{
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
      /*printf("CWD: %s\n", cwd);*/
      /*chdir(cwd);*/
    }
  
    Cwpx_CPageSettings *cPageSettings = NULL;
    int initResult = init_cwpxcpagesettings(&cPageSettings, argc, argv);
    if(initResult == 0){
      /*printf("CWP file: \"%s\"\n", cPageSettings->infilename);
      printf("C file: \"%s\"\n", cPageSettings->tmpfilename);
      printf("command: \"%s\"\n", cPageSettings->command);*/
      
      /*printf("Parsing html ...\n");*/
      int parseHtmlResult = parsehtml(&cPageSettings);
      /*printf("Parsing result: %d\n", parseHtmlResult);*/
      
      /*printf("Creating CWPX ...\n");*/
      int createcwpxResult = createcwpx(&cPageSettings);
      /*printf("Creating CWPX result: %d\n", createcwpxResult);*/      
      
      return parseHtmlResult && createcwpxResult;
      
    }
    else{
	  printf("Error: Could not initialize settings\n");
	}
  }
  
  return 0;
}


void printhelp(){
  printf("\n-------CWPX CC Help--------\n");
  printf(
    "This program reads a 'cwpx page' (.cwp), interprets it and produces a \n"
	"'cwpx script' (.cwpx) according to the specified command.\n\n");
  printf("The command should look like this one:\n\n"
    "\tcpwxcc [gcc] [compileflags] <yourcpage>.cwp [-Ipath_to_cwpx/include]\n"
	"\t[ -o <yourscriptdesiredname>.cwpx] [-Lpath_to_cwpx/lib] -lcwpx\n\n"
	"For example:\n\n"
	"\tcwpxcc src/index.cwp -Iinclude -o src/index.cwpx -Llib -lcwpx\n\n"
	"As can be seen, the command ressembles a normal 'gcc' command, and it \n"
	"is just it, this interpreter will execute the command, so be sure you\n"
	"have correctly set up the C compiler, as well as environment variables \n"
	"and the locations of the CWPX library.\n\n");
  printf("\n-------CWPX CC Help--------\n\n");
}

int init_cwpxcpagesettings(Cwpx_CPageSettings **cpagesettings, int argc,
  char **argv){

  *cpagesettings = malloc(sizeof(Cwpx_CPageSettings));
  Cwpx_CPageSettings *cpagesettingsPtr = *cpagesettings;
  
  if(cpagesettingsPtr == NULL) { /* free everything */ exit(1); }
  cpagesettingsPtr->infilename = NULL;
  cpagesettingsPtr->tmpfilename = NULL;
  cpagesettingsPtr->command = NULL;
  cpagesettingsPtr->mappedlinenode = NULL;
  cpagesettingsPtr->infilenameArgvPtr = NULL;
  
  size_t commandLen = 0;
  cpagesettingsPtr->command = calloc(1, sizeof(char));
  if(cpagesettingsPtr->command == NULL){/*free everything*/ exit(1);}

  int i = 0;
  while(i < argc){
    char *arg = argv[i];
	if(i != 0){ /* skip the first argument (program name, probably) */
	  char *ext = strrchr(arg, '.');
	  if(ext != NULL && (strcmp(ext, ".cwp") == 0 )){
		/* before getting the full path of the input file, keep a reference to 
		the short relative filename provided at argv. this name is useful when
		showing compilation error file */
		cpagesettingsPtr->infilenameArgvPtr = argv[i];
		
		/* this is the input file (with path), so make sure to get the 
		absolute path for preventing path mismatchings */
		char fullinfilename[CWPX_PATH_MAX];
        char *testFullRealPath = NULL;
        #ifdef _WIN32
        testFullRealPath = _fullpath(fullinfilename, arg, CWPX_PATH_MAX);
        #else
        testFullRealPath = realpath(arg, fullinfilename); 
        #endif
        if(testFullRealPath == NULL ){
          printf("Error: Could not resolve path: %s\n",
		    cpagesettingsPtr->infilename);
		  free(*cpagesettings);
		  *cpagesettings = NULL;
          return -1;
        }else{
		  unsigned int j; /* replace backslashes (a Windows matter...) */ 
          for(j = 0; j < strlen(fullinfilename); j++) 
		    if(fullinfilename[j] == '\\') fullinfilename[j] = '/';
		  //printf("FullRealPath %s\n", fullinfilename);
		}		
	    char *tryMalloc = malloc(strlen(fullinfilename) + 1);
	    if(tryMalloc == NULL) {/*free everything*/ exit(1);}
	    strcpy(tryMalloc, fullinfilename);
	    cpagesettingsPtr->infilename = tryMalloc;
	    
	    char *filenamePtr = strrchr(cpagesettingsPtr->infilename, '/');
	    short hasDirSep = filenamePtr != NULL ? 1 : 0;
	    filenamePtr = hasDirSep ? filenamePtr : cpagesettingsPtr->infilename;
	    
	    unsigned int onlyfilepathLen = 
		  (unsigned)(filenamePtr - cpagesettingsPtr->infilename);
	    if(hasDirSep) filenamePtr += 1; /* skip the '/' */
	    char *extFull = strrchr(cpagesettingsPtr->infilename, '.');
	    unsigned int onlyfilenameLen = (unsigned)(extFull - filenamePtr);
	    
	    char datestr[30];
	    struct tm *timenow;
        time_t now = time(NULL);
        timenow = gmtime(&now);
        strftime(datestr, 30, "_%Y-%m-%d_%H-%M-%S_", timenow);
        unsigned int datetimeLen = (unsigned int)strlen(datestr);
	    /* pathLen + strlen(/) + datetimeLen + filenameLen + strlen(.c) */ 
	    tryMalloc = 
		  malloc(onlyfilepathLen + datetimeLen + onlyfilenameLen + 10);
	    if(tryMalloc == NULL) {/*free everything*/ exit(1);}
	    
	    char *tmpfilename = tryMalloc;
	    unsigned int tmpfilenameLen = 0;
	    if(hasDirSep){
		  strncpy(tmpfilename, cpagesettingsPtr->infilename, onlyfilepathLen);
		  strcpy(tmpfilename + onlyfilepathLen, "/");
		  tmpfilenameLen += onlyfilepathLen + 1;
		}
		
	    sprintf(tmpfilename + tmpfilenameLen, "%s%.*s.c", 
		  datestr, onlyfilenameLen, filenamePtr);
		  
	    cpagesettingsPtr->tmpfilename = tmpfilename;
	    
	    tryMalloc = 
		  realloc(cpagesettingsPtr->command, 
		    commandLen + strlen(cpagesettingsPtr->tmpfilename) + 2); /*space*/
		if(tryMalloc == NULL) {/*free everything*/ exit(1);}
		cpagesettingsPtr->command = tryMalloc;
		sprintf(cpagesettingsPtr->command + commandLen, "%s ",
		  cpagesettingsPtr->tmpfilename);
		commandLen = strlen(cpagesettingsPtr->command);
	  }
	  else{
	    char *tryMalloc = 
		  realloc(cpagesettingsPtr->command, 
		    commandLen + strlen(arg) + 2); /*space*/
		if(tryMalloc == NULL) {/*free everything*/ exit(1);}
		cpagesettingsPtr->command = tryMalloc;
		sprintf(cpagesettingsPtr->command + commandLen, "%s ",
		  arg);
		commandLen = strlen(cpagesettingsPtr->command);
	  }
	}
    
    i++;
  }
  
  /* look for the substring 'gcc ' (with the space) in the command to detect if
  the C compiler is specified. if it's not, preppend it to current command */
  if(strncmp(cpagesettingsPtr->command, "gcc ", strlen("gcc ")) != 0){
    char *tryMalloc = malloc(commandLen + strlen("gcc ") + 1);
    if(tryMalloc == NULL) {/*free everything*/ exit(1);}
    sprintf(tryMalloc, "gcc %s", cpagesettingsPtr->command);
    free(cpagesettingsPtr->command);
    cpagesettingsPtr->command = tryMalloc;
    commandLen = strlen(cpagesettingsPtr->command);
  }
  
  /* finally, as the gcc prints error on STDERR, the command in 'popen' 
  function should redirect STDERR to STDOUT, like  2>&1 */
  char *tryMalloc = malloc(commandLen + strlen(" 2>&1") + 1);
  if(tryMalloc == NULL) {/*free everything*/ exit(1);}
  sprintf(tryMalloc, "%s 2>&1", cpagesettingsPtr->command);
  free(cpagesettingsPtr->command);
  cpagesettingsPtr->command = tryMalloc;
  commandLen = strlen(cpagesettingsPtr->command);
  
  return 0;
}

int parsehtml(Cwpx_CPageSettings **cpagesettings){
  Cwpx_CPageSettings *cpagesettingsPtr = *cpagesettings;
  
  /* PART I: Read the CWPX Page file and store it in a buffer */
  FILE *cwpFile = fopen(cpagesettingsPtr->infilename, "rb");//"rb");
  if(cwpFile == NULL){
    printf("Error: Could not read the file %s.\n", 
	  cpagesettingsPtr->infilename);
    return -1;
  }
  
  fseek(cwpFile, 0, SEEK_END);
  long testFileLen = ftell(cwpFile);
  fseek(cwpFile, 0, SEEK_SET);
  if(testFileLen == -1){
    printf("Could not determine the cwp file length\n");
    fclose(cwpFile);
    
    /*struct stat st;
    stat(cpagesettingsPtr->infilename, &st);
    unsigned long size = (unsigned long)st.st_size;
    printf("size: %lu\n", size);*/
    
    return -1;
  }
  
  unsigned long cwpFileLen = (unsigned long)testFileLen;
  
  char *buffer = (char *)malloc(cwpFileLen + 1);
  memset(buffer, 0, cwpFileLen + 1);
  if(buffer == NULL){
    fclose(cwpFile);
    /* free everything */ exit(1);
  }
  
  unsigned long totalRead = 0;
  unsigned long remaining = cwpFileLen;
  size_t read = 0;
  
  while((read = fread(buffer + totalRead, remaining, 1, cwpFile) > 0)){
    totalRead += read;
    remaining -= read;
  }
  buffer[cwpFileLen] = 0;
  fclose(cwpFile);
  
  /* PART II: CREATE THE TEMPORAL C FILE */
  FILE* cFile = fopen(cpagesettingsPtr->tmpfilename, "wb");//"wb");
  if(cFile == NULL){
    printf("Error: Could not write the file %s\n",
	  cpagesettingsPtr->tmpfilename);
    return -1;
  }
  
  char *start_tag;
  char *end_tag;
  unsigned long start_position[MAX_CODE_TAGS] = {0};
  unsigned long end_position[MAX_CODE_TAGS] = {0};
  char *bufferPtr = buffer;
  int tag_counter = 0;
  unsigned long tag_position;
  
  /* ///REGION INCLUDES/// */
  start_tag = strstr(bufferPtr, "<%@ ");
  while(start_tag){
    tag_position = (unsigned)(start_tag - bufferPtr);
    start_position[tag_counter] = tag_position;
    end_tag = strstr(start_tag, " %>");
    if(end_tag){
      end_tag = end_tag + 3;
      tag_position = (unsigned)(end_tag - bufferPtr);
      end_position[tag_counter] = tag_position;
      tag_counter++;
    }
    else{
      printf("Error: A <%%@ tag was opened but was not closed by a %%> tag\n");
      fclose(cFile);
      return -1;
    }
    start_tag = strstr(end_tag, "<%@ ");
  }
  
  int i;
  unsigned long bufferPosition = 0;
  int startCode = 0;
  
  if(tag_counter > 0){	/* if any include */
    for(i = 0; i < tag_counter; i++){
      /* write html code before <%@ tag */
      for(bufferPosition = bufferPosition; bufferPosition < start_position[i]; 
	    bufferPosition++){
		
        if(bufferPtr[bufferPosition] == '\n'){
          if(startCode != 0){
            fputs("\");\nprintf(\"",cFile);
          }
        }
        else{
          if(bufferPtr[bufferPosition] != 13){ /* if it's not CR (car return)*/
            if(startCode == 0){
              fputs("printf(\"",cFile);
              startCode = 1;
            }
            if(bufferPtr[bufferPosition] == '"'){
              fputc('\\', cFile);
              fputc('"', cFile);
            }
            else if(bufferPtr[bufferPosition] == '\\'){
              fputc('\\', cFile);
              fputc('\\', cFile);
            }
            else{
              fputc(bufferPtr[bufferPosition], cFile);
            }
          }
        }
      }
      if(startCode != 0){
        fputs("\");\n",cFile);
        printf("Error: #include directives must be set before any html ...\n");
        fclose(cFile);
        return -1;
      }
      
      /* write c(++) inside <%@ tag */
      for(bufferPosition = start_position[i] + 4; 
	    bufferPosition < end_position[i] - 3; bufferPosition++){
		
        fputc(bufferPtr[bufferPosition], cFile);
      }
      bufferPosition = end_position[i];
      fputs("\n",cFile);
      
      startCode = 0;
    }
  }
  /* ///REGION INCLUDES/// */
  
  fputs("\n",cFile);
  
  
  /* ///REGION REST OF CODE/// */
  fputs("#include <cwpx.h>\n",cFile);
  fputs("\n",cFile);
  
  fputs("void do_http(Request request, Response response){\n\n",cFile);
  
  tag_counter = 0;
  start_tag = strstr(bufferPtr, "<% ");
  while(start_tag){
    tag_position = (unsigned)(start_tag - bufferPtr);
    start_position[tag_counter] = tag_position;
    end_tag = strstr(start_tag, " %>");
    if(end_tag){
      end_tag = end_tag + 3;
      tag_position = (unsigned)(end_tag - bufferPtr);
      end_position[tag_counter] = tag_position;
      tag_counter++;
    }
    else{
      printf("Error: A <%% tag was opened but was not closed by a %%> tag\n");
      return -1;
    }
    start_tag = strstr(end_tag, "<% ");
  }
  
  startCode = 0;
  
  if(tag_counter > 0){	/* if any code */
    for(i = 0; i < tag_counter; i++){
      /* write html code before <% tag */
      for(bufferPosition = bufferPosition; bufferPosition < start_position[i]; 
	    bufferPosition++){
      
        if(bufferPtr[bufferPosition] == '\n'){
          if(startCode != 0){
            fputs("\\n\");\nresponse.write(\"",cFile);
          }
        }
        else{
          if(bufferPtr[bufferPosition] != 13){ /* if it's not CR (car return)*/
            if(startCode == 0){
              fputs("response.write(\"",cFile);
              startCode = 1;
            }
            if(bufferPtr[bufferPosition] == '"'){
              fputc('\\', cFile);
              fputc('"', cFile);
            }
            else if(bufferPtr[bufferPosition] == '\\'){
              fputc('\\', cFile);
              fputc('\\', cFile);
            }
            else{
              fputc(bufferPtr[bufferPosition], cFile);
            }
          }
        }
      }
      
      if(startCode != 0){
        fputs("\");\n",cFile);
      }
      
      /* write c(++) inside <%@ tag */
      for(bufferPosition = start_position[i] + 3; 
	    bufferPosition < end_position[i] - 3; bufferPosition++){
		
        fputc(bufferPtr[bufferPosition], cFile);
      }
      bufferPosition = end_position[i];
      fputs("\nresponse.write(\"\");\n",cFile);
      
      startCode = 0;
    }
  }
  
  /* write html after the last %> tag */
  for(bufferPosition = bufferPosition; 
    bufferPosition < cwpFileLen; bufferPosition++){
	
    if(bufferPtr[bufferPosition] == '\n'){
      if(startCode != 0){
        fputs("\\n\");\nresponse.write(\"",cFile);
      }
    }
    else{
      if(bufferPtr[bufferPosition] != 13){ /* if it's not CR (car return) */
        if(startCode == 0){
          fputs("response.write(\"",cFile);
          startCode = 1;
        }
        if(bufferPtr[bufferPosition] == '"'){
          fputc('\\', cFile);
          fputc('"', cFile);
        }
        else if(bufferPtr[bufferPosition] == '\\'){
          fputc('\\', cFile);
          fputc('\\', cFile);
        }
        else{
          fputc(bufferPtr[bufferPosition], cFile);
        }
      }
    }
  }
  
  if(startCode != 0){
    fputs("\\n\");\n",cFile);
  }
  
  fputs("\nreturn;\n",cFile);
  
  fputs("\n} /* end of do_http */\n",cFile);
  
  /* ///REGION REST OF CODE/// */
  
  fclose(cFile);
  
  pairlines(&*cpagesettings, buffer);
  
  free(buffer);
    
  return 0;
}

int pushline(Cwpx_CPageSettings **cpagesettings, Cwpx_MappedLine *newline){
  Cwpx_CPageSettings *cpagesettingsPtr = *cpagesettings;
  newline->next = NULL;
  
  if(cpagesettingsPtr->mappedlinenode == NULL){
    cpagesettingsPtr->mappedlinenode = newline;
  }
  else{
	Cwpx_MappedLine *it = cpagesettingsPtr->mappedlinenode;
	while(it->next != NULL){
	  it = it->next;
	}
	it->next = newline;
  }
  
  return 0;
}

int pairlines(Cwpx_CPageSettings **cpagesettings, char *infilebuffer){
  Cwpx_CPageSettings *cpagesettingsPtr = *cpagesettings;
  
  /* PART I: Read the temporary C file and store it in a buffer */
  FILE *cFile = fopen(cpagesettingsPtr->tmpfilename, "rb");//"rb");
  #ifdef _WIN32
  //setmode(fileno(cFile), O_BINARY);
  #endif
  if(cFile == NULL){
    printf("Error: Could not read the file %s.\n", 
	  cpagesettingsPtr->tmpfilename);
    return -1;
  }
  
  fseek(cFile, 0, SEEK_END);
  long testFileLen = ftell(cFile);
  fseek(cFile, 0, SEEK_SET);
  if(testFileLen == -1){
    printf("Could not determine the temp c file length\n");
    fclose(cFile);    
    return -1;
  }
  
  unsigned long cFileLen = (unsigned)testFileLen;
  
  char *outfilebuffer = (char *)malloc(cFileLen + 1);
  if(outfilebuffer == NULL){
    fclose(cFile);
    if(cpagesettingsPtr->tmpfilename != NULL)
	  remove(cpagesettingsPtr->tmpfilename);
    /* free everything */ exit(1);
  }
  
  unsigned long totalRead = 0;
  unsigned long remaining = cFileLen;
  size_t read = 0;
  
  while((read = fread(outfilebuffer + totalRead, remaining, 1, cFile) > 0)){
    totalRead += read;
    remaining -= read;
  }
  outfilebuffer[cFileLen] = 0;
  fclose(cFile);
  
  char *outfilebufferPtr = outfilebuffer; /* IMPORTANT not to work with buffer
  directly, but using a pointer, to avoid free errors ...*/
  
  /* PART II: Now with the two buffers, infilebuffer and outfilebuffer, search
  the lines from one file in the other file and populate the lines' queue */
  
  char *inNewLinePtr = NULL; unsigned int inLineCounter = 0;
  char *outNewLinePtr = NULL; unsigned int outLineCounter = 0;
  char *inLineBuffer = NULL; char *inLineBufferPtr;
  char *outLineBuffer = NULL; char *outLineBufferPtr;
  /* inLineBufferPtr & outLineBufferPtr are really needed for not working 
  directly with inLineBuffer & outLineBuffer (mallocced/realloced constantly),
  thus avoiding memory problems ... */
  
  char *backR = "\r"; char *backN = "\n"; char *backRbackN = "\r\n";
  #ifndef _WIN32
  if(strstr(infilebuffer, backRbackN) == NULL)
  backRbackN = "\n";
  #endif
  while((inNewLinePtr = strstr(infilebuffer, backRbackN)) != NULL){
	/* new line validation */
	char *inbackRPtr = strstr(infilebuffer, backR);
	inNewLinePtr = inbackRPtr != NULL? inbackRPtr: strstr(infilebuffer, backN);
	
	inLineCounter++;
	
	/* get the line */
    unsigned long inlineLen = (unsigned long)(inNewLinePtr - infilebuffer);
	char *tryMalloc = inLineBuffer == NULL ? malloc(inlineLen + 1) : 
	  realloc(inLineBuffer, inlineLen + 1);
	if(tryMalloc == NULL){/* free everything */ 
	  if(cpagesettingsPtr->tmpfilename != NULL) 
	    remove(cpagesettingsPtr->tmpfilename);
	  exit(1);
	}
    inLineBuffer = tryMalloc;
    inLineBufferPtr = inLineBuffer;
    
    strncpy(inLineBufferPtr, infilebuffer, inlineLen);
    inLineBufferPtr[inlineLen] = 0;
    
    /* now that we have the line, move the pointer to the next line for
	avoiding problems */
	infilebuffer = 
	  inNewLinePtr + (inbackRPtr != NULL ? strlen(backRbackN): strlen(backN));	
    
    /* trim the line */
    while(inLineBufferPtr[inlineLen - 1] == ' '){
	  inLineBufferPtr[inlineLen-1] = 0; inlineLen--;
	}
    while(inLineBufferPtr[0] == ' '){
	  inLineBufferPtr++; inlineLen--;
    }
    
    /* now, with a line from the input buffer, find it in the output buffer,
	but skip some irrelevant lines to filter the important ones. as the 
	irrelevant ones are just a few, no need of regex patterns, just hardcode'em
	in the 'if' */
	if(strcmp(inLineBufferPtr, "") != 0 && 
	  strstr(inLineBufferPtr, "#include <cwpx.h>") == NULL &&
	  strstr(inLineBufferPtr, "#include<cwpx.h>") == NULL  &&
	  strstr(inLineBufferPtr, "#include  <cwpx.h>") == NULL  &&
	  strstr(inLineBufferPtr, "#include   <cwpx.h>") == NULL  &&
	  strstr(inLineBufferPtr, "#include    <cwpx.h>") == NULL  &&
	  strcmp(inLineBufferPtr, "{") != 0 &&
	  strcmp(inLineBufferPtr, "}") != 0 &&
	  strcmp(inLineBufferPtr, "<%@") != 0 &&
	  strcmp(inLineBufferPtr, "<%") != 0 &&
	  strcmp(inLineBufferPtr, "%>") != 0){
	  
	  /* repeat ALMOST the same procedure for getting a line, but, 
	  now from outfilebuffer */
	  while((outNewLinePtr = strstr(outfilebufferPtr, backN)) != NULL){
	
	    outLineCounter++;
	
	    /* get the line */
        unsigned long outlineLen = 
		  (unsigned long)(outNewLinePtr-outfilebufferPtr);
	    tryMalloc = outLineBuffer == NULL ? malloc(outlineLen + 1) : 
	    realloc(outLineBuffer, outlineLen + 1);
	    if(tryMalloc == NULL){/* free everything */ 
		  if(cpagesettingsPtr->tmpfilename != NULL) 
		    remove(cpagesettingsPtr->tmpfilename);
		  exit(1);
		}
        outLineBuffer = tryMalloc;
        outLineBufferPtr = outLineBuffer;
    
        strncpy(outLineBufferPtr, outfilebufferPtr, outlineLen);
        outLineBufferPtr[outlineLen] = 0;
    
        /* now that we have the line, move the pointer to the next line for
	    avoiding problems */
	    outfilebufferPtr = 
	      outNewLinePtr + strlen(backN);
    
        /* trim the line */
        while(outLineBufferPtr[outlineLen - 1] == ' '){
	      outLineBufferPtr[outlineLen-1] = 0; outlineLen--;
	    }
        while(outLineBufferPtr[0] == ' '){
	      outLineBufferPtr++; outlineLen--;
        }
        
        /* A PROBLEM WITH WINDOWS MAKES THIS 'IF' EXIST ... */
        if(outLineBufferPtr[outlineLen - 1] == 13){
			outLineBufferPtr[outlineLen - 1] = 0;
			outlineLen--;
		}
        
        short lineWasFoundInOut = 0;
        
        /* now, with a line from the output buffer, compare it with the one 
		from input buffer, but, again, skip some irrelevant lines to filter the
		important ones. as the irrelevant ones are just a few, no need of regex
		patterns, just hardcode'em in the 'if' */
	    if(strcmp(outLineBufferPtr, "") != 0 && 
	      strstr(outLineBufferPtr, "#include <cwpx.h>") == NULL &&
	      strstr(outLineBufferPtr, "#include<cwpx.h>") == NULL  &&
	      strstr(outLineBufferPtr, "#include  <cwpx.h>") == NULL  &&
	      strstr(outLineBufferPtr, "#include   <cwpx.h>") == NULL  &&
	      strstr(outLineBufferPtr, "#include    <cwpx.h>") == NULL  &&
	      strcmp(outLineBufferPtr, 
	        "void do_http(Request request, Response response){") != 0 &&
	      strcmp(outLineBufferPtr, "{") != 0 &&
	      strcmp(outLineBufferPtr, "}") != 0 &&
	      strcmp(outLineBufferPtr, "response.write("");") != 0){
		    
		    /*the actual look up of one line from 'in' and one line from 'out'. 
			for the moment the function just checks whether A contains B or if 
			B contains A, but this should have a deeper algorithm if it fails, 
			for example,
			strstr(outLineBufferPtr, "response.write(inLineBufferPtr)")*/
			/* mandatory: replace \" by " or the other way around to compare */
			char *withoutQuotes = malloc(outlineLen + 1);
			if(withoutQuotes == NULL){ /* free everything*/ 
			  if(cpagesettingsPtr->tmpfilename != NULL) 
			    remove(cpagesettingsPtr->tmpfilename);
			  exit(1);
			}
			unsigned int i1, i2 = 0;
			short replaced = 0;
			for(i1 = 0; i1 < outlineLen; i1++){
			  /* special escaped chars */
			  char *quotePtr = strstr(outLineBufferPtr + i1, "\\\""); /* \" */
			  char *backslashPtr = strstr(outLineBufferPtr + i1, "\\\\");/*\\*/
			  if(quotePtr != NULL && quotePtr == outLineBufferPtr + i1){ 
				withoutQuotes[i2++] = '\"';
			    i1++; 
				replaced = 1; 
			  }
			  else if(backslashPtr != NULL && 
			    backslashPtr == outLineBufferPtr + i1){ 
				withoutQuotes[i2++] = '\\';
			    i1++; 
				replaced = 1;
			  }
			  else{
			    withoutQuotes[i2++] = outLineBufferPtr[i1];
			  }
			  if(i2 < outlineLen) 
			    withoutQuotes[i2] = 0;/* for keeping a null terminator ...*/
			}
			
			if(replaced){
			  /*printf("outline(replaced) %d: .%s.\n", 
			    outLineCounter, withoutQuotes);*/
			}
			
			if(strstr(outLineBufferPtr, inLineBufferPtr) != NULL || 
			  strstr(inLineBufferPtr, outLineBufferPtr) != NULL || 
			  strstr(withoutQuotes, inLineBufferPtr) != NULL || 
			  strstr(inLineBufferPtr, withoutQuotes) != NULL){
			  
			  /* push the lines in the queue. remember:
			    [originalLine, mappedStartingLine, mappedEndingLine]
			  */
			  Cwpx_MappedLine *newMappedLine = malloc(sizeof(Cwpx_MappedLine));
			  newMappedLine->lineMapping[0] = inLineCounter;
			  newMappedLine->lineMapping[1] = outLineCounter;
			  newMappedLine->next = NULL;
			  pushline(&*cpagesettings, newMappedLine);
			  
			  lineWasFoundInOut = 1;
			  
			  /*printf("MATCHING LINES WERE FOUND\n");*/
			}  
			free(withoutQuotes);	
		    
	    }/* end outLineBufferPtr 'if' filter */
	    else{
	      /*printf("skipping outline: %d\n", outLineCounter);*/
	    }
	    
	    if(lineWasFoundInOut) break;
	    
	  }/* end outNewLinePtr */
			
    }/* end inLineBufferPtr 'if' filter */
	else{
	  /*printf("skipping inline: %d\n", inLineCounter);*/
	}
	
  }/* end inNewLinePtr */
  
  /* find 'EOF', but, as it is a buffer, the EOF is just the 0 terminated str*/
  unsigned long inleftoverLen = strlen(infilebuffer);
  if(inleftoverLen > 0){
	inLineCounter++;
    
    unsigned long outleftoverLen = strlen(outfilebufferPtr);
    if(outleftoverLen > 0){
      outLineCounter++;
      
      Cwpx_MappedLine *newMappedLine = malloc(sizeof(Cwpx_MappedLine));
	  newMappedLine->lineMapping[0] = inLineCounter;
	  newMappedLine->lineMapping[1] = outLineCounter;
	  newMappedLine->next = NULL;
	  pushline(&*cpagesettings, newMappedLine);
    }
    
  }
  
  if(outLineBuffer != NULL) free(outLineBuffer);
  if(inLineBuffer != NULL) free(inLineBuffer);
  if(outfilebuffer != NULL) free(outfilebuffer);
  
  return 0;
}

int createcwpx(Cwpx_CPageSettings **cpagesettings){
  Cwpx_CPageSettings *cpagesettingsPtr = *cpagesettings;

  /* the same issue with binary mode in Windows ... */
  #ifdef _WIN32
  FILE *gccProcess = popen(cpagesettingsPtr->command, "rb");
  #else
  FILE *gccProcess = popen(cpagesettingsPtr->command, "r");
  #endif
  if(gccProcess == NULL){
    printf("Error: Unable to open gccProcess\n");
    return -1;
  }

  char buffer[BUFSIZ];
  char *processStdout = NULL;
  unsigned long stdoutLen = 0;
  unsigned long bytesRead = 0;
    
  while((bytesRead = fread(buffer, 1, BUFSIZ, gccProcess)) > 0){
    char *tryMalloc = processStdout == NULL ?
     malloc(bytesRead + 1) : realloc(processStdout, stdoutLen + bytesRead + 1);
    if(!tryMalloc){ /* free everything ... */ 
	  if(cpagesettingsPtr->tmpfilename != NULL) 
	    remove(cpagesettingsPtr->tmpfilename);
	  exit(1); 
	}
    processStdout = tryMalloc;
    memcpy(processStdout + stdoutLen, buffer, bytesRead);
    stdoutLen += bytesRead;
  }
  
  if(processStdout == NULL){
	/*printf("Error: Could not read any gcc output");*/
	/* if gcc succeeds, it won't output anything, it's still a valid result */
    return 0;/*-1;*/
  }
  
  processStdout[stdoutLen] = 0;
  
  int gccResult = pclose(gccProcess);
  
  /* read gcc output line by line and produce another output. repeat ALMOST
  the same procedure as in 'pairlines', but this time, intead of comparing two
  buffers, read one buffer line and create another buffer line with the desired
  syntax (converted line) */
  char *infilebuffer = processStdout;
  char *outfilebuffer = NULL;
  unsigned long outfilebufferLen = 0;
  
  char *inNewLinePtr = NULL; unsigned int inLineCounter = 0;
  char *inLineBuffer = NULL; char *inLineBufferPtr;
  /* inLineBufferPtr is really needed for not working directly with 
  inLineBuffer (mallocced/realloced constantly), thus, avoiding memory 
  problems ... */
  
  char *backR = "\r"; char *backN = "\n"; char *backRbackN = "\r\n";
  #ifndef _WIN32
  /* Unlike the function 'pairlines', in this one function,
  this program's Linux version doesn't use "\r\n" */
  backRbackN = backN; 
  #endif
  while((inNewLinePtr = strstr(infilebuffer, backRbackN)) != NULL){
	/* new line validation */
	char *inbackRPtr = strstr(infilebuffer, backR);
	inNewLinePtr = inbackRPtr != NULL? inbackRPtr: strstr(infilebuffer, backN);
	
	inLineCounter++;
	
	/* get the line */
    unsigned long inlineLen = (unsigned long)(inNewLinePtr - infilebuffer);
	//if(strstr(infilebuffer, backRbackN) != NULL) lineLen--;
	char *tryMalloc = inLineBuffer == NULL ? malloc(inlineLen + 1) : 
	  realloc(inLineBuffer, inlineLen + 1);
	if(tryMalloc == NULL){/* free everything */ 
	  if(cpagesettingsPtr->tmpfilename != NULL) 
	    remove(cpagesettingsPtr->tmpfilename);
	  exit(1);
	}
    inLineBuffer = tryMalloc;
    inLineBufferPtr = inLineBuffer;
    
    strncpy(inLineBufferPtr, infilebuffer, inlineLen);
    inLineBufferPtr[inlineLen] = 0;
    
    /* now that we have the line, move the pointer to the next line for
	avoiding problems */
	infilebuffer = 
	  inNewLinePtr + (inbackRPtr != NULL ? strlen(backRbackN): strlen(backN));	
    
    /* trim the line */
    while(inLineBufferPtr[inlineLen - 1] == ' '){
	  inLineBufferPtr[inlineLen-1] = 0; inlineLen--;
	}
    while(inLineBufferPtr[0] == ' '){
	  inLineBufferPtr++; inlineLen--;
    }
    
    
    /* now, with a line from the input buffer, read it and parse it to produce
	the same line, but with the changes on names and line numbers. the lines 
	from gcc start with the C filename, including any path given, then the line
	and the column, and after that, the error description, for example:
	  src/file.c: In function 'function':
      src/file.c:795:4: error: 'm' undeclared (first use in this function)
      src/file.c:795:4: note: each undeclared identifier is reported only once 
      src/file.c:797:3: error: expected ';' before 'char'
      src/file.c:802:28: error: 'buff' undeclared (first use in this function)
    */
    char *lineFinder = inLineBufferPtr;
    short isErrorLine = 0;
    int errorLineNumber = 0;
    if(strncmp(lineFinder, cpagesettingsPtr->tmpfilename, 
	  strlen(cpagesettingsPtr->tmpfilename)) == 0){
	  lineFinder += strlen(cpagesettingsPtr->tmpfilename);
	  if(lineFinder[0] == ':'){ /* try line */
	    lineFinder++;
	    errorLineNumber = atoi(lineFinder);
	    if(errorLineNumber > 0){
		  isErrorLine = 1;
		  /* put lineFinder at the beginning of the error message */
		  char *errorFinder = strstr(lineFinder, ": ");
		  if(errorFinder != NULL){
			errorFinder += strlen(": ");
		    lineFinder = errorFinder;
		  }
		}
		else{/* if got here, but no line number, then it's a title */
		  isErrorLine = 1;
		  lineFinder = lineFinder;
		}
	  }
	}
	
	if(isErrorLine){
      /* create the parsed line and store it in final buffer */
      
      /* replace 'errorLineNumber' with the matching line pair */
      Cwpx_MappedLine *it = cpagesettingsPtr->mappedlinenode;
      while(it != NULL){
		if(it->lineMapping[1] == (unsigned)errorLineNumber){
		  errorLineNumber = (int)it->lineMapping[0];
		  break;
		}
        it = it->next;
      }
      
      
      char lineNumberStr[10];
      sprintf(lineNumberStr, "%d", errorLineNumber);
      size_t lineLen = 
	    strlen(cpagesettingsPtr->infilenameArgvPtr) + strlen(":") + 
	    strlen(lineNumberStr) + strlen(": ") + strlen(lineFinder) + 
		strlen("\n");

	  char *newParsedLine = malloc(lineLen + 1);
	  if(newParsedLine == NULL){/* free everything */ 
	    if(cpagesettingsPtr->tmpfilename != NULL) 
		  remove(cpagesettingsPtr->tmpfilename);
		exit(1);
	  }
	  sprintf(newParsedLine, "%s:%s: %s\n", 
	    cpagesettingsPtr->infilenameArgvPtr, lineNumberStr, lineFinder);
	  if(errorLineNumber == 0){
	    sprintf(newParsedLine, "%s:%s\n", 
	      cpagesettingsPtr->infilenameArgvPtr, lineFinder);
	      lineLen = strlen(newParsedLine);
	  }
	  
	  /* skip the lines that contain "In function ‘do_http’", 
	     "unused parameter ‘request’" and 
		 "void do_http(Request request, Response response)" */
	  short skipLine = 0;
	  char *containsPtr = strstr(newParsedLine, "In function");
	  if(containsPtr != NULL) 
	    containsPtr = strstr(containsPtr + strlen("In function"), "do_http");
	  if(containsPtr != NULL) skipLine = 1;
	  if(!skipLine){ containsPtr = strstr(newParsedLine, "unused parameter");
	  if(containsPtr != NULL) 
	   containsPtr = strstr(containsPtr +strlen("unused parameter"),"request");
	  if(containsPtr != NULL) skipLine = 1; }	  
	  
      if(!skipLine){
	    char *tryMallocOut = outfilebuffer == NULL ? malloc(lineLen + 1) : 
	      realloc(outfilebuffer, outfilebufferLen + lineLen + 1);
	    if(tryMallocOut == NULL){
	      /* free everything */
	      free(newParsedLine);
	      free(inLineBuffer);
	      if(outfilebuffer != NULL) free(outfilebuffer);
	      if(cpagesettingsPtr->tmpfilename != NULL) 
		    remove(cpagesettingsPtr->tmpfilename);
	      exit(1);
	    }
	    outfilebuffer = tryMallocOut;
	    strcpy(outfilebuffer + outfilebufferLen, newParsedLine);
	    outfilebufferLen += lineLen; 
	  }
	  
	  free(newParsedLine);
	    
    }else{
	  /* only store it in final buffer */
	  short skipLine = 0; 
	  char *containsPtr = strstr(inLineBufferPtr, 
		    "void do_http(Request request, Response response)");
	    if(containsPtr != NULL) skipLine = 1;
	  
	  if(!skipLine){ 
	   char *tryMallocOut = 
	   outfilebuffer == NULL ? malloc(inlineLen + strlen("\n") + 1) : 
	   realloc(outfilebuffer, outfilebufferLen + inlineLen + strlen("\n") + 1);
	    if(tryMallocOut == NULL){
	      /* free everything */
	      free(inLineBuffer);
	      if(outfilebuffer != NULL) free(outfilebuffer);
	      if(cpagesettingsPtr->tmpfilename != NULL) 
		    remove(cpagesettingsPtr->tmpfilename);
	      exit(1);
	    }
	    outfilebuffer = tryMallocOut;
	    strcpy(outfilebuffer + outfilebufferLen, inLineBufferPtr);
	    outfilebufferLen += inlineLen; 
	    strcat(outfilebuffer, "\n");
	    outfilebufferLen += strlen("\n"); 
	  }
	}
	
  }/* end inNewLinePtr */
  
  /* find 'EOF', but, as it is a buffer, the EOF is just the 0 terminated str*/
  unsigned long inleftoverLen = strlen(infilebuffer);
  if(inleftoverLen > 0 && inNewLinePtr != NULL){
	inLineCounter++;
    
    /* just copy the rest of content ...*/
	char *tryMallocOut = outfilebuffer == NULL ? malloc(inleftoverLen + 1) : 
	  realloc(outfilebuffer, outfilebufferLen + inleftoverLen + 1);
	if(tryMallocOut == NULL){
	  /* free everything */
	  free(inLineBuffer);
	  if(outfilebuffer != NULL) free(outfilebuffer);
	  if(cpagesettingsPtr->tmpfilename != NULL) 
	    remove(cpagesettingsPtr->tmpfilename);
	  exit(1);
	}
	outfilebuffer = tryMallocOut;
	strcpy(outfilebuffer + outfilebufferLen, inLineBufferPtr);
	outfilebufferLen += inleftoverLen; 
  }
  
  /* THE FINAL CWPX_CC OUTPUT (ONLY GCC). Notice the "(null)" validation. 
  In Windows there is no problem, it prints that when printing NULL, but other
  Linux versions crash if we make such an assumption */
  printf("%s\n", outfilebuffer != NULL ? outfilebuffer: "(null)");
  
  if(cpagesettingsPtr->tmpfilename != NULL) 
    remove(cpagesettingsPtr->tmpfilename);
  
  if(inLineBuffer != NULL) free(inLineBuffer);
  if(outfilebuffer != NULL) free(outfilebuffer);
  if(processStdout != NULL)free(processStdout);
  
  return gccResult;
}
