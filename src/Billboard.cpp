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
   Vector<float> camera_position;
}

// Common functions used by billboards
class BillboardCommon : public IBillboard {
public:
   BillboardCommon(ITexturePtr a_texture)
      : texture(a_texture),
        position(make_vector(0.0f, 0.0f, 0.0f)),
        scale(1.0f),
        colour(make_colour(1.0f, 1.0f, 1.0f)) {}
   virtual ~BillboardCommon() {}

   // IBillboard interface
   void set_position(float x, float y, float z);
   void set_scale(float a_scale);
   void set_colour(float r, float g, float b, float a);
   void render() const;

   static void render_saved();
   
private:
   const ITexturePtr texture;
   
protected:
   void draw_texture_quad() const;
   void translate() const;

   void save_me() const;

   virtual void real_render() const = 0;
   
   Vector<float> position;
   float scale;
   Colour colour;

   struct CmpDistanceToCam {
      bool operator()(const BillboardCommon* lhs,
         const BillboardCommon* rhs)
      {
         return distance_to_camera(lhs->position)
            > distance_to_camera(rhs->position);
      }
   };
   
   // List of billboards to draw at end of this frame
   static vector<const BillboardCommon*> to_draw;
};

vector<const BillboardCommon*> BillboardCommon::to_draw;

void BillboardCommon::render_saved()
{
   using namespace placeholders;
   
   // Depth sort the saved billboards and render them

   sort(to_draw.begin(), to_draw.end(), CmpDistanceToCam());

   for_each(to_draw.begin(), to_draw.end(),
      bind(&BillboardCommon::real_render, placeholders::_1));
   
   to_draw.clear();
}

void BillboardCommon::save_me() const
{
   // Remember to draw this billboard at the end of the frame
   to_draw.push_back(this);
}

void BillboardCommon::render() const
{
   save_me();
}      

void BillboardCommon::set_position(float x, float y, float z)
{
   position = make_vector(x, y, z);
}

void BillboardCommon::set_colour(float r, float g, float b, float a)
{
   colour = make_colour(r, g, b, a);
}

void BillboardCommon::set_scale(float a_scale)
{
   scale = a_scale;
}

void BillboardCommon::translate() const
{
   gl::translate(position);
}
      
// Draw the actual quad containing the texture
void BillboardCommon::draw_texture_quad() const
{
   glPushAttrib(GL_ENABLE_BIT);

   glEnable(GL_BLEND);
   glEnable(GL_TEXTURE_2D);
   glDisable(GL_LIGHTING);

   gl::colour(colour);
   
   texture->bind();

   const float w = scale / 2.0f;

   glBegin(GL_QUADS);
   {
      glTexCoord2f(1.0f, 0.0f);
      glVertex2f(w, w);
      glTexCoord2f(0.0f, 0.0f);
      glVertex2f(-w, w);
      glTexCoord2f(0.0f, 1.0f);
      glVertex2f(-w, -w);
      glTexCoord2f(1.0f, 1.0f);
      glVertex2f(w, -w);
   }
   glEnd();
   
   glPopAttrib();
}

// A billboard which always faces the viewer in the xy and yz planes
// but not xz
class CylindricalBillboard : public BillboardCommon {
public:
   CylindricalBillboard(ITexturePtr a_texture)
      : BillboardCommon(a_texture) {}

   void real_render() const;
};

void CylindricalBillboard::real_render() const
{
   // Based on code from
   // http://www.lighthouse3d.com/opengl/billboarding/index.php?bill_cyl
   Vector<float> look_at, obj_to_camProj, up_aux, obj_to_cam;
   float angle_cosine;  

   glPushAttrib(GL_DEPTH_BUFFER_BIT);
   glDepthMask(GL_FALSE);
   
   glPushMatrix();

   translate();
   
   // obj_to_camProj is the vector in world coordinates from the 
   // local origin to the camera projected in the XZ plane
   obj_to_camProj = make_vector(
      camera_position.x - position.x,
      0.0f,
      camera_position.z - position.z);

   // This is the original look_at vector for the object 
   // in world coordinates
   look_at = make_vector(0.0f, 0.0f, 1.0f);

   // normalize both vectors to get the cosine directly afterwards
   obj_to_camProj.normalise();

   // easy fix to determine wether the angle is negative or positive
   // for positive angles up_aux will be a vector pointing in the 
   // positive y direction, otherwise up_aux will point downwards
   // effectively reversing the rotation.
   up_aux = look_at * obj_to_camProj;

   // compute the angle
   angle_cosine = look_at.dot(obj_to_camProj);

   // perform the rotation. The if statement is used for stability reasons
   // if the look_at and obj_to_camProj vectors are too close together then 
   // |angle_cosine| could be bigger than 1 due to lack of precision
   //if ((angle_cosine < 0.999999f) && (angle_cosine > -0.999999f))
   glRotatef(acos(angle_cosine)*180.0f/M_PI, up_aux.x, up_aux.y, up_aux.z);	

   draw_texture_quad();

   glPopMatrix();
   glPopAttrib();
}

// A billboard which always faces the viewer
class SphericalBillboard : public BillboardCommon {
public:
   SphericalBillboard(ITexturePtr a_texture)
      : BillboardCommon(a_texture) {}

   void real_render() const;   
};

void SphericalBillboard::real_render() const
{ 
   // Based on code from
   // http://www.lighthouse3d.com/opengl/billboarding/index.php?bill_sphe
   Vector<float> look_at, obj_to_camProj, up_aux, obj_to_cam;
   float angle_cosine;
   
   glPushAttrib(GL_DEPTH_BUFFER_BIT);
   glDepthMask(GL_FALSE);
   
   glPushMatrix();

   translate();
   
   // obj_to_camProj is the vector in world coordinates from the 
   // local origin to the camera projected in the XZ plane
   obj_to_camProj = make_vector(camera_position.x - position.x,
                             0.0f,
                             camera_position.z - position.z);

   // This is the original look_at vector for the object 
   // in world coordinates
   look_at = make_vector(0.0f, 0.0f, 1.0f);

   // normalize both vectors to get the cosine directly afterwards
   obj_to_camProj.normalise();

   // easy fix to determine wether the angle is negative or positive
   // for positive angles up_aux will be a vector pointing in the 
   // positive y direction, otherwise up_aux will point downwards
   // effectively reversing the rotation.

   up_aux = look_at * obj_to_camProj;

   // compute the angle
   angle_cosine = look_at.dot(obj_to_camProj);

   // perform the rotation. The if statement is used for stability reasons
   // if the look_at and obj_to_camProj vectors are too close together then 
   // |angle_cosine| could be bigger than 1 due to lack of precision
   glRotatef(acos(angle_cosine)*180.0f/M_PI, up_aux.x, up_aux.y, up_aux.z);	
      
   // so far it is just like the cylindrical billboard. The code for the 
   // second rotation comes now
   // The second part tilts the object so that it faces the camera

   // obj_to_cam is the vector in world coordinates from 
   // the local origin to the camera
   obj_to_cam = camera_position - position;

   // Normalize to get the cosine afterwards
   obj_to_cam.normalise();

   // Compute the angle between obj_to_camProj and obj_to_cam, 
   //i.e. compute the required angle for the lookup vector

   angle_cosine = obj_to_camProj.dot(obj_to_cam);

   // Tilt the object. The test is done to prevent instability 
   // when obj_to_cam and obj_to_camProj have a very small
   // angle between them

   if ((angle_cosine < 0.99990) && (angle_cosine > -0.9999)) {
      if (obj_to_cam.y < 0)
         glRotatef(acos(angle_cosine)*180/M_PI,1,0,0);	
      else
         glRotatef(acos(angle_cosine)*180/M_PI,-1,0,0);
   }

   draw_texture_quad();

   glPopMatrix();
   glPopAttrib();
}

IBillboardPtr make_cylindrical_billboard(ITexturePtr a_texture)
{
   return IBillboardPtr(new CylindricalBillboard(a_texture));
}

IBillboardPtr make_spherical_billboard(ITexturePtr a_texture)
{
   return IBillboardPtr(new SphericalBillboard(a_texture));
}

void set_billboard_cameraOrigin(Vector<float> a_position)
{
   camera_position = a_position;
}

float distance_to_camera(Vector<float> a_position)
{
   return (camera_position - a_position).length();
}

void render_billboards()
{
   BillboardCommon::render_saved();
}
