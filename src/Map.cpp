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

#include <stdexcept>
#include <sstream>
#include <cassert>
#include <fstream>

#include <GL/gl.h>

using namespace std;
using namespace std::tr1;

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
public:
   Map();
   ~Map();

   // IMap interface
   int width() const { return myWidth; }
   int depth() const { return myDepth; }
   double heightAt() const { return 0.0; }

   void setStart(int x, int y) { myStartLocation = makePoint(x, y); }
   void setGrid(bool onOff);
   
   Track::Connection startLocation() const;
   ITrackSegmentPtr trackAt(const Point<int>& aPoint) const;
   void setTrackAt(const Point<int>& aPoint, ITrackSegmentPtr aTrack);
   bool isValidTrack(const Point<int>& aPoint) const;
   void render(IGraphicsPtr aContext) const;
   void highlightTile(IGraphicsPtr aContext, const Point<int>& aPoint) const;
   void rebuildDisplayLists();
   void resetMap(int aWidth, int aDepth);

   void save(const string& aFileName);
   
   // ISectorRenderable interface
   void renderSector(IGraphicsPtr aContext,
                     Point<int> botLeft, Point<int> topRight);
private:
   // Tiles on the map
   struct Tile {
      struct Vertex {
         Vector<double> pos, normal;
      } v[4];
      TrackNodePtr track;  // Track at this location, if any
   } *myTiles;

   static const unsigned TILE_NAME_BASE	= 1000;	  // Base of tile naming
   static const unsigned NULL_OBJECT		= 0;		  // Non-existant object
   static const double TILE_HEIGHT		  = 0.2;	  // Standard height increment

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
   
   int myWidth, myDepth;
   Point<int> myStartLocation;
   IQuadTreePtr myQuadTree;
   IFogPtr myFog;
   bool shouldDrawGridLines;
};

Map::Map()
   : myTiles(NULL), myWidth(0), myDepth(0),
     myStartLocation(makePoint(1, 1)),
     shouldDrawGridLines(false)
{
   myFog = makeFog(0.6, 0.7, 0.8,  // Colour
                   0.35,           // Density
                   20.0, 30.0);    // Start and end distance
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

void Map::rebuildDisplayLists()
{
   resetMarks();
   
   // TODO: We should keep a list of dirty points and only rebuild the
   // quads that cover those
   myQuadTree->rebuildDisplayLists();
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
   rebuildDisplayLists();
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
   glClearColor(0.6, 0.7, 0.8, 1.0);
   
   myFog->apply();
   
   glPushAttrib(GL_ALL_ATTRIB_BITS);

   // Thick lines for grid
   glLineWidth(2.0f);

   // Use the value of glColor rather than materials
   glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
   glEnable(GL_COLOR_MATERIAL);

   glDisable(GL_TEXTURE_2D);
   
   glPushMatrix();
   myQuadTree->render(aContext);
   glPopMatrix();
   
   glPopAttrib();
}

// Draw a thick border around a single tile
void Map::highlightTile(IGraphicsPtr aContext, const Point<int>& aPoint) const
{
   Tile::Vertex* v = myTiles[index(aPoint.x, aPoint.y)].v;

   // User should be able to click on the highlight as well
   glPushName(tileName(aPoint.x, aPoint.y));
         
   glDisable(GL_TEXTURE_2D);
   glEnable(GL_BLEND);
   glDisable(GL_LIGHTING);
   
   glPushMatrix();
   glTranslated(static_cast<double>(aPoint.x), 0,
                static_cast<double>(aPoint.y));
   glColor4d(1.0, 1.0, 1.0, 0.5);
   glBegin(GL_POLYGON);
   
   for (int i = 0; i < 4; i++) {
      glNormal3d(v[i].normal.x, v[i].normal.y, v[i].normal.z);
      glVertex3d(v[i].pos.x, v[i].pos.y, v[i].pos.z);
   }
   
   glEnd();
   glPopMatrix();

   glPopName();
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
         glColor3f(0.7f, 1.0f, 0.7f);
         glBegin(GL_POLYGON);

         for (int i = 0; i < 4; i++) {
            glNormal3d(v[i].normal.x, v[i].normal.y, v[i].normal.z);
            glVertex3d(v[i].pos.x, v[i].pos.y, v[i].pos.z);
         }
         
         glEnd();
         glPopMatrix();

         if (shouldDrawGridLines) {
            // Render grid lines
            glPushMatrix();
            glTranslated(static_cast<double>(x), 0, static_cast<double>(y));
            glColor3f(0.0f, 0.0f, 0.0f);
            glBegin(GL_LINE_LOOP);
            
            for (int i = 0; i < 4; i++) 
               glVertex3d(v[i].pos.x, v[i].pos.y, v[i].pos.z);
            
            glEnd();
            glPopMatrix();
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
         
         glPopName();
      }			
   }
}

// Turn the map into XML
void Map::save(const string& aFileName)
{
   log() << "Saving map to " << aFileName;

   ofstream of(aFileName.c_str());
   if (!of.good())
      throw runtime_error("Failed to open " + aFileName + " for writing");

   xml::element root("map");

   root.addChild(xml::element("name").addText("No Name"));
   
   root.addChild
      (xml::element("start")
       .addAttribute("x", 1)
       .addAttribute("y", 1));

   xml::element tileset("tileset");
   tileset.addAttribute("width", myWidth);
   tileset.addAttribute("height", myDepth);
   
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
      if (localName == "tileset")
         handleTileset(attrs);
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
   
private:
   void handleTileset(const AttributeSet& attrs)
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

   map->rebuildDisplayLists();
      
   return IMapPtr(map);
}
