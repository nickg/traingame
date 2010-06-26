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
#include "IXMLParser.hpp"
#include "XMLBuilder.hpp"

#include <map>
#include <stdexcept>
#include <fstream>
#include <cstdlib>
#include <typeinfo>

#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>

namespace {
   typedef tuple<const char*, IConfig::Option> Default;
   
   // All valid options   
   const Default defaults[] = {
      Default("XRes", 800),
      Default("YRes", 600),
      Default("NearClip", 0.1f),
      Default("FarClip", 70.0f),
   };
}

// Concrete config file
class Config : public IConfig, public IXMLCallback {
public:
   Config();
   ~Config();

   // IConfig interface
   const IConfig::Option& get(const string& a_key) const;
   void set(const string& a_key, const IConfig::Option& a_value);
   void flush();

   // IXMLCallback interface
   void start_element(const string& local_name, const AttributeSet& attrs);
   void text(const string& local_name, const string& a_string);
   
private:
   static string config_file_name();

   template <class T>
   void set_from_string(const string& a_key, const string& a_string);
   
   template <class T>
   void bind_next_option(const AttributeSet& attrs);
   
   typedef map<string, IConfig::Option> ConfigMap;
   ConfigMap config_map;

   string config_file;
   bool am_dirty;

   // Used by the XML parser
   string my_active_option;
};

// Read the config file from disk
Config::Config()
   : am_dirty(false)
{
   for (size_t i = 0; i < sizeof(::defaults)/sizeof(Default); i++)
      config_map[::get<0>(::defaults[i])] = ::get<1>(::defaults[i]);

   config_file = config_file_name();

   if (boost::filesystem::exists(config_file)) {
      log() << "Reading config from " << config_file;

      IXMLParserPtr parser = makeXMLParser("schemas/config.xsd");
      parser->parse(config_file, *this);

      // Ignore all the set() calls made by the XML parser
      am_dirty = false;
   }
   else {
      warn() << "Config file not present: " << config_file;

      // Write a default config file when we exit
      am_dirty = true;
   }
}

Config::~Config()
{
   flush();
}

// Find the config file location on this platform
string Config::config_file_name()
{   
#ifdef WIN32
   throw runtime_error("TODO: find config dir on Win32");

#else  // #ifdef WIN32

   // This is based on the XDG standard
   // See: http://standards.freedesktop.org/basedir-spec/latest/

   ostringstream ss;
   
   char* XDG_CONFIG_HOME = getenv("XDG_CONFIG_HOME");
   if (XDG_CONFIG_HOME == NULL || *XDG_CONFIG_HOME == '\0') {
      warn() << "XDG_CONFIG_HOME not set: using ~/.config";

      char* HOME = getenv("HOME");
      if (HOME == NULL)
         throw runtime_error("$HOME not set");

      ss << HOME << "/.config";
   }
   else
      ss << XDG_CONFIG_HOME;

   ss << "/" PACKAGE "/config.xml";
            
   return ss.str();

#endif // #ifdef WIN32
}

void Config::start_element(const string& local_name, const AttributeSet& attrs)
{
   if (local_name == "option")
      attrs.get("name", my_active_option);
}

void Config::text(const string& local_name, const string& a_string)
{
   if (local_name == "string")
      set_from_string<string>(my_active_option, a_string);
   else if (local_name == "int")
      set_from_string<int>(my_active_option, a_string);
   else if (local_name == "bool")
      set_from_string<bool>(my_active_option, a_string);
   else if (local_name == "float")
      set_from_string<float>(my_active_option, a_string);
}

template <class T>
void Config::set_from_string(const string& a_key, const string& a_string)
{
   config_map[a_key] = boost::lexical_cast<T>(a_string);
}

// Write the config file back to disk
void Config::flush()
{
   using namespace boost::filesystem;
   using namespace boost;
   
   if (!am_dirty)
      return;
   
   log() << "Saving config to " << config_file;

   create_directories(path(config_file).remove_filename());

   ofstream ofs(config_file.c_str());
   if (!ofs.good())
      throw runtime_error("Failed to write to config file");

   xml::element root("config");
   for (ConfigMap::const_iterator it = config_map.begin();
        it != config_map.end(); ++it) {

      // We can only serialize some types
      const any& a = (*it).second;
      const type_info& t = a.type();
      string text, type_name;
      if (t == typeid(string)) {
         type_name = "string";
         text = any_cast<string>(a);
      }
      else if (t == typeid(int)) {
         type_name = "int";
         text = lexical_cast<string>(any_cast<int>(a));
      }
      else if (t == typeid(float)) {
         type_name = "float";
         text = lexical_cast<string>(any_cast<float>(a));
      }
      else if (t == typeid(bool)) {
         type_name = "bool";
         text = lexical_cast<string>(any_cast<bool>(a));
      }
      else
         throw runtime_error(
            "Cannot XMLify objects of type "
            + boost::lexical_cast<string>(t.name()));

      xml::element option("option");
      option.add_attribute("name", (*it).first);

      xml::element type(type_name);
      type.add_text(text);
      option.add_child(type);
      
      root.add_child(option);
   }
   
   ofs << xml::document(root);

   am_dirty = false;
}

// Read a single option
const IConfig::Option& Config::get(const string& a_key) const
{
   ConfigMap::const_iterator it = config_map.find(a_key);
   if (it != config_map.end())
      return (*it).second;
   else
      throw runtime_error("Bad config key " + a_key);
}

void Config::set(const string& a_key, const IConfig::Option& a_value)
{
   config_map[a_key] = a_value;
   am_dirty = true;
}

// Return the single config file instance
IConfigPtr get_config()
{
   static IConfigPtr cfg(new Config);

   return cfg;
}

