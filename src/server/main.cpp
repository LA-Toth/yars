/*  main.cpp - Main part

    Copyright (C) 2004, 2006 Laszlo Attila Toth

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA */

#include <csignal>
#include <string>
#include <getopt.h>

////////////////////////// global variables ////////////////////
bool g_running = true;
char * argv0;
#define HAS_CONFIGURATION
#define EXTERN_CFG
#include "config.hh"
#undef HAS_CONFIGURATION
#undef EXTERN_CFG
#define EXTERN_REQUEST
#include "request.hh"
#undef EXTERN_REQUEST

////////////////////////// global variables ////////////////////



typedef void (*threadproc_t)(void*);
bool parseConfig(const std::string& str);
void config_thread(void*);
void server_thread(void*);
void info_thread(void*);
void restoreThread(void*);
const char*const VCFG="/etc/vissza/server.conf";
std::string cfgfile = VCFG;

pthread_t startProc(threadproc_t threadProc, void * data=0)
{
      pthread_t             threadId;
      pthread_attr_t        pthreadAttr;
      
      int rc;
      if ((rc = pthread_attr_init(&pthreadAttr))) {
	  printf( "pthread_attr_init ERROR.\n" );
	  exit(1);
          }
      
      if ((rc = pthread_attr_setstacksize(&pthreadAttr, 120*1024))) {
	  printf( "pthread_attr_setstacksize ERROR.\n" );
	  exit(1);
      }
      
      if ((rc = pthread_create(&threadId,
                              &pthreadAttr,
                              (void*(*)(void*))threadProc,
                              data ))) {
	  printf( "pthread_create ERROR.\n" );
	  exit(1);
      }
      return threadId;
}

int main(int argc, char * argv[])
{
    signal(SIGINT, SIG_IGN);

    argv0 = argv[0];
  
    while (true) {
	int option_index = 0;
	struct option long_options[] = {
	    {"config", 1, 0, 0},
	    {NULL, 0, 0, 0}
	};
    
	int c = getopt_long (argc, argv, "f:",
			     long_options, &option_index);
	if (c == -1)
	    break;
    
	switch (c) {
	case 0:
	    cfgfile = optarg;
	    break;
      
	case 'f':
	    cfgfile = optarg;
	    break;
	}
    }
    
    /// initialization
    configuration.data = 0;
    
    /// main part
    parseConfig(cfgfile);
    startProc(config_thread);
    startProc(server_thread);
    startProc(info_thread);
    startProc(restoreThread);
    while (g_running) { sleep(1); }

    if (configuration.data) delete configuration.data;
}


/** EMACS **
 * Local variables:
 * mode: c++
 * c-basic-offset: 4
 * End:
 */
