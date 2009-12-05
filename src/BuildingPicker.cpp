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

#include "IBuildingPicker.hpp"
#include "ILogger.hpp"
#include "ILight.hpp"
#include "OpenGLHelper.hpp"
#include "gui/Label.hpp"

class BuildingPicker : public IBuildingPicker {
public:
   BuildingPicker(gui::ILayoutPtr l)
      : layout(l)
   {
      using namespace placeholders;
      
      enumResources("buildings", buildingList);
      
      if (buildingList.empty())
         warn() << "No buildings found";
      else {
         buildingIt = buildingList.begin();
         changeActive(loadBuilding((*buildingIt)->name()));
      }
      
      layout->get("/building_wnd/preview").connect(gui::Widget::SIG_RENDER,
         bind(&::BuildingPicker::renderBuildingPreview, this, _1));
      layout->get("/building_wnd/next").connect(gui::Widget::SIG_CLICK,
         bind(&::BuildingPicker::next, this));
      layout->get("/building_wnd/prev").connect(gui::Widget::SIG_CLICK,
         bind(&::BuildingPicker::prev, this));
   }
      
   void next()
   {
      if (++buildingIt == buildingList.end())
         buildingIt = buildingList.begin();
      
      changeActive(loadBuilding((*buildingIt)->name()));      
   }
   
   void prev()
   {
      if (buildingIt == buildingList.begin())
         buildingIt = buildingList.end();
      buildingIt--;
      
      changeActive(loadBuilding((*buildingIt)->name()));
   }
   
   IBuildingPtr get() const
   {
      return activeBuilding;
   }
   
   void renderBuildingPreview(gui::Widget& canvas)
   {
      static ILightPtr sun = makeSunLight();
      
      sun->apply();

      glRotatef(45.0f, 1.0f, 0.0f, 0.0f);
      glRotatef(45.0f, 0.0f, 1.0f, 0.0f);
      glTranslatef(1.5f, -2.6f, -1.5f);
      glColor3f(1.0f, 1.0f, 1.0f);
      glRotatef(35.0f, 0.0f, 1.0f, 0.0f);
      
      activeBuilding->model()->render();
   }

private:
   void changeActive(IBuildingPtr b);
   
   ResourceList buildingList;
   ResourceList::const_iterator buildingIt;
   IBuildingPtr activeBuilding;
   gui::ILayoutPtr layout;
};

void BuildingPicker::changeActive(IBuildingPtr b)
{
   activeBuilding = b;

   layout->cast<gui::Label&>("/building_wnd/bld_name").text(b->name());
}

IBuildingPickerPtr makeBuildingPicker(gui::ILayoutPtr layout)
{
   return IBuildingPickerPtr(new BuildingPicker(layout));
}
