/*  config.cpp - These functions for config file parsing, checking etc

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


#include <iostream>
#include <iomanip>
#include <fstream>
//#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>


//#include <vector>
#include "md5.h"
#define HAS_CONFIGURATION
#include "config.hh"
#undef HAS_CONFIGURATION
#include "checksums.hh"

bool validateConfig(const ConfParser::ConfigParser& parser);
void loadConfig();

using std::cerr;
using std::endl;
using std::setw;

bool g_hasConfig = false;

extern char* argv0;


bool parseConfig(const std::string& cfgfile)
{
    ConfParser::ConfigParser parser;

    if (!parser.parse(cfgfile)) {
	std::string s;
	s = argv0;
	s += ": invalid configuration file format:";
	s += cfgfile;
	perror(s.c_str());
	if (!g_hasConfig)
	    exit(1);
	else return false;
    } else if (! validateConfig(parser) ) {
	if (!g_hasConfig)
	    exit(1);
	else return false;
    }
    {
	RWLock::Writer w = configuration.lock.getWriter();
	// now we have a valid config.
	if ( !configuration.data )
	    configuration.data = new Configuration;
	configuration.data->parser = parser;
	loadConfig();
	g_hasConfig = true;
    }
    return true;
}

int split(const std::string& buf,std::vector<std::string>& vec)
{
  
    //parsing this line
    std::string::const_iterator p1=buf.begin(),p2;
    std::string str;
    vec.clear();
  
    while(1) {
	while(p1 != buf.end() && (*p1==' ' || *p1=='\t'))
	    ++p1; // first non-whitespace char
	if (p1 == buf.end()) break;

	p2=p1;
	while (p2 != buf.end() && *p2 != ' '&& *p2 != '\t')  {  // a valid name
	    ++p2; 
	}
      
	str.assign(p1,p2);
      
	std::vector<std::string>::const_iterator p = find(vec.begin(),vec.end(),str); // for multiple entries/
	if (p != vec.end())
	    return 1;
	p1 = p2;
	vec.push_back(str);
    }

    return 0;
}


void loadConfig()
{
    ImageInfoList lst, list;
    std::ifstream f((configuration.data->parser.getValue("cache-directory") + "/imageinfo").c_str());
    if (!f.fail()) {
	f >> lst;
	f.close();
    } else {
	lst = configuration.data->infoList;
    }

    std::vector<std::string> images;
    split(configuration.data->parser.getValue("names"), images);
  
    // all image names into lst
    bool found;
    for (size_t i=0; i!= images.size(); ++i) {
	found = false;
	for (size_t j=0; j!= lst.size() && !found; ++j) {
	    found = lst[j].name == images[i];
	}
	if ( ! found ) {
	    ImageInfo imgInfo;
	    imgInfo.name = images[i];
	    md5_buffer(images[i].c_str(), images[i].size(), imgInfo.digest);
	    lst.add(imgInfo);
	}
    }
  
    // check all available images
    ImageInfo img;
    for (size_t i=0; i!= lst.size(); ++i) {
	// available?
	found = false;
	for (size_t j =0; !found && j!= images.size(); ++j) {
	    found = lst[i].name == images[j];
	}
	std::string fname = configuration.data->parser.getValueDefault(lst[i].name, "none");
	if (fname == "none")
	    continue;
	int fd;
	if ((fd=open(fname.c_str(),O_LARGEFILE)) == -1) {
	    perror("loadConfig: open");
	    continue;
	}
	off64_t len  = lseek64(fd,0,SEEK_END);    
	img = lst[i];
	img.size = len;

	img.blockSize = 1024*1024;
	img.lastBlockSize = len % (off64_t) (img.blockSize);
	if (img.lastBlockSize == 0)
	    img.lastBlockSize = img.blockSize;
	struct stat  st;
	fstat(fd, &st);
	img.lastModified = st.st_mtime;
	// update required?
    
	if (img.size != lst[i].size || st.st_mtime > lst[i].lastModified) {
	    calculateChecksum(fd, &img.checksums);
	} 
	close(fd);
	list.add(img);
    }
    configuration.data->infoList = list;
    std::ofstream file((configuration.data->parser.getValue("cache-directory") + "/imageinfo").c_str());
    if (!file.fail()) {
	file << configuration.data->infoList;
	file.close();
    }
}


void printError(const std::string&error, const std::string& s, const int index){
    cerr << error << s << endl;
    cerr << setw(index) << "^" << endl;
}

bool validateConfig(const ConfParser::ConfigParser& parser)
{
    std::string s;
    int i,size,/*j,*/k;
    std::string err;
  
    /*
    // checking `broadcast'
    s = parser.getValue("broadcast");
    size = s.size();
    if (s == "") {
	cerr << "config file parser: `broadcast' line missing." << endl;
	return false;
    } else {
	err = "invalid broadcast address (`broadcast'): ";
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
    */ 

    // checking `port'
    s = parser.getValue("port");
    size = s.size();
    if (s == "") {
	cerr << "config file parser: `port' line missing." << endl;
	return false;
    } else {
	err = "invalid port number (`port'): ";
	for (i=0; i != size && isdigit(s[i]) ;++i);
	if ( i == size && !isdigit(s[i-1]) || i != size || (k = atoi(s.c_str())) > 65535 || k <=0)  {
	    printError(err,s,err.size()+i+1);
	    return false;	    
	}	
    }  
    /*
    // checking `info-port'
    s = parser.getValue("info-port");
    size = s.size();
    if (s == "") {
	cerr << "config file parser: `info-port' line missing." << endl;
	return false;
    } else {
	err = "invalid port number (`info-port'): ";
	for (i=0; i != size && isdigit(s[i]) ;++i);
	if ( i == size && !isdigit(s[i-1]) || i != size || (k = atoi(s.c_str())) > 65535 || k <=0)  {
	    printError(err,s,err.size()+i+1);
	    return false;	    
	}	
    }

    // checking `delay-count'
    s = parser.getValue("delay-count");
    size = s.size();
    if (s == "") {
	cerr << "config file parser: `delay-count' line missing." << endl;
	return false;
    } else {
	err = "invalid number (`delay-count'): ";
	for (i=0; i != size && isdigit(s[i]) ;++i);
	if ( i == size && !isdigit(s[i-1]) || i != size || (k = atoi(s.c_str())) > 1000000 || k <=0)  {
	    printError(err,s,err.size()+i+1);
	    return false;	    
	}	
    }
    */
    
    // checking `names'
    s = parser.getValue("names");
    if (s == "") {
	cerr << "config file parser: `names' line missing." << endl;
	return false;
    }
    cerr << "WARNING: if all available image names (via `names') have invalid file names, the server is useless." << endl;
    return true;
}

/** EMACS **
 * Local variables:
 * mode: c++
 * c-basic-offset: 4
 * End:
 */
