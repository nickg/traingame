//
//  Copyright (C) 2011  Nick Gasson
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

#include "Paths.hpp"
#include "ILogger.hpp"

#include <stdexcept>
#include <sstream>

#include <boost/optional.hpp>

using namespace boost::filesystem;

#ifdef WIN32
#error "Implement path functions for Win32"
#endif

// The UNIX code is based on the XDG standard
// See: http://standards.freedesktop.org/basedir-spec/latest/

static void xdg_dir(ostream& os, const char* env, const char *def)
{
   const char* env_value = getenv(env);
   if (env_value == NULL || *env_value == '\0') {
      warn() << env << " not set: using ~/" << def;

      const char* home = getenv("HOME");
      if (home == NULL)
         throw runtime_error("$HOME not set");

      os << home << "/" << def;
   }
   else
      os << env_value;
}

const path& get_config_dir()
{
   static boost::optional<path> cached_path;

   if (!cached_path) {
      ostringstream ss;
      xdg_dir(ss, "XDG_CONFIG_HOME", ".config");
      ss << "/" << PACKAGE;

      cached_path.reset(ss.str());
      create_directories(*cached_path);

   }
   
   return *cached_path;
}

const path& get_cache_dir()
{
   static boost::optional<path> cached_path;

   if (!cached_path) {
      ostringstream ss;
      xdg_dir(ss, "XDG_CACHE_HOME", ".cache");
      ss << "/" << PACKAGE;

      cached_path.reset(ss.str());
      create_directories(*cached_path);
   }

   return *cached_path;
}
