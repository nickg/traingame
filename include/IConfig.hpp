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

#ifndef INC_ICONFIG_HPP
#define INC_ICONFIG_HPP

#include "Platform.hpp"

#include <boost/any.hpp>

// Interface to config file
struct IConfig {
   virtual ~IConfig() {}

   typedef boost::any Option;

   virtual const Option& get(const string& aKey) const = 0;
   virtual void flush() = 0;
 
   template <class T>
   T get(const string& aKey) const
   {
      return boost::any_cast<T>(get(aKey));
   }

   template <class T>
   void get(const string& aKey, T& t) const
   {
      t = boost::any_cast<T>(get(aKey));
   }
};

typedef shared_ptr<IConfig> IConfigPtr;

IConfigPtr getConfig();

#endif
