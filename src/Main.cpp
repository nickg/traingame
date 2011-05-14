//
//  Copyright (C) 2009-2011  Nick Gasson
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
#include "ITrackGraph.hpp"

#include <stdexcept>
#include <iostream>

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <signal.h>

using namespace boost::filesystem;

namespace {
   IWindowPtr window;

   // Options set from command line
   int new_map_width = 32;
   int new_map_height = 32;
   int run_cycles = 0;
   string map_file;
   string action;
}

static void dump_track_graph(IMapPtr map)
{
   make_track_graph(map)->write_dot_file(map_file + ".dot");
}

static void parse_options(int argc, char** argv)
{
   using namespace boost::program_options;

   options_description desc("Allowed options");
   desc.add_options()
      ("help", "Display this help message")
      ("width", value<int>(&new_map_width), "Set new map width")
      ("height", value<int>(&new_map_height), "Set new map height")
      ("action", value<string>(&action), "Either `play' or `edit'")
      ("map", value<string>(&map_file), "Name of map to load or create")
      ("cycles", value<int>(&run_cycles), "Run for N frames")
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

IWindowPtr get_game_window()
{
   return ::window;
}

int main(int argc, char** argv)
{
   parse_options(argc, argv);
   
   cout << PACKAGE << " " << VERSION << "." << PATCH << endl << endl
        << "Copyright (C) 2009-2011  Nick Gasson" << endl
        << "This program comes with ABSOLUTELY NO WARRANTY. "
        << "This is free software, and" << endl
        << "you are welcome to redistribute it under certain conditions. "
        << "See the GNU" << endl
        << "General Public Licence for details." << endl << endl;
   
   log() << "Program started";

   try {
      if (::action == "" || (::map_file == "" && ::action != "uidemo"))
         throw runtime_error("Usage: TrainGame (edit|play) [map]");
      
      init_resources();
      
      IConfigPtr cfg = get_config();

      bool no_window = action == "graph";

      if (!no_window)
         ::window = make_sdl_window();
      
      IScreenPtr screen;
      if (::action == "edit") {
         if (resource_exists(map_file, "maps"))
            screen = make_editor_screen(load_map(::map_file));
         else {
            screen = make_editor_screen(
               make_empty_map(::map_file, ::new_map_width, ::new_map_height));
         }
      }
      else if (::action == "play") {
         screen = make_game_screen(load_map(::map_file));
      }
      else if (::action == "uidemo") {
         screen = makeUIDemo();
      }
      else if (::action == "graph") {
         dump_track_graph(load_map(::map_file));
      }
      else
         throw runtime_error("Unrecognised command: " + ::action);

      if (::window)
         ::window->run(screen, run_cycles);

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
