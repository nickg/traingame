//
//  Copyright (C) 2009-2010  Nick Gasson
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

#include "IQuadTree.hpp"
#include "ILogger.hpp"

#include <stdexcept>
#include <sstream>
#include <cstdlib>
#include <list>

using namespace std;

class QuadTree : public IQuadTree {
public:
   QuadTree(ISectorRenderablePtr aRenderable);
   ~QuadTree();

   void buildTree(int width, int height);
   
   void render(IGraphicsPtr aContext);
   int leafSize() const { return QT_LEAF_SIZE; }

private:
   enum QuadType { QT_LEAF, QT_BRANCH };

   struct Sector {
      Point<int> botLeft, topRight;
      unsigned int id;
      unsigned int children[4];
      QuadType type;
   } *sectors;

   int calcNumSectors(int aWidth);
   int buildNode(int anId, int aParent, int x1, int y1, int x2, int y2);
   void visibleSectors(IGraphicsPtr aContext, list<Sector*>& aList, int aSector);
   
   int size, numSectors, usedSectors;
   ISectorRenderablePtr renderer;

   // We support non-square maps using a kludge where we make a quad tree
   // of size equal to the the largest dimension and then ignore sectors
   // that fall outside the real dimensions
   int realWidth, realHeight;

   int killCount;

   static const int QT_LEAF_SIZE = 8; 	// Number of tiles in a QuadTree leaf
};

QuadTree::QuadTree(ISectorRenderablePtr aRenderable)
   : sectors(NULL), size(0), numSectors(0), usedSectors(0),
     renderer(aRenderable),
     realWidth(0), realHeight(0),
     killCount(0)
{
   
}

QuadTree::~QuadTree()
{
   if (sectors)
      delete[] sectors;
}

void QuadTree::render(IGraphicsPtr aContext)
{
   list<Sector*> visible;
   killCount = 0;
   visibleSectors(aContext, visible, 0);

   list<Sector*>::const_iterator it;
   
   for (it = visible.begin(); it != visible.end(); ++it)
      renderer->renderSector(aContext, (*it)->id,
         (*it)->botLeft, (*it)->topRight);

   for (it = visible.begin(); it != visible.end(); ++it)
      renderer->postRenderSector(aContext, (*it)->id,
         (*it)->botLeft, (*it)->topRight);
}

// Creates a blank QuadTree
void QuadTree::buildTree(int width, int height)
{
   size = max(width, height);

   realWidth = width;
   realHeight = height;

   // Error checking
   if (size % QT_LEAF_SIZE != 0)
      throw runtime_error("Invalid QuadTree dimensions!");
   
   // Allocate memory
   numSectors = calcNumSectors(size);
   usedSectors = 0;
   if (sectors)
      delete[] sectors;
   sectors = new Sector[numSectors];
   
   // Build the tree
   buildNode(0, 0, 0, 0, size, size);
}

// Builds a node in the tree
int QuadTree::buildNode(int anId, int aParent, int x1, int y1, int x2, int y2)
{   
   // Store this sector's data
   sectors[anId].id = anId;
   sectors[anId].botLeft.x = x1;
   sectors[anId].botLeft.y = y1;
   sectors[anId].topRight.x = x2;
   sectors[anId].topRight.y = y2;

   // Check to see if it's a leaf
   if (abs(x1 - x2) == QT_LEAF_SIZE && abs(y1 - y2) == QT_LEAF_SIZE)
      sectors[anId].type = QT_LEAF;
   else {
      sectors[anId].type = QT_BRANCH;
      
      int w = x2 - x1;
      int h = y2 - y1;
      
      // Build children
      unsigned int* c = sectors[anId].children;
      c[0] = buildNode(++usedSectors, anId, x1,		  y1,     x1+w/2, y1+h/2);
      c[1] = buildNode(++usedSectors, anId, x1,		  y1+h/2, x1+w/2, y2	);
      c[3] = buildNode(++usedSectors, anId, x1+w/2, y1+h/2, x2,		  y2    );
      c[2] = buildNode(++usedSectors, anId, x1+w/2, y1,		  x2,     y1+h/2);
   }

   return anId;
}

// Work out how many sectors are required in the QuadTree
int QuadTree::calcNumSectors(int aWidth)
{
   int count = 0;
   
   if (aWidth > QT_LEAF_SIZE) {
      for (int i = 0; i < 4; i++)
         count += calcNumSectors(aWidth/2);
      return count + 1;
   }
   else
      return 1;		
}

// Find all the visible sectors
void QuadTree::visibleSectors(IGraphicsPtr aContext, list<Sector*>& aList,
                              int aSector)
{
   if (aSector >= numSectors) {
      ostringstream ss;
      ss << "displaySector(" << aSector << ") out of range";
      throw runtime_error(ss.str());
   }

   Sector& s = sectors[aSector];

   bool botLeftOutside =
      s.botLeft.x >= realWidth
      || s.botLeft.y >= realHeight;
   if (botLeftOutside) {
      // A non-square map
      return;
   }      
 
   // See if it's a leaf
   if (s.type == QT_LEAF)
      aList.push_back(&s);
   else {
      // Loop through each sector
      for (int i = 3; i >= 0; i--) {
         int childID = s.children[i];
         Sector* child = &sectors[childID];
         
         int w = child->topRight.x - child->botLeft.x;
         int h = child->topRight.y - child->botLeft.y;
         
         int x = child->botLeft.x + w/2;
         int y = child->botLeft.y + h/2;

         if (aContext->cubeInViewFrustum((float)x, 0.0f, (float)y, (float)w/2))
            visibleSectors(aContext, aList, childID);
         else
            killCount++;
      }
   }
}

IQuadTreePtr makeQuadTree(ISectorRenderablePtr aRenderer, int width, int height)
{
   auto_ptr<QuadTree> ptr(new QuadTree(aRenderer));
   ptr->buildTree(width, height);
   return IQuadTreePtr(ptr);
}
