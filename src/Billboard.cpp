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

#include "IBillboard.hpp"
#include "Colour.hpp"
#include "OpenGLHelper.hpp"
#include "ILogger.hpp"

#include <vector>
#include <algorithm>

#include <GL/gl.h>

using namespace std;

namespace {
   Vector<float> cameraPosition;
}

// Common functions used by billboards
class BillboardCommon : public IBillboard {
public:
   BillboardCommon(ITexturePtr aTexture)
      : texture(aTexture),
        position(makeVector(0.0f, 0.0f, 0.0f)),
        scale(1.0f),
        colour(makeColour(1.0f, 1.0f, 1.0f)) {}
   virtual ~BillboardCommon() {}

   // IBillboard interface
   void setPosition(float x, float y, float z);
   void setScale(float aScale);
   void setColour(float r, float g, float b, float a);
   void render() const;

   static void renderSaved();
   
private:
   const ITexturePtr texture;
   
protected:
   void drawTextureQuad() const;
   void translate() const;

   void saveMe() const;

   virtual void realRender() const = 0;
   
   Vector<float> position;
   float scale;
   Colour colour;

   struct CmpDistanceToCam {
      bool operator()(const BillboardCommon* lhs,
         const BillboardCommon* rhs)
      {
         return distanceToCamera(lhs->position)
            > distanceToCamera(rhs->position);
      }
   };
   
   // List of billboards to draw at end of this frame
   static vector<const BillboardCommon*> toDraw;
};

vector<const BillboardCommon*> BillboardCommon::toDraw;

void BillboardCommon::renderSaved()
{
   using namespace placeholders;
   
   // Depth sort the saved billboards and render them

   sort(toDraw.begin(), toDraw.end(), CmpDistanceToCam());

   for_each(toDraw.begin(), toDraw.end(),
      bind(&BillboardCommon::realRender, placeholders::_1));
   
   toDraw.clear();
}

void BillboardCommon::saveMe() const
{
   // Remember to draw this billboard at the end of the frame
   toDraw.push_back(this);
}

void BillboardCommon::render() const
{
   saveMe();
}      

void BillboardCommon::setPosition(float x, float y, float z)
{
   position = makeVector(x, y, z);
}

void BillboardCommon::setColour(float r, float g, float b, float a)
{
   colour = makeColour(r, g, b, a);
}

void BillboardCommon::setScale(float aScale)
{
   scale = aScale;
}

void BillboardCommon::translate() const
{
   gl::translate(position);
}
      
// Draw the actual quad containing the texture
void BillboardCommon::drawTextureQuad() const
{
   glPushAttrib(GL_ENABLE_BIT);

   glEnable(GL_BLEND);
   glEnable(GL_TEXTURE_2D);
   glDisable(GL_LIGHTING);

   gl::colour(colour);
   
   texture->bind();

   const float w = scale / 2.0f;

   glBegin(GL_QUADS);
   glTexCoord2f(1.0f, 0.0f);
   glVertex2f(w, w);
   glTexCoord2f(0.0f, 0.0f);
   glVertex2f(-w, w);
   glTexCoord2f(0.0f, 1.0f);
   glVertex2f(-w, -w);
   glTexCoord2f(1.0f, 1.0f);
   glVertex2f(w, -w);
   glEnd();
   
   glPopAttrib();
}

// A billboard which always faces the viewer in the xy and yz planes
// but not xz
class CylindricalBillboard : public BillboardCommon {
public:
   CylindricalBillboard(ITexturePtr aTexture)
      : BillboardCommon(aTexture) {}

   void realRender() const;
};

void CylindricalBillboard::realRender() const
{
   // Based on code from
   // http://www.lighthouse3d.com/opengl/billboarding/index.php?billCheat1
   
   float modelview[16];

   glPushAttrib(GL_DEPTH_BUFFER_BIT);
   glDepthMask(GL_FALSE);
   
   glPushMatrix();

   translate();
   
	 glGetFloatv(GL_MODELVIEW_MATRIX , modelview);

   for (int i = 0; i < 3; i += 2) 
	    for (int j = 0; j < 3; j++) {
         if (i == j)
            modelview[i*4+j] = 1.0;
         else
            modelview[i*4+j] = 0.0;
	    }

   glLoadMatrixf(modelview);

   drawTextureQuad();

   glPopMatrix();
   glPopAttrib();
}

// A billboard which always faces the viewer
class SphericalBillboard : public BillboardCommon {
public:
   SphericalBillboard(ITexturePtr aTexture)
      : BillboardCommon(aTexture) {}

   void realRender() const;   
};

void SphericalBillboard::realRender() const
{ 
   // Based on code from
   // http://www.lighthouse3d.com/opengl/billboarding/index.php?billSphe
   Vector<float> lookAt, objToCamProj, upAux, objToCam;
   float angleCosine;
   
   glPushAttrib(GL_DEPTH_BUFFER_BIT);
   glDepthMask(GL_FALSE);
   
   glPushMatrix();

   translate();
   
   // objToCamProj is the vector in world coordinates from the 
   // local origin to the camera projected in the XZ plane
   objToCamProj = makeVector(cameraPosition.x - position.x,
                             0.0f,
                             cameraPosition.z - position.z);

   // This is the original lookAt vector for the object 
   // in world coordinates
   lookAt = makeVector(0.0f, 0.0f, 1.0f);

   // normalize both vectors to get the cosine directly afterwards
   objToCamProj.normalise();

   // easy fix to determine wether the angle is negative or positive
   // for positive angles upAux will be a vector pointing in the 
   // positive y direction, otherwise upAux will point downwards
   // effectively reversing the rotation.

   upAux = lookAt * objToCamProj;

   // compute the angle
   angleCosine = lookAt.dot(objToCamProj);

   // perform the rotation. The if statement is used for stability reasons
   // if the lookAt and objToCamProj vectors are too close together then 
   // |angleCosine| could be bigger than 1 due to lack of precision
   if ((angleCosine < 0.99990) && (angleCosine > -0.9999))
      glRotatef(acos(angleCosine)*180/3.14, upAux.x, upAux.y, upAux.z);	
      
   // so far it is just like the cylindrical billboard. The code for the 
   // second rotation comes now
   // The second part tilts the object so that it faces the camera

   // objToCam is the vector in world coordinates from 
   // the local origin to the camera
   objToCam = cameraPosition - position;

   // Normalize to get the cosine afterwards
   objToCam.normalise();

   // Compute the angle between objToCamProj and objToCam, 
   //i.e. compute the required angle for the lookup vector

   angleCosine = objToCamProj.dot(objToCam);


   // Tilt the object. The test is done to prevent instability 
   // when objToCam and objToCamProj have a very small
   // angle between them

   if ((angleCosine < 0.99990) && (angleCosine > -0.9999)) {
      if (objToCam.y < 0)
         glRotatef(acos(angleCosine)*180/M_PI,1,0,0);	
      else
         glRotatef(acos(angleCosine)*180/M_PI,-1,0,0);
   }

   drawTextureQuad();

   glPopMatrix();
   glPopAttrib();
}

IBillboardPtr makeCylindricalBillboard(ITexturePtr aTexture)
{
   return IBillboardPtr(new CylindricalBillboard(aTexture));
}

IBillboardPtr makeSphericalBillboard(ITexturePtr aTexture)
{
   return IBillboardPtr(new SphericalBillboard(aTexture));
}

void setBillboardCameraOrigin(Vector<float> aPosition)
{
   cameraPosition = aPosition;
}

float distanceToCamera(Vector<float> aPosition)
{
   return (cameraPosition - aPosition).length();
}

void renderBillboards()
{
   BillboardCommon::renderSaved();
}
