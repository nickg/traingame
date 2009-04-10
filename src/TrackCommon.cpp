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

#include "TrackCommon.hpp"
#include "ILogger.hpp"
#include "Maths.hpp"

#include <cmath>

#include <GL/gl.h>

namespace Track {
   const double railWidth = 0.05;
   const double railHeight = 0.1;
   const double gauge = 0.7;

   const int sleepersPerUnit = 4;
}

// Draw a sleeper in the current maxtrix location
void renderSleeper()
{
   const double sleeperWidth = 0.1;
   const double sleeperDepth = 0.05;
   const double sleeperOff = sleeperWidth / 2.0;
   
   glPushMatrix();
   glColor3d(0.5, 0.3, 0.0);
   glBegin(GL_QUADS);

   // Top
   glNormal3d(0.0, 1.0, 0.0);  // Up
   glVertex3d(-sleeperOff, sleeperDepth, -0.5);
   glVertex3d(-sleeperOff, sleeperDepth, 0.5);
   glVertex3d(sleeperOff, sleeperDepth, 0.5);
   glVertex3d(sleeperOff, sleeperDepth, -0.5);

   // Side 1
   glNormal3d(1.0, 0.0, 0.0);  // +ve x
   glVertex3d(sleeperOff, sleeperDepth, -0.5);
   glVertex3d(sleeperOff, 0.0, -0.5);
   glVertex3d(-sleeperOff, 0.0, -0.5);
   glVertex3d(-sleeperOff, sleeperDepth, -0.5);

   // Side 2
   glNormal3d(-1.0, 0.0, 0.0);  // -ve x
   glVertex3d(-sleeperOff, sleeperDepth, 0.5);
   glVertex3d(-sleeperOff, 0.0, 0.5);
   glVertex3d(sleeperOff, 0.0, 0.5);
   glVertex3d(sleeperOff, sleeperDepth, 0.5);

   // Front
   glNormal3d(0.0, 0.0, 1.0);  // +ve z
   glVertex3d(sleeperOff, 0.0, 0.5);
   glVertex3d(sleeperOff, 0.0, -0.5);
   glVertex3d(sleeperOff, sleeperDepth, -0.5);
   glVertex3d(sleeperOff, sleeperDepth, 0.5);

   // Back
   glNormal3d(0.0, 0.0, -1.0);  // -ve z
   glVertex3d(-sleeperOff, sleeperDepth, 0.5);
   glVertex3d(-sleeperOff, sleeperDepth, -0.5);
   glVertex3d(-sleeperOff, 0.0, -0.5);
   glVertex3d(-sleeperOff, 0.0, 0.5);
   
   
   glEnd();  // glBegin(GL_QUADS)
   glPopMatrix();
}

static void renderOneRail()
{
   glPushMatrix();
   glTranslated(-Track::railWidth/2.0, 0.0, 0.0);
   
   glBegin(GL_QUADS);
   
   // Top side
   glNormal3d(0.0, 1.0, 0.0);
   glVertex3d(0.0, Track::railHeight, 0.0);
   glVertex3d(0.0, Track::railHeight, 1.0);
   glVertex3d(Track::railWidth, Track::railHeight, 1.0);
   glVertex3d(Track::railWidth, Track::railHeight, 0.0);
   
   // Outer side
   glNormal3d(0.0, 0.0, -1.0);
   glVertex3d(0.0, Track::railHeight, 0.0);
   glVertex3d(0.0, 0.0, 0.0);
   glVertex3d(0.0, 0.0, 1.0);
   glVertex3d(0.0, Track::railHeight, 1.0);
   
   // Inner side
   glNormal3d(0.0, 0.0, 1.0);
   glVertex3d(Track::railWidth, Track::railHeight, 1.0);
   glVertex3d(Track::railWidth, 0.0, 1.0);
   glVertex3d(Track::railWidth, 0.0, 0.0);
   glVertex3d(Track::railWidth, Track::railHeight, 0.0);
   
   glEnd();
   glPopMatrix();
}

void renderStraightRail()
{
   glPushMatrix();
   glColor3d(0.7, 0.7, 0.7);

   glTranslated(-Track::gauge/2.0, 0.0, -0.5);
   renderOneRail();
   
   glTranslated(Track::gauge, 0.0, 0.0);
   renderOneRail();

   glPopMatrix();
}

// A utility function for debugging normal calculation
namespace {
   void drawNormal(const Vector<double>& aPosition,
                   const Vector<double>& aNormal)
   {
      glPushAttrib(GL_ENABLE_BIT);
      
      glDisable(GL_CULL_FACE);
      glDisable(GL_LIGHTING);
      glDisable(GL_BLEND);
      glDisable(GL_TEXTURE_2D);

      glPushAttrib(GL_CURRENT_BIT);
      glColor3d(1.0, 0.0, 0.0);

      Vector<double> normPos = aPosition + aNormal;
      
      glBegin(GL_LINES);

      glVertex3d(aPosition.x, aPosition.y, aPosition.z);
      glVertex3d(normPos.x, normPos.y, normPos.z);

      glEnd();
      
      glPopAttrib();
      glPopAttrib();
   }
}

enum RailType {
   InnerRail, OuterRail
};

static void makeCurveRail(double baseRadius, double startAngle,
                          double finishAngle, RailType type)
{
   const double edgeWidth = (1 - Track::gauge - Track::railWidth)/2.0;
   const double R = baseRadius - edgeWidth
      - (type == OuterRail ? 0 : Track::gauge);
   const double r = R - Track::railWidth;

   const double step = 0.1;

   glBegin(GL_LINES);
   glColor3d(1.0, 0.0, 0.0);
   glVertex3d(0.0, -5.0, 0.0);
   glVertex3d(0.0, 5.0, 0.0);
   glEnd();

   glPushMatrix();
   glRotated(startAngle * 180.0 / M_PI, 0.0, 1.0, 0.0);
   glTranslated((baseRadius-1)*-sin(startAngle) - 0.5, 0.0,
                (baseRadius-1)*cos(startAngle) - 0.5);

   glColor3d(0.7, 0.7, 0.7);

   // Top of rail
   glBegin(GL_QUADS);
   for (double theta = 0; theta < finishAngle - startAngle; theta += step) {
      glNormal3d(0.0, 1.0, 0.0);
      glVertex3d(r * cos(theta), 0.1, r * sin(theta)); 
      glVertex3d(r * cos(theta + step), 0.1, r * sin(theta + step));
      glVertex3d(R * cos(theta + step), 0.1, R * sin(theta + step));
      glVertex3d(R * cos(theta), 0.1, R * sin(theta));
   }
   glEnd();

   // Outer edge
   for (double theta = 0; theta < finishAngle - startAngle; theta += step) {
      const double sinT = sin(theta);
      const double cosT = cos(theta);
      const double sinT1 = sin(theta + step);
      const double cosT1 = cos(theta + step);
      
      glBegin(GL_QUADS);
   
      glNormal3d(cosT1, 0.0, sinT1);
      glVertex3d(R * cosT1, 0.1, R * sinT1);
      
      glNormal3d(cosT1, 0.0, sinT1);
      glVertex3d(R * cosT1, 0.0, R * sinT1);
      
      glNormal3d(cosT, 0.0, sinT);
      glVertex3d(R * cosT, 0.0, R * sinT);
      
      glNormal3d(cosT, 0.0, sinT);
      glVertex3d(R * cosT, 0.1, R * sinT);
      
      glEnd();
   }

   // Inner edge
   glBegin(GL_QUADS);
   for (double theta = 0; theta < finishAngle - startAngle; theta += step) {
      const double sinT = sin(theta);
      const double cosT = cos(theta);
      const double sinT1 = sin(theta + step);
      const double cosT1 = cos(theta + step);
      
      glNormal3d(-cosT, 0.0, -sinT);
      glVertex3d(r * cosT, 0.1, r * sinT);
      
      glNormal3d(-cosT, 0.0, -sinT);
      glVertex3d(r * cosT, 0.0, r * sinT);
      
      glNormal3d(-cosT1, 0.0, -sinT1);
      glVertex3d(r * cosT1, 0.0, r * sinT1);
      
      glNormal3d(-cosT1, 0.0, -sinT1);
      glVertex3d(r * cosT1, 0.1, r * sinT1);
   }
   glEnd();

   glPopMatrix();
}

// `baseRadius' is measured in tiles
void renderCurveRail(int baseRadius, double startAngle, double endAngle)
{
   const double baseRadiusD = static_cast<double>(baseRadius);
   makeCurveRail(baseRadiusD, startAngle, endAngle, OuterRail);
   makeCurveRail(baseRadiusD, startAngle, endAngle, InnerRail);

   const double length = (endAngle - startAngle) * baseRadius;
   const int numSleepers = length * Track::sleepersPerUnit;
   const double sleeperAngle =
      ((endAngle - startAngle) / numSleepers) * (180.0 / M_PI);

   glPushMatrix();
   glTranslated((baseRadius-1)*-sin(startAngle) - 0.5, 0.0,
                (baseRadius-1)*cos(startAngle) - 0.5);

   const double startAngleDeg = startAngle * 180.0 / M_PI;
   
   for (int i = 0; i < numSleepers; i++) {
      glPushMatrix();
      
      glRotated(startAngleDeg + (i + 0.5)*sleeperAngle, 0.0, 1.0, 0.0);
      glTranslated(0.0, 0.0, static_cast<double>(baseRadius) - 0.5);
      
      renderSleeper();
      
      glPopMatrix();
   }

   glPopMatrix();
}
