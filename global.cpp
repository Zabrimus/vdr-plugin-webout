#include <vdr/tools.h>
#include <vdr/plugin.h>
#include "global.h"

cString sendSVDRPCommand(const char *name, bool prefix, const char *command, const char *option, int &reply_code) {
    reply_code = 900;

    cPlugin *plugin;
    for (int i = 0; (plugin = cPluginManager::GetPlugin(i)) != nullptr; i++) {
        bool found;
        if (prefix) {
            found = strncmp(plugin->Name(), name, strlen(name)) == 0;
        } else {
            found = strcmp(plugin->Name(), name) == 0;
        }

        if (found) {
            cString result = plugin->SVDRPCommand(command, option, reply_code);
            return result;
        }
    }

    return nullptr;
}
