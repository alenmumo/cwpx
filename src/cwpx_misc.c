/*
 * gcc -Wall -Wextra -Wconversion -c src/cwpx_misc.c -I"include" -o lib/cwpx_mi
 * sc.o
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <math.h>
#include <unistd.h>
#ifdef _WIN32
  #include <windows.h>
#else
  #include <netdb.h>
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <paths.h>
#endif
#include <cwpx_misc.h>
#include <cwpx_globals.h>

char *cwpx_memstr(char **mem, const char *str, char **memlen){
  if(*mem == NULL || str == NULL || *memlen == NULL) return NULL;
	
  size_t str_length = strlen(str);
  char *mem_ptr = (char *)*mem;
  char *memlen_ptr = (char *)*memlen;
	
  if(mem_ptr < memlen_ptr){
    while((mem_ptr + str_length) <= memlen_ptr){
      size_t cmp_result = (size_t)memcmp(mem_ptr, str, str_length);
      if(cmp_result == 0){
        return mem_ptr;
      }
      mem_ptr += 1;
    }
  }
  return NULL;
}


int cwpx_generate_uuid(char *result_buffer){
  if(result_buffer == NULL) 
    return -1;
  strcpy(result_buffer, "");
	
  /* Copied from https://stackoverflow.com/questions/7399069/how-to-generate-a-
  guid-in-c by "E Net Arch" */
  /* a little less simple ps-random*/
  srand ((unsigned)time(NULL)  + (unsigned int)getpid()); 
  char *GUID = result_buffer; /* or char GUID[40]; */
  unsigned t = 0;
  char *szTemp = "xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx";
  char *szHex = "0123456789ABCDEF-";
  size_t nLen = strlen (szTemp);

  for(t = 0; t < nLen + 1; t++){
    int r = rand () % 16;
    char c = ' ';   

    switch (szTemp[t]){
      case 'x' : { c = szHex [r]; } break;
      /*case 'y' : { c = szHex [r & 0x03 | 0x08]; } break;*/
      case 'y' : { c = szHex [r & (0x03 | 0x08)]; } break;
      case '-' : { c = '-'; } break;
      case '4' : { c = '4'; } break;
    }

    GUID[t] = (char)(( t < nLen ) ? c : 0x00);
  }
  
  return 0;
}


void cwpx_decode_x_www_form_urlencoded(char *str_source, char *str_dest){
  if(str_source == NULL || str_dest == NULL) return;
	
  /* Copied from https://stackoverflow.com/questions/1634271/url-encoding-the-s 
  pace-character-or-20 */
	
  int n_length;
  for (n_length = 0; *str_source; n_length++) {
    if(*str_source == '+'){
      str_dest[n_length] = ' ';
      str_source += 1;
      continue;
    }
    if (*str_source == '%' && str_source[1] && str_source[2] && 
	  isxdigit(str_source[1]) && isxdigit(str_source[2])) {
      
      /*str_source[1] -= 
	    str_source[1] <= '9' ? '0' : (str_source[1] <= 'F' ? 'A' : 'a')-10;*/
	  str_source[1] = (char)(str_source[1] - 
	    (str_source[1] <= '9' ? '0' : (str_source[1] <= 'F' ? 'A' : 'a')-10));
	  
      /*str_source[2] -= 
	    str_source[2] <= '9' ? '0' : (str_source[2] <= 'F' ? 'A' : 'a')-10;*/
	  str_source[2] = (char)(str_source[2] - 
	    (str_source[2] <= '9' ? '0' : (str_source[2] <= 'F' ? 'A' : 'a')-10));
	    
      str_dest[n_length] = (char)(16 * str_source[1] + str_source[2]);
      str_source += 3;
      continue;
    }
    str_dest[n_length] = *str_source++;
  }
  str_dest[n_length] = '\0';
  /*return nLength;*/
}


void cwpx_encode_x_www_form_urlencoded(char *str_source, char *str_dest){
  if(str_source == NULL || str_dest == NULL) return;
	
  /* Copied from https://gist.github.com/jesobreira/4ba48d1699b7527a4a514bfa1d7
  0f61a */

  const char *hex = "0123456789abcdef";
  int n_length;
  for (n_length = 0; *str_source; n_length++) {
    if(*str_source == ' '){
      str_dest[n_length] = '+';
      str_source += 1;
      continue;
    }
    if ((*str_source >= 'a' && *str_source <= 'z') ||
	  (*str_source >= 'A' && *str_source <= 'Z') ||
	  (*str_source >= '0' && *str_source <= '9')) {
	  
	  str_dest[n_length] = *str_source;
	}
	else{
	  str_dest[n_length] = '%';
	  str_dest[n_length] = hex[*str_source >> 4];
	  str_dest[n_length] = hex[*str_source & 15];
	  continue;
	}
	
    str_dest[n_length] = *str_source++;
  }
  str_dest[n_length] = '\0';
}


int cwpx_strltrim(char* str_source, char* str_dest){
  if(str_source == NULL || str_dest == NULL) return -1;

  char *sourcePtr = str_source;
  int spaces = 0;
  while(sourcePtr[0] == ' '){
    sourcePtr++;
    spaces = 1;
  }
  
  if(!spaces){
	size_t srcLen = strlen(str_source);
	strncpy(str_dest, str_source, srcLen);
	str_dest[srcLen] = 0;
    return 0;
  }
  
  unsigned long destLen = 
    strlen(str_source) - (unsigned)(sourcePtr - str_source);
  
  strncpy(str_dest, sourcePtr, destLen);
  str_dest[destLen] = 0;
  
  return 0;
}


int cwpx_strrtrim(char* str_source, char* str_dest){
  if(str_source == NULL || str_dest == NULL) return -1;

  char *sourceEndPtr = str_source + strlen(str_source) - 1;
  int spaces = 0;
  while(sourceEndPtr[0] == ' '){
    sourceEndPtr--;
    spaces = 1;
  }
  
  if(!spaces){
	size_t srcLen = strlen(str_source);
	strncpy(str_dest, str_source, srcLen);
	str_dest[srcLen] = 0;
    return 0;
  }
  
  /* sourceEndPtr would be pointing to the last non-empty space, 
  let's care that by adding +1 in the next instruction*/
  unsigned destLen = (unsigned)((sourceEndPtr + 1) - str_source);
  strncpy(str_dest, str_source, destLen);
  str_dest[destLen] = 0;
  
  return 0;
}


int cwpx_strtrim(char* str_source, char* str_dest){
  if(str_source == NULL || str_dest == NULL) return -1;

  /* left trimming */
  cwpx_strltrim(str_source, str_dest);
  
  /* right trimming */
  char *sourceEndPtr = str_dest + strlen(str_dest) - 1;
  int spaces = 0;
  while(sourceEndPtr[0] == ' '){
    sourceEndPtr--;
    spaces = 1;
  }
  
  if(!spaces){
	size_t srcLen = strlen(str_source);
	strncpy(str_dest, str_source, srcLen);
	str_dest[srcLen] = 0;
    return 0;
  }
  
  /* sourceEndPtr would be pointing to the last non-empty space, 
  let's care that by adding +1 in the next instruction*/  
  unsigned destLen = (unsigned)((sourceEndPtr + 1) - str_source);
  /*strncpy(str_dest, str_source, destLen);*/
  str_dest[destLen] = 0;
  
  /* another way to work this is to malloc a temporary char * for the right
  trimming, but it is better to avoid mallocing */
  
  return 0;
}


/*
 * char *dest needs to be at least ((strlen(source) * 4) + 1) of memory size
 */
void cwpx_strtohexbytes(char *source, char *dest){
  if(source == NULL || dest == NULL) return;
  
  unsigned int i;
  size_t destLen = 0;
  for(i = 0; i < strlen(source); i++){
    sprintf(dest + destLen, "\\x%02x", source[i]);
    destLen += 4;
  }
}

/*
 * char *dest needs to be at least ((unsigned)ceil(strlen(source) / 4) + 2) 
 * of memory size (+2 in a rare case that strlen(source) / 4 is not an integer)
 */
void cwpx_hexbytestostr(char *source, char *dest){
  if(source == NULL || dest == NULL) return;
  
  unsigned int i;
  size_t destLen = 0;
  
  char *sourcePtr = source + 2; /* +2 for skipping the first '\x' prefix */
  for(i = 0; i < strlen(source);){
    char hexVal[3];
    sprintf(hexVal, "%.*s", 2, sourcePtr);
    int hexNum = (int)strtol(hexVal, NULL, 16); /* number base 16 */
    
    sprintf(dest + destLen, "%c", hexNum); /* sprintf it as a char */
    destLen += 1;
    
    sourcePtr += 4;
    i += 4;
  }
}


int cwpx_gethostname(char *hostname){
  if(hostname != NULL)
    strcpy(hostname, "");
	
  char l_hostname[128];
  #ifdef WIN32
  WSADATA wsaData;
  WSAStartup(MAKEWORD(2, 2), &wsaData);
  #endif
  int get_hostname = gethostname(l_hostname, sizeof(l_hostname));
  #ifdef WIN32
  WSACleanup();
  #endif
	
  strcpy(hostname, l_hostname);
  return get_hostname;
}

int cwpx_getipaddress(char *hostname, char *ipaddress){
  if(hostname == NULL) return 0;
  if(ipaddress != NULL) strcpy(ipaddress, "");
		
  struct hostent *host_entry;
  char *l_ip_address = NULL;
	
  #ifdef WIN32
  WSADATA wsaData;
  WSAStartup(MAKEWORD(2, 2), &wsaData);
  #endif
  host_entry = gethostbyname(hostname);
  #ifdef WIN32
  WSACleanup();
  #endif
    
  l_ip_address = inet_ntoa(*((struct in_addr*) host_entry->h_addr_list[0]));
  if(l_ip_address == NULL) return -1;
    
  strncpy(ipaddress, l_ip_address, strlen(l_ip_address));
  ipaddress[strlen(l_ip_address)] = 0;
  return 0;
}


int cwpx_gettimezonestr(long timezone_sec, char *timezone_str){
  long hours = timezone_sec / 3600;
  long minutes = (timezone_sec / 60) % 60;
  /*int seconds = timezone_sec % 60;*/
  	
  strcpy(timezone_str, "");
  if(timezone_sec < 0) strcat(timezone_str, "-");
  else strcat(timezone_str, "+");
	
  char hours_str[5];
  /*hours = (hours > 0) ? hours : (hours * -1);*/
  if(hours >= 10) sprintf(hours_str, "%lu", hours);
  else sprintf(hours_str, "0%lu", hours);
  strcat(timezone_str, hours_str);
	
  char minutes_str[5];
  minutes = (minutes > 0) ? minutes : (minutes * -1);
  if(minutes >= 10) sprintf(minutes_str, "%lu", minutes);
  else sprintf(minutes_str, "0%lu", minutes);
  strcat(timezone_str, minutes_str);
	
  return 0;
}

int cwpx_gettempdir(char *tempdir){
  #ifdef _WIN32
  return (int)GetTempPath(CWPX_PATH_MAX, tempdir);
  #else
  char *envOption = getenv("TMPDIR");
  if(envOption != NULL){
    strcpy(tempdir, envOption);
    return (int)strlen(envOption);
  }
  #ifdef P_tmpdir
  if(P_tmpdir != NULL){
    strcpy(tempdir, P_tmpdir);
    return (int)strlen(P_tmpdir);
  }
  #endif
  #ifdef _PATH_TMP
  if(_PATH_TMP != NULL){
    strcpy(tempdir, _PATH_TMP);
    return (int)strlen(_PATH_TMP);
  }
  #endif
  strcpy(tempdir, ".");
  return 1;
  #endif
}

unsigned int cwpx_getpid(){
  #ifdef _WIN32
  return (unsigned int)GetCurrentProcessId();
  #else
  return (unsigned int)getpid();
  #endif
}

void cwpx_replacebackslashes(char *path){
  unsigned int i; 
  for(i = 0; i < strlen(path); i++) if(path[i] == '\\') path[i] = '/';
}

int cwpx_getgmtdateandzone(char *timezonestr){
	
  if(timezonestr == NULL) return -1;

  time_t gmt, rawtime = time(NULL);
  struct tm *ptm;
  #if !defined(WIN32)
  struct tm gbuf;
  ptm = gmtime_r(&rawtime, &gbuf);
  #else
  ptm = gmtime(&rawtime);
  #endif
  /* Request that mktime() looksup dst in timezone database */
  ptm->tm_isdst = -1;
  gmt = mktime(ptm);

  int timezone_sec = (int)difftime(rawtime, gmt);
  
  time_t now = time(NULL);
  struct tm gmt_time = *gmtime(&now);
  char datetime_str[50 + 1];	// 00/Xxx/0000:00:00:00
  strftime(datetime_str, 50, "%d/%b/%Y:%H:%M:%S", &gmt_time);
    
  int hours = timezone_sec / 3600;
  int minutes = (timezone_sec / 60) % 60;
  /*int seconds = timezone_sec % 60;*/
  	
  char timezone_str[50 + 1]; /* 00/Xxx/0000:00:00:00 */
  strcpy(timezone_str, "");
  if(timezone_sec < 0)
    strcat(timezone_str, "-");
  else
    strcat(timezone_str, "+");
	
  char hours_str[5];
  hours = (hours > 0) ? hours : (hours * -1);
  if(hours >= 10)
    sprintf(hours_str, "%d", hours);
  else
    sprintf(hours_str, "0%d", hours);
  strcat(timezone_str, hours_str);
	
  char minutes_str[5];
  minutes = (minutes > 0) ? minutes : (minutes * -1);
  if(minutes >= 10)
    sprintf(minutes_str, "%d", minutes);
  else
    sprintf(minutes_str, "0%d", minutes);
  strcat(timezone_str, minutes_str);

  sprintf(timezonestr, "%s %s", datetime_str, timezone_str);

  return 0;
}
