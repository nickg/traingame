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

#ifndef INC_RESOURCES_HPP
#define INC_RESOURCES_HPP

#include "Platform.hpp"

#include <string>
#include <list>
#include <fstream>

// Interface to game resource
// A game resource is a directory containing related media files
// E.g. a building resource might contain the model, textures,
// and the XML file describing it
struct IResource {
   virtual ~IResource() {}

   virtual string name() const = 0;
   virtual string xml_file_name() const = 0;  // REMOVE
   // (Should be replaced by Handle open_xml_file()
   
   // A handle for reading data out of files in resources
   class Handle {
   public:
      enum Mode { READ, WRITE };
      
      explicit Handle(const string& file_name, Mode mode = READ);
      ~Handle() { commit(); }
      
      ifstream& rstream() { return *read_stream; }
      ofstream& wstream() { return *write_stream; }
      
      string file_name() const { return file_name_; }
      Mode mode() const { return mode_; }
      
      void commit();
      void rollback();
   private:
      string tmp_file_name() const;
      
      shared_ptr<ifstream> read_stream;
      shared_ptr<ofstream> write_stream;
      const string file_name_;
      const Mode mode_;
      bool aborted;
   };

   virtual Handle open_file(const string& a_name) = 0;
   virtual Handle write_file(const string& a_name) = 0;
};

typedef shared_ptr<IResource> IResourcePtr;

typedef list<IResourcePtr> ResourceList;
typedef ResourceList::iterator ResourceListIt;

// Generic interface to game resources
void init_resources();
void enum_resources(const string& a_class, ResourceList& a_list);
IResourcePtr find_resource(const string& a_res_id, const string& a_class);
IResourcePtr make_new_resource(const string& a_res_id, const string& a_class);
bool resource_exists(const string& a_res_id, const string& a_class);

#endif
