/*
 * gcc  -Wall -Wextra -Wconversion -fPIC -c -Iinclude src/cwpx_config.c -o lib/
 * cwpx_config.o
 * gcc  -Wall -Wextra -Wconversion -o lib/cwpxconf.dll -shared lib/cwpx_config.
 * o
 *
 * https://itecnote.com/tecnote/can-i-declare-a-global-variable-in-a-shared-lib
 * rary/
*/
#include <stdio.h>
#include <cwpx_config.h>

/* The following variables define some configuration settings that apply to 
 * cwpx scripts. A 'char *' with empty value "", NULL or not declared (like 
 * the ones comented in this sample) are 'auto-configurable' by default values.
 * The same rule applies to number types with a 0 value or not declared. None 
 * of the variables configure or affect the web server, they only affect the 
 * current executing script. */

/* The server's executable path. It's recommended to set this variable with the 
 * the real webserver's directory, the path to 'bin'. If not set, the default 
 * value is the current working directory ... */
CWPXCFG char *cwpx_workdir = ""; 

/* The server's root 'www' web directory. It is mandatory to set this variable 
 * with the real webserver's web directory, the path to 'www', 'htdocs' ... */
CWPXCFG char *cwpx_rootdir = "";

/* Optional settings, just descriptive ... */
/*CWPXCFG char *cwpx_hostname;*/
/*CWPXCFG char *cwpx_servip = "";*/
/*CWPXCFG unsigned int cwpx_servpt = 80908;*/
/*CWPXCFG char *cwpx_tempdir = NULL;*/
/*CWPXCFG char *cwpx_sessdir = ".";*/
/*CWPXCFG char *cwpx_sessid = "";*/
/*CWPXCFG unsigned long cwpx_sessdur;*/
/*CWPXCFG char *cwpx_logfile = "logfile.txt";*/

/* Some script handing settings */
/*CWPXCFG unsigned long cwpx_buf_size;*/
/*CWPXCFG unsigned int cwpx_max_queueelems;*/
/*CWPXCFG unsigned short cwpx_allow_post;*/
/*CWPXCFG unsigned long cwpx_max_post;*/
