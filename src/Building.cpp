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

// Concrete implementation of buildings
class Building : public IBuilding, public IXMLCallback {
public:
   Building(IResourcePtr aRes);

   // IBuildingInterface
   IModelPtr model() const { return model_; }
   const string& name() const { return name_; }
   string resId() const { return resource->name(); }

   // IXMLCallback interface
   void text(const string& localName, const string& aString);
private:
   IModelPtr model_;
   string name_;
   IResourcePtr resource;
};

Building::Building(IResourcePtr aRes)
   : name_("???"), resource(aRes)
{
   static IXMLParserPtr parser = makeXMLParser("schemas/building.xsd");

   parser->parse(aRes->xmlFileName(), *this);
}

void Building::text(const string& localName, const string& aString)
{
   if (localName == "name")
      name_ = aString;
   else if (localName == "model")
      model_ = loadModel(resource, aString);
}

namespace {
   Building* loadBuildingXml(IResourcePtr aRes)
   {      
      log() << "Loading building from " << aRes->xmlFileName();

      return new Building(aRes);
   }
}

IBuildingPtr loadBuilding(const string& aResId)
{
   static ResourceCache<Building> cache(loadBuildingXml, "buildings");
   return cache.load(aResId);
}
