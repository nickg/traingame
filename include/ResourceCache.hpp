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
   
   ResourceCache(LoaderType aLoader, const string& aClass)
      : myLoader(aLoader), myClass(aClass) {}

   // Load one single copy of this object
   // -> use this if the object has no state
   shared_ptr<T> load(const string& aResId)
   {
      typename CacheType::iterator it = myCache.find(aResId);
      if (it != myCache.end())
         return (*it).second;
      else {
         T* loaded = myLoader(findResource(aResId, myClass));
         shared_ptr<T> ptr(loaded);
         myCache[aResId] = ptr;
         return ptr;
      }
   }

   // Make a copy each time a new object is loaded but only
   // parse the XML once
   // -> use this if the object has state 
   shared_ptr<T> loadCopy(const string& aResId)
   {
      shared_ptr<T> original = load(aResId);
      return shared_ptr<T>(new T(*original.get()));
   }
   
private:
   LoaderType myLoader;
   const string myClass;
   
   typedef map<string, shared_ptr<T> > CacheType;
   CacheType myCache;
};

#endif
