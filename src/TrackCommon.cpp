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

namespace track {
   const double railWidth = 0.05;
   const double gauge = 0.5;

   const int sleepersPerUnit = 4;

   const float SLEEPER_LENGTH = 0.8;
}

// Draw a sleeper in the current maxtrix location
void renderSleeper()
{
   const double sleeperWidth = 0.1;
   const double sleeperDepth = 0.05;
   const double sleeperOff = sleeperWidth / 2.0;

   glPushAttrib(GL_ENABLE_BIT);
   glDisable(GL_TEXTURE_2D);
   glDisable(GL_BLEND);
   
   glPushMatrix();
   glColor3d(0.5, 0.3, 0.0);
   glBegin(GL_QUADS);

   const double r = track::SLEEPER_LENGTH / 2.0;

   // Top
   glNormal3d(0.0, 1.0, 0.0);  // Up
   glVertex3d(-sleeperOff, sleeperDepth, -r);
   glVertex3d(-sleeperOff, sleeperDepth, r);
   glVertex3d(sleeperOff, sleeperDepth, r);
   glVertex3d(sleeperOff, sleeperDepth, -r);

   // Side 1
   glNormal3d(1.0, 0.0, 0.0);  // +ve x
   glVertex3d(sleeperOff, sleeperDepth, -r);
   glVertex3d(sleeperOff, 0.0, -r);
   glVertex3d(-sleeperOff, 0.0, -r);
   glVertex3d(-sleeperOff, sleeperDepth, -r);

   // Side 2
   glNormal3d(-1.0, 0.0, 0.0);  // -ve x
   glVertex3d(-sleeperOff, sleeperDepth, r);
   glVertex3d(-sleeperOff, 0.0, r);
   glVertex3d(sleeperOff, 0.0, r);
   glVertex3d(sleeperOff, sleeperDepth, r);

   // Front
   glNormal3d(0.0, 0.0, 1.0);  // +ve z
   glVertex3d(sleeperOff, 0.0, r);
   glVertex3d(sleeperOff, 0.0, -r);
   glVertex3d(sleeperOff, sleeperDepth, -r);
   glVertex3d(sleeperOff, sleeperDepth, r);

   // Back
   glNormal3d(0.0, 0.0, -1.0);  // -ve z
   glVertex3d(-sleeperOff, sleeperDepth, r);
   glVertex3d(-sleeperOff, sleeperDepth, -r);
   glVertex3d(-sleeperOff, 0.0, -r);
   glVertex3d(-sleeperOff, 0.0, r);
   
   
   glEnd();  // glBegin(GL_QUADS)
   
   glPopMatrix();
   glPopAttrib();
}

static void renderOneRail()
{
   glPushMatrix();
   glTranslated(-track::railWidth/2.0, 0.0, 0.0);
   
   glBegin(GL_QUADS);
   
   // Top side
   glNormal3d(0.0, 1.0, 0.0);
   glVertex3d(0.0, track::RAIL_HEIGHT, 0.0);
   glVertex3d(0.0, track::RAIL_HEIGHT, 1.0);
   glVertex3d(track::railWidth, track::RAIL_HEIGHT, 1.0);
   glVertex3d(track::railWidth, track::RAIL_HEIGHT, 0.0);
   
   // Outer side
   glNormal3d(-1.0, 0.0, 0.0);
   glVertex3d(0.0, track::RAIL_HEIGHT, 0.0);
   glVertex3d(0.0, 0.0, 0.0);
   glVertex3d(0.0, 0.0, 1.0);
   glVertex3d(0.0, track::RAIL_HEIGHT, 1.0);
   
   // Inner side
   glNormal3d(1.0, 0.0, 0.0);
   glVertex3d(track::railWidth, track::RAIL_HEIGHT, 1.0);
   glVertex3d(track::railWidth, 0.0, 1.0);
   glVertex3d(track::railWidth, 0.0, 0.0);
   glVertex3d(track::railWidth, track::RAIL_HEIGHT, 0.0);
   
   glEnd();
   glPopMatrix();
}

void renderStraightRail()
{
   glPushMatrix();
   glColor3d(0.7, 0.7, 0.7);

   glTranslated(-track::gauge/2.0, 0.0, -0.5);
   renderOneRail();
   
   glTranslated(track::gauge, 0.0, 0.0);
   renderOneRail();

   glPopMatrix();
}

enum RailType {
   InnerRail, OuterRail
};

static void makeCurveRail(double baseRadius, double startAngle,
                          double finishAngle, RailType type)
{
   const double edgeWidth = (1 - track::gauge - track::railWidth)/2.0;
   const double R = baseRadius - edgeWidth
      - (type == OuterRail ? 0 : track::gauge);
   const double r = R - track::railWidth;

   const double step = 0.1;

   glPushAttrib(GL_ENABLE_BIT);
   glDisable(GL_TEXTURE_2D);
   glDisable(GL_BLEND);

   glPushMatrix();
   
   glRotated(startAngle * 180.0 / M_PI, 0.0, 1.0, 0.0);

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
   glPopAttrib();
}

// Move to the origin of a curved section of track
void transformToOrigin(int baseRadius, double startAngle)
{
   glTranslated((baseRadius-1)*-sin(startAngle) - 0.5, 0.0,
                (baseRadius-1)*-cos(startAngle) - 0.5);

   // There *must* be a way to incorporate this in the above translation
   // as a neat formula, but I really can't think of it
   // This is a complete a hack, but whatever...
   const double safe = 0.01;
   if (startAngle >= M_PI / 2.0 - safe
       && startAngle <= M_PI + safe)
      glTranslated(0.0, 0.0, 1.0);
   
   if (startAngle >= M_PI - safe && startAngle <= 3.0 * M_PI / 2.0 + safe)
      glTranslated(1.0, 0.0, 0.0);
}

// `baseRadius' is measured in tiles
void renderCurvedTrack(int baseRadius, double startAngle, double endAngle)
{
   glPushMatrix();
   
   transformToOrigin(baseRadius, startAngle);

   const double baseRadiusD = static_cast<double>(baseRadius);
   makeCurveRail(baseRadiusD, startAngle, endAngle, OuterRail);
   makeCurveRail(baseRadiusD, startAngle, endAngle, InnerRail);

   const double length = (endAngle - startAngle) * baseRadius;
   const int numSleepers = length * track::sleepersPerUnit;
   const double sleeperAngle =
      ((endAngle - startAngle) / numSleepers) * (180.0 / M_PI);

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
