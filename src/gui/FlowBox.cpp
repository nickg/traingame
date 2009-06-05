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
#include "gui/Internal.hpp"

#include <vector>
#include <functional>

using namespace gui;
using namespace std;
using namespace std::tr1;
using namespace std::tr1::placeholders;

// A container which grows either horizontally or vertically as controls
// are added
class FlowBox : public Moveable<IContainer> {
public:
   FlowBox(FlowBoxStyle aStyle, bool doesWantSpacing);
   ~FlowBox() {}

   // IContainer interface
   void addChild(IControlPtr aControl);

   // IControl interface
   int width() const;
   int height() const;
   bool handleClick(int x, int y);
   bool handleMouseRelease(int x, int y);
   void render(int x, int y) const = 0;
private:
   typedef function<void (IControlPtr, int, int)> PositionVisitor;
   void eachControl(PositionVisitor aFunc) const;

   static void clickTest(IControlPtr aControl, int ctrlX, int ctrlY,
                         int mouseX, int mouseY, bool* handled);
   
   FlowBoxStyle myStyle;
   bool wantSpacing;

   typedef vector<IControlPtr> ControlList;
   ControlList myControls;

   static const int SPACING = 3;
};

FlowBox::FlowBox(FlowBoxStyle aStyle, bool doesWantSpacing)
   : myStyle(aStyle), wantSpacing(doesWantSpacing)
{

}

bool FlowBox::handleClick(int x, int y)
{
   bool handled = false;
   eachControl(bind(&FlowBox::clickTest, _1, _2, _3, x, y, &handled));
   return handled;
}

bool FlowBox::handleMouseRelease(int x, int y)
{
   eachControl(bind(&IControl::handleMouseRelease, _1, x, y));
   return false;
}

void FlowBox::clickTest(IControlPtr aControl, int ctrlX, int ctrlY,
                        int mouseX, int mouseY, bool* handled)
{
   if (mouseX >= ctrlX && mouseX < ctrlX + aControl->width()
       && mouseY >= mouseY && mouseY < ctrlY + aControl->height()) {
      aControl->handleClick(mouseX, mouseY);
      *handled = true;
   }
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

void FlowBox::eachControl(PositionVisitor aFunc) const
{
   int x, y;
   origin(x, y);
   
   for (ControlList::const_iterator it = myControls.begin();
        it != myControls.end(); ++it) {
      aFunc(*it, x, y);

      if (myStyle == FLOW_BOX_VERT)
         y += (*it)->height() + (wantSpacing ? SPACING : 0);
      else
         x += (*it)->width() + (wantSpacing ? SPACING : 0);
   }
}

void FlowBox::render(int x, int y) const
{
   eachControl(&IControl::render);
}

void FlowBox::addChild(IControlPtr aControl)
{
   myControls.push_back(aControl);
}

IContainerPtr gui::makeFlowBox(FlowBoxStyle aStyle, bool wantSpacing)
{
   return IContainerPtr(new Hideable<FlowBox>(aStyle, wantSpacing));
}

