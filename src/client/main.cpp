/*  main.cpp - Main part of the Client

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



#include <arpa/inet.h>
#include <poll.h>
#include <errno.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <vector>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <iomanip>
#include "confparser.hh"
#include "types/bitvectex.hh"
#include "types/imageinfo.hh"
#include "types/udpdata.hh"
#include "rs/rsclientrequest.hh"
#include "rs/rsclientresponse.hh"
#include "md5.h"
#include "checksums.hh"
#include "helpers.hh"
#include <sstream>


    //config fajl
#define VCFG "/etc/vissza/client.conf"
    //egy broadcast csomagban az adat merete bajtban
#define BUFFER 1024
#define SRV_INFOPORT 8767
#define BUFSIZE 4096
#define RECVBUFSIZE 1024

#define PERROR(a) {  if ( debug ) {  printf("In %s at line %d: ",__FILE__,__LINE__);  fflush (stdout); } perror (a); }


using namespace std;


bool interactive = false, debug = false;
std::string selecting;
std::string targetfile;

const struct option long_options[] = {
    {"config-file", 1, 0, 'f'},
    {"debug", 0, 0, 'd'},
    {"target-file", 1, 0, 't'},
    {"image", 1, 0, 'm'},
    {"help",0,0,'h'},
    {0, 0, 0, 0}
};


bool getImageInfo(const ConfParser::ConfigParser& parser, const std::string& imgName, ImageInfo& info);
void help(const char* progname);
bool sendRequest(ConfParser::ConfigParser &parser,int from, int to,const ImageInfo& info);
bool validateConfig(const ConfParser::ConfigParser& parser);


int main(int argc,char* argv[]){
    
    ConfParser::ConfigParser parser;
    std::string cfgfile = VCFG;
    bool hasImage = false;
    
    while (true) { 
	int option_index = 0;
	int c = getopt_long(argc, argv, "t:im:hf:d", long_options, &option_index);
	if (c == -1)
	    break;
	
	switch (c) {
	case 'f':
	    cfgfile = optarg;
	    break;
		
	case 'i':
	    interactive = true;
	    break;
	case 'm':
	    selecting = optarg;
	    if (selecting.size()) hasImage = true;
	    break;
	case 'h':
	    help(argv[0]);
	    exit(0);
	    break;
	case 't':
	    targetfile = optarg;
	    break;
	case 'd':
	    debug = true;
	    break;
	default:
	    exit (4);
	} 
    }    
    if (! hasImage) {
	fprintf(stderr,"Image is not selected. Try -h for details\n");
	exit(5);
    }  else if ( ! targetfile.size() ) {
	fprintf(stderr,"target file not selected or invalid. Try -h for details\n");
	exit(5);
    }   
    
    if (!parser.parse(cfgfile)) {
	cerr << "invalid configuration file format" << endl;
	perror("conffile parser");
	exit(1);
    } else if (! validateConfig(parser))  {
	exit(1);
    }

    ImageInfo imageInfo;
    if (!getImageInfo(parser, selecting, imageInfo)) {
	cerr << "invalid image name!!" << endl;
	exit (2);
    }
    
    int sbroadcast = socket(AF_INET,SOCK_DGRAM,0);
    if (sbroadcast == -1) {
	perror("broadcast socket");
	exit(1);
    }
    const int yes = 1;
    setsockopt(sbroadcast, SOL_SOCKET, SO_BROADCAST, &yes, sizeof(yes)); //BROADCAST engedelyezese
    
    struct sockaddr_in addrbr;
    addrbr.sin_family = AF_INET;
    addrbr.sin_port = htons( atoi(parser.getValue("broadcast-port").c_str()) ); //broadcast port
    addrbr.sin_addr.s_addr=INADDR_ANY;
    bind(sbroadcast, (sockaddr*)&addrbr, sizeof(addrbr));


    
    if ( ! imageInfo.size ) {
	fprintf(stderr,"%s: Invalid image name.\n",argv[0]);
	exit (1);	
    }
    
    std::string fname = targetfile;   // and the name of img's file
    int end = (imageInfo.size  + RECVBUFSIZE - 1) / RECVBUFSIZE; --end;
    
    if (debug) 
	cerr << "IMAGE:\t\t" << imageInfo.name << endl
	     << "TARGETFILE\t" <<  targetfile << endl
	     << "FSIZE:\t\t" << imageInfo.size << endl
	     << "COUNT:\t\t" << end << endl;

    struct timeval timeout = {3,0};
    fd_set fdRead;
    int retval;
    
    UDPData recvBuf;	
    BitVectorEx ready(end+1);

    int fd = open64 (fname.c_str(), O_CREAT/*|O_EXCL*/|O_LARGEFILE|O_RDWR, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    int checksum;

    if ( fd == -1 ) {
	PERROR("read");
	exit(1);
    }
    
    ChecksumInfoList checksums;
    calculateChecksum(fd, &checksums);
 

    if (checksums.size() != imageInfo.checksums.size()) {
	if (imageInfo.size) {
	    lseek64(fd, imageInfo.size-1, SEEK_SET);
	    write(fd, "r", 1);
	}
	close(fd);
	fd = open64 (fname.c_str(), O_CREAT/*|O_EXCL*/|O_LARGEFILE|O_RDWR, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
	lseek64(fd, 0, SEEK_SET);
	checksums.clear();
	calculateChecksum(fd, &checksums);
    } 
	 
    if (checksums.size() != imageInfo.checksums.size()) {
	cout << "EFDFSA " << checksums.size() << " d" << imageInfo.checksums.size()<<endl;
    }
    for (size_t i = 0 ; i< imageInfo.checksums.size()-1; ++i) {
	if (!memcmp(checksums[i].digest, imageInfo.checksums[i].digest, 16)) {
	    for (int j=0; j!=1024;++j) {
		ready.set( i*1024 + j);
	    }
	}
    }
	     
    // 	// last part
    if (imageInfo.checksums[checksums.size()-1] == checksums[ checksums.size()-1 ]) {
	for (int j=0; j!=((imageInfo.lastBlockSize / 1024)); ++j) {
	    cout << "j" << endl;
	    ready.set( (checksums.size()-1) * 1024  + j);
	}
    }
  

    for ( bool l = true; l;) {
	
	FD_ZERO(&fdRead);
	FD_SET(sbroadcast, &fdRead);
	
	retval = select(sbroadcast+1,&fdRead,NULL,NULL,&timeout);
	if ( debug ) {
	    cerr << "select retval: " << retval << endl;
	}
	if (retval >0) { // we may have new data
    
	    /*if (FD_ISSET(sbroadcast, &fdRead)) */{ // this socket? or an other (lower) file descriptor?
		
		int res = recv(sbroadcast,&recvBuf,sizeof(recvBuf),MSG_WAITALL);
		if ( res == -1 )		{
		    PERROR("recv");
		    exit(2);
		}
		if (!memcmp((char*)imageInfo.digest,(char*)recvBuf.digest,16) && (int32_t)ntohl((uint32_t)recvBuf.index) <= end &&
		    ! ready [  ntohl(recvBuf.index) ] )
		    // new part of the requested file
		    {
					
			checksum = 0;	    
			for (int i=0;i!=RECVBUFSIZE;++i)
			    checksum += recvBuf.data[i]+1;
		    
			if (checksum != (int32_t)ntohl(recvBuf.checksum)) {
			    continue;
			}
			lseek64(fd,(int32_t)ntohl(recvBuf.index) * (int64_t)RECVBUFSIZE,SEEK_SET);

			if ( debug ) cerr << "part: " <<  ntohl(recvBuf.index) << "; end: " << end << endl;
			
			if ((int32_t)ntohl(recvBuf.index) != end) {
			    if ( write(fd,recvBuf.data,RECVBUFSIZE) == -1 ) {
				PERROR("write");
				continue;	    
			    
			    }
			} else {
			    off64_t count = imageInfo.size;
			    while (count > RECVBUFSIZE) count -= RECVBUFSIZE;
			    // now 0 <  count <= RECVBUFSIZE
			    // only `count' byte should be written
			    // if count == 0 then it is not the last datagram: last +1
			    // if count > RECVBUFSIZE: not last: last but one
			
			    if ( write(fd,recvBuf.data,count) == -1 ) {
				PERROR("write");
				continue;
			    }
			}
			ready.set(  ntohl(recvBuf.index) );
		    }
	    }
	} else if (!retval){ // timeout
	    int from, to;
	    ready.intv(from,to);
	    if (to == -1) { // we are ready
		l = false;
		cerr << endl;
		cerr << "\t\t\t\e[37;41;1m--==== R E A D Y ====--\e[0;0m" << endl;		
	    } else {
		if ( debug ) cerr << "Request from " << from << " to "  << to << "." << endl;
		if ( ! sendRequest(parser, from, to, imageInfo) ) {
		    perror("sendRequest");
		}		
	    }
	    sleep (2);
	    
	}
	else {
	    perror ("select, poll");
	}
    }
    cerr << endl;
    return 0;
}



bool getImageInfo(const ConfParser::ConfigParser& parser, const std::string& imgName, ImageInfo& info)
{
    std::string buf="",tmp;
    std::string::const_iterator ci,ci1;
    
    int sock = socket(AF_INET,SOCK_STREAM,0);
    if (sock == -1 ) return false;
    char buffer[4096];
    int res;
    struct sockaddr_in addr;
    addr.sin_family=AF_INET;
    addr.sin_port=htons( atoi(parser.getValue("server-port").c_str()) );
    addr.sin_addr.s_addr=inet_addr(parser.getValue("server-ip").c_str());
    
    if (debug)
	std::cerr << parser.getValue("server-port") << "\n" <<  parser.getValue("server-ip") << "\n"
		  <<  int(atoi(parser.getValue("server-port").c_str())) << endl;

    
    if ( connect (sock,(sockaddr*)&addr,sizeof(addr)) == -1 ) return false;


    RS::ClientRequest request;
    request.setImageName(imgName);

    Helpers::Digests digest;
    memcpy(digest.digest, info.digest, 16);
    Helpers::digest2str(digest);
    
    request.setHeader("dig", digest.hexstr);


    string msg = request.getLines();
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
	    return false; // fatal error (but only in this thread)
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

    shutdown(sock,SHUT_WR);

    RS::Parser rsparser;
    
    
    for(;;) {
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
	r = rsparser.parseHeader(sock, &rd, &er);
	if (r <= 0 || sock == -1) break; 
	usleep(1);
    }
    
    // check wether the socket is valid
    if (sock == -1) {
	return false;
    }
    
    
    
    RS::ClientResponse response(rsparser);
    if (response.getStatus() != "200" ) {
	cerr << "Problem with the request (getImageInfo):" << endl;
	cerr << "\t" << response.getHeader("error") << endl;
	return false;
    }    

    char fname[] = "/tmp/df98432.\\arwe.XXXXXXX";
    mkstemp(fname);
    int fdtmp = open(fname, O_WRONLY);
    std::string rem = rsparser.getRemainingData();
    write(fdtmp, rem.data(), rem.size() );
    while ( (res = read(sock,buffer,4096)) > 0 ) {
	write(fdtmp, buffer, res);
    }
    close(fdtmp);
    shutdown(sock,SHUT_RD);
    close(sock);
    
    ifstream f(fname);
    f >> info;
    f.close();
    unlink(fname);
    return true;
}


bool sendRequest(ConfParser::ConfigParser &parser,int from, int to, const ImageInfo& info)
{
    int sock = socket(AF_INET,SOCK_STREAM,0);
    if (sock == -1 ) return false;
    struct sockaddr_in addr;
    addr.sin_family=AF_INET;
    addr.sin_port=htons( atoi(parser.getValue("server-req-port").c_str()) );
    addr.sin_addr.s_addr=inet_addr(parser.getValue("server-ip").c_str());
    if ( connect (sock,(sockaddr*)&addr,sizeof(addr)) == -1 ) return false;
    

    if (debug)  cerr << "generating request" << endl;


    RS::ClientRequest request;
    request.setImageName(info.name);

//     Helpers::Digests digest;
//     memcpy(digest.digest, info.digest, 16);
//     Helpers::digest2str(digest);
//     request.setHeader("digest", digest.hexstr);
    
    {
	stringstream ss;
	string s;
	
	ss << from;
	ss >> s;
	request.setHeader("from", s);
    }
    {
	stringstream ss;
	string s;
	
	ss << to;
	ss >> s;
	request.setHeader("to", s);
    }
    string msg = request.getLines();
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
	    return false; // fatal error (but only in this thread)
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


    RS::Parser rsparser;
    if (debug)  cerr << "parsing response" << endl;
	
    
    for(;;) {
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
	r = rsparser.parseHeader(sock, &rd, &er);
	if (r <= 0 || sock == -1) break; 
	usleep(1);
    }
    
    // check wether the socket is valid
    if (sock == -1) {
	return false;
    }
    
    RS::ClientResponse response(rsparser);
    if (response.getStatus() != "200") {
	cerr << "Problem with the request:" << rsparser.getLines()[1] <<endl;
	cerr << "\t" << response.getHeader("error") << endl;
	return false;
    }    
    shutdown(sock,SHUT_RDWR);
    close(sock);
    if (debug)  cerr << "imgReq finished" << endl;
    return true;
    
}

void inline help(const char*progname) {
    printf("Backup-client\nUsage: %s [-h] [-f configile] -m imagename -t target.filename\n"
	   "\nOptions:\n"
	   "-h\n"
	   "  --help\t\tprints this page\n"
	   "-f fname,\n"
	   "   --config-file fname\tconfig file. Default is \"" VCFG "\"\n"
	   "-m imagename,\n"
	   "   --image imagename\tSet the name of the backup image. If it is invalid, the program\n"
	   "\t\t\tsilently exit\n"
	   "-t fname\n"
	   "   --targetfile fname\ttarget file. The requested backup image is saved into this file\n",
	   progname);
  
}

void printError(const std::string& error, const std::string& s, const int index){
    cerr << error << s << endl;
    cerr << setw(index) << "^" << endl;
}
bool validateConfig(const ConfParser::ConfigParser& parser)
{
    std::string s;
    int i,size,j,k;
    std::string err;

    // checking `server-ip'
    s = parser.getValue("server-ip");
    size = s.size();
    if (s == "") {
	cerr << "config file parser: `server-ip' line missing." << endl;
	return false;
    } else {
	err = "invalid ip address (`server-ip'): ";
	for (k=0,i=0; k != 4; ++k) {
	    for (j=i; i != size && isdigit(s[i]) ;++i);
	    if (s[i] != '.'  && k !=3)   {
		printError(err,s,err.size()+i+1);
		return false;
	    } else if (s[j] == '0' && i - j > 1 || i - j == 3 && s[j]>'2') {
		printError(err,s,err.size()+j+1);
		return false;
	    } else if (i - j > 3) {
		printError(err,s,err.size()+j+4);
		return false;
	    } else if (k == 3 && ( i == size && !isdigit(s[i-1]) || i != size ) ) {
		printError(err,s,err.size()+i+1);
		return false;

	    }
	    ++i;
	}
    }
    
    // checking `broadcast-port'
    s = parser.getValue("broadcast-port");
    size = s.size();
    if (s == "") {
	cerr << "config file parser: `broadcast-port' line missing." << endl;
	return false;
    } else {
	err = "invalid port number (`broadcast-port'): ";
	for (i=0; i != size && isdigit(s[i]) ;++i);
	if ( i == size && !isdigit(s[i-1]) || i != size || (k = atoi(s.c_str())) > 65535 || k <=0)  {
	    printError(err,s,err.size()+i+1);
	    return false;	    
	}	
    }  

    // checking `server-req-port'
    s = parser.getValue("server-req-port");
    size = s.size();
    if (s == "") {
	cerr << "config file parser: `server-req-port' line missing." << endl;
	return false;
    } else {
	err = "invalid port number (`server-req-port'): ";
	for (i=0; i != size && isdigit(s[i]) ;++i);
	if ( i == size && !isdigit(s[i-1]) || i != size || (k = atoi(s.c_str())) > 65535 || k <=0)  {
	    printError(err,s,err.size()+i+1);
	    return false;
	    
	}	
    }  

    // checking `server-port'
    s = parser.getValue("server-port");
    size = s.size();
    if (s == "") {
	cerr << "config file parser: `server-port' line missing." << endl;
	return false;
    } else {
	err = "invalid port number (`server-port'): ";
	for (i=0; i != size && isdigit(s[i]) ;++i);
	if ( i == size && !isdigit(s[i-1]) || i != size || (k = atoi(s.c_str())) > 65535 || k <=0)  {
	    printError(err,s,err.size()+i+1);
	    return false;
	    
	}	
    }
    return true;
}



/** EMACS **
 * Local variables:
 * mode: c++
 * c-basic-offset: 4
 * End:
 */
