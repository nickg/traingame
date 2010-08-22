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
   Waggon(IResourcePtr a_res);
   ~Waggon() {}

   // IRollingStock interface
   void update(int delta, double gravity);
   void render() const;
   IControllerPtr controller();
   double speed() const { return 0.0; }
   double mass() const { return 1.0; }
   float length() const { return model->dimensions().x; }
   ICargoPtr cargo() const;

   // IXMLCallback interface
   void text(const string& local_name, const string& a_string);
private:
   IModelPtr model;
   IResourcePtr resource;

   static const float MODEL_SCALE;
};

const float Waggon::MODEL_SCALE(0.4f);

Waggon::Waggon(IResourcePtr a_res)
   : resource(a_res)
{
   static IXMLParserPtr parser = makeXMLParser("schemas/waggon.xsd");

   parser->parse(resource->xml_file_name(), *this);
}

ICargoPtr Waggon::cargo() const
{
   return ICargoPtr();
}

// Load information from the XML file
void Waggon::text(const string& local_name, const string& a_string)
{
   if (local_name == "model") {
      model = load_model(resource, a_string, MODEL_SCALE);
      model->cache();
   }
}

void Waggon::update(int delta, double gravity)
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
   Waggon* load_waggon_xml(IResourcePtr a_res)
   {
      log() << "Loading waggon from " << a_res->xml_file_name();

      return new Waggon(a_res);
   }
}

// Load a waggon from a resource file
IRollingStockPtr load_waggon(const string& a_res_id)
{
   static ResourceCache<Waggon> cache(load_waggon_xml, "waggons");
   return cache.load_copy(a_res_id);
}

