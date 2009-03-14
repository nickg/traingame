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

#include "Editor.hpp"
#include "ILogger.hpp"
#include "IModel.hpp"
#include "IMap.hpp"

#include <GL/gl.h>

using namespace std;

// Concrete editor class
class Editor : public IScreen {
public:
   Editor();
   
   void display(IGraphicsPtr aContext);

private:
   IModelPtr m;
   IMapPtr myMap;
};

Editor::Editor()
{
   const string fileName("/home/nick/stompstomp.obj");
   m = loadModel(fileName);

   myMap = makeEmptyMap(32, 32);
}

// Render the next frame
void Editor::display(IGraphicsPtr aContext)
{
   aContext->setAmbient(0.5, 0.5, 0.5);
   aContext->setDiffuse(1.0, 1.0, 1.0);
   aContext->moveLight(0.0, 0.0, 2.0);
   
   glTranslatef(0.0f, 0.0f, -15.0f);
   glBegin(GL_TRIANGLES);
   glColor3f(1.0f, 0.0f, 0.0f);
   glVertex3f(1.0f, -1.0f, 0.0f);
   glColor3f(0.0f, 1.0f, 0.0f);
   glVertex3f(1.0f, 0.0f, 0.0f);
   glColor3f(0.0f, 0.0f, 1.0f);
   glVertex3f(0.0f, 0.0f, 0.0f);
   glEnd();

   glTranslatef(-5.0f, 0.0f, 0.0f);
   m->render();

   glTranslated(0.0, -4.0, -12.0);
   myMap->render();
}

// Create an instance of the editor screen
IScreenPtr makeEditorScreen()
{
   
   return IScreenPtr(new Editor);
}
