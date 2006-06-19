/*  server_thread.cpp - Communication with the relay server

    Copyright (C) 2006 Laszlo Attila Toth

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


#define HAS_CONFIGURATION
#include "config.hh"
#undef  HAS_CONFIGURATION
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include "requestthread.hh"

#undef max
namespace {
    template <class T> 
    T max(T a, T b) {
	return a > b ? a : b;
    }
}

extern bool g_running;
typedef void (*threadproc_t)(void*);

pthread_t startProc(threadproc_t threadProc, void * data=0);

void serverThreadProc(int fd);
void restoreThreadProc(void * fd) {
    RequestThread thread((int) fd);
    thread.run();
}
void server_thread(void*)
{
    int sockListen;
    const int yes = 1;

    sockaddr_in addr;
   
    sockListen = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(sockListen,SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)); // in case of TIME_WAIT the socket can LISTEN

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    {
	RWLock::Reader r = configuration.lock.getReader();
	addr.sin_port = htons( atoi( configuration.data->parser.getValueDefault("port", "8766").c_str() ) );
    }
    if ( bind(sockListen,(sockaddr*)&addr, sizeof(addr)) == -1 ) {
	perror("Server Thread: bind(listen)");
	exit (1);
    }
	    
    serverThreadProc(sockListen);

    shutdown(sockListen, SHUT_RDWR);
    close(sockListen);

}


void serverThreadProc(int socket) {
    listen(socket, 10);

    while (::g_running) {
	int r;
	fd_set rd, wr, er;
	FD_ZERO (&rd);
	FD_ZERO (&wr);
	FD_ZERO (&er);
	FD_SET (socket, &rd);
	r = select (socket + 1, &rd, &wr, &er, NULL);
	if (r == -1 && errno == EINTR) {
                   continue;
               }
               if (r < 0) {
                   perror ("select()");
                   exit (1);
               }
               if (FD_ISSET (socket, &rd)) {
                   unsigned int l;
                   struct sockaddr_in client_address;
                   memset (&client_address, 0, l =
                           sizeof (client_address));
                   r = accept (socket, (struct sockaddr *)
                               &client_address, &l);
                                   if (r < 0) {
                       perror ("accept()");
                   } else {
		       startProc(restoreThreadProc, (void*)r);
		   }
	       }
    }  
}

/** EMACS **
 * Local variables:
 * mode: c++
 * c-basic-offset: 4
 * End:
 */
