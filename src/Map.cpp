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
#include "IFog.hpp"
#include "IXMLParser.hpp"
#include "XMLBuilder.hpp"
#include "IMesh.hpp"

#include <stdexcept>
#include <sstream>
#include <cassert>
#include <fstream>

#include <GL/gl.h>
#include <boost/filesystem.hpp>

using namespace std;

// A single piece of track may appear multiple times in the map - this
// will be true for all track segments that cover multiple tiles
// For various map algorithms (e.g. drawing) it is useful to keep track
// of which track segments have already been visited. So track segments
// are wrapped inside TrackNode which is used to store map-specific
// information about the track.
class TrackNode {
public:
   TrackNode(ITrackSegmentPtr aTrack, int x, int y)
      : myTrack(aTrack), amMarked(false), myX(x), myY(y) {}

   inline void setMark() { amMarked = true; }
   inline void resetMark() { amMarked = false; }
   inline bool marked() const { return amMarked; }

   inline ITrackSegmentPtr get() { return myTrack; }

   inline int originX() const { return myX; }
   inline int originY() const { return myY; }
private:
   ITrackSegmentPtr myTrack;
   bool amMarked;
   int myX, myY;   // Position of origin
};

typedef shared_ptr<TrackNode> TrackNodePtr;

class Map : public IMap, public ISectorRenderable,
            public enable_shared_from_this<Map> {
   friend class MapLoader;
public:
   Map();
   ~Map();

   // IMap interface
   int width() const { return myWidth; }
   int depth() const { return myDepth; }
   double heightAt() const { return 0.0; }

   void setStart(int x, int y) { myStartLocation = makePoint(x, y); }
   void setGrid(bool onOff);
   void setPickMode(bool onOff) { inPickMode = onOff; }
   
   Track::Connection startLocation() const;
   ITrackSegmentPtr trackAt(const Point<int>& aPoint) const;
   void setTrackAt(const Point<int>& aPoint, ITrackSegmentPtr aTrack);
   bool isValidTrack(const Point<int>& aPoint) const;
   void render(IGraphicsPtr aContext) const;
   void highlightTile(IGraphicsPtr aContext, const Point<int>& aPoint) const;
   void resetMap(int aWidth, int aDepth);

   void raiseArea(const Point<int>& aStartPos,
                  const Point<int>& aFinishPos);
   void lowerArea(const Point<int>& aStartPos,
                  const Point<int>& aFinishPos);
   
   void save(const string& aFileName);
   
   // ISectorRenderable interface
   void renderSector(IGraphicsPtr aContext, int id,
                     Point<int> botLeft, Point<int> topRight);
   void postRenderSector(IGraphicsPtr aContext, int id,
                         Point<int> botLeft, Point<int> topRight);
private:
   // Tiles on the map
   struct Tile {
      TrackNodePtr track;  // Track at this location, if any
   } *myTiles;

   // Vertices on the terrain
   struct Vertex {
      Vector<float> pos, normal;
   } *myHeightMap;

   static const unsigned TILE_NAME_BASE	= 1000;	  // Base of tile naming
   static const unsigned NULL_OBJECT		= 0;		  // Non-existant object
   static const double TILE_HEIGHT		  = 0.2;	  // Standard height increment

   // Meshes for each terrain sector
   vector<IMeshPtr> myTerrainMeshes;
   
   inline int index(int x, int y) const
   {
      assert(x < myWidth && y < myDepth && x >= 0 && y >= 0);
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

   inline Vertex& heightAt(int i) const
   {
      assert(i >= 0 && i < (myWidth + 1) * (myDepth + 1));
      return myHeightMap[i];
   }
   
   bool isValidTileName(unsigned aName) const
   {
      return aName >= TILE_NAME_BASE
         && aName < TILE_NAME_BASE + myWidth * myDepth;
   }

   Point<int> pickPosition(unsigned aName) const
   {      
      assert(isValidTileName(aName));
      
      int a = aName - TILE_NAME_BASE;
      return makePoint(a % myWidth, a / myWidth);
   }

   void resetMarks() const;
   void writeHeightMap(const string& aFileName) const;
   void readHeightMap(const string& aFileName);
   void tileVertices(int x, int y, int* indexes) const;
   void renderPickSector(Point<int> botLeft, Point<int> topRight);

   // Mesh modification
   void buildMesh(int id, Point<int> botLeft, Point<int> topRight);
   bool haveMesh(int id, Point<int> botLeft, Point<int> topRight);
   void dirtyTile(int x, int y);
      
   // Terrain modification
   void changeAreaHeight(const Point<int>& aStartPos,
                         const Point<int>& aFinishPos, float aHeightDelta);
   void raiseTile(int x, int y, float deltaHeight);
   void levelTile(int x, int y);
   void fixNormals(int x, int y);
   
   int myWidth, myDepth;
   Point<int> myStartLocation;
   IQuadTreePtr myQuadTree;
   IFogPtr myFog;
   bool shouldDrawGridLines, inPickMode;
   list<Point<int>> myDirtyTiles;
};

Map::Map()
   : myTiles(NULL), myHeightMap(NULL), myWidth(0), myDepth(0),
     myStartLocation(makePoint(1, 1)),
     shouldDrawGridLines(false), inPickMode(false)
{
   myFog = makeFog(0.6, 0.7, 0.8,  // Colour
                   0.25,           // Density
                   40.0, 50.0);    // Start and end distance
}

Map::~Map()
{
   delete myTiles;
}

ITrackSegmentPtr Map::trackAt(const Point<int>& aPoint) const
{
   TrackNodePtr ptr = tileAt(aPoint.x, aPoint.y).track;
   if (ptr)
      return ptr->get();
   else {
      ostringstream ss;
      ss << "No track segment at " << aPoint;
      throw runtime_error(ss.str());
   }
}

void Map::setTrackAt(const Point<int>& aPoint, ITrackSegmentPtr aTrack)
{
   aTrack->setOrigin(aPoint.x, aPoint.y);

   TrackNodePtr node(new TrackNode(aTrack, aPoint.x, aPoint.y));  

   // Attach the track node to every endpoint
   list<Point<int> > endpoints;
   aTrack->getEndpoints(endpoints);

   for (list<Point<int> >::iterator it = endpoints.begin();
        it != endpoints.end(); ++it) {
      tileAt((*it).x, (*it).y).track = node;
   }
}

bool Map::isValidTrack(const Point<int>& aPoint) const
{
   if (aPoint.x < 0 || aPoint.y < 0
       || aPoint.x >= myWidth || aPoint.y >= myDepth)
       return false;

   return tileAt(aPoint.x, aPoint.y).track;
}

// Return a location where the train may start
Track::Connection Map::startLocation() const
{
   return make_pair(myStartLocation, makeVector(0, 0, 1));
}

void Map::setGrid(bool onOff)
{
   shouldDrawGridLines = onOff;
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

   if (myHeightMap)
      delete[] myHeightMap;
   myHeightMap = new Vertex[(aWidth + 1) * (aDepth + 1)];
   
   // Make a flat map
   for (int x = 0; x <= aWidth; x++) {
      for (int y = 0; y <= aDepth; y++) {
         Vertex& v = myHeightMap[x + y*(aWidth+1)];

         const float xf = static_cast<float>(x) - 0.5f;
         const float yf = static_cast<float>(y) - 0.5f;

         v.pos = makeVector(xf, 0.0f, yf);
         v.normal = makeVector(0.0f, 1.0f, 0.0f);
      }
   }
   
   // Create quad tree
   myQuadTree = makeQuadTree(shared_from_this(), myWidth);
}

void Map::resetMarks() const
{   
   // Clear the mark bit of every track segment
   // This is set whenever we render a track endpoint to ensure
   // the track is only drawn once
   for (int x = 0; x < myWidth; x++) {
      for (int y = 0; y < myDepth; y++) {
         if (tileAt(x, y).track)
            tileAt(x, y).track->resetMark();
      }
   }  
}

void Map::render(IGraphicsPtr aContext) const
{
   resetMarks();
   
   glClearColor(0.6, 0.7, 0.8, 1.0);
   
   myFog->apply();
   
   glPushAttrib(GL_ALL_ATTRIB_BITS);

   // Thick lines for grid
   glLineWidth(2.0f);

   // Use the value of glColor rather than materials
   glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
   glEnable(GL_COLOR_MATERIAL);

   glDisable(GL_TEXTURE_2D);
   glEnable(GL_CULL_FACE);
   
   glPushMatrix();
   myQuadTree->render(aContext);
   glPopMatrix();
   
   glPopAttrib();
}

// Draw a thick border around a single tile
void Map::highlightTile(IGraphicsPtr aContext, const Point<int>& aPoint) const
{
   // User should be able to click on the highlight as well
   glPushName(tileName(aPoint.x, aPoint.y));
         
   glDisable(GL_TEXTURE_2D);
   glEnable(GL_BLEND);
   glDisable(GL_LIGHTING);
   
   glPushMatrix();
   glColor4f(1.0f, 1.0f, 1.0f, 0.5f);
   glBegin(GL_POLYGON);

   int indexes[4];
   tileVertices(aPoint.x, aPoint.y, indexes);
   
   for (int i = 0; i < 4; i++) {
      Vertex& v = myHeightMap[indexes[i]];
      glNormal3f(v.normal.x, v.normal.y, v.normal.z);
      glVertex3f(v.pos.x, v.pos.y + 0.1, v.pos.z);
   }
   
   glEnd();
   glPopMatrix();

   glPopName();
}

// Check to see if the given id contains a valid mesh and ensure the
// array is large enough to hold it
bool Map::haveMesh(int id, Point<int> botLeft, Point<int> topRight)
{
   if (id >= static_cast<int>(myTerrainMeshes.size())) {
      myTerrainMeshes.resize(id + 1);
      return false;
   }
   else if (myDirtyTiles.empty())
      return myTerrainMeshes[id];
   else {
      bool ok = myTerrainMeshes[id];
      for (list<Point<int>>::iterator it = myDirtyTiles.begin();
           it != myDirtyTiles.end(); ++it) {
         if ((*it).x >= botLeft.x && (*it).x < topRight.x
             && (*it).y >= botLeft.y && (*it).y < topRight.y) {
            ok = false;
            it = myDirtyTiles.erase(it);
         }
      }
      return ok;
   }
}

// Record that the mesh containing a tile needs rebuilding
void Map::dirtyTile(int x, int y)
{
   myDirtyTiles.push_back(makePoint(x, y));

   // Push its neighbours as well since the vertices of a tile sit
   // on mesh boundaries
   myDirtyTiles.push_back(makePoint(x, y + 1));
   myDirtyTiles.push_back(makePoint(x, y - 1));
   myDirtyTiles.push_back(makePoint(x + 1, y));
   myDirtyTiles.push_back(makePoint(x - 1, y));
}

// Generate a terrain mesh for a particular sector
void Map::buildMesh(int id, Point<int> botLeft, Point<int> topRight)
{
#define RGB(r, g, b) r/255.0f, g/255.0f, b/255.0f
   
   static const tuple<float, float, float, float> colourMap[] = {
      //          Start height         colour
      make_tuple(    5.0f,      RGB(255, 255, 255) ),
      make_tuple(    3.0f,      RGB(187, 156, 83) ),
      make_tuple(    0.0f,      RGB(133, 204, 98) ),
      make_tuple(   -1.0f,     RGB(224, 223, 134) ),
      make_tuple(   -1e10f,     RGB(178, 247, 220) )
   };
   
   IMeshBufferPtr buf = makeMeshBuffer();
   
   for (int x = topRight.x-1; x >= botLeft.x; x--) {
      for (int y = botLeft.y; y < topRight.y; y++) {
         int indexes[4];
         tileVertices(x, y, indexes);
         
         const int order[6] = {
            indexes[1], indexes[2], indexes[3],
            indexes[3], indexes[0], indexes[1]
         };
         
         for (int i = 0; i < 6; i++) {
            const Vertex& v = myHeightMap[order[i]];
            
            const float h = v.pos.y;
            tuple<float, float, float, float> hcol;
            int j = 0;
            do {
               hcol = colourMap[j++];
            } while (get<0>(hcol) > h);

            buf->add(makeVector(v.pos.x, v.pos.y, v.pos.z),
                     makeVector(v.normal.x, v.normal.y, v.normal.z),
                     make_tuple(get<1>(hcol), get<2>(hcol), get<3>(hcol)));
         }
      }			
   }

   // Draw the sides of the map if this is an edge sector
   const float x1 = static_cast<float>(botLeft.x) - 0.5f;
   const float x2 = static_cast<float>(topRight.x) - 0.5f;
   const float y1 = static_cast<float>(botLeft.y) - 0.5f;
   const float y2 = static_cast<float>(topRight.y) - 0.5f;

   const IMeshBuffer::Colour brown = make_tuple(0.9f, 0.5f, 0.3f);
   const float depth = -3.0f;
   
   int index[4];
   
   if (botLeft.x == 0) {
      for (int y = botLeft.y; y < topRight.y; y++) {
         const float yf = static_cast<float>(y) - 0.5f;

         tileVertices(0, y, index);
         
         const float h1 = heightAt(index[3]).pos.y;
         const float h2 = heightAt(index[0]).pos.y;
         
         buf->addQuad(makeVector(x1, h1, yf),
                      makeVector(x1, depth, yf),
                      makeVector(x1, depth, yf + 1.0f),
                      makeVector(x1, h2, yf + 1.0f),
                      brown);
      }
   }

   if (topRight.x == myWidth) {
      for (int y = botLeft.y; y < topRight.y; y++) {
         const float yf = static_cast<float>(y) - 0.5f;

         tileVertices(myWidth - 1, y, index);

         const float h1 = heightAt(index[2]).pos.y;
         const float h2 = heightAt(index[1]).pos.y;
         
         buf->addQuad(makeVector(x2, depth, yf),
                      makeVector(x2, h1, yf),
                      makeVector(x2, h2, yf + 1.0f),
                      makeVector(x2, depth, yf + 1.0f),
                      brown);
      }
   }

   if (botLeft.y == 0) {
      for (int x = botLeft.x; x < topRight.x; x++) {
         const float xf = static_cast<float>(x) - 0.5f;
         
         tileVertices(x, 0, index);
       
         const float h1 = heightAt(index[3]).pos.y;
         const float h2 = heightAt(index[2]).pos.y;
         
         buf->addQuad(makeVector(xf, depth, y1),
                      makeVector(xf, h1, y1),
                      makeVector(xf + 1.0f, h2, y1),
                      makeVector(xf + 1.0f, depth, y1),
                      brown);
      }
   }
   
   if (topRight.y == myDepth) {
      for (int x = botLeft.x; x < topRight.x; x++) {
         const float xf = static_cast<float>(x) - 0.5f;
         
         tileVertices(x, myDepth - 1, index);
       
         const float h1 = heightAt(index[0]).pos.y;
         const float h2 = heightAt(index[1]).pos.y;
      
         buf->addQuad(makeVector(xf, h1, y2),
                      makeVector(xf, depth, y2),
                      makeVector(xf + 1.0f, depth, y2),
                      makeVector(xf + 1.0f, h2, y2),
                      brown);
      }
   }
   
   myTerrainMeshes[id] = makeMesh(buf);
}

// A special rendering mode when selecting tiles
void Map::renderPickSector(Point<int> botLeft, Point<int> topRight)
{
   glColor3f(1.0f, 1.0f, 1.0f);
   
   for (int x = topRight.x-1; x >= botLeft.x; x--) {
      for (int y = botLeft.y; y < topRight.y; y++) {
         // Name this tile
         glPushName(tileName(x, y));

         int indexes[4];
         tileVertices(x, y, indexes);

         glBegin(GL_QUADS);
         for (int i = 0; i < 4; i++) {
            const Vertex& v = myHeightMap[indexes[i]];
            glNormal3f(v.normal.x, v.normal.y, v.normal.z);
            glVertex3f(v.pos.x, v.pos.y, v.pos.z);
         }
         glEnd();
         
         glPopName();
      }			
   }
}

// Render a small part of the map as directed by the quad tree
void Map::renderSector(IGraphicsPtr aContext, int id,
                       Point<int> botLeft, Point<int> topRight)
{
   if (inPickMode) {
      renderPickSector(botLeft, topRight);
      return;
   }
   
   if (!haveMesh(id, botLeft, topRight))
      buildMesh(id, botLeft, topRight);

   myTerrainMeshes[id]->render();

   // Draw the overlays
   for (int x = topRight.x-1; x >= botLeft.x; x--) {
      for (int y = botLeft.y; y < topRight.y; y++) {
         //for (int i = 0; i < 4; i++) {
         //   const Vertex& v = myHeightMap[indexes[i]];
         //   drawNormal(v.pos, v.normal);
         //}
         
         if (shouldDrawGridLines) {
            // Render grid lines
            glColor3f(0.0f, 0.0f, 0.0f);
            glBegin(GL_LINE_LOOP);

            int indexes[4];
            tileVertices(x, y, indexes);
            for (int i = 0; i < 4; i++) {
               const Vertex& v = myHeightMap[indexes[i]];
               glVertex3f(v.pos.x, v.pos.y, v.pos.z);
            }
            
            glEnd();
         }

         // Draw the track, if any
         Tile& tile = tileAt(x, y);
         if (tile.track && !tile.track->marked()) {
            glPushMatrix();
            glTranslated(static_cast<double>(tile.track->originX()), 0,
                         static_cast<double>(tile.track->originY()));
            tile.track->get()->render();
            glPopMatrix();
            
            tile.track->setMark();
         }
      }			
   }
}

// Render the semi-transparent overlays such as water
void Map::postRenderSector(IGraphicsPtr aContext, int id,
                           Point<int> botLeft, Point<int> topRight)
{
   // Draw the water
   if (!inPickMode) {      
      static const float seaLevel = -0.6f;
      glColor4f(0.0f, 0.2f, 0.8f, 0.4f);
      glNormal3f(0.0f, 1.0f, 0.0f);
      glBegin(GL_QUADS);
      glVertex3f(botLeft.x - 0.5f, seaLevel, botLeft.y - 0.5f);
      glVertex3f(botLeft.x - 0.5f, seaLevel, topRight.y - 0.5f);
      glVertex3f(topRight.x - 0.5f, seaLevel, topRight.y - 0.5f);
      glVertex3f(topRight.x - 0.5f, seaLevel, botLeft.y - 0.5f);
      glEnd();
   }
}

// Called when we've changed the height of part of a tile
// This readjusts all the normals and those of its neighbours
// to point in the right direction
void Map::fixNormals(int x, int y)
{
   if ((x < 0) || (y < 0) || (x >= myWidth) || (y >= myDepth))
      return;

   int indexes[4];
   tileVertices(x, y, indexes);

   for (int n = 0; n < 4; n++) {
      const int i = indexes[n];
      Vertex& v = myHeightMap[i];

      Vector<float> west, north, east, south;
      bool haveWest = true, haveNorth = true,
         haveEast = true, haveSouth = true;
      
      if (i > 0 && i % (myWidth + 1) > 0)
         west = heightAt(i-1).pos;
      else
         haveWest = false;
         
      if (i < (myWidth + 1) * myDepth - 1)
         north = heightAt(i + (myWidth + 1)).pos;
      else
         haveNorth = false;

      if (i < (myWidth + 1) * (myDepth + 1) - 1
          && i % (myWidth + 1) < myWidth)
         east = heightAt(i + 1).pos;
      else
         haveEast = false;
      
      if (i > (myWidth + 1))
         south = heightAt(i - (myWidth + 1)).pos;
      else
         haveSouth = false;

      float count = 4.0f;
      Vector<float> avg = makeVector(0.0f, 0.0f, 0.0f);

      if (haveWest && haveNorth)
         avg += surfaceNormal(north, v.pos, west);
      else
         count -= 1.0f;

      if (haveEast && haveNorth)
         avg += surfaceNormal(east, v.pos, north);
      else
         count -= 1.0f;

      if (haveSouth && haveEast)
         avg += surfaceNormal(south, v.pos, east);
      else
         count -= 1.0f;

      if (haveWest && haveSouth)      
         avg += surfaceNormal(west, v.pos, south);
      else
         count -= 1.0f;

      v.normal = avg / count;
   }
}

// Find the terrain vertices that border a tile
void Map::tileVertices(int x, int y, int* indexes) const
{
   assert(x >= 0 && x < myWidth && y >= 0 && y < myDepth);
          
   indexes[3] = x + (y * (myWidth+1));
   indexes[2] = (x+1) + (y * (myWidth+1));
   indexes[1] = (x+1) + ((y+1) * (myWidth+1));
   indexes[0] = x + ((y+1) * (myWidth+1));          
}

// Changes the height of a complete tile
void Map::raiseTile(int x, int y, float deltaHeight)
{
   int indexes[4];
   tileVertices(x, y, indexes);

   for (int i = 0; i < 4; i++)
      myHeightMap[indexes[i]].pos.y += deltaHeight;

   fixNormals(x, y);
   dirtyTile(x, y);
}

// Levels off a tile
void Map::levelTile(int x, int y)
{
   // Adjust to the average height
   /* float heights[4];
   float sum = 0.0f, average = 0.0f;
   for (int i = 0; i < 4; i++)
      sum += tileAt(x, y).v[i].pos.y;
   
   average = sum / 4.0f;
   
   for (int j = 0; j < 4; j++) {
      setVertexHeight(x, y, j, average);
      heights[j] = tileAt(x, y).v[j].pos.y;
   }

   setVertexHeight(x, y - 1, 1, heights[0]);
   setVertexHeight(x, y - 1, 2, heights[1]);
   
   setVertexHeight(x, y + 1, 3, heights[2]);
   setVertexHeight(x, y + 1, 0, heights[3]);

   setVertexHeight(x - 1, y, 2, heights[1]);
   setVertexHeight(x - 1, y, 3, heights[2]);
   
   setVertexHeight(x + 1, y, 0, heights[3]);
   setVertexHeight(x + 1, y, 1, heights[0]);
   
   setVertexHeight(x + 1, y + 1, 0, heights[3]);
   setVertexHeight(x + 1, y - 1, 1, heights[0]);
   setVertexHeight(x - 1, y + 1, 3, heights[2]);
   setVertexHeight(x - 1, y - 1, 2, heights[1]);

   fixNormals(x, y);*/
}

void Map::changeAreaHeight(const Point<int>& aStartPos,
                           const Point<int>& aFinishPos,
                           float aHeightDelta)
{
   const int xmin = min(aStartPos.x, aFinishPos.x);
   const int xmax = max(aStartPos.x, aFinishPos.x);

   const int ymin = min(aStartPos.y, aFinishPos.y);
   const int ymax = max(aStartPos.y, aFinishPos.y);
   
   for (int x = xmin; x <= xmax; x++) {
      for (int y = ymin; y <= ymax; y++)
         raiseTile(x, y, aHeightDelta);
   }
}

void Map::raiseArea(const Point<int>& aStartPos,
                    const Point<int>& aFinishPos)
{
   changeAreaHeight(aStartPos, aFinishPos, 0.1f);
}

void Map::lowerArea(const Point<int>& aStartPos,
                    const Point<int>& aFinishPos)
{
   changeAreaHeight(aStartPos, aFinishPos, -0.1f);
}

// Write the terrain height map into a binary file
// Binary file format is very simple:
//   Bytes 0-3   Width of map
//   Bytes 4-7   Depth of map
//   Bytes 8+    Raw height data
void Map::writeHeightMap(const string& aFileName) const
{
   log() << "Writing terrain height map to " << aFileName;

   ofstream of(aFileName.c_str(), ios::binary);
   if (!of.good())
      throw runtime_error("Failed to open " + aFileName + " for writing");

   const long wl = static_cast<long>(myWidth);
   const long dl = static_cast<long>(myDepth);
   of.write(reinterpret_cast<const char*>(&wl), sizeof(long));
   of.write(reinterpret_cast<const char*>(&dl), sizeof(long));

   for (int i = 0; i < (myWidth + 1) * (myDepth + 1); i++)
      of.write(reinterpret_cast<const char*>(&myHeightMap[i].pos.y),
               sizeof(float));
}

// Read the height data back out of a binary file
void Map::readHeightMap(const string& aFileName)
{
   log() << "Reading height map from " << aFileName;
   
   ifstream is(aFileName.c_str(), ios::binary);
   if (!is.good())
      throw runtime_error("Failed to open " + aFileName + " for reading");

   // Check the dimensions of the binary file match the XML file
   long wl, dl;
   is.read(reinterpret_cast<char*>(&wl), sizeof(long));
   is.read(reinterpret_cast<char*>(&dl), sizeof(long));

   if (wl != myWidth || dl != myDepth) {
      error() << "Expected width " << myWidth << " got " << wl;
      error() << "Expected height " << myDepth << " got " << dl;
      throw runtime_error
         ("Binary file " + aFileName + " dimensions are incorrect");
   }

   for (int i = 0; i < (myWidth + 1) * (myDepth + 1); i++)
      is.read(reinterpret_cast<char*>(&myHeightMap[i].pos.y),
              sizeof(float));

   for (int x = 0; x < myWidth; x++) {
      for (int y = 0; y < myDepth; y++)
         fixNormals(x, y);
   }
}

// Turn the map into XML
void Map::save(const string& aFileName)
{
   using namespace boost::filesystem;
   
   log() << "Saving map to " << aFileName;

   ofstream of(aFileName.c_str());
   if (!of.good())
      throw runtime_error("Failed to open " + aFileName + " for writing");

   xml::element root("map");
   root.addAttribute("width", myWidth);
   root.addAttribute("height", myDepth);

   root.addChild(xml::element("name").addText("No Name"));
   
   root.addChild
      (xml::element("start")
       .addAttribute("x", myStartLocation.x)
       .addAttribute("y", myStartLocation.y));

   // Generate the height map
   // Note: basename is deprecated (use .replace_extension() instead when
   // boost is updated in Debian)
   const string binFile(change_extension(path(aFileName), ".bin").file_string());
   writeHeightMap(binFile);

   root.addChild
      (xml::element("heightmap")
       .addText(binFile));

   xml::element tileset("tileset");
   
   for (int x = 0; x < myWidth; x++) {
      for (int y = 0; y < myDepth; y++) {
         const Tile& tile = tileAt(x, y);

         if (tile.track
             && tile.track->originX() == x
             && tile.track->originY() == y) {
            
            tileset.addChild
               (xml::element("tile")
                .addAttribute("x", x)
                .addAttribute("y", y)
                .addChild(tile.track->get()->toXml()));
         }
      }
   }

   root.addChild(tileset);
   
   of << xml::document(root);
}

IMapPtr makeEmptyMap(int aWidth, int aDepth)
{
   shared_ptr<Map> ptr(new Map);
   ptr->resetMap(aWidth, aDepth);
   return IMapPtr(ptr);
}

// Build a map through XML callbacks
class MapLoader : public IXMLCallback {
public:
   MapLoader(shared_ptr<Map> aMap)
      : myMap(aMap), myXPtr(0), myYPtr(0) {}

   void startElement(const std::string& localName,
                     const AttributeSet& attrs)
   {
      if (localName == "map")
         handleMap(attrs);
      else if (localName == "tile")
         handleTile(attrs);
      else if (localName == "start")
         handleStart(attrs);
      else if (localName == "straightTrack")
         handleStraightTrack(attrs);
      else if (localName == "curvedTrack")
         handleCurvedTrack(attrs);
      else if (localName == "crossoverTrack")
         handleCrossoverTrack(attrs);
   }

   void text(const string& localName, const string& aString)
   {
      if (localName == "heightmap")
         myMap->readHeightMap(aString);
   }
   
private:
   void handleMap(const AttributeSet& attrs)
   {
      int width, height;
      attrs.get("width", width);
      attrs.get("height", height);

      debug() << "width=" << width << ", height=" << height;

      myMap->resetMap(width, height);
   }

   void handleStart(const AttributeSet& attrs)
   {
      int x, y;
      attrs.get("x", x);
      attrs.get("y", y);

      myMap->setStart(x, y);
   }

   void handleTile(const AttributeSet& attrs)
   {
      attrs.get("x", myXPtr);
      attrs.get("y", myYPtr);
   }

   void handleStraightTrack(const AttributeSet& attrs)
   {
      string align;
      attrs.get("align", align);

      Track::Direction axis = align == "x" ? Axis::X : Axis::Y;

      myMap->setTrackAt(makePoint(myXPtr, myYPtr),
                        makeStraightTrack(axis));
   }

   void handleCurvedTrack(const AttributeSet& attrs)
   {
      int startAngle, finishAngle, radius;
      attrs.get("startAngle", startAngle);
      attrs.get("finishAngle", finishAngle);
      attrs.get("radius", radius);

      myMap->setTrackAt(makePoint(myXPtr, myYPtr),
                        makeCurvedTrack(startAngle, finishAngle, radius));
   }

   void handleCrossoverTrack(const AttributeSet& attrs)
   {
      myMap->setTrackAt(makePoint(myXPtr, myYPtr),
                        makeCrossoverTrack());                        
   }
   
   shared_ptr<Map> myMap;
   int myXPtr, myYPtr;
}; 

IMapPtr loadMap(const string& aFileName)
{
   shared_ptr<Map> map(new Map);

   log() << "Loading map from file " << aFileName;

   static IXMLParserPtr xmlParser = makeXMLParser("schemas/map.xsd");

   MapLoader loader(map);
   xmlParser->parse(aFileName, loader);
      
   return IMapPtr(map);
}
