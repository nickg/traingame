//
//  Copyright (C) 2009-2010  Nick Gasson
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include "IWindow.hpp"
#include "ILogger.hpp"
#include "GameScreens.hpp"
#include "IResource.hpp"
#include "IConfig.hpp"

#include <stdexcept>
#include <iostream>

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <signal.h>

using namespace boost::filesystem;

namespace {
   IWindowPtr window;
   struct sigaction oldSIGINT;

   void clearSignalHandlers()
   {
      sigaction(SIGINT, &oldSIGINT, NULL);
   }
        
   void SIGINT_handler(int unused)
   {
      log() << "Caught SIGINT";
      ::window->quit();
      
      clearSignalHandlers();
   }
   
   void setupSignalHandlers()
   {
      struct sigaction sa;
      sa.sa_handler = SIGINT_handler;
      sa.sa_flags = 0;
      sigemptyset(&sa.sa_mask);

      sigaction(SIGINT, &sa, &oldSIGINT);
   }

   // Options set from command line
   int newMapWidth = 32;
   int newMapHeight = 32;
   string mapFile;
   string action;

   void parseOptions(int argc, char** argv)
   {
      using namespace boost::program_options;

      options_description desc("Allowed options");
      desc.add_options()
         ("help", "Display this help message")
         ("width", value<int>(&newMapWidth), "Set new map width")
         ("height", value<int>(&newMapHeight), "Set new map height")
         ("action", value<string>(&action), "Either `play' or `edit'")
         ("map", value<string>(&mapFile), "Name of map to load or create")
         ;

      positional_options_description p;
      p.add("action", 1).add("map", 2);

      variables_map vm;
      try {
         store(command_line_parser(argc, argv)
            .options(desc).positional(p).run(), vm);
      }
      catch (const exception& e) {
         cerr << "Error: " << e.what() << endl;
         exit(EXIT_FAILURE);
      }

      notify(vm);

      if (vm.count("help")) {
         cout << desc << endl;
         exit(EXIT_SUCCESS);
      }

   }
}

IWindowPtr getGameWindow()
{
   return ::window;
}

int main(int argc, char** argv)
{
   parseOptions(argc, argv);
   
   cout << PACKAGE << " " << VERSION << "." << PATCH << endl << endl
	<< "Copyright (C) 2009-2010  Nick Gasson" << endl
	<< "This program comes with ABSOLUTELY NO WARRANTY. "
	<< "This is free software, and" << endl
	<< "you are welcome to redistribute it under certain conditions. "
	<< "See the GNU" << endl
	<< "General Public Licence for details." << endl << endl;
   
   log() << "Program started";

   setupSignalHandlers();

   try {
      if (::action == "" || ::mapFile == "")
	 throw runtime_error("Usage: TrainGame (edit|play) [map]");
      
      initResources();
      
      IConfigPtr cfg = getConfig();
      
      ::window = makeSDLWindow();

      IScreenPtr screen;
      if (::action == "edit") {
	 if (resourceExists(mapFile, "maps"))
	    screen = makeEditorScreen(loadMap(::mapFile));
	 else {
	    screen = makeEditorScreen(
               makeEmptyMap(::mapFile, ::newMapWidth, ::newMapHeight));
	 }
      }
      else if (::action == "play") {
	 screen = makeGameScreen(loadMap(::mapFile));
      }
      else if (::action == "uidemo") {
	 screen = makeUIDemo();
      }
      else
	 throw runtime_error("Unrecognised command: " + ::action);
         
      ::window->run(screen);

      cfg->flush();
   }
   catch (const runtime_error& e) {
      error() << "Fatal: " << e.what();

#ifdef WIN32
      MessageBox(NULL, e.what(), "Fatal error", MB_ICONERROR | MB_OK);
#endif
   }

   log() << "Finished";   
   return 0;
}
