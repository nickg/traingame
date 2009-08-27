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

#include "IConfig.hpp"
#include "ILogger.hpp"

#include <map>
#include <stdexcept>

#include <boost/lexical_cast.hpp>

namespace {
   typedef tuple<const char*, boost::any> Default;
   const Default END("", 0);
   
   // All valid options   
   Default theDefaults[] = {
      Default("XRes", 800),
      Default("YRes", 600),
      Default("NearClip", 0.1f),
      Default("FarClip", 70.0f),
      END
   };
}

// Concrete config file
class Config : public IConfig {
public:
   Config();
   ~Config();

   // IConfig interface
   const boost::any& get(const string& aKey) const;
   
private:
   typedef map<string, boost::any> ConfigMap;
   ConfigMap myConfigMap;
};

// Read the config file from disk
Config::Config()
{
   for (Default* d = theDefaults; *::get<0>(*d) != '\0'; d++)
      myConfigMap[::get<0>(*d)] = ::get<1>(*d);
}

// Write the config file back to disk
Config::~Config()
{

}

// Read a single option
const boost::any& Config::get(const string& aKey) const
{
   ConfigMap::const_iterator it = myConfigMap.find(aKey);
   if (it != myConfigMap.end())
      return (*it).second;
   else
      throw runtime_error("Bad config key " + aKey);
}

// Return the single config file instance
IConfigPtr getConfig()
{
   static IConfigPtr cfg(new Config);

   return cfg;
}

