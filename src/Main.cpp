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
#include <signal.h>

using namespace boost::filesystem;

namespace {
   IWindowPtr window;

   void SIGINT_handler(int unused)
   {
      log() << "Caught SIGINT";
      ::window->quit();
   }
   
   void setupSignalHandlers()
   {
      struct sigaction sa;
      sa.sa_handler = SIGINT_handler;
      sa.sa_flags = 0;
      sigemptyset(&sa.sa_mask);

      sigaction(SIGINT, &sa, NULL);
   }
}

IWindowPtr getGameWindow()
{
   return ::window;
}

int main(int argc, char** argv)
{
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
#ifndef WIN32
      if (argc != 2 && argc != 3)
         throw runtime_error("Usage: TrainGame (edit|play) [map]");
      initResources();
      
      const string mapFile(argc == 3 ? argv[2] : "");
      const string cmd(argv[1]);
#else
      // For ease of debugging, specify a default map 
      const string mapFile("figure8");
      const string cmd("play");
#endif   // #ifndef WIN32
      
      IConfigPtr cfg = getConfig();
      
      ::window = makeSDLWindow();
      
      IScreenPtr screen;
      if (cmd == "edit") {
         if (resourceExists(mapFile, "maps"))
            screen = makeEditorScreen(loadMap(mapFile));
         else {
            screen = makeEditorScreen(makeEmptyMap(mapFile, 32, 32));
         }
      }
      else if (cmd == "play") {
         screen = makeGameScreen(loadMap(mapFile));
      }
      else if (cmd == "uidemo") {
         screen = makeUIDemo();
      }
      else
         throw runtime_error("Unrecognised command: " + cmd);
         
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
