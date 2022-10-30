/*
 * webout.c: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#include <vdr/plugin.h>
#include "server.h"
#include "webdevice.h"
#include "fpng.h"

static const char *VERSION        = "0.0.1";
static const char *DESCRIPTION    = "Enter description for 'webout' plugin";
static const char *MAINMENUENTRY  = "Webout";


class cPluginWebout : public cPlugin {
private:
  // Add any member variables or functions you may need here.
  cWebOsdServer *osdServer;

public:
  cPluginWebout();
  ~cPluginWebout() override;
  const char *Version() override { return VERSION; }
  const char *Description() override { return DESCRIPTION; }
  const char *CommandLineHelp() override;
  bool ProcessArgs(int argc, char *argv[]) override;
  bool Initialize() override;
  bool Start() override;
  void Stop() override;
  void Housekeeping() override;
  void MainThreadHook() override;
  cString Active() override;
  time_t WakeupTime() override;
  const char *MainMenuEntry() override { return MAINMENUENTRY; }
  cOsdObject *MainMenuAction() override;
  cMenuSetupPage *SetupMenu() override;
  bool SetupParse(const char *Name, const char *Value) override;
  bool Service(const char *Id, void *Data) override;
  const char **SVDRPHelpPages() override;
  cString SVDRPCommand(const char *Command, const char *Option, int &ReplyCode) override;
  };

cPluginWebout::cPluginWebout()
{
  // Initialize any member variables here.
  // DON'T DO ANYTHING ELSE THAT MAY HAVE SIDE EFFECTS, REQUIRE GLOBAL
  // VDR OBJECTS TO EXIST OR PRODUCE ANY OUTPUT!
}

cPluginWebout::~cPluginWebout()
{
  // Clean up after yourself!
}

const char *cPluginWebout::CommandLineHelp()
{
  // Return a string that describes all known command line options.
  return nullptr;
}

bool cPluginWebout::ProcessArgs(int argc, char *argv[])
{
  // Implement command line argument processing here if applicable.
  return true;
}

bool cPluginWebout::Initialize()
{
  // Initialize any background activities the plugin shall perform.
  osdServer = new cWebOsdServer();
  new cWebDevice();

  fpng::fpng_init();

  tr("dummy");

  return true;
}

bool cPluginWebout::Start()
{
  // Start any background activities the plugin shall perform.
  osdServer->Start();
  return true;
}

void cPluginWebout::Stop()
{
  // Stop any background activities the plugin is performing.
}

void cPluginWebout::Housekeeping()
{
  // Perform any cleanup or other regular tasks.
}

void cPluginWebout::MainThreadHook()
{
  // Perform actions in the context of the main program thread.
  // WARNING: Use with great care - see PLUGINS.html!
}

cString cPluginWebout::Active()
{
  // Return a message string if shutdown should be postponed
  return nullptr;
}

time_t cPluginWebout::WakeupTime()
{
  // Return custom wakeup time for shutdown script
  return 0;
}

cOsdObject *cPluginWebout::MainMenuAction()
{
  // Perform the action when selected from the main VDR menu.
  return nullptr;
}

cMenuSetupPage *cPluginWebout::SetupMenu()
{
  // Return a setup menu in case the plugin supports one.
  return nullptr;
}

bool cPluginWebout::SetupParse(const char *Name, const char *Value)
{
  // Parse your own setup parameters and store their values.
  return false;
}

bool cPluginWebout::Service(const char *Id, void *Data)
{
  // Handle custom service requests from other plugins
  return false;
}

const char **cPluginWebout::SVDRPHelpPages()
{
  // Return help text for SVDRP commands this plugin implements
  return nullptr;
}

cString cPluginWebout::SVDRPCommand(const char *Command, const char *Option, int &ReplyCode)
{
  // Process SVDRP commands this plugin implements
  return nullptr;
}

VDRPLUGINCREATOR(cPluginWebout); // Don't touch this!
