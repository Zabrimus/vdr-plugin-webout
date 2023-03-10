#ifndef GLOBAL_H
#define GLOBAL_H

#include <vdr/plugin.h>

#define DEBUG

#ifdef DEBUG
#define debug_plugin( x... ) \
  do  { printf("%-15s:%.3d %-50s: ",__FILE__,__LINE__,__PRETTY_FUNCTION__); \
        printf(x); \
        printf("\n"); \
        fflush(stdout); \
      } while(0)
#else
#define debug_plugin( x... )
#endif

cString sendSVDRPCommand(const char *plugin, bool prefix, const char *command, const char *option, int &reply_code);

#endif // GLOBAL_H