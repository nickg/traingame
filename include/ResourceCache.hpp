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

#include "Resources.hpp"

// A generic cache for resources
template <class T>
class ResourceCache {
public:
   typedef function<T (IResourcePtr)> LoaderType;
   
   ResourceCache(LoaderType aLoader, const string& aClass)
      : myLoader(aLoader), myClass(aClass) {}
   
   T load(const string& aResId)
   {
      typename CacheType::iterator it = myCache.find(aResId);
      if (it != myCache.end())
         return (*it).second;
      else {
         T loaded = myLoader(findResource(aResId, myClass));
         myCache[aResId] = loaded;
         return loaded;
      }
   }
   
private:
   LoaderType myLoader;
   const string myClass;
   
   typedef map<string, T> CacheType;
   CacheType myCache;
};

#endif
