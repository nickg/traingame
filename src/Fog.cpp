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

#ifndef INC_IFOG_HPP
#define INC_IFOG_HPP

#include "IFog.hpp"

#include <GL/gl.h>

// Concrete implementation of fog
class Fog : public IFog {
public:
   Fog(float r, float g, float b,
       float density, float start, float end)
      : r(r), g(g), b(b),
        density(density), start(start), end(end) {}
   
   void apply() const;

private:
   float r, g, b;
   float density, start, end;
};

void Fog::apply() const
{
   GLfloat fogColor[4] = { r, g, b, 1.0f };
   glFogi(GL_FOG_MODE, GL_LINEAR);
   glFogfv(GL_FOG_COLOR, fogColor);
   glFogf(GL_FOG_DENSITY, density);
   glHint(GL_FOG_HINT, GL_DONT_CARE);
   glFogf(GL_FOG_START, start);
   glFogf(GL_FOG_END, end);
   glEnable(GL_FOG);
}

IFogPtr make_fog(float r, float g, float b,
                float density, float start, float end)
{
   return IFogPtr(new Fog(r, g, b, density, start, end));
}

IFogPtr make_fog(float density, float start, float end)
{
   float params[4];
   glGetFloatv(GL_COLOR_CLEAR_VALUE, params);
   
   return make_fog(params[0], params[1], params[2],
      density, start, end);
}

#endif
