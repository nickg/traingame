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

#include <SDL/SDL_keysym.h>

enum MouseButton {
   MOUSE_UNKNOWN, MOUSE_LEFT, MOUSE_MIDDLE, MOUSE_RIGHT,
   MOUSE_WHEEL_UP, MOUSE_WHEEL_DOWN
};

// Interface for game screens
struct IScreen {
   virtual ~IScreen() {}

   // Draw the 3D part of the screen
   virtual void display(IGraphicsPtr a_context) const = 0;

   // Draw the 2D part of the screen
   virtual void overlay() const = 0;

   // Update the state of the game
   // Delta delay is milliseconds since last frame
   virtual void update(IPickBufferPtr a_pick_buffer, int a_delta) = 0;
   
   virtual void on_key_down(SDLKey a_key) = 0;
   virtual void on_key_up(SDLKey a_key) = 0;
   virtual void on_mouse_move(IPickBufferPtr a_pick_buffer, int x, int y,
                            int xrel, int yrel) = 0;
   virtual void on_mouse_click(IPickBufferPtr a_pick_buffer, int x, int y,
                             MouseButton a_button) = 0;
   virtual void on_mouse_release(IPickBufferPtr a_pick_buffer, int x, int y,
                               MouseButton a_button) = 0;
};

typedef shared_ptr<IScreen> IScreenPtr;

#endif
