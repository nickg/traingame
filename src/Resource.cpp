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
   FilesystemResource(const path& a_path)
      : my_path(a_path)
   {

   }

   // IResource interface
   string name() const { return my_path.filename().string(); }
   string xml_file_name() const
   {
      return (my_path / (name() + ".xml")).string();
   }

   Handle open_file(const string& a_file_name)
   {
      return Handle((my_path / a_file_name).string(), Handle::READ);
   }

   Handle write_file(const string& a_file_name)
   {
      return Handle((my_path / a_file_name).string(), Handle::WRITE);
   }
private:
   const path my_path;
};

IResource::Handle::Handle(const string& file_name, Mode mode)
   : file_name_(file_name), mode_(mode), aborted(false)
{
   if (mode == READ) {
      read_stream = shared_ptr<ifstream>(new ifstream(file_name.c_str()));

      if (!read_stream->good())
         throw runtime_error("Failed to open resource file " + file_name);
   }
   else if (mode == WRITE) {
      const string tmp = tmp_file_name();
      write_stream = shared_ptr<ofstream>(new ofstream(tmp.c_str()));

      if (!write_stream->good())
         throw runtime_error("Failed to open resource file " + file_name);
   }
   else
      throw runtime_error("Bad mode for Handle");
}

string IResource::Handle::tmp_file_name() const
{
   return file_name() + ".tmp";
}

void IResource::Handle::rollback()
{
   remove(tmp_file_name());
   aborted = true;
}

void IResource::Handle::commit()
{
   if (mode() == WRITE && !aborted) {
      remove(file_name());
      rename(tmp_file_name(), file_name());
      remove(tmp_file_name());
   }
}

namespace {
   const char* classes[] = {
      "maps", "buildings", "engines", "waggons", "trees",
      NULL
   };

   typedef map<string, ResourceList> ResourceMap;
   ResourceMap the_resources;
}

static ResourceList& res_class_list(const string& a_class)
{
   if (the_resources.find(a_class) == the_resources.end())
      the_resources[a_class] = ResourceList();
   return the_resources[a_class];
}

static void add_resource(const string& a_class, IResourcePtr a_res)
{
   res_class_list(a_class).push_back(a_res);
}

static void add_resource_dir(const char* a_class, const path& p)
{
   const path xml_file = p / (p.filename().replace_extension(".xml"));

   if (!exists(xml_file))
      warn() << "Missing resource XML file: " << xml_file;
   else
      add_resource(a_class, IResourcePtr(new FilesystemResource(p)));
}

static void look_in_dir(const path& a_path)
{
   log() << "Looking for resources in " << a_path;

   for (const char** p = classes; *p != NULL; ++p) {
      if (exists(a_path / *p)) {
         for (directory_iterator it(a_path / *p);
              it != directory_iterator(); ++it)
            if (is_directory(it->status()))
               add_resource_dir(*p, *it);
      }
   }
}

// Set up the resource database and cache available objects
void init_resources()
{
   look_in_dir(current_path());

   ostringstream ss;
   ss << "Found ";

   for (const char **it = classes; *it; ++it) {
      const ResourceList& lst = res_class_list(*it);

      if (it != classes)
         ss << ", ";
      ss << lst.size() << " " << *it;
   }

   log() << ss.str();
}

// Find all the resources of the given type
void enum_resources(const string& a_class, ResourceList& a_list)
{
   ResourceList& lst = res_class_list(a_class);
   copy(lst.begin(), lst.end(), back_inserter(a_list));
}

// Find a resource of a particular type
// Returns null pointer on failure
static IResourcePtr maybe_find_resource(const string& a_res_id,
                                        const string& a_class)
{
   ResourceList& rlist = res_class_list(a_class);
   for (ResourceListIt it = rlist.begin(); it != rlist.end(); ++it) {
      if ((*it)->name() == a_res_id)
         return *it;
   }

   return IResourcePtr();
}

// Find a resource or throw an exception on failure
IResourcePtr find_resource(const string& a_res_id, const string& a_class)
{
   IResourcePtr r = maybe_find_resource(a_res_id, a_class);
   if (r)
      return r;
   else
      throw runtime_error("Failed to find resource " + a_res_id
         + " in class " + a_class);
}

// True if the given resource exists
bool resource_exists(const string& a_res_id, const string& a_class)
{
   return static_cast<bool>(maybe_find_resource(a_res_id, a_class));
}

// Create an empty resource directory
IResourcePtr make_new_resource(const string& a_res_id, const string& a_class)
{
   const path p = path(a_class) / a_res_id;

   if (exists(p))
      throw runtime_error("Cannot create resource " + a_res_id
         + " in class " + a_class + ": already exists!");

   if (!create_directories(p))
      throw runtime_error("Failed to create resource directory " + p.string());

   IResourcePtr r = IResourcePtr(new FilesystemResource(p));
   add_resource(a_class, r);

   return r;
}
