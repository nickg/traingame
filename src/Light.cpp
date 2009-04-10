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

#include "ILight.hpp"

#include <GL/gl.h>

// Concrete non-directional light
struct SunLight : ILight {

   void apply() const
   {
      const GLfloat globalAmbient[] = { 0.4f, 0.4f, 0.4f, 1.0f };
      glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmbient);
      
      const GLfloat ambient[] = { 0.5f, 0.5f, 0.5f, 1.0f };
      const GLfloat diffuse[] = { 0.8f, 0.8f, 0.8f, 1.0f };
      const GLfloat position[] = { 0.0f, 100.0f, 0.0f,
                                   0.0f };  // => non-directional
      
      glLightfv(GL_LIGHT1, GL_AMBIENT, ambient);
      glLightfv(GL_LIGHT1, GL_DIFFUSE, diffuse);
      glLightfv(GL_LIGHT1, GL_POSITION, position);
      
      glEnable(GL_LIGHTING);
      glEnable(GL_LIGHT1);
   }

};

ILightPtr makeSunLight()
{
   return ILightPtr(new SunLight);
}


