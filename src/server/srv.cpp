#include <csignal>
#include <string>
//#include <unistd.h>
#include <getopt.h>
//#include "types/checksuminfo.hh"

bool parseConfig(const std::string& str);
void info_thread();
void config_thread();
void backup_thread();

bool g_running = true;
const char*const VCFG="/etc/vissza/server.conf";
std::string cfgfile = VCFG;
char * argv0;

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
  
    parseConfig(cfgfile);

}



/** EMACS **
 * Local variables:
 * mode: c++
 * c-basic-offset: 4
 * End:
 */
