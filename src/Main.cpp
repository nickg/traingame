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

#include "IWindow.hpp"
#include "ILogger.hpp"
#include "GameScreens.hpp"

#include <stdexcept>

#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/util/XMLString.hpp>

using namespace std;

namespace {
   IWindowPtr theWindow;
}

IWindowPtr getGameWindow()
{
   return theWindow;
}

int main(int argc, char** argv)
{
   using namespace xercesc;
   
   log() << "Program started";

   try {
      XMLPlatformUtils::Initialize();
   }
   catch (const XMLException& e) {
      char* message = XMLString::transcode(e.getMessage());
      error() << "Exception in Xerces startup: " << message;
      XMLString::release(&message);
      return 1;
   }

   log() << "Xerces initialised";

   try {
      theWindow = makeSDLWindow();
      
      IScreenPtr editor = makeEditorScreen();
      theWindow->run(editor);
   }
   catch (const runtime_error& e) {
      error() << "Fatal: " << e.what();
   }

   XMLPlatformUtils::Terminate();
   
   log() << "Finished";   
   return 0;
}
