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

#ifndef INC_GUI_ICONTAINER_HPP
#define INC_GUI_ICONTAINER_HPP

#include "Platform.hpp"
#include "IControl.hpp"

// GUI objects that contain other controls
namespace gui {

   struct IContainer : IControl {
      virtual ~IContainer() {}

      // Add a control to the container in the next available position
      virtual void addChild(IControlPtr aControl) = 0;

      // Set the absolute position of the container
      // Note that containers ignore the position on calls
      // to render
      virtual void setOrigin(int x, int y) = 0;

      // Get the position of the origin
      void origin(int& x, int& y) const;
   };

   typedef std::tr1::shared_ptr<IContainer> IContainerPtr;

   enum FlowBoxStyle {
      FLOW_BOX_HORIZ, FLOW_BOX_VERT
   };
   
   // Standard containers
   IContainerPtr makeFlowBox(FlowBoxStyle aStyle, bool wantSpacing=true);
}

#endif
