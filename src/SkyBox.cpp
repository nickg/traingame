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
   SkyBox(const string& aBaseName);

   // ISkyBox interface
   void apply(float anAngle) const;
   
private:
   void loadSkyTexture(int anIndex, const string& aSuffix);
   
   ITexturePtr textures[6];
   const string baseName;
};

// The base name is used to generate the file names of all six
// textures
SkyBox::SkyBox(const string& aBaseName)
   : baseName(aBaseName)
{
   loadSkyTexture(0, "bottom");
   loadSkyTexture(1, "top");
   loadSkyTexture(2, "front");
   loadSkyTexture(3, "front");
   loadSkyTexture(4, "front");
   loadSkyTexture(5, "front");
}

void SkyBox::loadSkyTexture(int anIndex, const string& aSuffix)
{
   textures[anIndex] =
      loadTexture("data/images/" + baseName + "_" + aSuffix + ".png");
}

void SkyBox::apply(float anAngle) const
{
   glColor3f(1.0f, 1.0f, 1.0f);

   const float r = 5.0f;

   glPushAttrib(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT);
   glDisable(GL_CULL_FACE);
   glDisable(GL_LIGHTING);

   glDepthMask(0);

   glPushMatrix();
   glLoadIdentity();

   glRotatef(radToDeg<float>(anAngle), 0.0f, 1.0f, 0.0f);
   
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

ISkyBoxPtr makeSkyBox(const string& aBaseName)
{
   return ISkyBoxPtr(new SkyBox(aBaseName));
}
