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

#include "IXMLParser.hpp"
#include "ILogger.hpp"

#include <stdexcept>
#include <sstream>

#include <xercesc/sax2/DefaultHandler.hpp>
#include <xercesc/sax2/XMLReaderFactory.hpp>
#include <xercesc/sax2/SAX2XMLReader.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/util/PlatformUtils.hpp>

using namespace xercesc;

// SAX2 handler to call our own methods
struct SAX2WrapperHandler : public DefaultHandler {
   
   SAX2WrapperHandler() : callback_ptr(NULL) {}
   
   void startElement(const XMLCh* const uri,
                     const XMLCh* const localname,
                     const XMLCh* const qname,
                     const Attributes& attrs)
   {
      char* ch_localname = XMLString::transcode(localname);

      callback_ptr->start_element(ch_localname, AttributeSet(attrs));
      
      XMLString::release(&ch_localname);
   }

   void characters(const XMLCh* const buf, const XMLSize_t length)
   {
      char* ch_buf = XMLString::transcode(buf);

      char_buf << ch_buf;

      XMLString::release(&ch_buf);
   }

   void endElement(const XMLCh* const uri,
                   const XMLCh* const localname,
                   const XMLCh* const qname)
   {
      char* ch_localname = XMLString::transcode(localname);

      if (char_buf.str().size() > 0) {
         callback_ptr->text(ch_localname, char_buf.str());
         char_buf.str("");
      }

      callback_ptr->end_element(ch_localname);
      
      XMLString::release(&ch_localname);
   }
   
   void error(const SAXParseException& e) { throw e; }
   void fatalError(const SAXParseException& e) { throw e; }

   IXMLCallback* callback_ptr;
   ostringstream char_buf;
};

// Concrete XML parser using Xerces
class XercesXMLParser : public IXMLParser {
public:
   XercesXMLParser(const string& a_schema_file);
   ~XercesXMLParser();

   void parse(const string& a_file_name, IXMLCallback& a_callback);
private:
   SAX2XMLReader* my_reader;
   SAX2WrapperHandler* my_handler;

   static int our_parser_count;
};

// Number of parsers in use
int XercesXMLParser::our_parser_count(0);

XercesXMLParser::XercesXMLParser(const string& a_schema_file)
{
   log() << "Creating parser for XML schema " << a_schema_file;

   if (our_parser_count++ == 0) {
      // Initialise Xerces for the first time
      try {
         XMLPlatformUtils::Initialize();
      }
      catch (const XMLException& e) {
         char* message = XMLString::transcode(e.getMessage());
         error() << "Exception in Xerces startup: " << message;
         XMLString::release(&message);

         throw runtime_error("Cannot continue");
      }
      
      atexit(XMLPlatformUtils::Terminate);

      log() << "Xerces initialised";
   }   

   my_reader = XMLReaderFactory::createXMLReader();
   
   my_reader->setFeature(XMLUni::fgSAX2CoreValidation, true);
   my_reader->setFeature(XMLUni::fgSAX2CoreNameSpaces, true);
   my_reader->setFeature(XMLUni::fgXercesValidationErrorAsFatal, true);
   my_reader->setFeature(XMLUni::fgXercesDynamic, false);
   my_reader->setFeature(XMLUni::fgXercesSchema, true);

   // Full checking (can be slow)
   my_reader->setFeature(XMLUni::fgXercesSchemaFullChecking, true);

   // Enable grammar caching
   my_reader->setFeature(XMLUni::fgXercesCacheGrammarFromParse, true);

   XMLCh* schema_name = XMLString::transcode(a_schema_file.c_str());

   my_handler = new SAX2WrapperHandler;
   my_reader->setContentHandler(my_handler);
   my_reader->setErrorHandler(my_handler);
   my_reader->setEntityResolver(my_handler);

   // Cache the grammar
   try {
      my_reader->loadGrammar(schema_name, Grammar::SchemaGrammarType, true);

      // Always use the cached grammar
      my_reader->setFeature(XMLUni::fgXercesUseCachedGrammarInParse, true);
   }
   catch (const SAXParseException& e) {
      char* message = XMLString::transcode(e.getMessage());
      error() << "SAXParseException: " << message;
      error() << "At " << a_schema_file << " line "
              << e.getLineNumber() << " col "
              << e.getColumnNumber();
      XMLString::release(&message);

      throw runtime_error("Failed to load XML schema");
   }

   XMLString::release(&schema_name);
}

XercesXMLParser::~XercesXMLParser()
{
   delete my_reader;
   delete my_handler;
}

void XercesXMLParser::parse(const string& a_file_name, IXMLCallback& a_callback)
{
   my_handler->callback_ptr = &a_callback;

   try {
      my_reader->parse(a_file_name.c_str());
   }
   catch (const SAXParseException& e) {
      char* message = XMLString::transcode(e.getMessage());
      error() << "SAXParseException: " << message;
      error() << "At " << a_file_name << " line "
              << e.getLineNumber() << " col "
              << e.getColumnNumber();
      XMLString::release(&message);

      throw runtime_error("Failed to load XML file");
   }

   my_handler->callback_ptr = NULL;
}

// Create a Xerces parser for a schema and return a handle to it
IXMLParserPtr make_xml_parser(const std::string& a_schema_file)
{
   return IXMLParserPtr(new XercesXMLParser(a_schema_file));
}
