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

#include "IResource.hpp"
#include "ILogger.hpp"

#include <map>
#include <stdexcept>
#include <sstream>

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
      return Handle((myPath / aFileName).file_string(), Handle::READ);
   }

   Handle writeFile(const string& aFileName)
   {
      return Handle((myPath / aFileName).file_string(), Handle::WRITE);
   }
private:
   const path myPath;
};

IResource::Handle::Handle(const string& fileName, Mode mode)
   : fileName_(fileName), mode_(mode), aborted(false)
{
   if (mode == READ) {
      readStream = shared_ptr<ifstream>(new ifstream(fileName.c_str()));
           
      if (!readStream->good())
         throw runtime_error("Failed to open resource file " + fileName);
   }
   else if (mode == WRITE) {
      const string tmp = tmpFileName();   
      writeStream = shared_ptr<ofstream>(new ofstream(tmp.c_str()));
           
      if (!writeStream->good())
         throw runtime_error("Failed to open resource file " + fileName);
   }
   else
      throw runtime_error("Bad mode for Handle");
}

string IResource::Handle::tmpFileName() const
{
   return fileName() + ".tmp";
}

void IResource::Handle::rollback()
{
   remove(tmpFileName());
   aborted = true;
}

void IResource::Handle::commit()
{
   if (mode() == WRITE && !aborted) {
      remove(fileName());
      rename(tmpFileName(), fileName());
      remove(tmpFileName());
   }
}

namespace {
   const char* classes[] = {
      "maps", "buildings", "engines", "waggons", "trees",
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

   void addResource(const string& aClass, IResourcePtr aRes)
   {
      resClassList(aClass).push_back(aRes);         
   }

   void addResourceDir(const char* aClass, const path& aPath)
   {
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

   ostringstream ss;
   ss << "Found ";
   
   for (const char **it = classes; *it; ++it) {
      const ResourceList& lst = resClassList(*it);

      if (it != classes)
         ss << ", ";
      ss << lst.size() << " " << *it;
   }

   log() << ss.str();
}

// Find all the resources of the given type
void enumResources(const string& aClass, ResourceList& aList)
{
   ResourceList& lst = resClassList(aClass);
   copy(lst.begin(), lst.end(), back_inserter(aList));
}

namespace {
   // Find a resource of a particular type
   // Returns null pointer on failure
   IResourcePtr maybeFindResource(const string& aResId, const string& aClass)
   {
      ResourceList& rlist = resClassList(aClass);
      for (ResourceListIt it = rlist.begin(); it != rlist.end(); ++it) {
         if ((*it)->name() == aResId)
            return *it;
      }

      return IResourcePtr();
   }
}

// Find a resource or throw an exception on failure
IResourcePtr findResource(const string& aResId, const string& aClass)
{
   IResourcePtr r = maybeFindResource(aResId, aClass);
   if (r)
      return r;
   else 
      throw runtime_error("Failed to find resource " + aResId
         + " in class " + aClass);
}

// True if the given resource exists
bool resourceExists(const string& aResId, const string& aClass)
{
   return maybeFindResource(aResId, aClass);
}

// Create an empty resource directory
IResourcePtr makeNewResource(const string& aResId, const string& aClass)
{
   const path p = path(aClass) / aResId;

   if (exists(p))
      throw runtime_error("Cannot create resource " + aResId
         + " in class " + aClass + ": already exists!");

   if (!create_directories(p))
      throw runtime_error("Failed to create resource directory "
         + p.file_string());

   IResourcePtr r = IResourcePtr(new FilesystemResource(p));
   addResource(aClass, r);

   return r;
}
