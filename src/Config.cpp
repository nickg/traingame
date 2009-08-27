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
#include <fstream>

#include <boost/lexical_cast.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/variant.hpp>

namespace {
   typedef tuple<const char*, IConfig::Option> Default;
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
   const IConfig::Option& get(const string& aKey) const;
   void flush();
   
private:
   friend class boost::serialization::access;

   template <class Archive>
   void serialize(Archive& anArchive, const unsigned int aVersion);
   
   typedef map<string, IConfig::Option> ConfigMap;
   ConfigMap myConfigMap;

   string myConfigFile;
   bool amDirty;
};

BOOST_CLASS_VERSION(Config, 1);

// Read the config file from disk
Config::Config()
   : amDirty(true)
{
   for (Default* d = theDefaults; *::get<0>(*d) != '\0'; d++)
      myConfigMap[::get<0>(*d)] = ::get<1>(*d);

   myConfigFile = "/tmp/config";

   log() << "Reading config from " << myConfigFile;

   ifstream ifs(myConfigFile.c_str());
   if (ifs.good()) {   
      boost::archive::text_iarchive ia(ifs);
      ia >> *this;
   }
   else
      warn() << "Failed to open config file " << myConfigFile;
}

Config::~Config()
{
   flush();
}

template <class Archive>
void Config::serialize(Archive& anArchive, const unsigned int aVersion)
{
   anArchive & myConfigMap;
}

// Write the config file back to disk
void Config::flush()
{
   if (!amDirty)
      return;
   
   log() << "Saving config to " << myConfigFile;

   ofstream ofs(myConfigFile.c_str());
   if (!ofs.good())
      throw runtime_error("Failed to write to config file");

   {
      boost::archive::text_oarchive oa(ofs);
      oa << *this;
   }

   amDirty = false;
}

// Read a single option
const IConfig::Option& Config::get(const string& aKey) const
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

