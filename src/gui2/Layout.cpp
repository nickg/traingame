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

#include "gui2/ILayout.hpp"
#include "IXMLParser.hpp"
#include "ILogger.hpp"

#include "gui2/Widget.hpp"
#include "gui2/ContainerWidget.hpp"
#include "gui2/Window.hpp"
#include "gui2/Button.hpp"
#include "gui2/Label.hpp"

#include <vector>
#include <sstream>
#include <map>

using namespace gui;

class Layout : public ILayout, private IXMLCallback {
public:
   Layout(const string& file_name);
   ~Layout();

   // ILayout interface
   Widget& get(const string& path) const;
   void render() const;

   void click(int x, int y);

   // IXMLCallback interface
   void startElement(const string& local_name, const AttributeSet &attrs);
   void endElement(const string& local_name);
private:

   // Manages paths during parsing
   class PathStack {
   public:
      void push(Widget* w) { path_comps.push_back(w); }
      void pop() { path_comps.pop_back(); }

      string str() const;
      Widget* top() const;
      
   private:
      vector<Widget*> path_comps;
   };

   // Root of widget hierarchy
   class RootWidget : public ContainerWidget {
   public:
      RootWidget(const AttributeSet& attrs) : ContainerWidget(attrs) {}
   };

   PathStack parse_path;
   Widget* root;
   
   typedef map<string, Widget*> WidgetMap;
   WidgetMap widgets;
};

Layout::Layout(const string& file_name)
{   
   IXMLParserPtr parser = makeXMLParser("schemas/layout.xsd");
   parser->parse(file_name, *this);

   assert(root);
   Theme dummy;
   root->adjust_for_theme(dummy);

   log() << "Loaded UI layout from " << file_name;
}

Layout::~Layout()
{
   for (WidgetMap::iterator it = widgets.begin();
        it != widgets.end(); ++it)
      delete (*it).second;
}

void Layout::startElement(const string& local_name,
   const AttributeSet &attrs)
{
   Widget* w = NULL;

   if (local_name == "layout") {
      root = new RootWidget(attrs);
      parse_path.push(root);
      return;
   }
   else if (local_name == "window")
      w = new Window(attrs);
   else if (local_name == "button")
      w = new Button(attrs);
   else if (local_name == "label")
      w = new Label(attrs);
   else
      throw runtime_error("Unexpected " + local_name);

   Widget* parent = parse_path.top();
   if (ContainerWidget* c = dynamic_cast<ContainerWidget*>(parent)) {
      c->add_child(w);
   }
   else {
      throw runtime_error("Widget " + parse_path.str()
         + " cannot contain children");
   }      

   parse_path.push(w);

   widgets[parse_path.str()] = w;
   
   debug() << "Add widget " << parse_path.str()
           << " (" << local_name << ")";
}

void Layout::endElement(const string& local_name)
{   
   parse_path.pop();
}

void Layout::render() const
{
   RenderContext rc;
   root->render(rc);
}

Widget& Layout::get(const string& path) const
{
   WidgetMap::const_iterator it = widgets.find(path);
   if (it != widgets.end())
      return *(*it).second;
   else
      throw runtime_error("Widget " + path + " does not exist");
}

void Layout::click(int x, int y)
{
   root->handle_click(x, y);
}

string Layout::PathStack::str() const
{
   ostringstream ss;

   if (path_comps.empty() || path_comps.size() == 1)
      return "/";
   else {
      // Skip over root element
      vector<Widget*>::const_iterator it = path_comps.begin() + 1;
      for (; it != path_comps.end(); ++it)
         ss << "/" << (*it)->name();
      
      return ss.str();
   }
}

Widget* Layout::PathStack::top() const
{
   assert(!path_comps.empty());

   return path_comps.back();
}

ILayoutPtr gui::make_layout(const string& file_name)
{
   return ILayoutPtr(new Layout(file_name));
}
