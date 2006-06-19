/*  info_thread.cpp - Sending information about an image

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
#include <sstream>
#include "rs/rsparser.hh"
#include "rs/rsserverrequest.hh"
#include "rs/rsserverresponse.hh"
#include "helpers.hh"
#include <fstream>
#include <sys/stat.h>
#include <fcntl.h>
#include "iostream.hh"


extern bool g_running;
namespace {
    const int64_t BUFSIZE=1024;

    ssize_t rw(int fd, char* buf, const int len) {
	ssize_t cur = 0;
	ssize_t idx;
  
	while ( cur != len) {
	    cur += (idx=read(fd,buf+cur,len-cur));
	    if (idx <= 0) break;
	}
	return cur;
    }
}

void infoThreadProc(int socket);

void infoThreadMainProc(int socket) {
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
		       infoThreadProc(r);
		   }
	       }
    }  
}


void info_thread(void*)
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
	addr.sin_port = htons( atoi( configuration.data->parser.getValueDefault("info-port", "8767").c_str() ) );
    }
    if ( bind(sockListen,(sockaddr*)&addr, sizeof(addr)) == -1 ) {
	perror("Info Thread: bind(listen)");
	exit (1);
    }
	    
    infoThreadMainProc(sockListen);

    shutdown(sockListen, SHUT_RDWR);
    close(sockListen);

}


void infoThreadProc(int sock)
{
    enum error_t { NOT_SET, INVALID_IMG, INVALID_DIGEST, INVALID_REQUEST };
    error_t error = NOT_SET;
    RS::Parser parser;
    
    
    for(;g_running;) {
	int r;
	fd_set rd, wr, er;
	FD_ZERO (&rd);
	FD_ZERO (&wr);
	FD_ZERO (&er);
	FD_SET (sock, &rd);
	r = select (sock + 1, &rd, &wr, &er, NULL);
	if (r == -1 && errno == EINTR) {
	    continue;
	}
	if (r < 0) {
	    perror ("select()");
	    exit (1);
	}
	r = parser.parseHeader(sock, &rd, &er);
	if (r <= 0 || sock == -1) break; 
	usleep(1);
    }
    
    // check wether the socket is valid
    if (sock == -1) {
	return;
    }
    
    RS::ServerRequest request(parser);
    RS::ServerResponse response;
    std::string str = request.getURL();
    std::string imgName = str;
    
    bool validRequest = false;
    
    char fname[] = "/tmp/a.X.g.XXXXXX";

    if (request.hasError()) {
	error = INVALID_REQUEST;
    } else {
	RWLock::Reader r = configuration.lock.getReader();
	for (size_t i = 0; i != configuration.data->infoList.size(); ++i) {
	    if (str == configuration.data->infoList[i].name) {
		validRequest = true; // may be valid. Check it!!
		// try to check the digest, if available
		str = request.getHeader("digest", "0");
		if (str.size() == 32) {
		    Helpers::Digests d;
		    memcpy(d.hexstr, str.c_str(), 32);
		    ChecksumInfo info, infoD;
		    memcpy(info.digest, d.digest, 16);
		    memcpy(infoD.digest, configuration.data->infoList[i].digest, 16);
		    if (info != infoD) { // invalid request!!
			validRequest = false;
			error = INVALID_DIGEST;
			break;
			
		    } 
		}else {
// 		    validRequest = false;
// 		    error = INVALID_DIGEST;
// 		    break;
		}
		
		// all done; valid request!
		mkstemp(fname);
		std::ofstream f(fname);
		f << configuration.data->infoList[i];
		f.close();
		break;
	    }
	}
    }
    if (!validRequest) {
	switch (error) {
	case NOT_SET:
	    response.setHeader("error", "Image not found");
	    break;
	case INVALID_DIGEST:
	    response.setHeader("error", "Image found but digest doesn't match");
	    break;
	case INVALID_REQUEST:
	    response.setHeader("error", "Request is invalid, see line no " + request.getError());
	    break;
	default:
	    assert("This shouldn't be reached - default branch in RestoreThread::Run");
	}
	response.setStatus("400");
	response.setStatusText("Problem with request - see 'error' header for details");
    } else {
	response.setStatus("200");
	response.setStatusText("OK");	
    }

    // send the response
    std::string msg = response.getLines();
    while (msg.size()) {
	int r;
	fd_set wr, er;
	FD_ZERO (&wr);
	FD_ZERO (&er);
	FD_SET (sock, &wr);
	r = select (sock + 1, 0, &wr, &er, NULL);
	if (r == -1 && errno == EINTR) {
                   continue;
	}
	if (r < 0) {
	    perror ("select()");
	    return; // fatal error (but only in this thread)
	}
	if (FD_ISSET (sock, &wr)) {
	    r = write (sock, msg.c_str(), msg.size());
	    if (r < 0) {
		perror ("write()");
	    } else {
		msg.erase(0,r);
	    }
	}
    }

    // response has been sent
    if (!validRequest)
	return;

    // request handling
    const static int bufSize = 1024*2048;
    static char buffer[bufSize];
    int fd =  open64(fname, O_RDONLY | O_LARGEFILE);
    off64_t current  = lseek64(fd,0,SEEK_END);
    lseek64(fd,0,SEEK_SET);
    rw(fd, buffer, current);
    close(fd);
    unlink(fname);
    while (current) {
	int r;
	fd_set wr, er;
	FD_ZERO (&wr);
	FD_ZERO (&er);
	FD_SET (sock, &wr);
	r = select (sock + 1, 0, &wr, &er, NULL);
	if (r == -1 && errno == EINTR) {
                   continue;
	}
	if (r < 0) {
	    perror ("select()");
	    return; // fatal error (but only in this thread)
	}
	if (FD_ISSET (sock, &wr)) {
	    r = write (sock, buffer, current);
	    if (r < 0) {
		perror ("write()");
	    } else {
		current -= r;
	    }
	}
    }
    shutdown(sock, SHUT_RDWR);
    close(sock);
}

/** EMACS **
 * Local variables:
 * mode: c++
 * c-basic-offset: 4
 * End:
 */
