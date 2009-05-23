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

#include "IQuadTree.hpp"
#include "ILogger.hpp"

#include <stdexcept>
#include <sstream>
#include <cstdlib>

using namespace std;

class QuadTree : public IQuadTree {
public:
   QuadTree(ISectorRenderablePtr aRenderable);
   ~QuadTree();

   void buildTree(int aDim);
   
   void render(IGraphicsPtr aContext)
   {
      myKillCount = 0;
      displaySector(aContext, 0);
   }

private:
   enum QuadType { QT_LEAF, QT_BRANCH };
   
   struct Sector {
      Point<int> botLeft, topRight;
      unsigned int id;
      unsigned int children[4];
      QuadType type;
   } *mySectors;

   int calcNumSectors(int aWidth);
   int buildNode(int anId, int aParent, int x1, int y1, int x2, int y2);
   void displaySector(IGraphicsPtr aContext, int aSector);
   
   int mySize, myNumSectors, myUsedSectors;
   ISectorRenderablePtr myRenderer;

   int myKillCount;

   static const int QT_LEAF_SIZE = 8; 	// Number of tiles in a QuadTree leaf
};

QuadTree::QuadTree(ISectorRenderablePtr aRenderable)
   : mySectors(NULL), mySize(0), myNumSectors(0), myUsedSectors(0),
     myRenderer(aRenderable), myKillCount(0)
{
   
}

QuadTree::~QuadTree()
{
   if (mySectors)
      delete[] mySectors;
}

// Creates a blank QuadTree
void QuadTree::buildTree(int aDim)
{
   mySize = aDim;

   // Error checking
   if (aDim % QT_LEAF_SIZE != 0)
      throw runtime_error("Invalid QuadTree dimensions!");
   
   // Allocate memory
   myNumSectors = calcNumSectors(aDim);
   myUsedSectors = 0;
   if (mySectors)
      delete[] mySectors;
   mySectors = new Sector[myNumSectors];
   
   // Build the tree
   buildNode(0, 0, 0, 0, mySize, mySize);
}

// Builds a node in the tree
int QuadTree::buildNode(int anId, int aParent, int x1, int y1, int x2, int y2)
{   
   // Store this sector's data
   mySectors[anId].id = anId;
   mySectors[anId].botLeft.x = x1;
   mySectors[anId].botLeft.y = y1;
   mySectors[anId].topRight.x = x2;
   mySectors[anId].topRight.y = y2;

   // Check to see if it's a leaf
   if (abs(x1 - x2) == QT_LEAF_SIZE && abs(y1 - y2) == QT_LEAF_SIZE)
      mySectors[anId].type = QT_LEAF;
   else {
      mySectors[anId].type = QT_BRANCH;
      
      int w = x2 - x1;
      int h = y2 - y1;
      
      // Build children
      unsigned int* c = mySectors[anId].children;
      c[0] = buildNode(++myUsedSectors, anId, x1,		  y1,     x1+w/2, y1+h/2);
      c[1] = buildNode(++myUsedSectors, anId, x1,		  y1+h/2, x1+w/2, y2		);
      c[3] = buildNode(++myUsedSectors, anId, x1+w/2, y1+h/2, x2,		  y2	  );
      c[2] = buildNode(++myUsedSectors, anId, x1+w/2, y1,		  x2,     y1+h/2);
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

// Render all the visible sectors
void QuadTree::displaySector(IGraphicsPtr aContext, int aSector)
{  
   if (aSector >= myNumSectors) {
      ostringstream ss;
      ss << "displaySector(" << aSector << ") out of range";
      throw runtime_error(ss.str());
   }
 
   // See if it's a leaf
   if (mySectors[aSector].type == QT_LEAF)
      myRenderer->renderSector
         (aContext, mySectors[aSector].botLeft, mySectors[aSector].topRight, true);
   else {
      // Loop through each sector
      for (int i = 3; i >= 0; i--) {
         int childID = mySectors[aSector].children[i];
         Sector* child = &mySectors[childID];
         
         int w = child->topRight.x - child->botLeft.x;
         int h = child->topRight.y - child->botLeft.y;
         
         int x = child->botLeft.x + w/2;
         int y = child->botLeft.y + h/2;

         if (aContext->cubeInViewFrustum((float)x, 0.0f, (float)y, (float)w/2))
            displaySector(aContext, childID);
         else
            myKillCount++;
      }
   }
}

IQuadTreePtr makeQuadTree(ISectorRenderablePtr aRenderer, int aDim)
{
   auto_ptr<QuadTree> ptr(new QuadTree(aRenderer));
   ptr->buildTree(aDim);
   return IQuadTreePtr(ptr);
}
