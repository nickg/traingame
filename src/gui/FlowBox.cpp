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

#include "gui/IContainer.hpp"

#include <vector>

using namespace gui;
using namespace std;

// A container which grows either horizontally or vertically as controls
// are added
class FlowBox : public IContainer {
public:
   FlowBox(FlowBoxStyle aStyle);
   ~FlowBox() {}

   // IContainer interface
   void addChild(IControlPtr aControl);

   // IControl interface
   void render(int x, int y) const;
   int width() const;
   int height() const;
   void setVisible(bool visible) { amVisible = visible; }
private:
   FlowBoxStyle myStyle;
   bool amVisible;

   typedef vector<IControlPtr> ControlList;
   ControlList myControls;

   static const int SPACING = 3;
};

FlowBox::FlowBox(FlowBoxStyle aStyle)
   : myStyle(aStyle), amVisible(true)
{

}

int FlowBox::width() const
{
   int w = 0;
   
   // Width is the maximum for BOX_VERT and sum for BOX_HORIZ
   for (ControlList::const_iterator it = myControls.begin();
        it != myControls.end(); ++it) {
      if (myStyle == FLOW_BOX_VERT)
         w = max(w, (*it)->width());
      else
         w += (*it)->width();
   }
   
   return w;
}

int FlowBox::height() const
{
   int h = 0;
   
   // Height is the sum for BOX_VERT and maximum for BOX_HORIZ
   for (ControlList::const_iterator it = myControls.begin();
        it != myControls.end(); ++it) {
      if (myStyle == FLOW_BOX_VERT)
         h = max(h, (*it)->height());
      else
         h += (*it)->height();
   }
   
   return h;
}

void FlowBox::render(int x, int y) const
{
   if (!amVisible)
      return;
   
   for (ControlList::const_iterator it = myControls.begin();
        it != myControls.end(); ++it) {
      (*it)->render(x, y);

      if (myStyle == FLOW_BOX_VERT)
         y += (*it)->height() + SPACING;
      else
         x += (*it)->width() + SPACING;
   }
}

void FlowBox::addChild(IControlPtr aControl)
{
   myControls.push_back(aControl);
}

IContainerPtr gui::makeFlowBox(FlowBoxStyle aStyle)
{
   return IContainerPtr(new FlowBox(aStyle));
}

