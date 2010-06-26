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
      const string& resource_class, const string& gui_path,
      const string& btn_gui_path);

protected: 
   void next();   
   void prev();
   void rotate();
   void render_preview(gui::Widget& canvas);
   void show();
   void hide();

   void change_active(const string& new_res_name);
   void select_first_item();
   
   ResourceList resource_list;
   ResourceList::const_iterator resource_it;
   ISceneryPtr active_item;
   gui::ILayoutPtr layout;
   float rotation;
   string res_name, gui_path, res_class;
};

SceneryPicker::SceneryPicker(gui::ILayoutPtr l,
   const string& resource_class, const string& gui_path,
   const string& btn_gui_path)
   : layout(l),
     rotation(0.0f),
     gui_path(gui_path),
     res_class(resource_class)
{
   using namespace placeholders;
   
   enum_resources(resource_class, resource_list);
   
   layout->get(gui_path + "/preview").connect(gui::Widget::SIG_RENDER,
      bind(&SceneryPicker::render_preview, this, _1));
   layout->get(gui_path + "/next").connect(gui::Widget::SIG_CLICK,
      bind(&SceneryPicker::next, this));
   layout->get(gui_path + "/prev").connect(gui::Widget::SIG_CLICK,
      bind(&SceneryPicker::prev, this));

   if (layout->exists(gui_path + "/rotate"))
      layout->get(gui_path + "/rotate").connect(gui::Widget::SIG_CLICK,
         bind(&SceneryPicker::rotate, this));

   layout->get(btn_gui_path).connect(gui::Widget::SIG_ENTER,
      bind(&SceneryPicker::show, this));
   layout->get(btn_gui_path).connect(gui::Widget::SIG_LEAVE,
      bind(&SceneryPicker::hide, this));

   hide();
}

void SceneryPicker::select_first_item()
{
   // A kludge to avoid calling the pure virtual get() in constructor
   
   if (resource_list.empty())
      warn() << "No scenery found in class " << res_class;
   else {
      resource_it = resource_list.begin();
      change_active((*resource_it)->name());
   }
}
    
void SceneryPicker::next()
{
   if (++resource_it == resource_list.end())
      resource_it = resource_list.begin();
   
   change_active((*resource_it)->name());      
}
   
void SceneryPicker::prev()
{
   if (resource_it == resource_list.begin())
      resource_it = resource_list.end();
   resource_it--;
   
   change_active((*resource_it)->name());
}

void SceneryPicker::show()
{
   layout->get(gui_path).visible(true);
}

void SceneryPicker::hide()
{
   layout->get(gui_path).visible(false);
}

void SceneryPicker::render_preview(gui::Widget& canvas)
{
   static ILightPtr sun = make_sun_light();
      
   glRotatef(45.0f, 1.0f, 0.0f, 0.0f);
   glRotatef(45.0f, 0.0f, 1.0f, 0.0f);
   glTranslatef(1.5f, -2.6f, -1.5f);
   glColor3f(1.0f, 1.0f, 1.0f);
   sun->apply();
   
   active_item->render();
}

void SceneryPicker::change_active(const string& new_res_name)
{
   if (new_res_name != res_name) {
      res_name = new_res_name;
      active_item = this->get();
      
      layout->cast<gui::Label&>(gui_path + "/name")
         .text(active_item->name());
   }   
}

void SceneryPicker::rotate()
{
   rotation += 90.0f;
   if (rotation >= 350.0f)
      rotation = 0.0f;

   active_item->set_angle(rotation);
}

class BuildingPicker : public SceneryPicker {
public:
   BuildingPicker(gui::ILayoutPtr layout)
      : SceneryPicker(layout, "buildings",
         "/building_wnd", "/tool_wnd/tools/building")
   {
      select_first_item();
   }

   virtual ISceneryPtr get() const
   {
      return load_building(res_name, rotation);
   }
};

class TreePicker : public SceneryPicker {
public:
   TreePicker(gui::ILayoutPtr layout)
      : SceneryPicker(layout, "trees",
         "/tree_wnd", "/tool_wnd/tools/tree")
   {
      select_first_item();
   }

   virtual ISceneryPtr get() const
   {
      return load_tree(res_name);
   }
};
   
ISceneryPickerPtr make_tree_picker(gui::ILayoutPtr layout)
{
   return ISceneryPickerPtr(new TreePicker(layout));
}

ISceneryPickerPtr make_building_picker(gui::ILayoutPtr layout)
{
   return ISceneryPickerPtr(new BuildingPicker(layout));
}
