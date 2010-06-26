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

#ifndef INC_IQUADTREE_HPP
#define INC_IQUADTREE_HPP

#include "Platform.hpp"
#include "Maths.hpp"
#include "IGraphics.hpp"

// Interface to things that can be rendered by sector
struct ISectorRenderable {
   virtual ~ISectorRenderable() {}

   virtual void render_sector(IGraphicsPtr a_context, int id,
                             Point<int> bot_left, Point<int> top_right) = 0;
   virtual void post_render_sector(IGraphicsPtr a_context, int id,
                                 Point<int> bot_left, Point<int> top_right) = 0;
};

typedef shared_ptr<ISectorRenderable> ISectorRenderablePtr;

// Inteface for constructing quad trees for space partioning
struct IQuadTree {
   virtual ~IQuadTree() {}

   virtual void render(IGraphicsPtr a_context) = 0;
   virtual int leaf_size() const = 0;
};

typedef shared_ptr<IQuadTree> IQuadTreePtr;

// Produce a quad tree of given square dimension
IQuadTreePtr make_quad_tree(ISectorRenderablePtr a_renderable,
   int width, int height);

#endif
