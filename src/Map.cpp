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
#include "IStation.hpp"
#include "IResource.hpp"
#include "IScenery.hpp"

#include <stdexcept>
#include <sstream>
#include <cassert>
#include <fstream>
#include <set>
#include <map>

#include <GL/gl.h>
#include <boost/filesystem.hpp>
#include <boost/cstdint.hpp>
#include <boost/lexical_cast.hpp>

// A single piece of track may appear multiple times in the map - this
// will be true for all track segments that cover multiple tiles.
// For various map algorithms (e.g. drawing) it is useful to keep track
// of which track segments have already been visited. So track segments
// are wrapped inside TrackNode which is used to store map-specific
// information about the track.
class TrackNode {
public:
   TrackNode(ITrackSegmentPtr aTrack, int x, int y)
      : track(aTrack), amMarked(false),
        origin(makePoint(x, y)) {}

   inline void setMark() { amMarked = true; }
   inline void resetMark() { amMarked = false; }
   inline bool marked() const { return amMarked; }

   inline ITrackSegmentPtr get() { return track; }

   inline int originX() const { return origin.x; }
   inline int originY() const { return origin.y; }
private:
   ITrackSegmentPtr track;
   bool amMarked;
   Point<int> origin;
};

typedef shared_ptr<TrackNode> TrackNodePtr;

class Map : public IMap, public ISectorRenderable,
            public enable_shared_from_this<Map> {
   friend class MapLoader;
public:
   Map(IResourcePtr aRes);
   ~Map();

   // IMap interface
   int width() const { return myWidth; }
   int depth() const { return myDepth; }
   double heightAt() const { return 0.0; }

   string name() const { return resource->name(); }

   void setStart(int x, int y);
   void setStart(int x, int y, int dirX, int dirY);
   void setGrid(bool onOff);
   void setPickMode(bool onOff) { inPickMode = onOff; }
   
   track::Connection start() const;
   ITrackSegmentPtr trackAt(const Point<int>& aPoint) const;
   IStationPtr stationAt(Point<int> aPoint) const;
   void setTrackAt(const Point<int>& aPoint, ITrackSegmentPtr aTrack);
   bool isValidTrack(const Point<int>& aPoint) const;
   void render(IGraphicsPtr aContext) const;
   void highlightTile(IGraphicsPtr aContext, const Point<int>& aPoint,
      HighlightColour aColour) const;
   void resetMap(int aWidth, int aDepth);
   void eraseTile(int x, int y);

   void raiseArea(const Point<int>& aStartPos,
      const Point<int>& aFinishPos);
   void lowerArea(const Point<int>& aStartPos,
      const Point<int>& aFinishPos);
   void levelArea(Point<int> aStartPos, Point<int> aFinishPos);
   
   void save();

   IStationPtr extendStation(Point<int> aStartPos,
      Point<int> aFinishPos);
   void placeBuilding(Point<int> aPoint, IBuildingPtr aBuilding,
      float anAngle);
   float heightAt(float x, float y) const;
   void addScenery(Point<int> where, ISceneryPtr s);
   
   // ISectorRenderable interface
   void renderSector(IGraphicsPtr aContext, int id,
      Point<int> botLeft, Point<int> topRight);
   void postRenderSector(IGraphicsPtr aContext, int id,
      Point<int> botLeft, Point<int> topRight);
   
private:
   // Tiles on the map
   struct Tile {
      TrackNodePtr track;    // Track at this location, if any
      IStationPtr station;   // Station on this tile, if any
      IBuildingPtr building; // Building on this tile, if any
      float buildingAngle;   // Orientation of building
      ISceneryPtr scenery;   // Static scenery, if any
   } *tiles;

   // Vertices on the terrain
   struct Vertex {
      Vector<float> pos, normal;
   } *heightMap;

   static const unsigned TILE_NAME_BASE	= 1000;	  // Base of tile naming
   static const unsigned NULL_OBJECT		= 0;		  // Non-existant object
   static const float TILE_HEIGHT;	   // Standard height increment

   // Meshes for each terrain sector
   vector<IMeshPtr> terrainMeshes;
   
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
      return tiles[index(x, z)];
   }

   inline Vertex& heightAt(int i) const
   {
      assert(i >= 0 && i < (myWidth + 1) * (myDepth + 1));
      return heightMap[i];
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
   void writeHeightMap() const;
   void readHeightMap(IResource::Handle aHandle);
   void tileVertices(int x, int y, int* indexes) const;
   void renderPickSector(Point<int> botLeft, Point<int> topRight);
   void drawStartLocation() const;
   void setStationAt(int x, int y, IStationPtr aStation);

   // Mesh modification
   void buildMesh(int id, Point<int> botLeft, Point<int> topRight);
   bool haveMesh(int id, Point<int> botLeft, Point<int> topRight);
   void dirtyTile(int x, int y);
      
   // Terrain modification
   void changeAreaHeight(const Point<int>& aStartPos,
      const Point<int>& aFinishPos, float aHeightDelta);
   void raiseTile(int x, int y, float deltaHeight);
   void setTileHeight(int x, int y, float h);
   void fixNormals(int x, int y);
   bool raiseWillCoverTrack(int x, int y) const;
   
   int myWidth, myDepth;
   Point<int> startLocation;
   track::Direction startDirection;
   IQuadTreePtr quadTree;
   IFogPtr fog;
   bool shouldDrawGridLines, inPickMode;
   list<Point<int> > dirtyTiles;
   IResourcePtr resource;
};

const float Map::TILE_HEIGHT(0.2f);

Map::Map(IResourcePtr aRes)
   : tiles(NULL), heightMap(NULL), myWidth(0), myDepth(0),
     startLocation(makePoint(1, 1)),
     startDirection(axis::X),
     shouldDrawGridLines(false), inPickMode(false),
     resource(aRes)
{
   fog = makeFog(0.005f,            // Density
      50.0f, 70.0f);     // Start and end distance
}

Map::~Map()
{
   delete tiles;
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

IStationPtr Map::stationAt(Point<int> aPoint) const
{
   return tileAt(aPoint.x, aPoint.y).station;
}

void Map::setStationAt(int x, int y, IStationPtr aStation)
{
   tileAt(x, y).station = aStation;
}

void Map::eraseTile(int x, int y)
{
   Tile& tile = tileAt(x, y);

   if (tile.track) {
      // We have to be a bit careful since a piece of track has multiple
      // endpoints

      list<Point<int> > endpoints;
      tile.track->get()->getEndpoints(endpoints);

      for (list<Point<int> >::iterator it = endpoints.begin();
           it != endpoints.end(); ++it)
         tileAt((*it).x, (*it).y).track.reset();
   }

   if (tile.building)
      tile.building.reset();
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
track::Connection Map::start() const
{
   return make_pair(startLocation, startDirection);
}

// Try to place the train on this tile
void Map::setStart(int x, int y)
{
   const track::Direction possDirs[] = {
      axis::X, axis::Y, -axis::X, -axis::Y
   };
   static int nextDir = 0;
   
   TrackNodePtr trackNode = tileAt(x, y).track;
   if (!trackNode) {
      warn() << "Must place start on track";
      return;
   }

   ITrackSegmentPtr track = trackNode->get();

   int tried = 0;
   do {
      if (track->isValidDirection(possDirs[nextDir]))
         break;
      else
         nextDir = (nextDir + 1) % 4;
   } while (++tried < 4);

   if (tried == 4) {
      warn() << "Cannot find suitable initial direction for this track";
      return;
   }

   startLocation = makePoint(x, y);
   startDirection = possDirs[nextDir];

   nextDir = (nextDir + 1) % 4;
}

// Force the train to start on this tile
void Map::setStart(int x, int y, int dirX, int dirY)
{
   startLocation = makePoint(x, y);
   startDirection = makeVector(dirX, 0, dirY);
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
   if (tiles)
      delete[] tiles;
   tiles = new Tile[aWidth * aDepth];

   if (heightMap)
      delete[] heightMap;
   heightMap = new Vertex[(aWidth + 1) * (aDepth + 1)];
   
   // Make a flat map
   for (int x = 0; x <= aWidth; x++) {
      for (int y = 0; y <= aDepth; y++) {
         Vertex& v = heightMap[x + y*(aWidth+1)];

         const float xf = static_cast<float>(x) - 0.5f;
         const float yf = static_cast<float>(y) - 0.5f;

         v.pos = makeVector(xf, 0.0f, yf);
         v.normal = makeVector(0.0f, 1.0f, 0.0f);
      }
   }
   
   // Create quad tree
   quadTree = makeQuadTree(shared_from_this(), myWidth);
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
   
   fog->apply();
   
   glPushAttrib(GL_ALL_ATTRIB_BITS);

   // Thick lines for grid
   glLineWidth(2.0f);

   // Use the value of glColor rather than materials
   glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
   glEnable(GL_COLOR_MATERIAL);

   glDisable(GL_TEXTURE_2D);
   glEnable(GL_CULL_FACE);
   
   glPushMatrix();
   quadTree->render(aContext);
   glPopMatrix();
   
   glPopAttrib();
}

// Draw a thick border around a single tile
void Map::highlightTile(IGraphicsPtr aContext, const Point<int>& aPoint,
   HighlightColour aColour) const
{
   // User should be able to click on the highlight as well
   glPushName(tileName(aPoint.x, aPoint.y));

   glPushAttrib(GL_ENABLE_BIT);
   
   glDisable(GL_TEXTURE_2D);
   glEnable(GL_BLEND);
   glDisable(GL_LIGHTING);
   
   glPushMatrix();
   glColor4f(get<0>(aColour), get<1>(aColour), get<2>(aColour), 0.5f);
   glBegin(GL_POLYGON);

   int indexes[4];
   tileVertices(aPoint.x, aPoint.y, indexes);
   
   for (int i = 0; i < 4; i++) {
      Vertex& v = heightMap[indexes[i]];
      glNormal3f(v.normal.x, v.normal.y, v.normal.z);
      glVertex3f(v.pos.x, v.pos.y + 0.1f, v.pos.z);
   }
   
   glEnd();
   glPopMatrix();

   glPopAttrib();

   glPopName();
}

// Draw an arrow on the start location
void Map::drawStartLocation() const
{
   glPushAttrib(GL_ENABLE_BIT);
   glPushMatrix();

   glEnable(GL_BLEND);
   glDisable(GL_TEXTURE_2D);

   int indexes[4];
   tileVertices(startLocation.x, startLocation.y, indexes);

   float avgHeight = 0.0f;
   for (int i = 0; i < 4; i++)
      avgHeight += heightMap[indexes[i]].pos.y;
   avgHeight /= 4.0f;

   glTranslatef(static_cast<float>(startLocation.x), 
      avgHeight + 0.1f, 
      static_cast<float>(startLocation.y));

   if (startDirection == axis::X)
      glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
   else if (startDirection == -axis::Y)
      glRotatef(180.0f, 0.0f, 1.0f, 0.0f);
   else if (startDirection == -axis::X)
      glRotatef(270.0f, 0.0f, 1.0f, 0.0f);

   glColor4f(0.0f, 0.9f, 0.0f, 0.8f);
   
   glBegin(GL_TRIANGLES);
   glNormal3f(0.0f, 1.0f, 0.0f);
   glVertex3f(0.5f, 0.0f, -0.5f);
   glVertex3f(-0.5f, 0.0f, -0.5f);
   glVertex3f(0.0f, 0.0f, 0.5f);
   glEnd();
   
   glPopMatrix();
   glPopAttrib();
}

// Check to see if the given id contains a valid mesh and ensure the
// array is large enough to hold it
bool Map::haveMesh(int id, Point<int> botLeft, Point<int> topRight)
{
   if (id >= static_cast<int>(terrainMeshes.size())) {
      terrainMeshes.resize(id + 1);
      return false;
   }
   else if (dirtyTiles.empty())
      return terrainMeshes[id];
   else {
      bool ok = terrainMeshes[id];
      for (list<Point<int> >::iterator it = dirtyTiles.begin();
           it != dirtyTiles.end(); ++it) {
         if ((*it).x >= botLeft.x && (*it).x <= topRight.x
            && (*it).y >= botLeft.y && (*it).y <= topRight.y) {
            ok = false;
            it = dirtyTiles.erase(it);
         }
      }
      return ok;
   }
}

// Record that the mesh containing a tile needs rebuilding
void Map::dirtyTile(int x, int y)
{
   dirtyTiles.push_back(makePoint(x, y));

   // Push its neighbours as well since the vertices of a tile sit
   // on mesh boundaries
   dirtyTiles.push_back(makePoint(x, y + 1));
   dirtyTiles.push_back(makePoint(x, y - 1));
   dirtyTiles.push_back(makePoint(x + 1, y));
   dirtyTiles.push_back(makePoint(x - 1, y));
}

// Generate a terrain mesh for a particular sector
void Map::buildMesh(int id, Point<int> botLeft, Point<int> topRight)
{
   static const tuple<float, Colour> colourMap[] = {
      //          Start height         colour
      make_tuple(    7.0f,     makeRGB(238, 233, 233) ),
      make_tuple(    5.0f,     makeRGB(124, 113, 36) ),
      make_tuple(    3.0f,     makeRGB(129, 142, 57) ),
      make_tuple(    0.0f,     makeRGB(103, 142, 57) ),
      make_tuple(   -1.0f,     makeRGB(224, 223, 134) ),
      make_tuple(   -1e10f,    makeRGB(178, 247, 220) )
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
            const Vertex& v = heightMap[order[i]];
            
            const float h = v.pos.y;
            tuple<float, Colour> hcol;
            int j = 0;
            do {
               hcol = colourMap[j++];
            } while (get<0>(hcol) > h);

            buf->add(makeVector(v.pos.x, v.pos.y, v.pos.z),
               makeVector(v.normal.x, v.normal.y, v.normal.z),
               get<1>(hcol));
         }
      }			
   }

   // Draw the sides of the map if this is an edge sector
   const float x1 = static_cast<float>(botLeft.x) - 0.5f;
   const float x2 = static_cast<float>(topRight.x) - 0.5f;
   const float y1 = static_cast<float>(botLeft.y) - 0.5f;
   const float y2 = static_cast<float>(topRight.y) - 0.5f;

   const Colour brown = makeRGB(104, 57, 12);
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

   terrainMeshes[id] = makeMesh(buf);
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
            const Vertex& v = heightMap[indexes[i]];
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

   terrainMeshes[id]->render();

   // Draw the overlays
   for (int x = topRight.x-1; x >= botLeft.x; x--) {
      for (int y = botLeft.y; y < topRight.y; y++) {
         //for (int i = 0; i < 4; i++) {
         //   const Vertex& v = heightMap[indexes[i]];
         //   drawNormal(v.pos, v.normal);
         //}
         
         if (shouldDrawGridLines) {
            // Render grid lines
            glColor3f(0.0f, 0.0f, 0.0f);
            glBegin(GL_LINE_LOOP);

            int indexes[4];
            tileVertices(x, y, indexes);
            for (int i = 0; i < 4; i++) {
               const Vertex& v = heightMap[indexes[i]];
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
            
            // Draw the endpoints for debugging
            //list<Point<int> > endpoints;
            //tile.track->get()->getEndpoints(endpoints);
            //for_each(endpoints.begin(), endpoints.end(),
            //        bind(&Map::highlightTile, this, aContext, placeholders::_1,
            //              make_tuple(0.9f, 0.1f, 0.1f)));
         }

         // Draw the station, if any
         if (tile.station
            && (shouldDrawGridLines || tile.station->highlightVisible()))
            highlightTile(aContext, makePoint(x, y),
               tile.station->highlightColour());

         // Draw the start location if it's on this tile
         if (startLocation.x == x && startLocation.y == y
            && shouldDrawGridLines)
            drawStartLocation();

         // Draw any buildings
         if (tile.building) {
            glPushMatrix();
            glTranslatef(static_cast<float>(x), 0.0f, static_cast<float>(y));
            glRotatef(tile.buildingAngle, 0.0f, 1.0f, 0.0f);
            tile.building->model()->render();
            glPopMatrix();
         }

         // Draw any scenery
         if (tile.scenery)
            tile.scenery->render();
      }			
   }
}

// Render the semi-transparent overlays such as water
void Map::postRenderSector(IGraphicsPtr aContext, int id,
   Point<int> botLeft, Point<int> topRight)
{
   // Draw the water
   if (!inPickMode) {
      glPushAttrib(GL_ENABLE_BIT);

      glEnable(GL_BLEND);
      glDisable(GL_TEXTURE_2D);
      
      static const float seaLevel = -0.6f;
      glColor4f(0.0f, 0.2f, 0.8f, 0.4f);
      glNormal3f(0.0f, 1.0f, 0.0f);
      glBegin(GL_QUADS);
      glVertex3f(botLeft.x - 0.5f, seaLevel, botLeft.y - 0.5f);
      glVertex3f(botLeft.x - 0.5f, seaLevel, topRight.y - 0.5f);
      glVertex3f(topRight.x - 0.5f, seaLevel, topRight.y - 0.5f);
      glVertex3f(topRight.x - 0.5f, seaLevel, botLeft.y - 0.5f);
      glEnd();

      glPopAttrib();
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
      Vertex& v = heightMap[i];

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

// True if changing the height of this tile will affect
// a piece of track
bool Map::raiseWillCoverTrack(int x, int y) const
{
   return tileAt(x, y).track
      || (x < myWidth - 1 && tileAt(x + 1, y).track)
      || (x > 0 && tileAt(x - 1, y).track)
      || (y < myDepth - 1 && tileAt(x, y + 1).track)
      || (y > 0 && tileAt(x, y - 1).track)
      || (x < myWidth - 1 && y < myDepth - 1 && tileAt(x + 1, y + 1).track)
      || (x > 0 && y < myDepth - 1 && tileAt(x - 1, y + 1).track)
      || (x > 0 && y > 0 && tileAt(x - 1, y - 1).track)
      || (x < myWidth - 1 && y > 0 && tileAt(x + 1, y - 1).track);
}

// Changes the height of a complete tile
void Map::raiseTile(int x, int y, float deltaHeight)
{
   if (raiseWillCoverTrack(x, y)) {
      warn() << "Cannot raise terrain over track";
      return;
   }
   
   int indexes[4];
   tileVertices(x, y, indexes);

   for (int i = 0; i < 4; i++)
      heightMap[indexes[i]].pos.y += deltaHeight;

   fixNormals(x, y);
   dirtyTile(x, y);
}

// Sets the absolute height of a tile
void Map::setTileHeight(int x, int y, float h)
{
   bool trackAffected = raiseWillCoverTrack(x, y);
   
   int indexes[4];
   tileVertices(x, y, indexes);

   for (int i = 0; i < 4; i++) {
      if (trackAffected
         && abs(heightMap[indexes[i]].pos.y - h) > 0.01f) {
         warn() << "Cannot level terrain under track";
         return;
      }        
      else
         heightMap[indexes[i]].pos.y = h;
   }
   
   fixNormals(x, y);
   dirtyTile(x, y);
}

float Map::heightAt(float x, float y) const
{
   const int xFloor = static_cast<int>(floorf(x));
   const int yFloor = static_cast<int>(floorf(y));

   if (xFloor < 0 || yFloor < 0
      || xFloor >= myWidth || yFloor >= myDepth)
      return 0.0f;

   int indexes[4];
   tileVertices(x, y, indexes);

   float avg = 0.0f;
   for (int i = 0; i < 4; i++)
      avg += heightMap[indexes[i]].pos.y;
   
   return avg / 4.0f;
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

void Map::levelArea(Point<int> aStartPos, Point<int> aFinishPos)
{
   const int xmin = min(aStartPos.x, aFinishPos.x);
   const int xmax = max(aStartPos.x, aFinishPos.x);

   const int ymin = min(aStartPos.y, aFinishPos.y);
   const int ymax = max(aStartPos.y, aFinishPos.y);

   int indexes[4];
   tileVertices(aStartPos.x, aStartPos.y, indexes);

   float avgHeight = 0.0f;
   for (int i = 0; i < 4; i++)
      avgHeight += heightMap[indexes[i]].pos.y;
   avgHeight /= 4.0f;
   
   for (int x = xmin; x <= xmax; x++) {
      for (int y = ymin; y <= ymax; y++)
         setTileHeight(x, y, avgHeight);
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

void Map::placeBuilding(Point<int> aPoint, IBuildingPtr aBuilding, float anAngle)
{
   if (tileAt(aPoint.x, aPoint.y).track)
      warn() << "Cannot place building on track";
   else {
      Tile& t = tileAt(aPoint.x, aPoint.y);
      t.building = aBuilding;
      t.buildingAngle = anAngle;
   }
}

void Map::addScenery(Point<int> where, ISceneryPtr s)
{
   if (tileAt(where.x, where.y).track)
      warn() << "Cannot place scenery on track";
   else {
      tileAt(where.x, where.y).scenery = s;
      s->setPosition(static_cast<float>(where.x),
         heightAt(where.x, where.y),
         static_cast<float>(where.y));
   }
}

// Either extend an existing station which borders this area
// or build a new station
IStationPtr Map::extendStation(Point<int> aStartPos, Point<int> aFinishPos)
{
   const int xmin = min(aStartPos.x, aFinishPos.x);
   const int xmax = max(aStartPos.x, aFinishPos.x);

   const int ymin = min(aStartPos.y, aFinishPos.y);
   const int ymax = max(aStartPos.y, aFinishPos.y);

   // Find all the tiles containing track in this region
   typedef list<Point<int> > PointList;
   PointList trackInArea;
   for (int x = xmin; x <= xmax; x++) {
      for (int y = ymin; y <= ymax; y++) {
         if (tileAt(x, y).track)
            trackInArea.push_back(makePoint(x, y));
      }
   }

   if (trackInArea.empty()) {
      warn() << "Stations must be placed on track";
      return IStationPtr();
   }

   IStationPtr station;

   // See if any of these track segments are adjacent to a station
   for (PointList::const_iterator it = trackInArea.begin();
        it != trackInArea.end(); ++it) {
      
      const Point<int> near[] = {
         makePoint(0, 0),
         makePoint(1, 0),
         makePoint(0, 1),
         makePoint(-1, 0),
         makePoint(0, -1)
      };
      
      for (int i = 0; i < 5; i++) {
         Point<int> neighbour = *it + near[i];
         if (neighbour.x >= 0 && neighbour.x < myWidth
            && neighbour.y >= 0 && neighbour.y < myDepth
            && tileAt(neighbour.x, neighbour.y).station) {

            IStationPtr candidate = tileAt(neighbour.x, neighbour.y).station;
            
            // Maybe extend this station
            if (station && station != candidate) {
               warn() << "Cannot merge stations";
               return IStationPtr();
            }
            else
               station = candidate;
         }
      }
   }

   if (station) {
      // Found a station to extend
      debug() << "Found station to extend";
   }
   else {
      debug() << "Creating new station";

      station = makeStation();
   }
   
   for (PointList::iterator it = trackInArea.begin();
        it != trackInArea.end(); ++it)
      tileAt((*it).x, (*it).y).station = station;
   
   return station;
}

// Write the terrain height map into a binary file
// Binary file format is very simple:
//   Bytes 0-3   Width of map
//   Bytes 4-7   Depth of map
//   Bytes 8+    Raw height data
void Map::writeHeightMap() const
{
   using namespace boost;

   IResource::Handle h = resource->writeFile(resource->name() + ".bin");

   log() << "Writing terrain height map to " << h.fileName();

   ofstream& of = h.wstream();
   
   const int32_t wl = static_cast<int32_t>(myWidth);
   const int32_t dl = static_cast<int32_t>(myDepth);
   of.write(reinterpret_cast<const char*>(&wl), sizeof(int32_t));
   of.write(reinterpret_cast<const char*>(&dl), sizeof(int32_t));

   for (int i = 0; i < (myWidth + 1) * (myDepth + 1); i++)
      of.write(reinterpret_cast<const char*>(&heightMap[i].pos.y),
         sizeof(float));
}

// Read the height data back out of a binary file
void Map::readHeightMap(IResource::Handle aHandle)
{
   using namespace boost;

   log() << "Reading height map from " << aHandle.fileName();

   istream& is = aHandle.rstream();
   
   // Check the dimensions of the binary file match the XML file
   int32_t wl, dl;
   is.read(reinterpret_cast<char*>(&wl), sizeof(int32_t));
   is.read(reinterpret_cast<char*>(&dl), sizeof(int32_t));

   if (wl != myWidth || dl != myDepth) {
      error() << "Expected width " << myWidth << " got " << wl;
      error() << "Expected height " << myDepth << " got " << dl;
      throw runtime_error
         ("Binary file " + aHandle.fileName() + " dimensions are incorrect");
   }

   for (int i = 0; i < (myWidth + 1) * (myDepth + 1); i++)
      is.read(reinterpret_cast<char*>(&heightMap[i].pos.y),
         sizeof(float));

   for (int x = 0; x < myWidth; x++) {
      for (int y = 0; y < myDepth; y++)
         fixNormals(x, y);
   }
}

// Turn the map into XML
void Map::save()
{
   using namespace boost::filesystem;

   IResource::Handle h = resource->writeFile(resource->name() + ".xml");
   
   log() << "Saving map to " << h.fileName();

   ofstream& of = h.wstream();
   
   xml::element root("map");
   root.addAttribute("width", myWidth);
   root.addAttribute("height", myDepth);

   root.addChild(xml::element("name").addText("No Name"));
   
   root.addChild
      (xml::element("start")
         .addAttribute("x", startLocation.x)
         .addAttribute("y", startLocation.y)
         .addAttribute("dirX", startDirection.x)
         .addAttribute("dirY", startDirection.z));

   // Write out all the stations
   set<IStationPtr> seenStations;
   
   for (int x = 0; x < myWidth; x++) {
      for (int y = 0; y < myDepth; y++) {
         IStationPtr s = tileAt(x, y).station;

         if (s && seenStations.find(s) == seenStations.end()) {
            // Not seen this station before
            root.addChild
               (xml::element("station")
                  .addAttribute("id", s->id())
                  .addChild(xml::element("name").addText(s->name())));

            seenStations.insert(s);
         }
      }
   }
   
   // Generate the height map
   writeHeightMap();

   root.addChild
      (xml::element("heightmap")
         .addText(resource->name() + ".bin"));

   xml::element tileset("tileset");
   
   for (int x = 0; x < myWidth; x++) {
      for (int y = 0; y < myDepth; y++) {
         const Tile& tile = tileAt(x, y);

         bool useful = false;
         xml::element tileXml("tile");

         tileXml.addAttribute("x", x);
         tileXml.addAttribute("y", y);

         if (tile.track
            && tile.track->originX() == x
            && tile.track->originY() == y) {

            tileXml.addChild(tile.track->get()->toXml());
            useful = true;
         }

         if (tile.station) {
            tileXml.addChild
               (xml::element("stationPart")
                  .addAttribute("id", tile.station->id()));
            useful = true;
         }

         if (tile.building) {
            tileXml.addChild
               (xml::element("building")
                  .addAttribute("angle", tile.buildingAngle)
                  .addAttribute("name", tile.building->resId()));
            useful = true;
         }

         if (useful)
            tileset.addChild(tileXml);
      }
   }

   root.addChild(tileset);
   
   of << xml::document(root);
}

IMapPtr makeEmptyMap(const string& aResId, int aWidth, int aDepth)
{
   IResourcePtr res = makeNewResource(aResId, "maps");
   
   shared_ptr<Map> ptr(new Map(res));
   ptr->resetMap(aWidth, aDepth);
   ptr->save();
   return IMapPtr(ptr);
}

// Build a map through XML callbacks
class MapLoader : public IXMLCallback {
public:
   MapLoader(shared_ptr<Map> aMap, IResourcePtr aRes)
      : myMap(aMap), myXPtr(0), myYPtr(0),
        resource(aRes) {}

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
      else if (localName == "points")
         handlePoints(attrs);
      else if (localName == "stationPart")
         handleStationPart(attrs);
      else if (localName == "station")
         handleStation(attrs);
      else if (localName == "building")
         handleBuilding(attrs);
   }

   void endElement(const string& localName)
   {
      if (localName == "station")
         myActiveStation.reset();
   }                   
   
   void text(const string& localName, const string& aString)
   {
      if (localName == "heightmap")
         myMap->readHeightMap(resource->openFile(aString));
      else if (myActiveStation) {
         if (localName == "name")
            myActiveStation->setName(aString);
      }
   }
   
private:
   void handleMap(const AttributeSet& attrs)
   {
      int width, height;
      attrs.get("width", width);
      attrs.get("height", height);

      myMap->resetMap(width, height);
   }

   void handleBuilding(const AttributeSet& attrs)
   {
      float angle;
      string name;
      attrs.get("name", name);
      attrs.get("angle", angle);

      myMap->placeBuilding(makePoint(myXPtr, myYPtr),
         loadBuilding(name), angle);
   }      
   
   void handleStation(const AttributeSet& attrs)
   {
      myActiveStation = makeStation();

      int id;
      attrs.get("id", id);
      myActiveStation->setId(id);

      myStations[id] = myActiveStation;
   }
   
   void handleStart(const AttributeSet& attrs)
   {
      int x, y, dirX, dirY;
      attrs.get("x", x);
      attrs.get("y", y);
      attrs.get("dirX", dirX);
      attrs.get("dirY", dirY);
      
      myMap->setStart(x, y, dirX, dirY);
   }

   void handleTile(const AttributeSet& attrs)
   {
      attrs.get("x", myXPtr);
      attrs.get("y", myYPtr);
   }

   void handleStationPart(const AttributeSet& attrs)
   {
      int id;
      attrs.get("id", id);

      map<int, IStationPtr>::iterator it = myStations.find(id);
      if (it == myStations.end())
         throw runtime_error("No station definition for ID "
            + boost::lexical_cast<string>(id));
      else
         myMap->setStationAt(myXPtr, myYPtr, (*it).second);
   }

   void handleStraightTrack(const AttributeSet& attrs)
   {
      string align;
      attrs.get("align", align);

      track::Direction axis = align == "x" ? axis::X : axis::Y;

      myMap->setTrackAt(makePoint(myXPtr, myYPtr),
         makeStraightTrack(axis));
   }

   void handlePoints(const AttributeSet& attrs)
   {
      string align;
      attrs.get("align", align);

      bool reflect;
      attrs.get("reflect", reflect);

      track::Direction dir =
         align == "x" ? axis::X
         : (align == "-x" ? -axis::X
            : (align == "y" ? axis::Y : -axis::Y));

      myMap->setTrackAt(makePoint(myXPtr, myYPtr),
         makePoints(dir, reflect));
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
   map<int, IStationPtr> myStations;
   IStationPtr myActiveStation;
   int myXPtr, myYPtr;

   IResourcePtr resource;
}; 

IMapPtr loadMap(const string& aResId)
{
   IResourcePtr res = findResource(aResId, "maps");
   
   shared_ptr<Map> map(new Map(res));

   log() << "Loading map from file " << res->xmlFileName();

   static IXMLParserPtr xmlParser = makeXMLParser("schemas/map.xsd");

   MapLoader loader(map, res);
   xmlParser->parse(res->xmlFileName(), loader);
      
   return IMapPtr(map);
}
