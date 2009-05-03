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

#include "gui/IControl.hpp"

#include <cstdarg>
#include <cstring>

using namespace gui;
using namespace std;

// A simple text label
class Label : public ITextControl {
public:
   Label(IFontPtr aFont, const std::string& aString);
   ~Label() {}
   
   // IControl interface
   void render(int x, int y) const;

   // ITextControl interface
   void setText(const string& aString) { myText = aString; }
   void setText(const char* fmt, ...);
   
private:
   string myText;
   IFontPtr myFont;
};

Label::Label(IFontPtr aFont, const std::string& aString)
   : myText(aString), myFont(aFont)
{

}

void Label::render(int x, int y) const
{
   myFont->print(x, y, myText.c_str());
}

void Label::setText(const char* fmt, ...)
{
   const int MAX_FMT_LENGTH = 1024;
   static char buf[MAX_FMT_LENGTH];

   va_list ap;
   va_start(ap, fmt);
   vsnprintf(buf, MAX_FMT_LENGTH, fmt, ap);
   va_end(ap);

   myText = buf;
}

ITextControlPtr gui::makeLabel(IFontPtr aFont, const std::string& aString)
{
   return ITextControlPtr(new Label(aFont, aString));
}
