//
//  Copyright (C) 2009-2011  Nick Gasson
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
#include "IConfig.hpp"
#include "OpenGLHelper.hpp"
#include "ClipVolume.hpp"

#include <stdexcept>
#include <sstream>
#include <cassert>
#include <fstream>
#include <set>
#include <map>

#include <boost/filesystem.hpp>
#include <boost/cstdint.hpp>
#include <boost/lexical_cast.hpp>

// A single piece of track, scenery, etc. may be connected to
// more than one tile. Anchor<T> is the association class
template <class T>
class Anchor {
public:
   Anchor(shared_ptr<T> obj, Point<int> origin)
      : owned(obj), last_frame(-1), origin_(origin) {}

   // These numbers don't necessarily refer to frames
   void rendered_on(int f) { last_frame = f; }
   bool needs_rendering(int f) const { return f != last_frame; }

   const Point<int>& origin() const { return origin_; }

   inline shared_ptr<T> get() { return owned; }
private:
   shared_ptr<T> owned;
   int last_frame;
   Point<int> origin_;
};

typedef shared_ptr<Anchor<ITrackSegment> > TrackAnchor;
typedef shared_ptr<Anchor<IScenery> > SceneryAnchor;

class Map : public IMap, public ISectorRenderable,
            public enable_shared_from_this<Map> {
   friend class MapLoader;
public:
   Map(IResourcePtr a_res);
   ~Map();

   // IMap interface
   int width() const { return my_width; }
   int depth() const { return my_depth; }
   double height_at() const { return 0.0; }

   string name() const { return resource->name(); }

   void set_start(int x, int y);
   void set_start(int x, int y, int dirX, int dirY);
   void set_grid(bool on_off);
   void set_pick_mode(bool on_off) { in_pick_mode = on_off; }
   
   track::Connection start() const;
   ITrackSegmentPtr track_at(const Point<int>& a_point) const;
   IStationPtr station_at(Point<int> a_point) const;
   void set_track_at(const Point<int>& a_point, ITrackSegmentPtr a_track);
   bool is_valid_track(const Point<int>& a_point) const;
   void render(IGraphicsPtr a_context) const;
   void highlight_tile(Point<int> point, Colour colour) const;
   void highlight_vertex(Point<int> point, Colour colour) const;
   void reset_map(int a_width, int a_depth);
   void erase_tile(int x, int y);
   bool empty_tile(Point<int> tile) const;

   void raise_area(const Point<int>& a_start_pos,
      const Point<int>& a_finish_pos);
   void lower_area(const Point<int>& a_start_pos,
      const Point<int>& a_finish_pos);
   void level_area(Point<int> a_start_pos, Point<int> a_finish_pos);
   void smooth_area(Point<int> start, Point<int> finish);
   
   void save();

   IStationPtr extend_station(Point<int> a_start_pos,
      Point<int> a_finish_pos);
   float height_at(float x, float y) const;
   float height_at(Point<int> where) const;
   Vector<float> slope_at(Point<int> where, track::Direction axis,
      bool& level) const;
   Vector<float> slope_before(Point<int> where,
      track::Direction axis, bool &valid) const;
   Vector<float> slope_after(Point<int> where,
      track::Direction axis, bool &valid) const;
   void add_scenery(Point<int> where, ISceneryPtr s);
   
   // ISectorRenderable interface
   void render_sector(IGraphicsPtr a_context, int id,
      Point<int> bot_left, Point<int> top_right);
   void post_render_sector(IGraphicsPtr a_context, int id,
      Point<int> bot_left, Point<int> top_right);
   
private:
   // Tiles on the map
   struct Tile {
      // TODO: Better to use a boost::variant here?
      TrackAnchor track;    // Track at this location, if any
      IStationPtr station;   // Station on this tile, if any
      SceneryAnchor scenery;   // Scenery, if any
   } *tiles;

   // Vertices on the terrain
   struct HeightMap {
      Vector<float> pos, normal;

      // How many track segments are locking the height at this node
      int lock_count;
   } *height_map;

   static const unsigned TILE_NAME_BASE	= 1000;	  // Base of tile naming
   static const unsigned NULL_OBJECT	= 0;	  // Non-existant object
   static const float TILE_HEIGHT;	   // Standard height increment

   // Meshes for each terrain sector
   vector<IMeshPtr> terrain_meshes;
   
   inline int index(int x, int y) const
   {
      assert(x < my_width && y < my_depth && x >= 0 && y >= 0);
      return x + y*my_width;
   }
   
   inline int tile_name(int x, int z) const
   {
      return TILE_NAME_BASE + index(x, z);
   }

   inline Tile& tile_at(int x, int z) const
   {
      return tiles[index(x, z)];
   }

   inline Tile& tile_at(const Point<int>& p) const
   {
      return tile_at(p.x, p.y);
   }

   inline HeightMap& height_at(int i) const
   {
      assert(i >= 0 && i < (my_width + 1) * (my_depth + 1));
      return height_map[i];
   }
   
   bool is_valid_tileName(unsigned a_name) const
   {
      return a_name >= TILE_NAME_BASE
         && a_name < TILE_NAME_BASE + my_width * my_depth;
   }

   Point<int> pick_position(unsigned a_name) const
   {      
      assert(is_valid_tileName(a_name));
      
      int a = a_name - TILE_NAME_BASE;
      return make_point(a % my_width, a / my_width);
   }

   void write_height_map() const;
   void save_to(ostream& of);
   void read_height_map(IResource::Handle a_handle);
   void tile_vertices(int x, int y, int* indexes) const;
   void render_pick_sector(Point<int> bot_left, Point<int> top_right);
   void draw_start_location() const;
   void set_station_at(Point<int> point, IStationPtr a_station);
   void render_highlighted_tiles() const;
   void lock_height_at(Point<int> p);
   void unlock_height_at(Point<int> p);

   // Mesh modification
   void build_mesh(int id, Point<int> bot_left, Point<int> top_right);
   bool have_mesh(int id, Point<int> bot_left, Point<int> top_right);
   void dirty_tile(int x, int y);
      
   // Terrain modification
   void change_area_height(const Point<int>& a_start_pos,
      const Point<int>& a_finish_pos, float a_height_delta);
   void raise_tile(int x, int y, float delta_height);
   void set_tile_height(int x, int y, float h);
   void fix_normals(int x, int y);
   bool raise_will_cover_track(int x, int y) const;
   
   int my_width, my_depth;
   Point<int> start_location;
   track::Direction start_direction;
   IQuadTreePtr quad_tree;
   IFogPtr fog;
   bool should_draw_grid_lines, in_pick_mode;
   list<Point<int> > dirty_tiles;
   IResourcePtr resource;
   vector<bool> sea_sectors;

   // Variables used during rendering
   mutable int frame_num;
   mutable vector<tuple<Point<int>, Colour> > highlighted_tiles;
};

const float Map::TILE_HEIGHT(0.2f);

Map::Map(IResourcePtr a_res)
   : tiles(NULL), height_map(NULL), my_width(0), my_depth(0),
     start_location(make_point(1, 1)),
     start_direction(axis::X),
     should_draw_grid_lines(false), in_pick_mode(false),
     resource(a_res), frame_num(0)
{
   const float far_clip = get_config()->get<float>("FarClip");
   fog = make_fog(0.005f,         // Density
      3.0f * far_clip / 4.0f,     // Start
      far_clip);                  // End distance
}

Map::~Map()
{
   delete tiles;
}

ITrackSegmentPtr Map::track_at(const Point<int>& a_point) const
{
   TrackAnchor ptr = tile_at(a_point.x, a_point.y).track;
   if (ptr)
      return ptr->get();
   else {
      ostringstream ss;
      ss << "No track segment at " << a_point;
      throw runtime_error(ss.str());
   }
}

IStationPtr Map::station_at(Point<int> a_point) const
{
   return tile_at(a_point.x, a_point.y).station;
}

void Map::set_station_at(Point<int> point, IStationPtr a_station)
{
   tile_at(point).station = a_station;
}

void Map::erase_tile(int x, int y)
{
   Tile& tile = tile_at(x, y);

   if (tile.track) {
      // We have to be a bit careful since a piece of track has multiple
      // endpoints

      vector<Point<int> > locked;      
      tile.track->get()->get_height_locked(locked);

      for (vector<Point<int> >::iterator it = locked.begin();
           it != locked.end(); ++it)
         unlock_height_at(*it);
      
      vector<Point<int> > covers;
      tile.track->get()->get_endpoints(covers);
      tile.track->get()->get_covers(covers);

      for (vector<Point<int> >::iterator it = covers.begin();
           it != covers.end(); ++it) {
         tile_at((*it).x, (*it).y).track.reset();
         dirty_tile((*it).x, (*it).y);
      }
   }

   if (tile.scenery) {
      // Like track, scenery may cover multiple tiles

      const Point<int> size = tile.scenery->get()->size();
      const Point<int>& where = tile.scenery->origin();
      
      for (int x = 0; x < size.x; x++) {
         for (int y = 0; y < size.y; y++) {
            tile_at(where.x + x, where.y + y).scenery.reset();
            dirty_tile(where.x + x, where.y + y);
         }
      }
   }

   if (tile.station) {
      tile.station.reset();
      dirty_tile(x, y);
   }
}

bool Map::empty_tile(Point<int> point) const
{
   Tile& tile = tile_at(point);

   return !(tile.track || tile.scenery);
}

void Map::set_track_at(const Point<int>& where, ITrackSegmentPtr track)
{   
   int indexes[4];
   tile_vertices(where.x, where.y, indexes);

   float lowest_height = 1.0e20f;
   for (int i = 0; i < 4; i++)
      lowest_height = min(height_map[indexes[i]].pos.y, lowest_height);
   
   track->set_origin(where.x, where.y, lowest_height);
         
   TrackAnchor node(new Anchor<ITrackSegment>(track, where));  

   // Attach the track node to every tile it covers
   vector<Point<int> > covers;
   track->get_endpoints(covers);
   track->get_covers(covers);

   for (vector<Point<int> >::iterator it = covers.begin();
        it != covers.end(); ++it) {
      tile_at((*it).x, (*it).y).track = node;

      dirty_tile((*it).x, (*it).y);
   }

   // Lock every height node touched by this track segment
   vector<Point<int> > locked;
   track->get_height_locked(locked);

   for (vector<Point<int> >::iterator it = locked.begin();
        it != locked.end(); ++it)
      lock_height_at(*it);
}

bool Map::is_valid_track(const Point<int>& where) const
{
   if (where.x < 0 || where.y < 0
      || where.x >= my_width || where.y >= my_depth)
      return false;

   return tile_at(where.x, where.y).track;
}

// Return a location where the train may start
track::Connection Map::start() const
{
   return make_pair(start_location, start_direction);
}

// Try to place the train on this tile
void Map::set_start(int x, int y)
{
   const track::Direction poss_dirs[] = {
      axis::X, axis::Y, -axis::X, -axis::Y
   };
   static int next_dir = 0;
   
   TrackAnchor track_node = tile_at(x, y).track;
   if (!track_node) {
      warn() << "Must place start on track";
      return;
   }

   ITrackSegmentPtr track = track_node->get();

   int tried = 0;
   do {
      if (track->is_valid_direction(poss_dirs[next_dir]))
         break;
      else
         next_dir = (next_dir + 1) % 4;
   } while (++tried < 4);

   if (tried == 4) {
      warn() << "Cannot find suitable initial direction for this track";
      return;
   }

   start_location = make_point(x, y);
   start_direction = poss_dirs[next_dir];

   next_dir = (next_dir + 1) % 4;
}

// Force the train to start on this tile
void Map::set_start(int x, int y, int dirX, int dirY)
{
   start_location = make_point(x, y);
   start_direction = make_vector(dirX, 0, dirY);
}

void Map::set_grid(bool on_off)
{
   should_draw_grid_lines = on_off;
}

void Map::reset_map(int a_width, int a_depth)
{   
   my_width = a_width;
   my_depth = a_depth;
   
   // Allocate memory
   if (tiles)
      delete[] tiles;
   tiles = new Tile[a_width * a_depth];

   if (height_map)
      delete[] height_map;
   height_map = new HeightMap[(a_width + 1) * (a_depth + 1)];
   
   // Make a flat map
   for (int x = 0; x <= a_width; x++) {
      for (int y = 0; y <= a_depth; y++) {
         HeightMap& v = height_map[x + y*(a_width+1)];

         const float xf = static_cast<float>(x) - 0.5f;
         const float yf = static_cast<float>(y) - 0.5f;

         v.pos = make_vector(xf, 0.0f, yf);
         v.normal = make_vector(0.0f, 1.0f, 0.0f);
         v.lock_count = 0;
      }
   }
   
   // Create quad tree
   quad_tree = make_quad_tree(shared_from_this(), my_width, my_depth);
}

void Map::highlight_vertex(Point<int> point, Colour colour) const
{
   assert(point.x >= 0 && point.x < my_width
          && point.y >= 0 && point.y < my_depth);

   int index = point.x + (point.y * (my_width + 1));
   
   gl::colour(colour);
   glPointSize(5.0f);
  
   glBegin(GL_POINTS);
   glVertex3f(static_cast<float>(point.x) - 0.5f,
              height_map[index].pos.y + 0.01f,
              static_cast<float>(point.y) - 0.5f);
   glEnd();
}

void Map::highlight_tile(Point<int> point, Colour colour) const
{
   highlighted_tiles.push_back(make_tuple(point, colour));
}

void Map::render_highlighted_tiles() const
{
   // At the end of the render loop, draw the highlighted tiles over
   // the top of all others - this is to get the transparency working
   
   glPushAttrib(GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT);
   
   glDisable(GL_TEXTURE_2D);
   glEnable(GL_BLEND);
   glDisable(GL_LIGHTING);
   
   glDepthMask(GL_FALSE);
      
   vector<tuple<Point<int>, Colour> >::const_iterator it;
   for (it = highlighted_tiles.begin(); it != highlighted_tiles.end(); ++it) {

      const Point<int>& point = get<0>(*it);
      Colour colour = get<1>(*it);
      
      // User should be able to click on the highlight as well
      glPushName(tile_name(point.x, point.y));

      colour.a = 0.5f;
      gl::colour(colour);
      glBegin(GL_POLYGON);
      
      int indexes[4];
      tile_vertices(point.x, point.y, indexes);
      
      for (int i = 0; i < 4; i++) {
         HeightMap& v = height_map[indexes[i]];
         glNormal3f(v.normal.x, v.normal.y, v.normal.z);
         glVertex3f(v.pos.x, v.pos.y + 0.1f, v.pos.z);
      }
      
      glEnd();
      
      glPopName();
   }

   glPopAttrib();
   
   highlighted_tiles.clear();
}

void Map::render(IGraphicsPtr a_context) const
{
   // The `frame_num' counter is used to ensure we draw each
   // track segment at most once per frame
   frame_num++;
   
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
   quad_tree->render(a_context);
   glPopMatrix();
   

   render_highlighted_tiles();
   
   glPopAttrib();
}

// Draw an arrow on the start location
void Map::draw_start_location() const
{
   glPushAttrib(GL_ENABLE_BIT);
   glPushMatrix();

   glEnable(GL_BLEND);
   glDisable(GL_TEXTURE_2D);

   int indexes[4];
   tile_vertices(start_location.x, start_location.y, indexes);

   float avg_height = 0.0f;
   for (int i = 0; i < 4; i++)
      avg_height += height_map[indexes[i]].pos.y;
   avg_height /= 4.0f;

   glTranslatef(static_cast<float>(start_location.x), 
      avg_height + 0.1f, 
      static_cast<float>(start_location.y));

   if (start_direction == axis::X)
      glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
   else if (start_direction == -axis::Y)
      glRotatef(180.0f, 0.0f, 1.0f, 0.0f);
   else if (start_direction == -axis::X)
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
bool Map::have_mesh(int id, Point<int> bot_left, Point<int> top_right)
{
   if (id >= static_cast<int>(terrain_meshes.size()))
      terrain_meshes.resize(id + 1);
   
   bool ok = terrain_meshes[id];
   list<Point<int> >::iterator it = dirty_tiles.begin();

   while (it != dirty_tiles.end()) {
      bool covered =
         (*it).x >= bot_left.x
         && (*it).x <= top_right.x
         && (*it).y >= bot_left.y
         && (*it).y <= top_right.y;
         
      if (covered) {
         ok = false;
         it = dirty_tiles.erase(it);
      }
      else
         ++it;
   }
   
   return ok;
}

// Record that the mesh containing a tile needs rebuilding
void Map::dirty_tile(int x, int y)
{
   dirty_tiles.push_back(make_point(x, y));

   // Push its neighbours as well since the vertices of a tile sit
   // on mesh boundaries
   dirty_tiles.push_back(make_point(x, y + 1));
   dirty_tiles.push_back(make_point(x, y - 1));
   dirty_tiles.push_back(make_point(x + 1, y));
   dirty_tiles.push_back(make_point(x - 1, y));
}

// Generate a terrain mesh for a particular sector
void Map::build_mesh(int id, Point<int> bot_left, Point<int> top_right)
{
   static const tuple<float, Colour> colour_map[] = {
      //          Start height         colour
      make_tuple(    7.0f,     make_rgb(238, 233, 233) ),
      make_tuple(    5.0f,     make_rgb(124, 113, 36) ),
      make_tuple(    3.0f,     make_rgb(129, 142, 57) ),
      make_tuple(    0.0f,     make_rgb(103, 142, 57) ),
      make_tuple(   -2.0f,     make_rgb(208, 207, 104)),
      make_tuple(   -1e10f,    make_rgb(177, 176, 96) )
   };

   IMeshBufferPtr buf = make_mesh_buffer();

   // Incrementing the frame counter here ensures that any track which spans
   // multiple sectors will be merged with each applicable mesh even when
   // the meshes are built on the same frame
   ++frame_num;

   static ITexturePtr noise = make_noise_texture(25, 512, 190, 15);
   buf->bind(noise);

   const float tmul = 1.0f / float(top_right.x - bot_left.x + 1);
   
   for (int x = top_right.x-1; x >= bot_left.x; x--) {
      for (int y = bot_left.y; y < top_right.y; y++) {
         int indexes[4];
         tile_vertices(x, y, indexes);
         
         const int order[6] = {
            indexes[1], indexes[2], indexes[3],
            indexes[3], indexes[0], indexes[1]
         };

         const IMeshBuffer::TexCoord tex_coords[4] = {
            make_point(x * tmul, (y + 1) * tmul),
            make_point((x + 1) * tmul, (y + 1) * tmul),
            make_point((x + 1) * tmul, y * tmul),
            make_point(x * tmul, y * tmul)
         };

         const IMeshBuffer::TexCoord tex_order[6] = {
            tex_coords[1], tex_coords[2], tex_coords[3],
            tex_coords[3], tex_coords[0], tex_coords[1]
         };
         
         for (int i = 0; i < 6; i++) {
            const HeightMap& v = height_map[order[i]];
            
            const float h = v.pos.y;
            tuple<float, Colour> hcol;
            int j = 0;
            do {
               hcol = colour_map[j++];
            } while (get<0>(hcol) > h);

            buf->add(v.pos, v.normal, get<1>(hcol), tex_order[i]);
         }
      }			
   }

   // Merge any static scenery
   for (int x = top_right.x-1; x >= bot_left.x; x--) {
      for (int y = bot_left.y; y < top_right.y; y++) {
         Tile& tile = tile_at(x, y);
         
         if (tile.scenery && tile.scenery->needs_rendering(frame_num)) {
            tile.scenery->get()->merge(buf);
            tile.scenery->rendered_on(frame_num);
         }

         // Draw the track, if any
         if (tile.track && tile.track->needs_rendering(frame_num)) {
            tile.track->get()->merge(buf);
            tile.track->rendered_on(frame_num);
         }
      }
   }

   // Draw the sides of the map if this is an edge sector
   const float x1 = static_cast<float>(bot_left.x) - 0.5f;
   const float x2 = static_cast<float>(top_right.x) - 0.5f;
   const float y1 = static_cast<float>(bot_left.y) - 0.5f;
   const float y2 = static_cast<float>(top_right.y) - 0.5f;

   const Colour brown = make_rgb(104, 57, 12);
   const float depth = -3.0f;

   buf->bind(ITexturePtr());   // No texture on sides
   
   int index[4];
   
   if (bot_left.x == 0) {
      for (int y = bot_left.y; y < top_right.y; y++) {
         const float yf = static_cast<float>(y) - 0.5f;

         tile_vertices(0, y, index);
         
         const float h1 = height_at(index[3]).pos.y;
         const float h2 = height_at(index[0]).pos.y;
         
         buf->add_quad(make_vector(x1, h1, yf),
            make_vector(x1, depth, yf),
            make_vector(x1, depth, yf + 1.0f),
            make_vector(x1, h2, yf + 1.0f),
            brown);
      }
   }

   if (top_right.x == my_width) {
      for (int y = bot_left.y; y < top_right.y; y++) {
         const float yf = static_cast<float>(y) - 0.5f;

         tile_vertices(my_width - 1, y, index);

         const float h1 = height_at(index[2]).pos.y;
         const float h2 = height_at(index[1]).pos.y;
         
         buf->add_quad(make_vector(x2, depth, yf),
            make_vector(x2, h1, yf),
            make_vector(x2, h2, yf + 1.0f),
            make_vector(x2, depth, yf + 1.0f),
            brown);
      }
   }

   if (bot_left.y == 0) {
      for (int x = bot_left.x; x < top_right.x; x++) {
         const float xf = static_cast<float>(x) - 0.5f;
         
         tile_vertices(x, 0, index);
       
         const float h1 = height_at(index[3]).pos.y;
         const float h2 = height_at(index[2]).pos.y;
         
         buf->add_quad(make_vector(xf, depth, y1),
            make_vector(xf, h1, y1),
            make_vector(xf + 1.0f, h2, y1),
            make_vector(xf + 1.0f, depth, y1),
            brown);
      }
   }
   
   if (top_right.y == my_depth) {
      for (int x = bot_left.x; x < top_right.x; x++) {
         const float xf = static_cast<float>(x) - 0.5f;
         
         tile_vertices(x, my_depth - 1, index);
       
         const float h1 = height_at(index[0]).pos.y;
         const float h2 = height_at(index[1]).pos.y;
      
         buf->add_quad(make_vector(xf, h1, y2),
            make_vector(xf, depth, y2),
            make_vector(xf + 1.0f, depth, y2),
            make_vector(xf + 1.0f, h2, y2),
            brown);
      }
   }

   terrain_meshes[id] = make_mesh(buf);

   // Check if this sector needs a sea quad drawn
   bool below_sea_level = false;
   for (int x = top_right.x-1; x >= bot_left.x; x--) {
      for (int y = bot_left.y; y < top_right.y; y++) {
         int index[4];
         tile_vertices(x, y, index);
   
         below_sea_level |=
            height_at(index[0]).pos.y < 0.0f
            || height_at(index[1]).pos.y < 0.0f
            || height_at(index[2]).pos.y < 0.0f
            || height_at(index[3]).pos.y < 0.0f;

         if (below_sea_level)
            goto below_sea_levelOut;
      }
   }

 below_sea_levelOut:

   size_t min_size = id + 1;
   if (sea_sectors.size() < min_size)
      sea_sectors.resize(min_size);
   sea_sectors.at(id) = below_sea_level;
   
   // Make sure we don't rebuild this mesh if any of the tiles are dirty
   have_mesh(id, bot_left, top_right);
}

// A special rendering mode when selecting tiles
void Map::render_pick_sector(Point<int> bot_left, Point<int> top_right)
{
   glColor3f(1.0f, 1.0f, 1.0f);
   
   for (int x = top_right.x-1; x >= bot_left.x; x--) {
      for (int y = bot_left.y; y < top_right.y; y++) {
         // Name this tile
         glPushName(tile_name(x, y));

         int indexes[4];
         tile_vertices(x, y, indexes);

         glBegin(GL_QUADS);
         for (int i = 0; i < 4; i++) {
            const HeightMap& v = height_map[indexes[i]];
            glNormal3f(v.normal.x, v.normal.y, v.normal.z);
            glVertex3f(v.pos.x, v.pos.y, v.pos.z);
         }
         glEnd();
         
         glPopName();
      }			
   }
}

// Render a small part of the map as directed by the quad tree
void Map::render_sector(IGraphicsPtr a_context, int id,
                        Point<int> bot_left, Point<int> top_right)
{
   if (in_pick_mode) {
      render_pick_sector(bot_left, top_right);
      return;
   }
   
   if (!have_mesh(id, bot_left, top_right))
      build_mesh(id, bot_left, top_right);

   {
      // Parts of track may extend outside the sector so these
      // are clipped off
      
      const float x = bot_left.x - 0.5f;
      const float w = quad_tree->leaf_size();
      const float z = bot_left.y - 0.5f;
      const float d = quad_tree->leaf_size();
      ClipVolume clip(x, w, z, d);
      
      terrain_meshes[id]->render();
   }

   // Draw the overlays
   for (int x = top_right.x-1; x >= bot_left.x; x--) {
      for (int y = bot_left.y; y < top_right.y; y++) {
         //for (int i = 0; i < 4; i++) {
         //   const Vertex& v = height_map[indexes[i]];
         //   draw_normal(v.pos, v.normal);
         //}
         
         if (should_draw_grid_lines) {
            // Render grid lines
            glColor3f(0.0f, 0.0f, 0.0f);
            glBegin(GL_LINE_LOOP);

            int indexes[4];
            tile_vertices(x, y, indexes);
            for (int i = 0; i < 4; i++) {
               const HeightMap& v = height_map[indexes[i]];
               glVertex3f(v.pos.x, v.pos.y, v.pos.z);
            }
            
            glEnd();
         }

         Tile& tile = tile_at(x, y);
         
         if (tile.track && tile.track->needs_rendering(frame_num)) {
#if 0
            // Draw the endpoints for debugging
            vector<Point<int> > tiles;
            tile.track->get()->get_endpoints(tiles);
            for_each(tiles.begin(), tiles.end(),
                    bind(&Map::highlight_tile, this, placeholders::_1,
                          make_colour(0.9f, 0.1f, 0.1f)));

            tiles.clear();
            tile.track->get()->get_covers(tiles);
            for_each(tiles.begin(), tiles.end(),
                    bind(&Map::highlight_tile, this, placeholders::_1,
                          make_colour(0.4f, 0.7f, 0.1f)));
#endif

#if 0
            // Draw vertices covered by track
            vector<Point<int> > vertices;
            tile.track->get()->get_height_locked(vertices);
            for_each(vertices.begin(), vertices.end(),
                     bind(&Map::highlight_vertex, this, placeholders::_1,
                          make_colour(1.0f, 0.0f, 0.0f)));
#endif
            
            // Draw track highlights
            tile.track->get()->render();
            
            tile.track->rendered_on(frame_num);
         }

#if 0
         // Highlight tiles covered by scenery
         if (tile.scenery)
            highlight_tile(make_point(x, y), colour::WHITE);
#endif

         // Draw the station, if any
         if (tile.station
             && (should_draw_grid_lines || tile.station->highlight_visible()))
            highlight_tile(make_point(x, y), tile.station->highlight_colour());

         // Draw the start location if it's on this tile
         if (start_location.x == x && start_location.y == y
            && should_draw_grid_lines)
            draw_start_location();
      }			
   }
}

// Render the semi-transparent overlays such as water
void Map::post_render_sector(IGraphicsPtr a_context, int id,
   Point<int> bot_left, Point<int> top_right)
{
   // Draw the water
   if (!in_pick_mode && sea_sectors.at(id)) {
      glPushAttrib(GL_ENABLE_BIT);

      glEnable(GL_BLEND);
      glDisable(GL_TEXTURE_2D);

      const float blX = static_cast<float>(bot_left.x);
      const float blY = static_cast<float>(bot_left.y);
      const float trX = static_cast<float>(top_right.x);
      const float trY = static_cast<float>(top_right.y);
      
      static const float sea_level = -0.6f;
      gl::colour(make_rgb(0, 80, 160, 150));
      glNormal3f(0.0f, 1.0f, 0.0f);
      glBegin(GL_QUADS);
      glVertex3f(blX - 0.5f, sea_level, blY - 0.5f);
      glVertex3f(blX - 0.5f, sea_level, trY - 0.5f);
      glVertex3f(trX - 0.5f, sea_level, trY - 0.5f);
      glVertex3f(trX - 0.5f, sea_level, blY - 0.5f);
      glEnd();

      glPopAttrib();
   }
}

// Called when we've changed the height of part of a tile
// This readjusts all the normals and those of its neighbours
// to point in the right direction
void Map::fix_normals(int x, int y)
{
   if ((x < 0) || (y < 0) || (x >= my_width) || (y >= my_depth))
      return;

   int indexes[4];
   tile_vertices(x, y, indexes);

   for (int n = 0; n < 4; n++) {
      const int i = indexes[n];
      HeightMap& v = height_map[i];

      Vector<float> west, north, east, south;
      bool have_west = true, have_north = true,
         have_east = true, have_south = true;
      
      if (i > 0 && i % (my_width + 1) > 0)
         west = height_at(i-1).pos;
      else
         have_west = false;
         
      if (i < (my_width + 1) * my_depth - 1)
         north = height_at(i + (my_width + 1)).pos;
      else
         have_north = false;

      if (i < (my_width + 1) * (my_depth + 1) - 1
         && i % (my_width + 1) < my_width)
         east = height_at(i + 1).pos;
      else
         have_east = false;
      
      if (i > (my_width + 1))
         south = height_at(i - (my_width + 1)).pos;
      else
         have_south = false;

      float count = 4.0f;
      Vector<float> avg = make_vector(0.0f, 0.0f, 0.0f);

      if (have_west && have_north)
         avg += surface_normal(north, v.pos, west);
      else
         count -= 1.0f;

      if (have_east && have_north)
         avg += surface_normal(east, v.pos, north);
      else
         count -= 1.0f;

      if (have_south && have_east)
         avg += surface_normal(south, v.pos, east);
      else
         count -= 1.0f;

      if (have_west && have_south)      
         avg += surface_normal(west, v.pos, south);
      else
         count -= 1.0f;

      v.normal = avg / count;
   }
}

// Find the terrain vertices that border a tile
void Map::tile_vertices(int x, int y, int* indexes) const
{
   assert(x >= 0 && x < my_width && y >= 0 && y < my_depth);
          
   indexes[3] = x + (y * (my_width+1));         // (X, Y)
   indexes[2] = (x+1) + (y * (my_width+1));     // (X+1, Y)
   indexes[1] = (x+1) + ((y+1) * (my_width+1)); // (X+1, Y+1)
   indexes[0] = x + ((y+1) * (my_width+1));     // (X, Y+1)
}

// True if changing the height of this tile will affect
// a piece of track
bool Map::raise_will_cover_track(int x, int y) const
{
#if 0
   return tile_at(x, y).track
      || (x < my_width - 1 && tile_at(x + 1, y).track)
      || (x > 0 && tile_at(x - 1, y).track)
      || (y < my_depth - 1 && tile_at(x, y + 1).track)
      || (y > 0 && tile_at(x, y - 1).track)
      || (x < my_width - 1 && y < my_depth - 1 && tile_at(x + 1, y + 1).track)
      || (x > 0 && y < my_depth - 1 && tile_at(x - 1, y + 1).track)
      || (x > 0 && y > 0 && tile_at(x - 1, y - 1).track)
      || (x < my_width - 1 && y > 0 && tile_at(x + 1, y - 1).track);
#else
   int indexes[4];
   tile_vertices(x, y, indexes);

   bool ok = true;
   for (int i = 0; i < 4; i++)
      ok &= height_map[indexes[i]].lock_count == 0;
   
   return !ok;
#endif
}

// Changes the height of a complete tile
void Map::raise_tile(int x, int y, float delta_height)
{
   if (raise_will_cover_track(x, y)) {
      warn() << "Cannot raise terrain over track";
      return;
   }
   
   int indexes[4];
   tile_vertices(x, y, indexes);

   for (int i = 0; i < 4; i++)
      height_map[indexes[i]].pos.y += delta_height;

   fix_normals(x, y);
   dirty_tile(x, y);
}

void Map::lock_height_at(Point<int> p)
{
   assert(p.x <= my_width);
   assert(p.y <= my_depth);

   height_map[p.x + (p.y * (my_width+1))].lock_count++;
}

void Map::unlock_height_at(Point<int> p)
{
   assert(p.x <= my_width);
   assert(p.y <= my_depth);

   HeightMap& h = height_map[p.x + (p.y * (my_width+1))];

   assert(h.lock_count > 0);
   h.lock_count--;
}

// Sets the absolute height of a tile
void Map::set_tile_height(int x, int y, float h)
{
   bool track_affected = raise_will_cover_track(x, y);
   
   int indexes[4];
   tile_vertices(x, y, indexes);

   for (int i = 0; i < 4; i++) {
      if (track_affected
         && abs(height_map[indexes[i]].pos.y - h) > 0.01f) {
         warn() << "Cannot level terrain under track";
         return;
      }        
      else
         height_map[indexes[i]].pos.y = h;
   }
   
   fix_normals(x, y);
   dirty_tile(x, y);
}

float Map::height_at(float x, float y) const
{
   const int x_floor = static_cast<int>(floorf(x));
   const int y_floor = static_cast<int>(floorf(y));

   return height_at(make_point(x_floor, y_floor));
}

float Map::height_at(Point<int> where) const
{
   if (where.x < 0 || where.y < 0
      || where.x >= my_width || where.y >= my_depth)
      return 0.0f;

   int indexes[4];
   tile_vertices(where.x, where.y, indexes);

   float avg = 0.0f;
   for (int i = 0; i < 4; i++)
      avg += height_map[indexes[i]].pos.y;
   
   return avg / 4.0f;
}

Vector<float> Map::slope_at(Point<int> where,
   track::Direction axis, bool &level) const
{
   int indexes[4];
   tile_vertices(where.x, where.y, indexes);

   // This is slightly consfusing since the track Y axis
   // is the Z axis in the height map

   Vector<float> v1, v2;

   if (axis == axis::X) {
      v1 = height_map[indexes[2]].pos - height_map[indexes[3]].pos;
      v2 = height_map[indexes[1]].pos - height_map[indexes[0]].pos;
   }
   else {
      v1 = height_map[indexes[0]].pos - height_map[indexes[3]].pos;
      v2 = height_map[indexes[1]].pos - height_map[indexes[2]].pos;
   }

   level = (v1 == v2);

#if 0
   debug() << "slope_at where=" << where
           << " axis=" << axis
           << " v1=" << v1 << " v2=" << v2
           << " level=" << level;
#endif
   
   return v1;
}

Vector<float> Map::slope_before(Point<int> where,
   track::Direction axis, bool &valid) const
{
   Point<int> before;

   if (axis == axis::X)
      before = where + make_point(-1, 0);
   else
      before = where + make_point(0, -1);
            
   const bool off_edge =
      (axis == axis::X && before.x < 0)
      || (axis == axis::Y && before.y < 0);

   valid = !off_edge;
         
   if (off_edge)
      return make_vector(0.0f, 0.0f, 0.0f);
   else {
      bool ignored;
      return slope_at(before, axis, ignored);
   }
}

Vector<float> Map::slope_after(Point<int> where,
   track::Direction axis, bool &valid) const
{
   Point<int> after;

   if (axis == axis::X)
      after = where + make_point(1, 0);
   else
      after = where + make_point(0, 1);
            
   const bool off_edge =
      (axis == axis::X && after.x >= width())
      || (axis == axis::Y && after.y >= depth());

   valid = !off_edge;
   
   if (off_edge)
      return make_vector(0.0f, 0.0f, 0.0f);
   else {
      bool ignored;
      return slope_at(after, axis, ignored);
   }
}

void Map::change_area_height(const Point<int>& a_start_pos,
   const Point<int>& a_finish_pos,
   float a_height_delta)
{
   const int xmin = min(a_start_pos.x, a_finish_pos.x);
   const int xmax = max(a_start_pos.x, a_finish_pos.x);

   const int ymin = min(a_start_pos.y, a_finish_pos.y);
   const int ymax = max(a_start_pos.y, a_finish_pos.y);
   
   for (int x = xmin; x <= xmax; x++) {
      for (int y = ymin; y <= ymax; y++)
         raise_tile(x, y, a_height_delta);
   }
}

void Map::level_area(Point<int> a_start_pos, Point<int> a_finish_pos)
{
   const int xmin = min(a_start_pos.x, a_finish_pos.x);
   const int xmax = max(a_start_pos.x, a_finish_pos.x);

   const int ymin = min(a_start_pos.y, a_finish_pos.y);
   const int ymax = max(a_start_pos.y, a_finish_pos.y);

   int indexes[4];
   tile_vertices(a_start_pos.x, a_start_pos.y, indexes);

   float avg_height = 0.0f;
   for (int i = 0; i < 4; i++)
      avg_height += height_map[indexes[i]].pos.y;
   avg_height /= 4.0f;
   
   for (int x = xmin; x <= xmax; x++) {
      for (int y = ymin; y <= ymax; y++)
         set_tile_height(x, y, avg_height);
   }
}

void Map::smooth_area(Point<int> start, Point<int> finish)
{
   const int xmin = min(start.x, finish.x);
   const int xmax = max(start.x, finish.x);

   const int ymin = min(start.y, finish.y);
   const int ymax = max(start.y, finish.y);

   Point<int> step;
   if (xmin == xmax)
      step = make_point(0, 1);
   else if (ymin == ymax)
      step = make_point(1, 0);
   else {
      warn() << "Can only smooth strips";
      return;
   }

   const Point<int> abs_start = make_point(xmin, ymin);
   const Point<int> abs_finish = make_point(xmax, ymax);

   const float height_start = height_at(abs_start);
   const float height_finish = height_at(abs_finish);

   set_tile_height(abs_start.x, abs_start.y, height_start);
   set_tile_height(abs_finish.x, abs_finish.y, height_finish);

   const float drop =
      (height_start - height_finish) / (xmax + ymax - xmin - ymin);
   debug() << "drop=" << drop;

   int i = 0;
   for (Point<int> it = abs_start; it != abs_finish; i++, it += step) {
      const bool track_affected = raise_will_cover_track(it.x, it.y);
      
      int indexes[4];
      tile_vertices(it.x, it.y, indexes);

      int targets[2];
      if (xmin == xmax) {
         targets[0] = 0;
         targets[1] = 1;
      }
      else {
         targets[0] = 1;
         targets[1] = 2;
      }
      
      for (int j = 0; j < 2; j++) {
         const float new_height = height_start - (i * drop);
         
         if (track_affected
            && abs(height_map[indexes[targets[j]]].pos.y - new_height) > 0.01f) {
            warn() << "Cannot change terrain under track";
            return;
         }        
         else
            height_map[indexes[targets[j]]].pos.y = new_height;
      }
      
      fix_normals(it.x, it.y);
      dirty_tile(it.x, it.y);
   }
      
}

void Map::raise_area(const Point<int>& a_start_pos,
   const Point<int>& a_finish_pos)
{
   change_area_height(a_start_pos, a_finish_pos, 0.1f);
}

void Map::lower_area(const Point<int>& a_start_pos,
   const Point<int>& a_finish_pos)
{
   change_area_height(a_start_pos, a_finish_pos, -0.1f);
}

void Map::add_scenery(Point<int> where, ISceneryPtr s)
{
   if (tile_at(where.x, where.y).track)
      warn() << "Cannot place scenery on track";
   else {
      SceneryAnchor indirect(new Anchor<IScenery>(s, where));

      const Point<int> size = s->size();
      
      for (int x = 0; x < size.x; x++) {
         for (int y = 0; y < size.y; y++) {
            tile_at(where.x + x, where.y + y).scenery = indirect;
            dirty_tile(where.x, where.y);
         }
      }
      
      s->set_position(static_cast<float>(where.x),
         height_at(where),
         static_cast<float>(where.y));
   }
}

// Either extend an existing station which borders this area
// or build a new station
IStationPtr Map::extend_station(Point<int> a_start_pos, Point<int> a_finish_pos)
{
   const int xmin = min(a_start_pos.x, a_finish_pos.x);
   const int xmax = max(a_start_pos.x, a_finish_pos.x);

   const int ymin = min(a_start_pos.y, a_finish_pos.y);
   const int ymax = max(a_start_pos.y, a_finish_pos.y);

   // Find all the tiles containing track in this region
   typedef list<Point<int> > PointList;
   PointList track_in_area;
   for (int x = xmin; x <= xmax; x++) {
      for (int y = ymin; y <= ymax; y++) {
         if (tile_at(x, y).track)
            track_in_area.push_back(make_point(x, y));
      }
   }

   if (track_in_area.empty()) {
      warn() << "Stations must be placed on track";
      return IStationPtr();
   }

   IStationPtr station;

   // See if any of these track segments are adjacent to a station
   for (PointList::const_iterator it = track_in_area.begin();
        it != track_in_area.end(); ++it) {
      
      const Point<int> near[] = {
         make_point(0, 0),
         make_point(1, 0),
         make_point(0, 1),
         make_point(-1, 0),
         make_point(0, -1)
      };
      
      for (int i = 0; i < 5; i++) {
         Point<int> neighbour = *it + near[i];
         if (neighbour.x >= 0 && neighbour.x < my_width
            && neighbour.y >= 0 && neighbour.y < my_depth
            && tile_at(neighbour.x, neighbour.y).station) {

            IStationPtr candidate = tile_at(neighbour.x, neighbour.y).station;
            
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

      station = make_station();
   }
   
   for (PointList::iterator it = track_in_area.begin();
        it != track_in_area.end(); ++it)
      tile_at((*it).x, (*it).y).station = station;
   
   return station;
}

// Write the terrain height map into a binary file
// Binary file format is very simple:
//   Bytes 0-3   Width of map
//   Bytes 4-7   Depth of map
//   Bytes 8+    Raw height data
void Map::write_height_map() const
{
   using namespace boost;

   IResource::Handle h = resource->write_file(resource->name() + ".bin");

   log() << "Writing terrain height map to " << h.file_name();

   try {
      ofstream& of = h.wstream();
      
      const int32_t wl = static_cast<int32_t>(my_width);
      const int32_t dl = static_cast<int32_t>(my_depth);
      of.write(reinterpret_cast<const char*>(&wl), sizeof(int32_t));
      of.write(reinterpret_cast<const char*>(&dl), sizeof(int32_t));
      
      for (int i = 0; i < (my_width + 1) * (my_depth + 1); i++)
         of.write(reinterpret_cast<const char*>(&height_map[i].pos.y),
            sizeof(float));
   }
   catch (std::exception& e) {
      h.rollback();
      throw e;
   }
}

// Read the height data back out of a binary file
void Map::read_height_map(IResource::Handle a_handle)
{
   using namespace boost;

   log() << "Reading height map from " << a_handle.file_name();

   istream& is = a_handle.rstream();
   
   // Check the dimensions of the binary file match the XML file
   int32_t wl, dl;
   is.read(reinterpret_cast<char*>(&wl), sizeof(int32_t));
   is.read(reinterpret_cast<char*>(&dl), sizeof(int32_t));

   if (wl != my_width || dl != my_depth) {
      error() << "Expected width " << my_width << " got " << wl;
      error() << "Expected height " << my_depth << " got " << dl;
      throw runtime_error
         ("Binary file " + a_handle.file_name() + " dimensions are incorrect");
   }

   for (int i = 0; i < (my_width + 1) * (my_depth + 1); i++) {
      is.read(reinterpret_cast<char*>(&height_map[i].pos.y),
              sizeof(float));
      height_map[i].lock_count = 0;
   }

   for (int x = 0; x < my_width; x++) {
      for (int y = 0; y < my_depth; y++)
         fix_normals(x, y);
   }
}

void Map::save_to(ostream& of)
{
   xml::element root("map");
   root.add_attribute("width", my_width);
   root.add_attribute("height", my_depth);

   root.add_child(xml::element("name").add_text("No Name"));
   
   root.add_child
      (xml::element("start")
         .add_attribute("x", start_location.x)
         .add_attribute("y", start_location.y)
         .add_attribute("dirX", start_direction.x)
         .add_attribute("dirY", start_direction.z));

   // Write out all the stations
   set<IStationPtr> seen_stations;
   
   for (int x = 0; x < my_width; x++) {
      for (int y = 0; y < my_depth; y++) {
         IStationPtr s = tile_at(x, y).station;

         if (s && seen_stations.find(s) == seen_stations.end()) {
            // Not seen this station before
            root.add_child
               (xml::element("station")
                  .add_attribute("id", s->id())
                  .add_child(xml::element("name").add_text(s->name())));

            seen_stations.insert(s);
         }
      }
   }
   
   // Generate the height map
   write_height_map();

   root.add_child
      (xml::element("heightmap")
         .add_text(resource->name() + ".bin"));

   xml::element tileset("tileset");

   // We abuse the frame number to ensure all scenery, etc. is
   // only written out once
   ++frame_num;

   for (int x = 0; x < my_width; x++) {
      for (int y = 0; y < my_depth; y++) {
         const Tile& tile = tile_at(x, y);

         bool useful = false;
         xml::element tile_xml("tile");

         tile_xml.add_attribute("x", x);
         tile_xml.add_attribute("y", y);

         if (tile.track
             && tile.track->origin() == make_point(x, y)) {

            tile_xml.add_child(tile.track->get()->to_xml());
            useful = true;
         }

         if (tile.station) {
            tile_xml.add_child
               (xml::element("station-part")
                  .add_attribute("id", tile.station->id()));
            useful = true;
         }

         if (tile.scenery && tile.scenery->needs_rendering(frame_num)) {
            tile_xml.add_child(tile.scenery->get()->to_xml());
            tile.scenery->rendered_on(frame_num);
            useful = true;
         }

         if (useful)
            tileset.add_child(tile_xml);
      }
   }

   root.add_child(tileset);
   
   of << xml::document(root);
}

// Turn the map into XML
void Map::save()
{
   using namespace boost::filesystem;

   IResource::Handle h = resource->write_file(resource->name() + ".xml");
   
   log() << "Saving map to " << h.file_name();

   ofstream& of = h.wstream();

   try {
      save_to(of);
   }
   catch (exception& e) {
      h.rollback();
      throw e;
   }
}

IMapPtr make_empty_map(const string& a_res_id, int a_width, int a_depth)
{
   IResourcePtr res = make_new_resource(a_res_id, "maps");
   
   shared_ptr<Map> ptr(new Map(res));
   ptr->reset_map(a_width, a_depth);
   ptr->save();
   return IMapPtr(ptr);
}

// Build a map through XML callbacks
class MapLoader : public IXMLCallback {
public:
   MapLoader(shared_ptr<Map> a_map, IResourcePtr a_res)
      : my_map(a_map), tile(make_point(0, 0)),
        resource(a_res) {}

   void start_element(const string& local_name, const AttributeSet& attrs);
   void end_element(const string& local_name);
   void text(const string& local_name, const string& a_string);
   
private:
   void handle_map(const AttributeSet& attrs);
   void handle_building(const AttributeSet& attrs);
   void handle_tree(const AttributeSet& attrs);
   void handle_station(const AttributeSet& attrs);
   void handle_start(const AttributeSet& attrs);
   void handle_tile(const AttributeSet& attrs);
   void handle_station_part(const AttributeSet& attrs);
   void handle_straight_track(const AttributeSet& attrs);
   void handle_slope_track(const AttributeSet& attrs);
   void handle_points(const AttributeSet& attrs);
   void handle_crossover_track(const AttributeSet& attrs);
   void handle_spline_track(const AttributeSet& attrs);
   
   shared_ptr<Map> my_map;
   map<int, IStationPtr> my_stations;
   IStationPtr my_active_station;
   Point<int> tile;

   IResourcePtr resource;
};

void MapLoader::start_element(const string& local_name,
                              const AttributeSet& attrs)
{
   if (local_name == "map")
      handle_map(attrs);
   else if (local_name == "tile")
      handle_tile(attrs);
   else if (local_name == "start")
      handle_start(attrs);
   else if (local_name == "straight-track")
      handle_straight_track(attrs);
   else if (local_name == "crossover-track")
      handle_crossover_track(attrs);
   else if (local_name == "gen-track"
            || local_name == "spline-track")
      handle_spline_track(attrs);
   else if (local_name == "points")
      handle_points(attrs);
   else if (local_name == "slope-track")
      handle_slope_track(attrs);
   else if (local_name == "station-part")
      handle_station_part(attrs);
   else if (local_name == "station")
      handle_station(attrs);
   else if (local_name == "building")
      handle_building(attrs);
   else if (local_name == "tree")
      handle_tree(attrs);
}

void MapLoader::end_element(const string& local_name)
{
   if (local_name == "station")
      my_active_station.reset();
}                   
   
void MapLoader::text(const string& local_name, const string& a_string)
{
   if (local_name == "heightmap")
      my_map->read_height_map(resource->open_file(a_string));
   else if (my_active_station) {
      if (local_name == "name")
         my_active_station->set_name(a_string);
   }
}

void MapLoader::handle_map(const AttributeSet& attrs)
{
   int width, height;
   attrs.get("width", width);
   attrs.get("height", height);

   my_map->reset_map(width, height);
}

void MapLoader::handle_building(const AttributeSet& attrs)
{
   my_map->add_scenery(tile, load_building(attrs));
}

void MapLoader::handle_tree(const AttributeSet& attrs)
{
   my_map->add_scenery(tile, load_tree(attrs));
}
   
void MapLoader::handle_station(const AttributeSet& attrs)
{
   my_active_station = make_station();

   int id;
   attrs.get("id", id);
   my_active_station->set_id(id);

   my_stations[id] = my_active_station;
}
   
void MapLoader::handle_start(const AttributeSet& attrs)
{
   int x, y, dirX, dirY;
   attrs.get("x", x);
   attrs.get("y", y);
   attrs.get("dirX", dirX);
   attrs.get("dirY", dirY);
      
   my_map->set_start(x, y, dirX, dirY);
}

void MapLoader::handle_tile(const AttributeSet& attrs)
{
   attrs.get("x", tile.x);
   attrs.get("y", tile.y);
}

void MapLoader::handle_station_part(const AttributeSet& attrs)
{
   int id;
   attrs.get("id", id);

   map<int, IStationPtr>::iterator it = my_stations.find(id);
   if (it == my_stations.end())
      throw runtime_error("No station definition for ID "
                          + boost::lexical_cast<string>(id));
   else
      my_map->set_station_at(tile, (*it).second);
}

void MapLoader::handle_straight_track(const AttributeSet& attrs)
{
   string align;
   attrs.get("align", align);

   track::Direction axis = align == "x" ? axis::X : axis::Y;

   my_map->set_track_at(tile, make_straight_track(axis));
}

void MapLoader::handle_slope_track(const AttributeSet& attrs)
{
   string align;
   attrs.get("align", align);
      
   track::Direction axis = align == "x" ? axis::X : axis::Y;

   bool level;
   Vector<float> slope = my_map->slope_at(tile, axis, level);
         
   bool a_valid, b_valid;
   Vector<float> slope_before = my_map->slope_before(tile, axis, b_valid);
   Vector<float> slope_after = my_map->slope_after(tile, axis, a_valid);

   if (!a_valid || !b_valid || !level)
      throw runtime_error("SlopeTrack in invalid location");

   my_map->set_track_at(tile,
                        make_slope_track(axis, slope, slope_before, slope_after));
}

void MapLoader::handle_points(const AttributeSet& attrs)
{
   string align;
   attrs.get("align", align);
   
   bool reflect;
   attrs.get("reflect", reflect);

   track::Direction dir =
      align == "x" ? axis::X
      : (align == "-x" ? -axis::X
         : (align == "y" ? axis::Y : -axis::Y));
   
   my_map->set_track_at(tile, make_points(dir, reflect));
}

void MapLoader::handle_crossover_track(const AttributeSet& attrs)
{
   my_map->set_track_at(tile, make_crossover_track());
}

void MapLoader::handle_spline_track(const AttributeSet& attrs)
{
   Vector<int> delta;
   track::Direction entry_dir, exit_dir;

   attrs.get("delta-x", delta.x);
   attrs.get("delta-y", delta.y);

   attrs.get("entry-dir-x", entry_dir.x);
   attrs.get("entry-dir-y", entry_dir.z);
   
   attrs.get("exit-dir-x", exit_dir.x);
   attrs.get("exit-dir-y", exit_dir.z);

   my_map->set_track_at(tile, make_spline_track(delta, entry_dir, exit_dir));
}

IMapPtr load_map(const string& a_res_id)
{
   IResourcePtr res = find_resource(a_res_id, "maps");
   
   shared_ptr<Map> map(new Map(res));

   log() << "Loading map from file " << res->xml_file_name();

   static IXMLParserPtr xml_parser = make_xml_parser("schemas/map.xsd");

   MapLoader loader(map, res);
   xml_parser->parse(res->xml_file_name(), loader);
      
   return IMapPtr(map);
}
