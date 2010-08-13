//
//  Copyright (C) 2009-2010  Nick Gasson
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

#include "IScenery.hpp"
#include "IResource.hpp"
#include "ResourceCache.hpp"
#include "ILogger.hpp"
#include "IXMLParser.hpp"
#include "XMLBuilder.hpp"
#include "OpenGLHelper.hpp"
#include "IModel.hpp"

// Concrete implementation of buildings
class Building : public IScenery, public IXMLCallback {
public:
   Building(IResourcePtr a_res);

   // ISceneryInterface
   const string& name() const { return name_; }
   void render() const;
   void set_angle(float a) { angle = a; }
   void set_position(float x, float y, float z);
   void merge(IMeshBufferPtr buf);
   Point<int> size() const;

   // IXMLSerialisable interface
   xml::element to_xml() const;
   
   // IXMLCallback interface
   void text(const string& local_name, const string& a_string);

private:
   IModelPtr model_;
   string name_;
   IResourcePtr resource;
   float angle;
   Vector<float> position;

   struct ParserState {
      string model_file;
   } *parser_state;
};

Building::Building(IResourcePtr a_res)
   : name_("???"), resource(a_res), angle(0.0f)
{
   static IXMLParserPtr parser = makeXMLParser("schemas/building.xsd");

   parser_state = new ParserState;   
   parser->parse(a_res->xml_file_name(), *this);

   Vector<float> shift = -make_vector(0.5f, 0.0f, 0.5f);
   model_ = load_model(a_res, parser_state->model_file,
                       1.0f, shift);

   delete parser_state;
}

Point<int> Building::size() const
{
   Vector<float> dim = model_->dimensions();

   return make_point(max(static_cast<int>(round(dim.x)), 1),
                     max(static_cast<int>(round(dim.z)), 1));
}

void Building::set_position(float x, float y, float z)
{
   position = make_vector(x, y, z);
}

void Building::render() const
{
   glPushMatrix();

   gl::translate(position);
   glRotatef(angle, 0.0f, 1.0f, 0.0f);
   model_->render();
   
   glPopMatrix();
}

void Building::merge(IMeshBufferPtr buf)
{
   model_->merge(buf, position/* + make_vector(-0.5f, 0.0f, -0.5f)*/, angle);
}

void Building::text(const string& local_name, const string& a_string)
{
   if (local_name == "name")
      name_ = a_string;
   else if (local_name == "model")
      parser_state->model_file = a_string;
}

xml::element Building::to_xml() const
{
   return xml::element("building")
      .add_attribute("angle", static_cast<int>(angle))
      .add_attribute("name", resource->name());
}

namespace {
   Building* load_building_xml(IResourcePtr a_res)
   {      
      log() << "Loading building from " << a_res->xml_file_name();

      return new Building(a_res);
   }
}

ISceneryPtr load_building(const string& a_res_id, float angle)
{
   static ResourceCache<Building> cache(load_building_xml, "buildings");
   
   shared_ptr<Building> bld = cache.load_copy(a_res_id);
   bld->set_angle(angle);

   return ISceneryPtr(bld);
}

ISceneryPtr load_building(const AttributeSet& attrs)
{
   float angle;
   string name;
   attrs.get("name", name);
   attrs.get("angle", angle);

   return load_building(name, angle);
}
