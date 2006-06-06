/*  restore_thread.cpp - sending parts of the image (size: 1024 bytes)

    Copyright (C) 2004,2006, Laszlo Attila Toth

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


#define __GNU_SOURCE
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <sys/time.h>
#define HAS_CONFIGURATION
#include "config.hh"
#include "rwlock.hh"
#define HAS_REQUEST
#define EXTERN_REQ
#include "request_thread.hh"

using std::cerr;
using std::endl;
using std::cout;

#define REQBUFSIZE 4396
#define PERROR(a) {printf("In %s at line %d: ",__FILE__,__LINE__);  fflush (stdout); perror (a); }


void recvRequest(int fd);

RWLock::Lock requestLock;
extern RWLock::Lock configLock;

extern bool g_running;
#ifdef DEBUG
bool debug = true;
#else
bool debug = false;
#endif

void request_thread(void*)
{ 
    int slisten;
    const int igaz(1);
    sockaddr_in addr;

    slisten=socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(slisten, SOL_SOCKET, SO_REUSEADDR, &igaz, sizeof(igaz)); // in case of TIME_WAIT the socket can LISTEN
    
    
    addr.sin_family = AF_INET;
    {
	RWLock::Reader r = configLock.getReader();
	addr.sin_port = htons( atoi( configuration->parser.getValue("port").c_str() ) );
    }
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    
    
    if ( bind(slisten, (sockaddr*)&addr, sizeof(addr)) == -1 ) {
	perror("Request Thread: bind(listen)");
	exit (1);
    }
    
    listen(slisten, 10);
    
    while (g_running){
	recvRequest(slisten);
    }
    close(slisten);
    usleep(100);    
}


void recvRequest(int fd)
{
    fd_set rfds;
    struct timeval tv;
    int retval;

    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);
    tv.tv_sec = 0;
    tv.tv_usec = 500000;
    retval = select(fd+1, &rfds, NULL, NULL, &tv);
    if (retval == -1) {
	PERROR("select()");
	return;
    } else if (retval == 0 )
	return;
    // FD_ISSET is now true

    int sock, count = 0, res, i;
    char reqbuf[REQBUFSIZE+1];
    ReceivedRequest imgreq;
    if ( debug ) cerr << "Backup-Thread: Waiting for a request" << endl;

    sock = accept(fd,0,0);
    shutdown(sock,SHUT_WR);
    
    while (count != REQBUFSIZE) {
	res = read(sock,reqbuf+count,REQBUFSIZE-count);
	if ( res == -1) {
	    perror("Backup Thread: read");
	    shutdown(sock,SHUT_RDWR);
	    close(sock);
	    return;
	}
	if (! res) {
	    break;
	}
	count += res;
    }
    
    reqbuf[count]=0;
    shutdown(sock,SHUT_RD);
    close(sock);
    
    
    // parse the request
    for (i=0;i != REQBUFSIZE-8 && reqbuf[i] != ' ';) ++i;
    imgreq.file.assign(reqbuf,i);
    ++i;


    // try to find this image
    { 
	bool found = false;
	size_t i;
	RWLock::Reader r = configLock.getReader();
	for (i=0; i!= configuration->infoList.size(); ++i) {
	    if (configuration->infoList[i].name == imgreq.file) {
		found = true;
		break;
	    }
	}
	if (!found) {
	    cout << "Backup Thread: unknown img name: " << imgreq.file << endl;
	    return;
	} else {
	    memcpy(imgreq.digest, configuration->infoList[i].digest, 16);
	}
	imgreq.file = configuration->parser.getValue(imgreq.file);
	    
    }
    
    
    memcpy(&imgreq.from,reqbuf+i,sizeof(int32_t));
    memcpy(&imgreq.to,reqbuf+i+sizeof(int32_t),sizeof(int32_t));
    
    imgreq.from = ntohl(imgreq.from);
    imgreq.to = ntohl(imgreq.to);

    if ( debug )
	cerr << "\n img name: " << imgreq.file
	     << " from " << imgreq.from 
	     << " to " << imgreq.to << endl;    

    {
	RWLock::Writer w = requestLock.getWriter();
	requests.push_back(imgreq);
    }
    return;
}


/** EMACS **
 * Local variables:
 * mode: c++
 * c-basic-offset: 4
 * End:
 */
