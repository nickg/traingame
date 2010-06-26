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

#include "ISkyBox.hpp"
#include "ITexture.hpp"
#include "Maths.hpp"

#include <GL/gl.h>

// Concrete implementation of skyboxes
class SkyBox : public ISkyBox {
public:
   SkyBox(const string& a_base_name);

   // ISkyBox interface
   void apply(float an_angle) const;
   
private:
   void load_sky_texture(int an_index, const string& a_suffix);
   
   ITexturePtr textures[6];
   const string base_name;
};

// The base name is used to generate the file names of all six
// textures
SkyBox::SkyBox(const string& a_base_name)
   : base_name(a_base_name)
{
   load_sky_texture(0, "bottom");
   load_sky_texture(1, "top");
   load_sky_texture(2, "front");
   load_sky_texture(3, "front");
   load_sky_texture(4, "front");
   load_sky_texture(5, "front");
}

void SkyBox::load_sky_texture(int an_index, const string& a_suffix)
{
   textures[an_index] =
      load_texture("images/" + base_name + "_" + a_suffix + ".png");
}

void SkyBox::apply(float an_angle) const
{
   glColor3f(1.0f, 1.0f, 1.0f);

   const float r = 5.0f;

   glPushAttrib(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT);
   glDisable(GL_CULL_FACE);
   glDisable(GL_LIGHTING);

   glDepthMask(0);

   glPushMatrix();
   glLoadIdentity();

   glRotatef(rad_to_deg<float>(an_angle), 0.0f, 1.0f, 0.0f);
   
   // Bottom
   textures[0]->bind();
   glBegin(GL_QUADS);
   glTexCoord2f(0.0f, 1.0f); glVertex3f(-r, -r, -r);
   glTexCoord2f(0.0f, 0.0f); glVertex3f(-r, -r, r);
   glTexCoord2f(1.0f, 0.0f); glVertex3f(r, -r, r);
   glTexCoord2f(1.0f, 1.0f); glVertex3f(r, -r, -r);
   glEnd();

   // Top
   textures[1]->bind();
   glBegin(GL_QUADS);
   glTexCoord2f(0.0f, 1.0f); glVertex3f(-r, r, -r);
   glTexCoord2f(0.0f, 0.0f); glVertex3f(-r, r, r);
   glTexCoord2f(1.0f, 0.0f); glVertex3f(r, r, r);
   glTexCoord2f(1.0f, 1.0f); glVertex3f(r, r, -r);
   glEnd();
   
   // Front
   textures[2]->bind();
   glBegin(GL_QUADS);
   glTexCoord2f(0.0f, 1.0f); glVertex3f(-r, -r, -r);
   glTexCoord2f(0.0f, 0.0f); glVertex3f(-r, r, -r);
   glTexCoord2f(1.0f, 0.0f); glVertex3f(r, r, -r);
   glTexCoord2f(1.0f, 1.0f); glVertex3f(r, -r, -r);
   glEnd();

   // Back
   textures[3]->bind();
   glBegin(GL_QUADS);
   glTexCoord2f(0.0f, 1.0f); glVertex3f(-r, -r, r);
   glTexCoord2f(0.0f, 0.0f); glVertex3f(-r, r, r);
   glTexCoord2f(1.0f, 0.0f); glVertex3f(r, r, r);
   glTexCoord2f(1.0f, 1.0f); glVertex3f(r, -r, r);
   glEnd();

   // Left
   textures[4]->bind();
   glBegin(GL_QUADS);
   glTexCoord2f(0.0f, 1.0f); glVertex3f(-r, -r, -r);
   glTexCoord2f(0.0f, 0.0f); glVertex3f(-r, r, -r);
   glTexCoord2f(1.0f, 0.0f); glVertex3f(-r, r, r);
   glTexCoord2f(1.0f, 1.0f); glVertex3f(-r, -r, r);
   glEnd();
   
   // Right
   textures[5]->bind();
   glBegin(GL_QUADS);
   glTexCoord2f(0.0f, 1.0f); glVertex3f(r, -r, -r);
   glTexCoord2f(0.0f, 0.0f); glVertex3f(r, r, -r);
   glTexCoord2f(1.0f, 0.0f); glVertex3f(r, r, r);
   glTexCoord2f(1.0f, 1.0f); glVertex3f(r, -r, r);
   glEnd();

   glPopMatrix();
   glPopAttrib();
}

ISkyBoxPtr make_sky_box(const string& a_base_name)
{
   return ISkyBoxPtr(new SkyBox(a_base_name));
}
