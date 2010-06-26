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

#ifndef INC_RESOURCE_CACHE_HPP
#define INC_RESOURCE_CACHE_HPP

#include <string>
#include <map>

#include "IResource.hpp"

// A generic cache for resources
template <class T>
class ResourceCache {
public:
   typedef function<T* (IResourcePtr)> LoaderType;
   
   ResourceCache(LoaderType a_loader, const string& a_class)
      : my_loader(a_loader), my_class(a_class) {}

   // Load one single copy of this object
   // -> use this if the object has no state
   shared_ptr<T> load(const string& a_res_id)
   {
      typename CacheType::iterator it = my_cache.find(a_res_id);
      if (it != my_cache.end())
         return (*it).second;
      else {
         T* loaded = my_loader(find_resource(a_res_id, my_class));
         shared_ptr<T> ptr(loaded);
         my_cache[a_res_id] = ptr;
         return ptr;
      }
   }

   // Make a copy each time a new object is loaded but only
   // parse the XML once
   // -> use this if the object has state 
   shared_ptr<T> load_copy(const string& a_res_id)
   {
      shared_ptr<T> original = load(a_res_id);
      return shared_ptr<T>(new T(*original.get()));
   }
   
private:
   LoaderType my_loader;
   const string my_class;
   
   typedef map<string, shared_ptr<T> > CacheType;
   CacheType my_cache;
};

#endif
