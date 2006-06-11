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
#include "confparser.h"
#include "bitvect.h"
#include "md5.h"


//config fajl
#define VCFG "/etc/vissza/client.conf"
//egy broadcast csomagban az adat merete bajtban
#define BUFFER 1024
#define SRV_INFOPORT 8767
#define BUFSIZE 4096
#define RECVBUFSIZE 1024

#define PERROR(a) {  if ( debug ) {  printf("In %s at line %d: ",__FILE__,__LINE__);  fflush (stdout); } perror (a); }


using namespace std;


typedef struct image {
    std::string name;
    char    digest[16];
    off64_t size;
} image_t;

typedef std::vector<image> imglist_t;


typedef struct {
    unsigned char digest[16];
    unsigned int index;
    int checksum;
    unsigned char data[1024];
    
} SENDDATA;


imglist_t imglist;
bool interactive = false, debug = false;
std::string selecting;
std::string targetfile;

const struct option long_options[] = {
    {"config-file", 1, 0, 'f'},
    {"debug", 0, 0, 'd'},
    {"target-file", 1, 0, 't'},
    {"interactive", 0, 0, 'i'},
    {"image", 1, 0, 'm'},
    {"help",0,0,'h'},
    {0, 0, 0, 0}
};


bool inline getImageList(const PantherSW::ConfigParser& parser);
void inline selectImage(image_t *selected);
void inline help(const char* progname);
bool sendRequest(PantherSW::ConfigParser &parser,int from, int to,const image_t* selected);
bool validateConfig(const PantherSW::ConfigParser& parser);


int main(int argc,char* argv[]){
    
    PantherSW::ConfigParser parser;
    std::string cfgfile = VCFG;
    bool noninteractive = false;
    
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
		if (selecting.size()) noninteractive = true;
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
    if (! noninteractive && ! interactive ) {
	fprintf(stderr,"Image is not selected. Try -h for details\n");
	exit(5);
    } else if ( interactive && noninteractive) {
	fprintf(stderr,"Multiple images selected. Try -h for details\n");
	exit(5);
    } else if ( ! targetfile.size() ) {
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
    
    
    int sbroadcast = socket(AF_INET,SOCK_DGRAM,0);
    if (sbroadcast == -1) {
	perror("broadcast socket");
	exit(1);
    }
    const int tru = 1;
    setsockopt(sbroadcast,SOL_SOCKET,SO_BROADCAST,&tru,sizeof(tru)); //BROADCAST engedelyezese
    
    struct sockaddr_in addrbr;
    addrbr.sin_family = AF_INET;
    addrbr.sin_port = htons( atoi(parser.getValue("broadcast-port").c_str()) ); //broadcast port
    addrbr.sin_addr.s_addr=INADDR_ANY;
    bind(sbroadcast, (sockaddr*)&addrbr, sizeof(addrbr));

    if ( ! getImageList(parser)) {
	perror("getImageList: connect");
	exit (2);
    }
    image_t selected; // selected image ....
    selected.size = 0; // for testing
    
    selectImage(&selected);
    
    if ( ! selected.size ) {
	fprintf(stderr,"%s: Invalid image name.\n",argv[0]);
	exit (1);	
    }
    
    std::string fname = targetfile;   // and the name of img's file
    unsigned end = (selected.size  + RECVBUFSIZE - 1) / RECVBUFSIZE; --end;
    
    if (debug) 
	cerr << "IMAGE:\t\t" << selected.name << endl
	     << "TARGETFILE\t" <<  targetfile << endl
	     << "FSIZE:\t\t" << selected.size << endl
	     << "COUNT:\t\t" << end << endl;

    struct timeval timeout = {6,0};
    fd_set fdRead;
    int retval;
    
    SENDDATA recvBuf;	
    PantherSW::BitVector ready(end+1);

    

    int fd = open64 (fname.c_str(), O_CREAT/*|O_EXCL*/|O_LARGEFILE|O_WRONLY, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    int checksum;
    
    if ( fd == -1 ) {
	PERROR("read");
	exit(1);
    }
    struct stat st;
    if (! fstat(fd ,&st )) {
	if ( S_ISREG(st.st_mode) ) {
	    cerr<< "Regular target file already exists. Truncating to 0 byte." << endl;
	    ftruncate(fd,0);
	}
    }
    
    for ( bool l = true; l;)
    {
	FD_ZERO(&fdRead);
	FD_SET(sbroadcast, &fdRead);
	
	retval = select(sbroadcast+1,&fdRead,NULL,NULL,&timeout);
	if ( debug ) {
	    cerr << "select retval: " << retval << endl;
	}
	if (retval >0) { // we may have new data
	    
	    if (FD_ISSET(sbroadcast, &fdRead)) { // this socket? or an other (lower) file descriptor?
		
		int res = recv(sbroadcast,&recvBuf,sizeof(recvBuf),MSG_WAITALL);
		if ( res == -1 )		{
		    PERROR("recv");
		    exit(2);
		}
		if (!strncmp((char*)selected.digest,(char*)recvBuf.digest,16) && recvBuf.index <= end &&
		    ! ready [recvBuf.index] )
		    // new part of the requested file
		{
		    
		    checksum = 0;	    
		    for (int i=0;i!=RECVBUFSIZE;++i)
			checksum += recvBuf.data[i]+1;
		    
		    if (checksum != recvBuf.checksum) {
			continue;
		    }
		    lseek64(fd,recvBuf.index * (long long)RECVBUFSIZE,SEEK_SET);

		    if ( debug ) cerr << "part: " <<  recvBuf.index << "; end: " << end << endl;

		    if (recvBuf.index != end) {
			if ( write(fd,recvBuf.data,RECVBUFSIZE) == -1 ) {
			    PERROR("write");
			    continue;	    
			    
			}
		    } else {
			off64_t count = selected.size;
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
		    ready.set(recvBuf.index);
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
		if ( ! sendRequest(parser,from,to,&selected) ) {
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



bool inline getImageList(const PantherSW::ConfigParser& parser)
{
    image_t imgcur;
    imglist.clear();
    
    //S//ocket sock;
    //if (!sock.create()) return false;
    
    std::string buf="",tmp;
    std::string::const_iterator ci,ci1;
    //if (! sock.connect(parser.getValue("server-ip"),atoi(parser.getValue("server-port").c_str())) ) {
    //return false;
    //}
    //while (sock.recv(tmp) >0) buf+= tmp;
    
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
    shutdown(sock,SHUT_WR);
    while ( (res = read(sock,buffer,4096)) > 0 ) {
	buf.append(buffer,res);
    }
    shutdown(sock,SHUT_RD);
    close(sock);
    ci = buf.begin();
    for (; ci != buf.end(); ++ci) {
	ci1 = ci;
	while(ci != buf.end() && *ci != ' ') ++ci;
	imgcur.name.assign(ci1,ci);
	md5_buffer(imgcur.name.c_str(),imgcur.name.size(),imgcur.digest);
	
	while (ci != buf.end() && *ci == ' ') ++ci;
	
	ci1 = ci;
	while (ci != buf.end() && *ci != '\n') ++ci;
	tmp.assign(ci1,ci);
	memcpy(&imgcur.size,tmp.c_str(),sizeof(off64_t));
	
	imglist.push_back(imgcur);
	if (ci == buf.end()) break;
    }
    return true;
}



void selectImageInteractive(image_t *selected) {
    int size = imglist.size();
    std::string line;
    int index;
    char ch = 'x';
    
 print_menu:    
    // print the menu
    printf("\n -1. List again.\n  0. Exit\n");
    for (int i=0; i!=size;++i) {
	printf("%3d. %s\n",i+1,imglist[i].name.c_str());
    }
    
    // while it is invalid, request for a new line
    for  (;;)
    {
	printf("\nChoose one (-1 - %d): ",size);
	fflush(stdout);
	getline(cin,line);
	index = atoi(line.c_str());
	if ( ! index && line != "0" ) continue; // not a number
	if ( index > size || index < -1 ) continue; // invalid number
	break;
    }
    if ( index == -1 ) goto print_menu;
    if ( index == 0 ) {
	printf ("Exiting.\n\n");
	exit (0);
    }
    
    --index; // because 0 means exit
    printf ("\nYou chose %s. Is it ok? (y/n)", imglist[index].name.c_str()); fflush(stdout);
    while  ( (ch=getchar()) != 'y' && ch != 'Y' && ch != 'N' && ch != 'n') {write(1,"Please answer y or n! ",22);}
    
    if ( ch == 'n' or ch == 'N' ) goto print_menu;
    
    // now image is selected, identified by variable 'index'
    
    selected->name = imglist[index].name;
    memcpy(selected->digest,imglist[index].digest,16);
    selected->size = imglist[index].size;
}


void inline selectImage(image_t* selected)
{
  if ( interactive ) {
      selectImageInteractive( selected );
  } else {
      int size = imglist.size();
      for (int i = 0; i != size; ++i) {
	  if (imglist[i].name == selecting) {
	      selected->name = selecting;
	      memcpy(selected->digest,imglist[i].digest,16);
	      selected->size = imglist[i].size;
	      break;
	  }
      }
  }
}




bool sendRequest(PantherSW::ConfigParser &parser,int from, int to, const image_t* selected)
{
    int sock = socket(AF_INET,SOCK_STREAM,0);
    if (sock == -1 ) return false;
    char buffer[4096];
    int res;
    int count;
    int sent = 0;
    struct sockaddr_in addr;
    addr.sin_family=AF_INET;
    addr.sin_port=htons( atoi(parser.getValue("server-req-port").c_str()) );
    addr.sin_addr.s_addr=inet_addr(parser.getValue("server-ip").c_str());
    
    count = selected->name.size();
    memcpy(buffer,selected->name.c_str(),count);
    buffer[count] = ' '; ++ count;
    
    memcpy(buffer+count,&from,sizeof(from));
    count += sizeof(from);

    memcpy(buffer+count,&to,sizeof(to));
    count += sizeof(to);
    buffer[count] = '\n'; ++ count;
    if (debug)  { 
	printf("sendrequest - count %d\n",count);
	write(1,buffer,count);
    }
    if ( connect (sock,(sockaddr*)&addr,sizeof(addr)) == -1 ) return false;
    shutdown(sock,SHUT_RD);

    while ( sent != count ) {
	res = write(sock,buffer+sent,count-sent);
	if (res < 0) return false;
	sent += res;
    }
    shutdown(sock,SHUT_WR);
    close(sock);
    return true;
}

void inline help(const char*progname) {
  printf("Backup-client\nUsage: %s [-h] [-f configile] [-i|-m imagename] -t target.filename\n"
	 "\nOptions:\n"
	 "-h\n"
	 "  --help\t\tprints this page\n"
	 "-f fname,\n"
	 "   --config-file fname\tconfig file. Default is \"" VCFG "\"\n"
	 "-i\n"
	 "   --interactive\tInteractive. A menu is shown to select the backup image to use\n"
	 "\t\t\tIt cannot be used with -m option\n"
	 "-m imagename,\n"
	 "   --image imagename\tSet the name of the backup image. If it is invalid, the program\n"
	 "\t\t\tsilently exit. Cannot be used with -i option\n"
	 "-t fname\n"
	 "   --targetfile fname\ttarget file. The requested backup image is saved into this file\n",
	 progname);
  
}

void printError(const std::string&error, const std::string& s, const int index){
    cerr << error << s << endl;
    cerr << setw(index) << "^" << endl;
}
bool validateConfig(const PantherSW::ConfigParser& parser)
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
