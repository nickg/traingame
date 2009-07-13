//
//  Copyright (C) 2009  Nick Gasson
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
#include "Resources.hpp"

#include <stdexcept>

#include <boost/filesystem.hpp>

using namespace boost::filesystem;

namespace {
   IWindowPtr theWindow;
}

IWindowPtr getGameWindow()
{
   return theWindow;
}

int main(int argc, char** argv)
{   
   log() << "Program started";

   try {
#ifndef WIN32
      if (argc != 3)
         throw runtime_error("Usage: TrainGame (edit|play) [map]");

      initResources();
      
      const string mapFile(argv[2]);
      const string cmd(argv[1]);
#else
      // For ease of debugging, specify a default map 
      const string mapFile("maps\\figure8.xml");
      const string cmd("play");
#endif   // #ifndef WIN32
      IScreenPtr screen;
      if (cmd == "edit") {
         theWindow = makeFLTKWindow("Train Game Editor", addEditorGUI);
         if (exists(mapFile))
            screen = makeEditorScreen(loadMap(mapFile), mapFile);
         else
            screen = makeEditorScreen(makeEmptyMap(64, 64), mapFile);
      }
      else if (cmd == "play") {
         theWindow = makeSDLWindow();
         screen = makeGameScreen(loadMap(mapFile));
      }
      else
         throw runtime_error("Unrecognised command: " + cmd);
         
      theWindow->run(screen);
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
