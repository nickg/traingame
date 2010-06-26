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

#ifndef INC_IXMLPARSER_HPP
#define INC_IXMLPARSER_HPP

#include "Platform.hpp"
#include "Colour.hpp"

#include <string>
#include <stdexcept>
#include <sstream>

#include <boost/lexical_cast.hpp>

#include <xercesc/sax2/Attributes.hpp>
#include <xercesc/util/XMLString.hpp>

namespace {

   template <class T>
   T xml_attr_cast(const string& str);

   template <>
   bool xml_attr_cast(const string& str)
   {
      istringstream ss(str);
      bool result;

      ss >> boolalpha >> result;  // Is boolalpha affected by locale?
      
      if (ss.fail())
         throw runtime_error(
            "Cannot parse Boolean attribute with value '"
            + str + "'");

      return result;
   }

   template <>
   Colour xml_attr_cast(const string& str)
   {
      istringstream ss(str);
      int r, g, b;

      ss >> r >> g >> b;

      if (ss.fail())
         throw runtime_error(
            "Cannot parse colour attribute with value '"
            + str + "'");

      return makeRGB(r, g, b);
   }

   template <class T>
   T xml_attr_cast(const string& str)
   {
      return boost::lexical_cast<T>(str);
   }
   
}

// Container for attributes
class AttributeSet {
public:
   AttributeSet(const xercesc::Attributes& attrs)
      : my_attrs(attrs) {}

   bool has(const string& name) const
   {
      XMLCh* xml_name = xercesc::XMLString::transcode(name.c_str());

      int index = my_attrs.getIndex(xml_name);
      xercesc::XMLString::release(&xml_name);

      return index != -1;
   }

   template <class T>
   T get(const std::string& a_name) const
   {
      XMLCh* xml_name = xercesc::XMLString::transcode(a_name.c_str());

      int index = my_attrs.getIndex(xml_name);
      xercesc::XMLString::release(&xml_name);
      
      if (index != -1) {
         char* ascii = xercesc::XMLString::transcode(
            my_attrs.getValue(index));

         T result = xml_attr_cast<T>(ascii);

         xercesc::XMLString::release(&ascii);
         return result;
      }
      else
         throw std::runtime_error("No attribute: " + a_name);
   }

   template <class T>
   void get(const std::string& a_name, T& aT) const
   {
      aT = get<T>(a_name);
   }

   // Get with default
   template <class T>
   T get(const string& name, const T& def) const
   {
      return has(name) ? get<T>(name) : def;
   }
   
private:
   const xercesc::Attributes& my_attrs;
};

// SAX-like interface to XML parsing
struct IXMLCallback {
   virtual ~IXMLCallback() {}

   virtual void start_element(const string& local_name,
                             const AttributeSet& attrs) {}
   virtual void end_element(const string& local_name) {}
   virtual void text(const string& local_name,
                     const string& a_string) {}
};

// Interface to a validating XML parser
struct IXMLParser {
   virtual ~IXMLParser() {}

   virtual void parse(const std::string& a_file_name,
      IXMLCallback& a_callback) = 0;
};

typedef std::tr1::shared_ptr<IXMLParser> IXMLParserPtr;
 
IXMLParserPtr makeXMLParser(const std::string& a_schema_file);
     
#endif

