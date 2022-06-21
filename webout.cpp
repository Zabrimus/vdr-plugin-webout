/*
 * webout.c: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#include <vdr/plugin.h>
#include "server.h"
#include "fpng.h"

static const char *VERSION        = "0.0.1";
static const char *DESCRIPTION    = "Enter description for 'webout' plugin";
static const char *MAINMENUENTRY  = "Webout";

class cPluginWebout : public cPlugin {
private:
  // Add any member variables or functions you may need here.
  cWebOsdServer *osdServer;

public:
  cPluginWebout(void);
  virtual ~cPluginWebout();
  virtual const char *Version(void) { return VERSION; }
  virtual const char *Description(void) { return DESCRIPTION; }
  virtual const char *CommandLineHelp(void);
  virtual bool ProcessArgs(int argc, char *argv[]);
  virtual bool Initialize(void);
  virtual bool Start(void);
  virtual void Stop(void);
  virtual void Housekeeping(void);
  virtual void MainThreadHook(void);
  virtual cString Active(void);
  virtual time_t WakeupTime(void);
  virtual const char *MainMenuEntry(void) { return MAINMENUENTRY; }
  virtual cOsdObject *MainMenuAction(void);
  virtual cMenuSetupPage *SetupMenu(void);
  virtual bool SetupParse(const char *Name, const char *Value);
  virtual bool Service(const char *Id, void *Data = NULL);
  virtual const char **SVDRPHelpPages(void);
  virtual cString SVDRPCommand(const char *Command, const char *Option, int &ReplyCode);
  };

cPluginWebout::cPluginWebout(void)
{
  // Initialize any member variables here.
  // DON'T DO ANYTHING ELSE THAT MAY HAVE SIDE EFFECTS, REQUIRE GLOBAL
  // VDR OBJECTS TO EXIST OR PRODUCE ANY OUTPUT!
}

cPluginWebout::~cPluginWebout()
{
  // Clean up after yourself!
}

const char *cPluginWebout::CommandLineHelp(void)
{
  // Return a string that describes all known command line options.
  return NULL;
}

bool cPluginWebout::ProcessArgs(int argc, char *argv[])
{
  // Implement command line argument processing here if applicable.
  return true;
}

bool cPluginWebout::Initialize(void)
{
  // Initialize any background activities the plugin shall perform.
  osdServer = new cWebOsdServer();

  fpng::fpng_init();

  return true;
}

bool cPluginWebout::Start(void)
{
  // Start any background activities the plugin shall perform.
  osdServer->Start();
  return true;
}

void cPluginWebout::Stop(void)
{
  // Stop any background activities the plugin is performing.
}

void cPluginWebout::Housekeeping(void)
{
  // Perform any cleanup or other regular tasks.
}

void cPluginWebout::MainThreadHook(void)
{
  // Perform actions in the context of the main program thread.
  // WARNING: Use with great care - see PLUGINS.html!
}

cString cPluginWebout::Active(void)
{
  // Return a message string if shutdown should be postponed
  return NULL;
}

time_t cPluginWebout::WakeupTime(void)
{
  // Return custom wakeup time for shutdown script
  return 0;
}

cOsdObject *cPluginWebout::MainMenuAction(void)
{
  // Perform the action when selected from the main VDR menu.
  return NULL;
}

cMenuSetupPage *cPluginWebout::SetupMenu(void)
{
  // Return a setup menu in case the plugin supports one.
  return NULL;
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

const char **cPluginWebout::SVDRPHelpPages(void)
{
  // Return help text for SVDRP commands this plugin implements
  return NULL;
}

cString cPluginWebout::SVDRPCommand(const char *Command, const char *Option, int &ReplyCode)
{
  // Process SVDRP commands this plugin implements
  return NULL;
}

VDRPLUGINCREATOR(cPluginWebout); // Don't touch this!
