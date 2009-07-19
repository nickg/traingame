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
   virtual string xmlFileName() const = 0;  // REMOVE
   // (Should be replaced by Handle openXmlFile()
   
   // A handle for reading data out of files in resources
   class Handle {
   public:
      enum Mode { READ, WRITE };
      
      explicit Handle(const string& aFileName, Mode aMode = READ);
      
      ifstream& rstream() { return *myReadStream; }
      ofstream& wstream() { return *myWriteStream; }
      
      string fileName() const { return myFileName; }
   private:
      shared_ptr<ifstream> myReadStream;
      shared_ptr<ofstream> myWriteStream;
      const string myFileName;
   };

   virtual Handle openFile(const string& aName) = 0;
   virtual Handle writeFile(const string& aName) = 0;
};

typedef shared_ptr<IResource> IResourcePtr;

typedef list<IResourcePtr> ResourceList;
typedef ResourceList::iterator ResourceListIt;

// Generic interface to game resources
void initResources();
void enumResources(const string& aClass, ResourceList& aList);
IResourcePtr findResource(const string& aResId, const string& aClass);
IResourcePtr makeNewResource(const string& aResId, const string& aClass);
bool resourceExists(const string& aResId, const string& aClass);

#endif
