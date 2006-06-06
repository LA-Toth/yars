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
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cerrno>
#include <unistd.h>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <sys/time.h>
#include <string>
#define HAS_CONFIGURATION
#include "config.hh"


using std::cerr;
using std::endl;
using std::flush;
using std::cout;

#define REQBUFSIZE 4396
#define PERROR(a) {printf("In %s at line %d: ",__FILE__,__LINE__);  fflush (stdout); perror (a); }


bool recvRequest(int fd, const PantherSW::ConfigParser& parser,const str2md5& checksums);
bool sendFile(int fd,  int replyfd,const sockaddr_in* addr);
ssize_t rw(int fd, unsigned char* buf, const int len);
ssize_t sendPacket(int s, int rs, const SENDDATA *buf, size_t len, int flags, const struct sockaddr *to, socklen_t tolen);
unsigned int chkbuf(const char*buf);
void updateTiming(const struct  timeval& t);
void updateTimingTimeout();
int  timeval_subtract (struct timeval *result,const struct timeval *x, struct timeval *y);


class ReceivedRequest
{
public:
    std::string file;
    unsigned int from;
    unsigned int to;
    unsigned char digest[16];
};

#ifdef DEBUG
bool debug = true;
#else
bool debug = false;
#endif

std::vector<ReceivedRequest> requests;

void restore_thread(void*)
{ 
    int slisten,sbroadcast, sreply;
    const int igaz(1);
    sockaddr_in addr,addrbr,addrbr2, addrreply;
    int brport = atoi( parser.getValue("broadcast-port").c_str() );
    int brsrcport = brport+1;
    int brreplyport = atoi( parser.getValue("broadcast-reply-port").c_str() );

    for(;;){
	slisten=socket(AF_INET,SOCK_STREAM,0);
	setsockopt(slisten,SOL_SOCKET,SO_REUSEADDR,&igaz,sizeof(igaz)); // in case of TIME_WAIT the socket can LISTEN
	
	sbroadcast = socket(AF_INET,SOCK_DGRAM,0);
	setsockopt(sbroadcast,SOL_SOCKET,SO_BROADCAST,&igaz,sizeof(igaz)); // enable BROADCAST
	
	sreply = socket(AF_INET,SOCK_DGRAM,0);
	setsockopt(sreply,SOL_SOCKET,SO_REUSEADDR,&igaz,sizeof(igaz));
	
	addr.sin_family = AF_INET;
	addr.sin_port = htons( atoi( parser.getValue("port").c_str() ) );
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	
	if ( bind(slisten,(sockaddr*)&addr,sizeof(addr)) == -1 ) {
	    perror("Backup Thread: bind(listen)");
	    exit (1);
	}

	addrbr.sin_family = AF_INET;
	addrbr.sin_port = htons( brport );
	addrbr.sin_addr.s_addr = inet_addr(parser.getValue("broadcast").c_str());

	addrbr2.sin_family = AF_INET;
	addrbr2.sin_port = htons( brsrcport );
	addrbr2.sin_addr.s_addr = htonl(INADDR_ANY);
	
	if ( bind(sbroadcast, (sockaddr*)&addrbr2, sizeof(addrbr2)) == -1 ) {
	    perror("Backup Thread: bind(broadcast)");
	    exit (1);
	}
	if ( connect(sbroadcast, (sockaddr*)&addrbr, sizeof(addrbr)) == -1 ) { 
	    perror("Backup Thread: connect(broadcast)");
	    exit (1);
	}
	
	addrreply.sin_family = AF_INET;
	addrreply.sin_port = htons( brreplyport );
	addrreply.sin_addr.s_addr = htonl(INADDR_ANY);

	if ( bind(sreply, (sockaddr*)&addrreply, sizeof(addrreply)) == -1 ) {
	    perror("Backup Thread: bind(reply)");
	    exit (1);
	}

	for (bool l = true; l;){
	    if (! recvRequest(slisten,parser,checksums)) continue;
	    /*if (! */sendFile(sbroadcast,sreply,&addrbr) /*) continue*/;
	}
	close(slisten);
	close(sbroadcast);
	close(sreply);
    }

    return 0;
}


bool recvRequest(int fd, const PantherSW::ConfigParser& parser,const str2md5& checksums)
{
    char file[REQBUFSIZE-8];
    int sock, count = 0, res, i;
    char reqbuf[REQBUFSIZE+1];
    if ( debug ) cerr << "Backup-Thread: Waiting for a request" << endl;
    listen(fd,1);
    sock = accept(fd,0,0);
    shutdown(sock,SHUT_WR);
    
    while (count != REQBUFSIZE) {
	res = read(sock,reqbuf+count,REQBUFSIZE-count);
	if ( res == -1) {
	    perror("Backup Thread: read");
	    shutdown(sock,SHUT_RDWR);
	    close(sock);
	    return false;
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
    
    str2md5_ci ciP = checksums.find(imgreq.file);
    if (ciP == checksums.end()) { // if not exists, go to next request
	cout << "Backup Thread: unknown img name: " << imgreq.file << endl;
	return false;
    }
    
    imgreq.file = parser.getValue(imgreq.file);
    
    memcpy(&imgreq.from,reqbuf+i,sizeof(int));
    memcpy(&imgreq.to,reqbuf+i+sizeof(int),sizeof(int));
    
    memcpy(imgreq.digest,ciP->second.c_str(),16);
    if ( debug )
	cerr << "\n img name: " << file
	     << " from " << imgreq.from 
	     << " to " << imgreq.to << endl;    
    
    return true;
}

bool sendFile( int sock,int replysock ,const sockaddr_in * addr)
{
    SENDDATA sendbuf;
    int  res;
    unsigned i;

    memcpy(sendbuf.digest, imgreq.digest, 16);
    sendbuf.version = 1;
    
    int fd = open64(imgreq.file.c_str(), O_RDONLY | O_LARGEFILE);
    
    if (fd == -1 ) {
	cerr << "unable to open file:" << imgreq.file << flush;
	perror(" reason");
	return false;
    }
    
    off64_t hossz = lseek64(fd,0,SEEK_END);
    unsigned int max = ( hossz + SND_BUFSIZE  -1 ) / SND_BUFSIZE; -- max; // (0 is the first)
    if ( debug ) printf("length %lld; max %d; requested max: %d\n",hossz,max,imgreq.to);

    if (imgreq.from > max or imgreq.to > max) {
	close(fd);
	cerr <<  "from or to is too large." << endl;
	return false;
    }
    if ( lseek64(fd, imgreq.from* SND_BUFSIZE, SEEK_SET) != imgreq.from * SND_BUFSIZE ) {
	cerr << "Backup Thread: seek: seek error" << endl;
	return false;
    }
    //if (tune==0)
    //tune =20;
    
    for(i=imgreq.from; i!=imgreq.to; ++i){
	sendbuf.index = i;
	res = 6;
	if ( (res=rw(fd, sendbuf.data, SND_BUFSIZE)) == -1 ) {
	    PERROR("read");
	    break;
	} else if (! res )
	    break;
	
	calculate_checksum(sendbuf);
	
	if (sendPacket(sock, replysock, &sendbuf,sizeof(SENDDATA),0,(sockaddr*)addr,sizeof(sockaddr_in)) == -1) {
	    PERROR ("send");
	    close (fd);
	    return false;
	}
	// TCP-like timer usage...
    }
    sendbuf.index = i;
    memset(sendbuf.data,0,SND_BUFSIZE);
    if ( rw(fd,sendbuf.data,SND_BUFSIZE) <= 0) {
	PERROR("rw");
	close(fd);
	return false;
    } 

    calculate_checksum(sendbuf);

    close(fd);
    if ( sendPacket(sock, replysock, &sendbuf,sizeof(sendbuf),0,(sockaddr*)addr,sizeof(sockaddr_in) ) == -1 ) { 
	PERROR("send");
	return false;
    } else
	return true;
}

ssize_t rw(int fd, unsigned char* buf, const int len)
{
    ssize_t cur = 0;
    ssize_t idx;
  
    while ( cur != len) {
	cur += (idx=read(fd,buf+cur,len-cur));
	if (idx <= 0) break; 
    }
    return cur;
}


ssize_t sendPacket(int s, int rs, const SENDDATA *buf, size_t len, int flags,
		   const struct sockaddr *to, socklen_t tolen)
{
    int retval;
    AckRecvdata recvdata;
    if (timing.rto > 3000000) {
	timing.rtt = 500000;
	timing.dev = 200;
	timing.rto = timing.rtt + timing.dev << 2;
    }
    if (sendto(s,buf,len,flags,to,tolen) < 0) return false;
    gettimeofday(&timing.sendtime,0);
    timing.rcount = 0;
    cout << "F: " << buf->index << " sent" << endl;
    fd_set fdRead;
    struct  timeval recvtime;
    for(;;) {
	FD_ZERO(&fdRead);
	FD_SET(rs, &fdRead);
	timing.timeout.tv_usec = timing.rto % 1000000;
	timing.timeout.tv_sec = timing.rto / 1000000;
	retval = select(rs+1,&fdRead,NULL,NULL,&timing.timeout);
	if (retval >0) { // new data
	    if (FD_ISSET(rs, &fdRead)) {
		gettimeofday(&recvtime,0);
		recv(rs,&recvdata,sizeof(recvdata),0);
		if (!strncmp((char*)buf->digest,(char*)recvdata.digest,16) && recvdata.index == buf->index) {
		    unsigned int chksum = recvdata.checksum;
		    recvdata.checksum = 0;
		    if (checkAckRecvData(recvdata) == chksum) { // we got the first reply (every previous is read)
			updateTiming(recvtime);
			cout << "F: " << buf->index << " sent & received" << endl;
			return true;
		    }  else {
			cout << "F: " << buf->index << " checksum error" << endl;
	    
		    }
		} else {
		    cout << "F: " << recvdata.index << " other index" << endl;	    
		}
	    }
	} else if (retval < 0) {
	    PERROR("select @SendPacket");
	    exit(1);
    
	} else { // timeout
	    updateTimingTimeout();
	    if (++timing.rcount == 5) return true;
	    if (sendto(s,buf,len,flags,to,tolen) < 0) return false;
	    gettimeofday(&timing.sendtime,0);      
	    cout << "F: " << buf->index << " sent & timeout" << endl;
	}    
    }
}



void updateTiming(const struct  timeval& t)
{
    struct timeval diff, tmp;
    memcpy(&tmp,&timing.sendtime,sizeof(timeval));
    timeval_subtract (&diff,&t,&tmp);
    timing.m = diff.tv_sec + diff.tv_usec*1000000;
    timing.err = timing.sendtime.tv_sec + timing.sendtime.tv_usec*1000000 - timing.m;
    if (timing.err <0) timing.err *= -1;
    
    timing.dev += (timing.err - timing.dev) >> 2; // timing.h = 1/4
    timing.rtt += timing.err >>3; // timing.g = 1/8
    timing.rto = timing.rtt + timing.dev << 2;
    timing.timeout.tv_usec = timing.rto/1000000; 
    timing.timeout.tv_sec = timing.rto%1000000 ;
}

void updateTimingTimeout()
{
    timing.timeout.tv_usec *=2;
    timing.timeout.tv_sec  *=2;
    if ( timing.timeout.tv_usec >= 1000000) {
	++timing.timeout.tv_sec;
	timing.timeout.tv_usec -= 1000000;
    }
    timing.rto *=2;
}

/* Subtract the `struct timeval' values X and Y,
   storing the result in RESULT.
   Return 1 if the difference is negative, otherwise 0.  */

int  timeval_subtract (struct timeval *result,const struct timeval *x,struct timeval *y)
{
    /* Perform the carry for the later subtraction by updating y. */
    if (x->tv_usec < y->tv_usec) {
	int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
	y->tv_usec -= 1000000 * nsec;
	y->tv_sec += nsec;
    }
    if (x->tv_usec - y->tv_usec > 1000000) {
	int nsec = (x->tv_usec - y->tv_usec) / 1000000;
	y->tv_usec += 1000000 * nsec;
	y->tv_sec -= nsec;
    }
    
    /* Compute the time remaining to wait.
       tv_usec is certainly positive. */
    result->tv_sec = x->tv_sec - y->tv_sec;
    result->tv_usec = x->tv_usec - y->tv_usec;
    
    /* Return 1 if result is negative. */
    return x->tv_sec < y->tv_sec;
}


/** EMACS **
 * Local variables:
 * mode: c++
 * c-basic-offset: 4
 * End:
 */
