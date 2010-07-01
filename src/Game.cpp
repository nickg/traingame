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

#include "IScreen.hpp"
#include "IGraphics.hpp"
#include "IMap.hpp"
#include "IRollingStock.hpp"
#include "ITrain.hpp"
#include "ILogger.hpp"
#include "ILight.hpp"
#include "GameScreens.hpp"
#include "IBillboard.hpp"
#include "IterateTrack.hpp"
#include "IConfig.hpp"
#include "IMessageArea.hpp"
#include "IRenderStats.hpp"

#include "gui/ILayout.hpp"
#include "gui/Label.hpp"
#include "gui/ThrottleMeter.hpp"
#include "gui/IFont.hpp"

// Implementation of the main play screen
class Game : public IScreen {
public:
   Game(IMapPtr a_map);
   ~Game();
   
   void display(IGraphicsPtr a_context) const;
   void overlay() const;
   void update(IPickBufferPtr a_pick_buffer, int a_delta);
   void on_key_down(SDLKey a_key);
   void on_key_up(SDLKey a_key);
   void on_mouse_move(IPickBufferPtr a_pick_buffer, int x, int y, int xrel,
      int yrel);
   void on_mouse_click(IPickBufferPtr a_pick_buffer, int x, int y,
      MouseButton a_button);
   void on_mouse_release(IPickBufferPtr a_pick_buffer, int x, int y,
      MouseButton a_button) {}
private:
   void look_ahead();
   void near_station(IStationPtr s);
   void left_station();
   Vector<float> camera_position(float a_radius) const;
   void switch_to_birdCamera();
   void stopped_at_station();

   enum TrackStateReq { NEXT, PREV };
   void alter_track_state(TrackStateReq req);
    
   IMapPtr map;
   ITrainPtr train;
   ILightPtr sun;

   // Station the train is either approaching or stopped at
   IStationPtr active_station;

   // Camera position
   float horiz_angle, vert_angle, view_radius;

   // Camera adjustment
   float cameraHTarget, cameraVTarget;
   float camera_speed;

   enum CameraMode { CAMERA_FLOATING, CAMERA_FIXED, CAMERA_BIRD };
   CameraMode camera_mode;
   
   gui::ILayoutPtr layout;
   IMessageAreaPtr message_area;
   IRenderStatsPtr render_stats;
};

Game::Game(IMapPtr a_map)
   : map(a_map),
     horiz_angle(M_PI/4.0f),
     vert_angle(M_PI/4.0f),
     view_radius(20.0f)
{
   train = make_train(map);
   sun = make_sun_light();

   map->set_grid(false);

   // Build the GUI
   layout = gui::make_layout("layouts/game.xml");
   message_area = make_message_area();
   render_stats = make_render_stats(layout, "/fps/fps_label");

   switch_to_birdCamera();
}

Game::~Game()
{
   
}

Vector<float> Game::camera_position(float a_radius) const
{
   // Two angles give unique position on surface of a sphere
   // Look up ``spherical coordinates''
   const float y_centre = 0.9f;
   Vector<float> position = train->front();
   position.x += a_radius * cosf(horiz_angle) * sinf(vert_angle);
   position.z += a_radius * sinf(horiz_angle) * sinf(vert_angle);
   position.y = a_radius * cosf(vert_angle) + y_centre;

   return position;
}

void Game::switch_to_birdCamera()
{
   camera_mode = CAMERA_BIRD;

   cameraHTarget = M_PI/4.0f;
   cameraVTarget = M_PI/4.0f;

   camera_speed = 100.0f;
}

void Game::display(IGraphicsPtr a_context) const
{
   Vector<float> train_pos = train->front();

   Vector<float> position = camera_position(view_radius);
   
   a_context->look_at(position, train_pos);
   set_billboard_cameraOrigin(position);
   
   sun->apply();
   
   map->render(a_context);
   train->render();

   render_billboards();
}

void Game::overlay() const
{
   layout->render();
   message_area->render();
}

void Game::stopped_at_station()
{
   //layout->get("/station").visible(true);
}

void Game::update(IPickBufferPtr a_pick_buffer, int a_delta)
{
   message_area->update(a_delta);
   render_stats->update(a_delta);
   
   train->update(a_delta);

   // Update the GUI elements
   layout->cast<gui::ThrottleMeter>("/throttle_meter").value(
      train->controller()->throttle());
   
   const double ms_toMPH = 2.237;
   layout->cast<gui::Label>("/speed_label").format(
      "Speed: %.1lfmph", abs(train->speed()) * ms_toMPH);

   IControllerPtr ctrl = train->controller();
   layout->get("/brake_label").visible(ctrl->brake_on());
   layout->get("/reverse_label").visible(ctrl->reverse_on());
   
   look_ahead();

   // Move the camera vertically if it's currently underground
#if 0
   // Calculate the location of the near clip plane
   const float near_clip = get_config()->get<float>("NearClip");
   Vector<float> clip_position = camera_position(view_radius - near_clip);

   // A hack because we don't calculate the height properly
   const float MIN_HEIGHT = 0.25f;
   float h = map->height_at(clip_position.x, clip_position.z);

   if (h + MIN_HEIGHT > clip_position.y) {    
      cameraVTarget -= 0.001f * static_cast<float>(a_delta);
      camera_speed = 200.0f;
   }
#endif
   
   // Bounce the camera if we need to
   vert_angle -= (vert_angle - cameraVTarget) / camera_speed;
   horiz_angle -= (horiz_angle - cameraHTarget) / camera_speed;
}

// Signal that we are approaching a station
void Game::near_station(IStationPtr s)
{
   left_station();  // Clear any previous station

   if (s != active_station) {
      active_station = s;
      s->set_highlight_visible(true);

      //gui::Widget& station_wnd = layout->get("/station");

      layout->cast<gui::Label>("/station/name").text(s->name());
   }
}

// Signal that we are no longer at or approaching a station
void Game::left_station()
{
   if (active_station) {
      active_station->set_highlight_visible(false);
      active_station.reset();

      //layout->get("/station").visible(false);
   }
}

// Look along the track and notify the player of any stations, points, etc.
// that they are approaching
void Game::look_ahead()
{
   TrackIterator it = iterate_track(map, train->tile(),
      train->direction());

   // Are we sitting on a station?
   if (it.status == TRACK_STATION) {
      near_station(it.station);

      if (train->controller()->stopped())
         stopped_at_station();
      else
         message_area->post("Stop here for station " + it.station->name());
      
      return;
   }

   const int max_look = 10;
   for (int i = 0; i < max_look; i++) {
      it = it.next();

      if (it.status != TRACK_OK) {
         bool clear_station = true;
         
         switch (it.status) {
         case TRACK_STATION:
            message_area->post("Approaching station " + it.station->name());
            near_station(it.station);
            clear_station = false;
            return;
         case TRACK_NO_MORE:
            message_area->post("Oh no! You're going to crash!");
            break;
         case TRACK_CHOICE:
            message_area->post("Oh no! You have to make a decision!");
            it.track->set_state_render_hint();
            break;
         default:
            break;
         }

         if (!clear_station)
            left_station();
         return;
      }
   }
   
   // We're not approaching any station
   left_station();
}

void Game::alter_track_state(TrackStateReq req)
{
   // Change the state of the nearest points, etc.
   TrackIterator it = iterate_track(map, train->tile(),
      train->direction());

   const int max_alter_look = 10;

   for (int i = 0; i < max_alter_look; i++) {

      // Skip over the first section of track which may be some
      // points - we don't want to alter the track we're on!
      it = it.next();
    
      if (it.status == TRACK_CHOICE) {
         switch (req) {
         case NEXT:
            it.track->next_state();
            break;
         case PREV:
            it.track->prev_state();
            break;
         }
		
         return;
      }
   }

   warn() << "No nearby track state to change";
}

void Game::on_key_down(SDLKey a_key)
{   
   switch (a_key) {
   case SDLK_PAGEUP:
      view_radius = max(view_radius - 0.2f, 0.1f);
      break;
   case SDLK_PAGEDOWN:
      view_radius += 0.2f;
      break;
   case SDLK_b:
      train->controller()->act_on(BRAKE_TOGGLE);
      break;
   case SDLK_r:
      if (train->controller()->throttle() == 0)
         train->controller()->act_on(TOGGLE_REVERSE);
      else
         message_area->post("Reduce power first!", 51, 1000);
      break;
   case SDLK_LCTRL:
      train->controller()->act_on(SHOVEL_COAL);
      break;
   case SDLK_PRINT:
      get_game_window()->take_screen_shot();
      break;
   case SDLK_LEFT:
      alter_track_state(PREV);
      break;
   case SDLK_RIGHT:
      alter_track_state(NEXT);
      break;
   case SDLK_UP:
      train->controller()->act_on(THROTTLE_UP);
      break;
   case SDLK_DOWN:
      train->controller()->act_on(THROTTLE_DOWN);
      break;
   case SDLK_TAB:
      if (camera_mode == CAMERA_FLOATING)
         camera_mode = CAMERA_FIXED;
      else if (camera_mode == CAMERA_FIXED)
         switch_to_birdCamera();
      else
         camera_mode = CAMERA_FLOATING;
      break;
   default:
      break;
   }
}

void Game::on_key_up(SDLKey a_key)
{   
 
}

void Game::on_mouse_click(IPickBufferPtr a_pick_buffer, int x, int y,
   MouseButton a_button)
{
   switch (a_button) {
   case MOUSE_WHEEL_UP:
      view_radius = max(view_radius - 1.0f, 0.1f);
      break;
   case MOUSE_WHEEL_DOWN:
      view_radius += 1.0f;
      break;
   default:
      break;
   }
}

void Game::on_mouse_move(IPickBufferPtr a_pick_buffer, int x, int y,
   int xrel, int yrel)
{
   if (camera_mode == CAMERA_FLOATING) {
      cameraHTarget -= xrel / 100.0f;
      cameraVTarget += yrel / 100.0f;
      
      // Don't allow the camera to go under the ground
      const float ground = (M_PI / 2.0f) - 0.01f;
      if (cameraVTarget > ground)
         cameraVTarget = ground;
      
      // Don't let the camera flip over the top
      const float top = 0.01f;
      if (cameraVTarget < top)
         cameraVTarget = top;

      camera_speed = 2.0f;
   }
}

// Create an instance of the play screen with the given map
IScreenPtr make_game_screen(IMapPtr a_map)
{
   return IScreenPtr(new Game(a_map));
}
