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

#include "IResource.hpp"
#include "ILogger.hpp"

#include <map>
#include <stdexcept>

#include <boost/filesystem.hpp>

using namespace boost::filesystem;

// A resource that reads data from a filesystem directory
class FilesystemResource : public IResource {
public:
   FilesystemResource(const path& aPath)
      : myPath(aPath)
   {

   }

   // IResource interface
   string name() const { return myPath.filename(); }
   string xmlFileName() const
   {
      return (myPath / (name() + ".xml")).file_string();
   }

   Handle openFile(const string& aFileName)
   {
      return Handle((myPath / aFileName).file_string());
   }
private:
   const path myPath;
};

IResource::Handle::Handle(const string& aFileName)
   : myStream(new ifstream(aFileName.c_str())),
     myFileName(aFileName)
{
   if (!myStream->good())
      throw runtime_error("Failed to open resource file " + aFileName);
}

namespace {
   const char* classes[] = {
      "maps", "buildings", "engines", "waggons",
      NULL
   };

   typedef map<string, ResourceList> ResourceMap;
   ResourceMap theResources;

   ResourceList& resClassList(const string& aClass)
   {
      if (theResources.find(aClass) == theResources.end())
         theResources[aClass] = ResourceList();
      return theResources[aClass];
   }

   void addResource(const char* aClass, IResourcePtr aRes)
   {
      resClassList(aClass).push_back(aRes);         
   }

   void addResourceDir(const char* aClass, const path& aPath)
   {
      debug() << "Add " << aClass << " from " << aPath;

      const path xmlFile = aPath / (aPath.filename() + ".xml");

      if (!exists(xmlFile))
         warn() << "Missing resource XML file: " << xmlFile;
      else 
         addResource(aClass, IResourcePtr(new FilesystemResource(aPath)));
   }
   
   void lookInDir(const path& aPath)
   {
      log() << "Looking for resources in " << aPath;

      for (const char** p = classes; *p != NULL; ++p) {
         if (exists(aPath / *p)) {
            debug() << "Adding class " << *p;

            for (directory_iterator it(aPath / *p);
                 it != directory_iterator(); ++it)
               if (is_directory(it->status()))
                  addResourceDir(*p, *it);
         }
      }
   }
}

// Set up the resource database and cache available objects
void initResources()
{
   lookInDir(current_path());
}

// Find all the resources of the given type
void enumResources(const string& aClass, ResourceList& aList)
{

}

// Find a resource of a particular type
IResourcePtr findResource(const string& aResId, const string& aClass)
{
   ResourceList& rlist = resClassList(aClass);
   for (ResourceListIt it = rlist.begin(); it != rlist.end(); ++it) {
      if ((*it)->name() == aResId)
         return *it;
   }

   throw runtime_error("Failed to find resource " + aResId
                       + " in class " + aClass);
}
