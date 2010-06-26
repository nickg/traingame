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

#ifndef INC_XMLBUILDER_HPP
#define INC_XMLBUILDER_HPP

#include <string>
#include <sstream>
#include <stdexcept>
#include <ostream>

#include <boost/lexical_cast.hpp>

// Helper functions for building XML files
namespace xml {
   using namespace std;
   using namespace boost;

   struct element {
      element(const string& name)
         : has_children(false), name(name)
      {
         str = "<" + name;
      }
      
      template <class T>
      element& add_attribute(const string& name, T t)
      {
         if (has_children)
            throw runtime_error(
               "Cannot add XML attributes after children");
         else {
            ostringstream ss;
            ss << boolalpha;
            ss << " " << name << "=\"" << t << "\"";
            
            str += ss.str();
         }

         return *this;
      }

      element& add_child(const element& e)
      {
         if (!has_children)
            str += ">";
         
         str += "\n" + e.finish();
         has_children = true;
         return *this;
      }

      element& add_text(const string& text)
      {
         if (!has_children)
            str += ">";

         str += text;
         has_children = true;
         return *this;
      }

      string finish() const
      {
         if (has_children)
            return str + "</" + name + ">\n";
         else
            return str + "/>\n";
      }

      bool has_children;
      string str, name;
   };

   struct document {
      document(const element& root) : root(root) {}

      const element& root;
   };
};

inline std::ostream& operator<<(std::ostream& os, const xml::document& doc)
{
   os << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
      << std::endl << doc.root.finish();
   return os;
}

#endif


