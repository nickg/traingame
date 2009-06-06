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

#ifndef INC_ISCREEN_HPP
#define INC_ISCREEN_HPP

#include "Platform.hpp"
#include "IGraphics.hpp"
#include "IPickBuffer.hpp"

#include <SDL.h>

enum MouseButton {
   MOUSE_UNKNOWN, MOUSE_LEFT, MOUSE_MIDDLE, MOUSE_RIGHT,
   MOUSE_WHEEL_UP, MOUSE_WHEEL_DOWN
};

// Interface for game screens
struct IScreen {
   virtual ~IScreen() {}

   // Draw the 3D part of the screen
   virtual void display(IGraphicsPtr aContext) const = 0;

   // Draw the 2D part of the screen
   virtual void overlay() const = 0;

   // Update the state of the game
   // Delta delay is milliseconds since last frame
   virtual void update(IPickBufferPtr aPickBuffer, int aDelta) = 0;
   
   virtual void onKeyDown(SDLKey aKey) = 0;
   virtual void onKeyUp(SDLKey aKey) = 0;
   virtual void onMouseMove(IPickBufferPtr aPickBuffer, int x, int y,
                            int xrel, int yrel) = 0;
   virtual void onMouseClick(IPickBufferPtr aPickBuffer, int x, int y,
                             MouseButton aButton) = 0;
   virtual void onMouseRelease(IPickBufferPtr aPickBuffer, int x, int y,
                               MouseButton aButton) = 0;
};

typedef shared_ptr<IScreen> IScreenPtr;

#endif
