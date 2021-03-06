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

#ifndef INC_IWINDOW_HPP
#define INC_IWINDOW_HPP

#include "Platform.hpp"
#include "IScreen.hpp"

// Interface to the game window
class IWindow {
public:
   virtual ~IWindow() {}
   
   virtual void run(IScreenPtr a_screen, int frames = 0) = 0;
   virtual void switch_screen(IScreenPtr a_screen) = 0;
   virtual void quit() = 0;
   virtual void take_screen_shot() = 0;
   virtual int width() const = 0;
   virtual int height() const = 0;
   virtual int get_fps() const = 0;
};

typedef shared_ptr<IWindow> IWindowPtr;

// Implementors
IWindowPtr make_sdl_window();

#endif
