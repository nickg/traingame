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

#include "IBuilding.hpp"
#include "IResource.hpp"
#include "ResourceCache.hpp"
#include "ILogger.hpp"
#include "IXMLParser.hpp"
#include "XMLBuilder.hpp"
#include "OpenGLHelper.hpp"

// Concrete implementation of buildings
class Building : public IBuilding, public IXMLCallback {
public:
   Building(IResourcePtr aRes);

   // IBuildingInterface
   const string& name() const { return name_; }
   string resId() const { return resource->name(); }
   void render() const;
   void setAngle(float a) { angle = a; }

   // IXMLSerialisable interface
   xml::element toXml() const;
   
   // IXMLCallback interface
   void text(const string& localName, const string& aString);

private:
   IModelPtr model_;
   string name_;
   IResourcePtr resource;
   float angle;
};

Building::Building(IResourcePtr aRes)
   : name_("???"), resource(aRes), angle(0.0f)
{
   static IXMLParserPtr parser = makeXMLParser("schemas/building.xsd");

   parser->parse(aRes->xmlFileName(), *this);
}

void Building::render() const
{
   glPushMatrix();

   glRotatef(angle, 0.0f, 1.0f, 0.0f);
   model_->render();
   
   glPopMatrix();
}

void Building::text(const string& localName, const string& aString)
{
   if (localName == "name")
      name_ = aString;
   else if (localName == "model")
      model_ = loadModel(resource, aString);
}

xml::element Building::toXml() const
{
   return xml::element("building")
      .addAttribute("angle", static_cast<int>(angle))
      .addAttribute("name", resource->name());
}

namespace {
   Building* loadBuildingXml(IResourcePtr aRes)
   {      
      log() << "Loading building from " << aRes->xmlFileName();

      return new Building(aRes);
   }
}

IBuildingPtr loadBuilding(const string& aResId, float angle)
{
   static ResourceCache<Building> cache(loadBuildingXml, "buildings");
   
   shared_ptr<Building> bld = cache.loadCopy(aResId);
   bld->setAngle(angle);

   return IBuildingPtr(bld);
}

IBuildingPtr loadBuilding(const AttributeSet& attrs)
{
   float angle;
   string name;
   attrs.get("name", name);
   attrs.get("angle", angle);

   return loadBuilding(name, angle);
}
