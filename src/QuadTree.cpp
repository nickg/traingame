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
   QuadTree(ISectorRenderablePtr a_renderable);
   ~QuadTree();

   void build_tree(int width, int height);
   
   void render(IGraphicsPtr a_context);
   int leaf_size() const { return QT_LEAF_SIZE; }

private:
   enum QuadType { QT_LEAF, QT_BRANCH };

   struct Sector {
      Point<int> bot_left, top_right;
      unsigned int id;
      unsigned int children[4];
      QuadType type;
   } *sectors;

   int calc_num_sectors(int a_width);
   int build_node(int an_id, int a_parent, int x1, int y1, int x2, int y2);
   void visible_sectors(IGraphicsPtr a_context, list<Sector*>& a_list, int a_sector);
   
   int size, num_sectors, used_sectors;
   ISectorRenderablePtr renderer;

   // We support non-square maps using a kludge where we make a quad tree
   // of size equal to the the largest dimension and then ignore sectors
   // that fall outside the real dimensions
   int real_width, real_height;

   int kill_count;

   static const int QT_LEAF_SIZE = 8; 	// Number of tiles in a QuadTree leaf
};

QuadTree::QuadTree(ISectorRenderablePtr a_renderable)
   : sectors(NULL), size(0), num_sectors(0), used_sectors(0),
     renderer(a_renderable),
     real_width(0), real_height(0),
     kill_count(0)
{
   
}

QuadTree::~QuadTree()
{
   if (sectors)
      delete[] sectors;
}

void QuadTree::render(IGraphicsPtr a_context)
{
   list<Sector*> visible;
   kill_count = 0;
   visible_sectors(a_context, visible, 0);

   list<Sector*>::const_iterator it;
   
   for (it = visible.begin(); it != visible.end(); ++it)
      renderer->render_sector(a_context, (*it)->id,
         (*it)->bot_left, (*it)->top_right);

   for (it = visible.begin(); it != visible.end(); ++it)
      renderer->post_render_sector(a_context, (*it)->id,
         (*it)->bot_left, (*it)->top_right);
}

// Creates a blank QuadTree
void QuadTree::build_tree(int width, int height)
{
   size = max(width, height);

   real_width = width;
   real_height = height;

   // Error checking
   if (size % QT_LEAF_SIZE != 0)
      throw runtime_error("Invalid QuadTree dimensions!");
   
   // Allocate memory
   num_sectors = calc_num_sectors(size);
   used_sectors = 0;
   if (sectors)
      delete[] sectors;
   sectors = new Sector[num_sectors];
   
   // Build the tree
   build_node(0, 0, 0, 0, size, size);
}

// Builds a node in the tree
int QuadTree::build_node(int an_id, int a_parent, int x1, int y1, int x2, int y2)
{   
   // Store this sector's data
   sectors[an_id].id = an_id;
   sectors[an_id].bot_left.x = x1;
   sectors[an_id].bot_left.y = y1;
   sectors[an_id].top_right.x = x2;
   sectors[an_id].top_right.y = y2;

   // Check to see if it's a leaf
   if (abs(x1 - x2) == QT_LEAF_SIZE && abs(y1 - y2) == QT_LEAF_SIZE)
      sectors[an_id].type = QT_LEAF;
   else {
      sectors[an_id].type = QT_BRANCH;
      
      int w = x2 - x1;
      int h = y2 - y1;
      
      // Build children
      unsigned int* c = sectors[an_id].children;
      c[0] = build_node(++used_sectors, an_id, x1,		  y1,     x1+w/2, y1+h/2);
      c[1] = build_node(++used_sectors, an_id, x1,		  y1+h/2, x1+w/2, y2	);
      c[3] = build_node(++used_sectors, an_id, x1+w/2, y1+h/2, x2,		  y2    );
      c[2] = build_node(++used_sectors, an_id, x1+w/2, y1,		  x2,     y1+h/2);
   }

   return an_id;
}

// Work out how many sectors are required in the QuadTree
int QuadTree::calc_num_sectors(int a_width)
{
   int count = 0;
   
   if (a_width > QT_LEAF_SIZE) {
      for (int i = 0; i < 4; i++)
         count += calc_num_sectors(a_width/2);
      return count + 1;
   }
   else
      return 1;		
}

// Find all the visible sectors
void QuadTree::visible_sectors(IGraphicsPtr a_context, list<Sector*>& a_list,
                               int a_sector)
{
   if (a_sector >= num_sectors) {
      ostringstream ss;
      ss << "display_sector(" << a_sector << ") out of range";
      throw runtime_error(ss.str());
   }

   Sector& s = sectors[a_sector];

   bool bot_left_outside =
      s.bot_left.x >= real_width
      || s.bot_left.y >= real_height;
   if (bot_left_outside) {
      // A non-square map
      return;
   }      
 
   // See if it's a leaf
   if (s.type == QT_LEAF)
      a_list.push_back(&s);
   else {
      // Loop through each sector
      for (int i = 3; i >= 0; i--) {
         int childID = s.children[i];
         Sector* child = &sectors[childID];
         
         int w = child->top_right.x - child->bot_left.x;
         int h = child->top_right.y - child->bot_left.y;
         
         int x = child->bot_left.x + w/2;
         int y = child->bot_left.y + h/2;

         if (a_context->cube_in_view_frustum(
                static_cast<float>(x) - 0.5f,
                0.0f,
                static_cast<float>(y) - 0.5f,
                static_cast<float>(w) / 2.0f))
            visible_sectors(a_context, a_list, childID);
         else
            kill_count++;
      }
   }
}

IQuadTreePtr make_quad_tree(ISectorRenderablePtr a_renderer,
                            int width, int height)
{
   auto_ptr<QuadTree> ptr(new QuadTree(a_renderer));
   ptr->build_tree(width, height);
   return IQuadTreePtr(ptr);
}
