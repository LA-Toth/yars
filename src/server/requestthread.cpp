/*  requestthread.cpp

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


#include "requestthread.hh"
#include <sys/socket.h>
#include <unistd.h>
#include "rs/rsparser.hh"
#include "rs/rsserverrequest.hh"
#include "rs/rsserverresponse.hh"
#include <errno.h>
#include "helpers.hh"
#define HAS_CONFIGURATION
#include "config.hh"
#undef HAS_CONFIGURATION
#include "types/udpdata.hh"
#include <fcntl.h>
#include "request.hh"

extern bool g_running;

RequestThread::RequestThread(int s) {
    this->sock = s;
      int opts = fcntl ( sock, F_GETFL );

      if ( opts >= 0 ) {
	  opts |= O_NONBLOCK;
	  fcntl ( sock,   F_SETFL,opts );
      }
}


RequestThread::~RequestThread() {
    if (sock > -1) {
	shutdown(sock, SHUT_RDWR);
	close(sock);
    }    
}

void RequestThread::run() {
    enum error_t { NOT_SET, INVALID_IMG, INVALID_DIGEST, INVALID_FROM, INVALID_TO, INVALID_PARTS, INVALID_REQUEST };
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
    int from = 0, to = 0, iTmp = 1;
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
		}
		
		// additional parts
		from = atoi(request.getHeader("from", "-1").c_str());
		to = atoi(request.getHeader("to", "-1").c_str());
		
		if (from != -1  && to != -1 && (from > to || from < -1 || to < -1)) {
		    error = INVALID_PARTS;
		    validRequest = false;
		    break;
		} 
		if (from == -1) from = 0;
		iTmp = ((int32_t)( (configuration.data->infoList[i].size + 1023 )/ (int64_t)1024)) - 1; 
		if (to > iTmp) {
		    error = INVALID_TO;
		    validRequest = false;
		    break;		    
		}
		if (from > iTmp) {
		    error = INVALID_FROM;
		    validRequest = false;
		    break;		    
		}
		if (to == -1) to = iTmp;
		// all done; valid request!
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
	case INVALID_PARTS:
	    response.setHeader("error", "Value of 'from' or 'to' is out of bound");
	    break;
	case INVALID_FROM:
	    response.setHeader("error", "Value of 'from' is out of bound");
	    break;
	case INVALID_TO:
	    response.setHeader("error", "Value of 'to' is out of bound");
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
    RequestInfo info = { imgName, from, to, iTmp };
    {
	RWLock::Writer w = ::requests.lock.getWriter();
	::requests.data.push_back(info);
    }    
}




/** EMACS **
 * Local variables:
 * mode: c++
 * c-basic-offset: 4
 * End:
 */
