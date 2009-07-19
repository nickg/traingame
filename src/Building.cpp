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
   IModelPtr model() const { return myModel; }

   // IXMLCallback interface
   void text(const string& localName, const string& aString);
private:
   IModelPtr myModel;
   string myName;
   IResourcePtr myResource;
};

Building::Building(IResourcePtr aRes)
   : myName("???"), myResource(aRes)
{
   static IXMLParserPtr parser = makeXMLParser("schemas/building.xsd");

   parser->parse(aRes->xmlFileName(), *this);
}

void Building::text(const string& localName, const string& aString)
{
   debug() << "text " << localName << " = " << aString;
   
   if (localName == "name")
      myName = aString;
   else if (localName == "model")
      myModel = loadModel(myResource, aString);
}

namespace {
   
   IBuildingPtr loadBuildingXml(IResourcePtr aRes)
   {      
      log() << "Loading building from " << aRes->xmlFileName();

      return IBuildingPtr(new Building(aRes));
   }
}

IBuildingPtr loadBuilding(const string& aResId)
{
   static ResourceCache<IBuildingPtr> cache(loadBuildingXml, "buildings");
   return cache.load(aResId);
}
