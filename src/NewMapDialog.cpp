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

#include "NewMapDialog.hpp"

#include <cstdlib>

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Input.H>

namespace {
   Fl_Window* theDlgWindow = NULL;   
   Fl_Button* theCancelBtn;
   Fl_Button* theOKBtn;
   Fl_Input* theNameInput;

   NewMapDlgCallback theCallback;

   void onCancelClick(Fl_Widget* aWidget)
   {
      theCallback(IMapPtr(), CANCEL);
   }

   void onOKClick(Fl_Widget* aWidget)
   {
      theCallback(makeEmptyMap("yah", 64, 64), OK);
   }
}

void showNewMapDialog(NewMapDlgCallback aCallback)
{
   if (theDlgWindow == NULL) {
      theDlgWindow = new Fl_Window(300, 150, "New Map");
      
      theNameInput = new Fl_Input(50, 10, 100, 25, "Name");
      
      theCancelBtn = new Fl_Button(10, 100, 100, 25, "Cancel");
      theCancelBtn->callback(onCancelClick);
      
      theOKBtn = new Fl_Button(120, 100, 50, 25, "OK");
      theOKBtn->callback(onOKClick);
      
      theDlgWindow->set_modal();
   }

   theCallback = aCallback;

   theDlgWindow->hotspot(theDlgWindow);
   theDlgWindow->show();
}
