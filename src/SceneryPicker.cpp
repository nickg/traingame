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

#include "ISceneryPicker.hpp"
#include "ILogger.hpp"
#include "ILight.hpp"
#include "OpenGLHelper.hpp"
#include "gui/Label.hpp"

#include <stdexcept>

class SceneryPicker : public ISceneryPicker {
public:
   SceneryPicker(gui::ILayoutPtr l,
      const string& resourceClass, const string& guiPath,
      const string& btnGuiPath);

protected: 
   void next();   
   void prev();
   void rotate();
   void renderPreview(gui::Widget& canvas);
   void show();
   void hide();

   void changeActive(const string& newResName);
   void selectFirstItem();
   
   ResourceList resourceList;
   ResourceList::const_iterator resourceIt;
   ISceneryPtr activeItem;
   gui::ILayoutPtr layout;
   float rotation;
   string resName, guiPath, resClass;
};

SceneryPicker::SceneryPicker(gui::ILayoutPtr l,
   const string& resourceClass, const string& guiPath,
   const string& btnGuiPath)
   : layout(l),
     rotation(0.0f),
     guiPath(guiPath),
     resClass(resourceClass)
{
   using namespace placeholders;
   
   enumResources(resourceClass, resourceList);
   
   layout->get(guiPath + "/preview").connect(gui::Widget::SIG_RENDER,
      bind(&SceneryPicker::renderPreview, this, _1));
   layout->get(guiPath + "/next").connect(gui::Widget::SIG_CLICK,
      bind(&SceneryPicker::next, this));
   layout->get(guiPath + "/prev").connect(gui::Widget::SIG_CLICK,
      bind(&SceneryPicker::prev, this));

   if (layout->exists(guiPath + "/rotate"))
      layout->get(guiPath + "/rotate").connect(gui::Widget::SIG_CLICK,
         bind(&SceneryPicker::rotate, this));

   layout->get(btnGuiPath).connect(gui::Widget::SIG_ENTER,
      bind(&SceneryPicker::show, this));
   layout->get(btnGuiPath).connect(gui::Widget::SIG_LEAVE,
      bind(&SceneryPicker::hide, this));

   hide();
}

void SceneryPicker::selectFirstItem()
{
   // A kludge to avoid calling the pure virtual get() in constructor
   
   if (resourceList.empty())
      warn() << "No scenery found in class " << resClass;
   else {
      resourceIt = resourceList.begin();
      changeActive((*resourceIt)->name());
   }
}
    
void SceneryPicker::next()
{
   if (++resourceIt == resourceList.end())
      resourceIt = resourceList.begin();
   
   changeActive((*resourceIt)->name());      
}
   
void SceneryPicker::prev()
{
   if (resourceIt == resourceList.begin())
      resourceIt = resourceList.end();
   resourceIt--;
   
   changeActive((*resourceIt)->name());
}

void SceneryPicker::show()
{
   layout->get(guiPath).visible(true);
}

void SceneryPicker::hide()
{
   layout->get(guiPath).visible(false);
}

void SceneryPicker::renderPreview(gui::Widget& canvas)
{
   static ILightPtr sun = makeSunLight();
      
   glRotatef(45.0f, 1.0f, 0.0f, 0.0f);
   glRotatef(45.0f, 0.0f, 1.0f, 0.0f);
   glTranslatef(1.5f, -2.6f, -1.5f);
   glColor3f(1.0f, 1.0f, 1.0f);
   sun->apply();
   
   activeItem->render();
}

void SceneryPicker::changeActive(const string& newResName)
{
   if (newResName != resName) {
      resName = newResName;
      activeItem = this->get();
      
      layout->cast<gui::Label&>(guiPath + "/name")
         .text(activeItem->name());
   }   
}

void SceneryPicker::rotate()
{
   rotation += 90.0f;
   if (rotation >= 350.0f)
      rotation = 0.0f;

   activeItem->setAngle(rotation);
}

class BuildingPicker : public SceneryPicker {
public:
   BuildingPicker(gui::ILayoutPtr layout)
      : SceneryPicker(layout, "buildings",
         "/building_wnd", "/tool_wnd/tools/building")
   {
      selectFirstItem();
   }

   virtual ISceneryPtr get() const
   {
      return loadBuilding(resName, rotation);
   }
};

class TreePicker : public SceneryPicker {
public:
   TreePicker(gui::ILayoutPtr layout)
      : SceneryPicker(layout, "trees",
         "/tree_wnd", "/tool_wnd/tools/tree")
   {
      selectFirstItem();
   }

   virtual ISceneryPtr get() const
   {
      return loadTree(resName);
   }
};
   
ISceneryPickerPtr makeTreePicker(gui::ILayoutPtr layout)
{
   return ISceneryPickerPtr(new TreePicker(layout));
}

ISceneryPickerPtr makeBuildingPicker(gui::ILayoutPtr layout)
{
   return ISceneryPickerPtr(new BuildingPicker(layout));
}
