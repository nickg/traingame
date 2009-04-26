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

#include <xercesc/sax2/DefaultHandler.hpp>
#include <xercesc/sax2/XMLReaderFactory.hpp>
#include <xercesc/sax2/SAX2XMLReader.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/util/PlatformUtils.hpp>

using namespace std;
using namespace std::tr1;
using namespace xercesc;

// SAX2 handler to call our own methods
struct SAX2WrapperHandler : public DefaultHandler {
   
   SAX2WrapperHandler() : callbackPtr(NULL) {}
   
   void startElement(const XMLCh* const uri,
                     const XMLCh* const localname,
                     const XMLCh* const qname,
                     const Attributes& attrs)
   {
      char* chLocalname = XMLString::transcode(localname);

      callbackPtr->startElement(chLocalname, AttributeSet(attrs));
      
      XMLString::release(&chLocalname);
   }

   void error(const SAXParseException& e) { throw e; }
   void fatalError(const SAXParseException& e) { throw e; }

   IXMLCallback* callbackPtr;
};

// Concrete XML parser using Xerces
class XercesXMLParser : public IXMLParser {
public:
   XercesXMLParser(const string& aSchemaFile);
   ~XercesXMLParser();

   void parse(const string& aFileName, IXMLCallback& aCallback);
private:
   SAX2XMLReader* myReader;
   SAX2WrapperHandler* myHandler;

   static int ourParserCount;
};

// Number of parsers in use
int XercesXMLParser::ourParserCount(0);

XercesXMLParser::XercesXMLParser(const string& aSchemaFile)
{
   log() << "Creating parser for XML schema " << aSchemaFile;

   if (ourParserCount++ == 0) {
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

      log() << "Xerces initialised";
   }   
   
   myReader = XMLReaderFactory::createXMLReader();
   
   myReader->setFeature(XMLUni::fgSAX2CoreValidation, true);
   myReader->setFeature(XMLUni::fgSAX2CoreNameSpaces, true);
   myReader->setFeature(XMLUni::fgXercesValidationErrorAsFatal, true);
   myReader->setFeature(XMLUni::fgXercesDynamic, false);
   myReader->setFeature(XMLUni::fgXercesSchema, true);

   // Full checking (can be slow)
   myReader->setFeature(XMLUni::fgXercesSchemaFullChecking, true);

   // Enable grammar caching
   myReader->setFeature(XMLUni::fgXercesCacheGrammarFromParse, true);

   XMLCh* schemaName = XMLString::transcode(aSchemaFile.c_str());
   ArrayJanitor<XMLCh> janSchemaName(schemaName);

   myHandler = new SAX2WrapperHandler;
   myReader->setContentHandler(myHandler);
   myReader->setErrorHandler(myHandler);
   myReader->setEntityResolver(myHandler);

   // Cache the grammar
   try {
      myReader->loadGrammar(schemaName, Grammar::SchemaGrammarType, true);

      // Always use the cached grammar
      myReader->setFeature(XMLUni::fgXercesUseCachedGrammarInParse, true);
   }
   catch (const SAXParseException& e) {
      char* message = XMLString::transcode(e.getMessage());
      error() << "SAXParseException: " << message;
      XMLString::release(&message);

      throw runtime_error("Failed to load XML schema: " + aSchemaFile);
   }
}

XercesXMLParser::~XercesXMLParser()
{
   delete myReader;
   delete myHandler;

   if (--ourParserCount == 0)
      XMLPlatformUtils::Terminate();
}

void XercesXMLParser::parse(const string& aFileName, IXMLCallback& aCallback)
{
   myHandler->callbackPtr = &aCallback;
   
   myReader->parse(aFileName.c_str());

   myHandler->callbackPtr = NULL;
}

// Create a Xerces parser for a schema and return a handle to it
IXMLParserPtr makeXMLParser(const std::string& aSchemaFile)
{
   return IXMLParserPtr(new XercesXMLParser(aSchemaFile));
}
