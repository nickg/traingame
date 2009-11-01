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

#include "IRollingStock.hpp"
#include "IModel.hpp"
#include "IResource.hpp"
#include "IXMLParser.hpp"
#include "ILogger.hpp"
#include "ResourceCache.hpp"

#include <stdexcept>

using namespace std;

// All cargo waggons
class Waggon : public IRollingStock, public IXMLCallback {
public:
   Waggon(IResourcePtr aRes);
   ~Waggon() {}

   // IRollingStock interface
   void update(int aDelta);
   void render() const;
   IControllerPtr controller();
   double speed() const { return 0.0; }
   double length() const { return model->dimensions().x; }

   // IXMLCallback interface
   void text(const string& localName, const string& aString);
private:
   IModelPtr model;
   IResourcePtr resource;

   static const float MODEL_SCALE;
};

const float Waggon::MODEL_SCALE(0.4f);

Waggon::Waggon(IResourcePtr aRes)
   : resource(aRes)
{
   static IXMLParserPtr parser = makeXMLParser("schemas/waggon.xsd");

   parser->parse(resource->xmlFileName(), *this);
}

// Load information from the XML file
void Waggon::text(const string& localName, const string& aString)
{
   if (localName == "model")
      model = loadModel(resource, aString, MODEL_SCALE);
}

void Waggon::update(int aDelta)
{
   
}

void Waggon::render() const
{
   model->render();
}

IControllerPtr Waggon::controller()
{
   throw runtime_error("Cannot control a waggon!");
}

namespace {
   Waggon* loadWaggonXml(IResourcePtr aRes)
   {
      log() << "Loading waggon from " << aRes->xmlFileName();

      return new Waggon(aRes);
   }
}

// Load a waggon from a resource file
IRollingStockPtr loadWaggon(const string& aResId)
{
   static ResourceCache<Waggon> cache(loadWaggonXml, "waggons");
   return cache.loadCopy(aResId);
}

