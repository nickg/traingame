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
   BuildingPicker(gui::ILayoutPtr l);
     
private: 
   void next();   
   void prev();
   void rotate();
   ISceneryPtr get() const;
   void renderBuildingPreview(gui::Widget& canvas);
   void show();
   void hide();

   void changeActive(const string& newResName);
   
   ResourceList buildingList;
   ResourceList::const_iterator buildingIt;
   ISceneryPtr activeBuilding;
   gui::ILayoutPtr layout;
   float rotation;
   string resName;
};

BuildingPicker::BuildingPicker(gui::ILayoutPtr l)
   : layout(l),
     rotation(0.0f)
{
   using namespace placeholders;
   
   enumResources("buildings", buildingList);
   
   if (buildingList.empty())
      warn() << "No buildings found";
   else {
      buildingIt = buildingList.begin();
      changeActive((*buildingIt)->name());
   }
   
   layout->get("/building_wnd/preview").connect(gui::Widget::SIG_RENDER,
      bind(&BuildingPicker::renderBuildingPreview, this, _1));
   layout->get("/building_wnd/next").connect(gui::Widget::SIG_CLICK,
      bind(&BuildingPicker::next, this));
   layout->get("/building_wnd/prev").connect(gui::Widget::SIG_CLICK,
      bind(&BuildingPicker::prev, this));
   layout->get("/building_wnd/rotate").connect(gui::Widget::SIG_CLICK,
      bind(&BuildingPicker::rotate, this));

   layout->get("/tool_wnd/tools/building").connect(gui::Widget::SIG_ENTER,
      bind(&BuildingPicker::show, this));
   layout->get("/tool_wnd/tools/building").connect(gui::Widget::SIG_LEAVE,
      bind(&BuildingPicker::hide, this));

   hide();
}
    
void BuildingPicker::next()
{
   if (++buildingIt == buildingList.end())
      buildingIt = buildingList.begin();
   
   changeActive((*buildingIt)->name());      
}
   
void BuildingPicker::prev()
{
   if (buildingIt == buildingList.begin())
      buildingIt = buildingList.end();
   buildingIt--;
   
   changeActive((*buildingIt)->name());
}

void BuildingPicker::show()
{
   layout->get("/building_wnd").visible(true);
}

void BuildingPicker::hide()
{
   layout->get("/building_wnd").visible(false);
}
   
ISceneryPtr BuildingPicker::get() const
{
   return loadBuilding(resName, rotation);
}

void BuildingPicker::renderBuildingPreview(gui::Widget& canvas)
{
   static ILightPtr sun = makeSunLight();
      
   glRotatef(45.0f, 1.0f, 0.0f, 0.0f);
   glRotatef(45.0f, 0.0f, 1.0f, 0.0f);
   glTranslatef(1.5f, -2.6f, -1.5f);
   glColor3f(1.0f, 1.0f, 1.0f);
   sun->apply();
   
   activeBuilding->render();
}

void BuildingPicker::changeActive(const string& newResName)
{
   if (newResName != resName) {
      activeBuilding = loadBuilding(newResName, rotation);
      resName = newResName;
      
      layout->cast<gui::Label&>("/building_wnd/bld_name")
         .text(activeBuilding->name());
   }   
}

void BuildingPicker::rotate()
{
   rotation += 90.0f;
   if (rotation >= 350.0f)
      rotation = 0.0f;

   activeBuilding->setAngle(rotation);
}

IBuildingPickerPtr makeBuildingPicker(gui::ILayoutPtr layout)
{
   return IBuildingPickerPtr(new BuildingPicker(layout));
}
