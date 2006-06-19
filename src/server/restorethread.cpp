/*  restorethread.cpp - Thread class which communicates with the relay server

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

#define _LARGEFILE64_SOURCE

#include "restorethread.hh"
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
#include <map>
#include <vector>
#include "types/bitvect.hh"
#include <arpa/inet.h>
#include <md5.h>
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

    bool sendImage( int sock, std::string file, std::string imgName, int32_t from, int32_t to, BitVector* vector, sockaddr_in* addr)
    {
	UDPData sendbuf;
	int checksum, res;
	unsigned j;
	int i;
	int fd = open64(file.c_str(), O_RDONLY | O_LARGEFILE);
	md5_buffer(imgName.data(), imgName.size(), sendbuf.digest);
	
	if (fd == -1 ) {
	    cerr << "unable to open file:" << file << std::flush;
	    perror(" reason");
	    return false;
	}
	
	off64_t hossz = lseek64(fd,0,SEEK_END);
	unsigned int max = ( hossz + BUFSIZE  -1 ) / BUFSIZE; -- max; // (0 is the first)
	
	if ( lseek64(fd, from* BUFSIZE, SEEK_SET) != from * BUFSIZE ) {
	    return false;
	}

	for(i = from; i != to; ++i){
	    sendbuf.index = htonl(i);
	    res = 6;
	    if ( (res=rw(fd, sendbuf.data, BUFSIZE)) == -1 ) {
		perror("read");
		break;
	    } else if (! res )
		break;
	    
	    checksum=0;
	    for(j=0; j!=BUFSIZE; ++j){
		checksum += sendbuf.data[j]+1;
	    }
	    sendbuf.checksum = htonl(checksum);
	    if (sendto(sock, &sendbuf,sizeof(UDPData),0,(sockaddr*)addr,sizeof(sockaddr_in)) == -1) {
		perror ("send");
		close (fd);
		return false;
	    }
	    vector->unset(i);
	}
	sendbuf.index = htonl(i);
	memset(sendbuf.data,0,BUFSIZE);
	if ( rw(fd,sendbuf.data,BUFSIZE) <= 0) {
	    perror("rw");
	    close(fd);
	    return false;
	} 
	sendbuf.checksum=0;
    
	for(j=0; j!=BUFSIZE; ++j){
	    sendbuf.checksum+=sendbuf.data[j]+1;
	}
	sendbuf.checksum = htonl(sendbuf.checksum);
	close(fd);
	if ( sendto(sock, &sendbuf,sizeof(UDPData),0,(sockaddr*)addr,sizeof(sockaddr_in))  == -1 ) { 
	    perror("send");
	    return false;
	} else {
	    vector->unset(i);
	    return true;
	}
    }


} 


RestoreThread::RestoreThread()
{
    this->sock = socket(AF_INET,SOCK_DGRAM,0);
    int yes = 1;
    setsockopt(sock,SOL_SOCKET,SO_BROADCAST,&yes,sizeof(yes)); // enable BROADCAST

    sockaddr_in addrbr2;
    addrbr2.sin_family = AF_INET;
    { 
	RWLock::Reader r = configuration.lock.getReader();
	int brsrcport = atoi( configuration.data->parser.getValueDefault("broadcast-port", "8770").c_str() ) +1;
	addrbr2.sin_port = htons( brsrcport > 0 ? brsrcport : 8770);
    }
    addrbr2.sin_addr.s_addr = htonl(INADDR_ANY);
    
    if ( bind(sock, (sockaddr*)&addrbr2, sizeof(addrbr2)) == -1 ) {
	perror("Backup Thread: bind(broadcast)");
	exit (1);
    }
	
    int opts = fcntl ( sock, F_GETFL );
    
    if ( opts >= 0 ) { opts |= O_NONBLOCK;
    fcntl ( sock,   F_SETFL,opts );
    }
}


RestoreThread::~RestoreThread()
{
    if (sock > -1) {
	shutdown(sock, SHUT_RDWR);
	close(sock);
    }    
}

void RestoreThread::run()
{
    sockaddr_in addrbr;

    std::vector<BitVector*> rVector;
    std::map<std::string, int> rMap;
    std::map<int, std::string> rMapRev;

    int index = 0; // index of rVector 

    {
	RWLock::Reader r = configuration.lock.getReader();
	addrbr.sin_family = AF_INET;
	addrbr.sin_port = htons( atoi( configuration.data->parser.getValue("broadcast-port").c_str() ) );
	addrbr.sin_addr.s_addr = inet_addr(configuration.data->parser.getValue("broadcast").c_str());
    }
    while (g_running) {
	usleep (160);
	{
	   RWLock::Writer w = ::requests.lock.getWriter();
	   std::vector<RequestInfo>::const_iterator it = requests.data.begin();
	   for (;it != requests.data.end(); ++it) {
	       std::map<std::string, int>::const_iterator p  = rMap.find(it->name);
	       if (p == rMap.end()) {
		   BitVector* vector = new BitVector(it->max + 1);
		   rVector.push_back(vector);
		   rMap[it->name] = rVector.size() - 1;
		   rMapRev[rVector.size() - 1] = it->name;
		   p = rMap.find(it->name);
	       }
	       for (int i=it->from; i<= it->to; ++i)
		   rVector[p->second]->set(i);
	   }
	   ::requests.data.clear();
	}

	// sending images
	if (!rVector.size()) continue;
	index = (index + 1) % rVector.size();
	
	int from, to;
	rVector[index]->intv1(from, to);
	to = Helpers::min(to, from + 1024);
	if (to == -1) continue;
	std::string filename;
	{
	    RWLock::Reader r = configuration.lock.getReader();
	    filename = configuration.data->parser.getValue(rMapRev[index]);
	}
	sendImage(sock, filename, rMapRev[index], from, to, rVector[index], &addrbr);
    }
    
}
 
 
void restoreThread(void*) {
    RestoreThread r;
    r.run();
} 
 
/** EMACS **
 * Local variables:
 * mode: c++
 * c-basic-offset: 4
 * End:
 */
