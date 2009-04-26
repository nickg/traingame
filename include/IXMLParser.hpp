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

#include <tr1/memory>
#include <string>
#include <stdexcept>

#include <xercesc/sax2/Attributes.hpp>
#include <xercesc/util/XMLString.hpp>
#include <boost/lexical_cast.hpp>

// Container for attributes
class AttributeSet {
public:
   AttributeSet(const xercesc::Attributes& attrs)
      : myAttrs(attrs) {}
   
   template <class T>
   T get(const std::string& aName) const
   {
      XMLCh* xmlName = xercesc::XMLString::transcode(aName.c_str());

      int index = myAttrs.getIndex(xmlName);
      xercesc::XMLString::release(&xmlName);
      
      if (index != -1) {
         char* ascii = xercesc::XMLString::transcode(myAttrs.getValue(index));
         T result = boost::lexical_cast<T>(ascii);
         xercesc::XMLString::release(&ascii);
         return result;
      }
      else
         throw std::runtime_error("No attribute: " + aName);
   }

   template <class T>
   void get(const std::string& aName, T& aT) const
   {
      aT = get<T>(aName);
   }
private:
   const xercesc::Attributes& myAttrs;
};

// SAX-like interface to XML parsing
struct IXMLCallback {
   virtual ~IXMLCallback() {}

   virtual void startElement(const std::string& localName,
                             const AttributeSet& attrs) = 0;
};

// Interface to a validating XML parser
struct IXMLParser {
   virtual ~IXMLParser() {}

   virtual void parse(const std::string& aFileName, IXMLCallback& aCallback) = 0;
};

typedef std::tr1::shared_ptr<IXMLParser> IXMLParserPtr;

IXMLParserPtr makeXMLParser(const std::string& aSchemaFile);

#endif

