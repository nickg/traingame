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

#include "gui2/Widget.hpp"
#include "gui2/ContainerWidget.hpp"
#include "gui2/Window.hpp"
#include "gui2/Button.hpp"
#include "gui2/Label.hpp"

#include <vector>
#include <sstream>

using namespace gui;

class Layout : public ILayout, private IXMLCallback {
public:
   Layout(const string& file_name);

   // ILayout interface
   IWidgetPtr get(const string& path) const;
   void render() const;

   // IXMLCallback interface
   void startElement(const string& local_name, const AttributeSet &attrs);
private:

   // Manages paths during parsing
   class PathStack {
   public:
      void push(const string& s) { path_comps.push_back(s); }
      void pop() { path_comps.pop_back(); }

      string str() const;
      
   private:
      vector<string> path_comps;
   };

   PathStack parse_path;
};

Layout::Layout(const string& file_name)
{   
   IXMLParserPtr parser = makeXMLParser("schemas/layout.xsd");
   parser->parse(file_name, *this);
}

void Layout::startElement(const string& local_name,
   const AttributeSet &attrs)
{
   Widget* w = NULL;

   if (local_name == "layout") {
      parse_path.push("<<layout>>");
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

   parse_path.push(w->name());
}

void Layout::render() const
{

}

IWidgetPtr Layout::get(const string& path) const
{
   return IWidgetPtr();
}

string Layout::PathStack::str() const
{
   ostringstream ss;

   if (path_comps.empty())
      return "/";
   else {
      for (vector<string>::const_iterator it = path_comps.begin();
           it != path_comps.end(); ++it)
         ss << "/" << *it;
      
      return ss.str();
   }
}

ILayoutPtr gui::make_layout(const string& file_name)
{
   return ILayoutPtr(new Layout(file_name));
}
