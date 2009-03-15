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

#include "IMap.hpp"
#include "IQuadTree.hpp"
#include "Maths.hpp"
#include "ILogger.hpp"
#include "ITrackSegment.hpp"

#include <stdexcept>
#include <cassert>

#include <GL/gl.h>

using namespace std;
using namespace std::tr1;

class Map : public IMap, public ISectorRenderable,
            public enable_shared_from_this<Map> {
public:
   Map();
   ~Map();

   int width() const { return myWidth; }
   int depth() const { return myDepth; }
   double heightAt() const { return 0.0; }

   void render(IGraphicsPtr aContext) const;

   void resetMap(int aWidth, int aDepth);

   // ISectorRenderable interface
   void renderSector(IGraphicsPtr aContext,
                     Point<int> botLeft, Point<int> topRight);
private:
   // Tiles on the map
   struct Tile {
      struct Vertex {
         Vector<double> pos, normal;
      } v[4];
      ITrackSegmentPtr track;  // Track at this location, if any
   } *myTiles;

   static const unsigned TILE_NAME_BASE	= 32000;	// Base of tile naming
   static const unsigned NULL_OBJECT		= 0;		  // Non-existant object
   static const double TILE_HEIGHT		  = 0.2;	  // Standard height increment

   inline int index(int x, int y) const
   {
      assert(x < myWidth && y < myDepth);
      return x + y*myWidth;
   }
   
   inline int tileName(int x, int z) const
   {
      return TILE_NAME_BASE + index(x, z);
   }

   inline Tile& tileAt(int x, int z) const
   {
      return myTiles[index(x, z)];
   }

   int myWidth, myDepth;
   IQuadTreePtr myQuadTree;
};

Map::Map()
   : myTiles(NULL), myWidth(0), myDepth(0)
{
   
}

Map::~Map()
{
   delete myTiles;
}

void Map::resetMap(int aWidth, int aDepth)
{   
   if (aWidth != aDepth)
      throw runtime_error("Maps must be square");
 
   myWidth = aWidth;
   myDepth = aDepth;
   
   // Allocate memory
   if (myTiles)
      delete[] myTiles;
   myTiles = new Tile[aWidth * aDepth];
   
   // Clear map
   for (int i = 0; i < aWidth * aDepth; i++) {
      myTiles[i].v[0].pos = makeVector(-0.5, 0.0, -0.5);
      myTiles[i].v[1].pos = makeVector(-0.5, 0.0, 0.5);
      myTiles[i].v[2].pos = makeVector(0.5, 0.0, 0.5);
      myTiles[i].v[3].pos = makeVector(0.5, 0.0, -0.5);
      
      Vector<double> n = surfaceNormal(myTiles[i].v[0].pos,
                                       myTiles[i].v[1].pos,
                                       myTiles[i].v[2].pos);
      myTiles[i].v[0].normal = n;
      myTiles[i].v[1].normal = n;
      myTiles[i].v[2].normal = n;
      myTiles[i].v[3].normal = n;
   }

   // Create a straight line of track along the side of the map
   for (int i = 0; i < 10; i++)
      tileAt(1, i).track = makeStraightTrack();
   
   // Create quad tree
   myQuadTree = makeQuadTree(shared_from_this(), myWidth);
}

void Map::render(IGraphicsPtr aContext) const
{
   glClearColor(0.6, 0.7, 0.8, 1.0);
   glPushAttrib(GL_ALL_ATTRIB_BITS);
   glColorMaterial(GL_FRONT, GL_DIFFUSE);
   glEnable(GL_COLOR_MATERIAL);
   
   myQuadTree->render(aContext);
   glPopAttrib();
}

// Render a small part of the map as directed by the quad tree
void Map::renderSector(IGraphicsPtr aContext,
                       Point<int> botLeft, Point<int> topRight)
{   
   for (int x = topRight.x-1; x >= botLeft.x; x--) {
      for (int y = botLeft.y; y < topRight.y; y++) {
         // Name this tile
         glPushName(tileName(x, y));

         Tile::Vertex* v = myTiles[index(x, y)].v;
            
         // Render tile
         glPushMatrix();
         glTranslated(static_cast<double>(x), 0, static_cast<double>(y));
         glColor3f(0.8f, 1.0f, 0.8f);
         glBegin(GL_POLYGON);

         for (int i = 0; i < 4; i++) {
            glNormal3d(v[i].normal.x, v[i].normal.y, v[i].normal.z);
            glVertex3d(v[i].pos.x, v[i].pos.y, v[i].pos.z);
         }
         
         glEnd();
         glPopMatrix();
         
         // Render grid lines
         glPushMatrix();
         glTranslated(static_cast<double>(x), 0, static_cast<double>(y));
         glColor3f(0.0f, 0.0f, 0.0f);
         glBegin(GL_LINE_LOOP);
         
         for (int i = 0; i < 4; i++) 
            glVertex3d(v[i].pos.x, v[i].pos.y, v[i].pos.z);
         
         glEnd();
         glPopMatrix();

         // Draw the track, if any
         if (tileAt(x, y).track) {
            glPushMatrix();
            glTranslated(static_cast<double>(x), 0, static_cast<double>(y));
            tileAt(x, y).track->render();
            glPopMatrix();
         }
      }			
      glPopName();
   }
}

IMapPtr makeEmptyMap(int aWidth, int aDepth)
{
   shared_ptr<Map> ptr(new Map);
   ptr->resetMap(aWidth, aDepth);
   return IMapPtr(ptr);
}
