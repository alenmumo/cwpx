#ifndef _CWPX_MISC_H_
#define _CWPX_MISC_H_

#define cwpx_vartostr(var) #var
#define cwpx_concatstr(first, second) first second

char *cwpx_memstr(char **mem, const char *str, char **memlen);
int cwpx_generate_uuid(
  char *result_buffer ); /* result_buffer must be init'ed */
void cwpx_decode_x_www_form_urlencoded(
  char *str_source, 
  char *str_dest); /* str_source and str_dest must be the same reference */
void cwpx_encode_x_www_form_urlencoded(
  char *str_source, 
  char *str_dest);
int cwpx_strltrim(char* str_source, char* str_dest);
int cwpx_strrtrim(char* str_source, char* str_dest);
int cwpx_strtrim(char* str_source, char* str_dest);
void cwpx_strtohexbytes(char *source, char *dest);
void cwpx_hexbytestostr(char *source, char *dest);

int cwpx_gethostname(char *hostname);
int cwpx_getipaddress(char *hostname, char *ipaddress);
int cwpx_gettimezonestr(long timezone_sec, char *timezone_str);
int cwpx_gettempdir(char *tempdir);
unsigned int cwpx_getpid();
void cwpx_replacebackslashes(char *path);
int cwpx_getgmtdateandzone(char *timezonestr);

#endif
