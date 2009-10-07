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

using namespace gui;

class Layout : public ILayout, private IXMLCallback {
public:
   Layout(const string& file_name);

   // ILayout interface
   IElementPtr get(const string& path) const;

   // IXMLCallback interface
private:
};

Layout::Layout(const string& file_name)
{
   IXMLParserPtr parser = makeXMLParser("schemas/layout.xsd");
   parser->parse(file_name, *this);
}

IElementPtr Layout::get(const string& path) const
{
   return IElementPtr();
}

ILayoutPtr gui::make_layout(const string& file_name)
{
   return ILayoutPtr(new Layout(file_name));
}
