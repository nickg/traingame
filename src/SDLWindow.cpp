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

#include "IWindow.hpp"
#include "ILogger.hpp"
#include "IPickBuffer.hpp"
#include "Maths.hpp"
#include "OpenGLHelper.hpp"
#include "IConfig.hpp"
#include "IMesh.hpp"

#include <stdexcept>
#include <sstream>
#include <cstdlib>
#include <cassert>

#include <boost/lexical_cast.hpp>
#include <SDL.h>
#include <GL/gl.h>
#include <GL/glu.h>

using namespace boost;

// Concrete implementation of SDL window
class SDLWindow : public IWindow, public IGraphics, public IPickBuffer,
                  public enable_shared_from_this<SDLWindow> {
public:
   SDLWindow();
   ~SDLWindow();

   // IWindow interface
   void run(IScreenPtr a_screen);
   void switch_screen(IScreenPtr a_screen);
   void quit();
   void take_screen_shot();
   int width() const { return width_; }
   int height() const { return height_; }
   void redraw_hint() {}
   int get_fps() const;

   // IGraphics interface
   bool cuboid_in_view_frustum(float x, float y, float z,
                               float sizeX, float sizeY, float sizeZ);
   bool cube_in_view_frustum(float x, float y, float z, float size);
   bool point_in_view_frustum(float x, float y, float z);
   void set_camera(const Vector<float>& a_pos,
                  const Vector<float>& a_rotation);
   void look_at(const Vector<float> an_eye_point,
               const Vector<float> a_target_point);

   // IPickBuffer interface
   IGraphicsPtr begin_pick(int x, int y);
   unsigned end_pick();
private:
   void process_input();
   MouseButton from_sdl_button(Uint8 aSDLButton) const;
   void capture_frame() const;
   
   bool am_running;
   int width_, height_;
   IScreenPtr screen;
   bool will_skip_next_frame;
   bool will_take_screen_shot;
   Frustum view_frustum;

   // Picking data
   static const int SELECT_BUFFER_SZ = 128;
   GLuint my_select_buffer[SELECT_BUFFER_SZ];
};

// Calculation and display of the FPS rate
namespace {
   int the_frame_counter = 0;
   int the_last_fps = 0;
}

static Uint32 updateFPS(Uint32 an_interval, void* thread)
{
   the_last_fps = the_frame_counter;
   the_frame_counter = 0;
   
   return an_interval;
}

static void frame_complete()
{
   the_frame_counter++;
   
   update_render_stats();
}

// A wrapper around SDL times
struct FrameTimerThread {
   FrameTimerThread()
   {
      my_timer = SDL_AddTimer(1000, updateFPS, this);
   }
   
   ~FrameTimerThread()
   {
      // Finalise properly when an exception is thrown
      SDL_RemoveTimer(my_timer);
   }
   
   SDL_TimerID my_timer;
};

// Create the game window
SDLWindow::SDLWindow()
   : am_running(false), will_skip_next_frame(false),
     will_take_screen_shot(false)
{
   IConfigPtr cfg = get_config();
      
   // Start SDL
   if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) {
      ostringstream ss;
      ss << "Unable to initialise SDL: " << SDL_GetError();
      throw runtime_error(ss.str());
   }
   atexit(SDL_Quit);

   // Set the video mode
   cfg->get("XRes", width_);
   cfg->get("YRes", height_);

   SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
   if (SDL_SetVideoMode(width_, height_, 0, SDL_OPENGL) == NULL) {
      ostringstream ss;
      ss << "Unable to create OpenGL screen: " << SDL_GetError();
      throw runtime_error(ss.str());
   }

   SDL_WM_SetCaption("Trains!", NULL);

   // Turn on key repeat
   SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);

   // Hide the window manager cursor
   //SDL_ShowCursor(SDL_DISABLE);
   
   // Start OpenGL
   printGLVersion();
   initGL();

   log() << "Created " << width_ << "x" << height_ << " window";
}

// Destroy the game window
SDLWindow::~SDLWindow()
{
   
}

// Make a screen capture at the end of this frame
void SDLWindow::take_screen_shot()
{
   will_take_screen_shot = true;
}

// Change the active screen while the game is running
void SDLWindow::switch_screen(IScreenPtr a_screen)
{
   assert(am_running);

   screen = a_screen;
   will_skip_next_frame = true;
}

// Run the game until the user quits
void SDLWindow::run(IScreenPtr a_screen)
{
   assert(!am_running);
   
   screen = a_screen;

   FrameTimerThread fps_timer;

   unsigned last_tick = SDL_GetTicks();

   // Wait a few milliseconds to get a reasonable tick delta
   SDL_Delay(1);
   
   am_running = true;
   do {
      unsigned tick_start = SDL_GetTicks();
      int delta = static_cast<int>(tick_start - last_tick);

      try {
         process_input();
         screen->update(shared_from_this(), delta);
         
         if (!will_skip_next_frame) {
            drawGLScene(shared_from_this(), shared_from_this(), screen);
            SDL_GL_SwapBuffers();
         }
         else
            will_skip_next_frame = false;
      }
      catch (runtime_error& e) {
         error() << "Caught exception: " << e.what();
         am_running = false;
      }

      if (will_take_screen_shot) {
         capture_frame();
         will_take_screen_shot = false;
      }

      frame_complete();
      //fps_timer.update_title();
      last_tick = tick_start;
      update_render_stats();
   } while (am_running);
   
   screen.reset();
}

// Stop the game cleanly
void SDLWindow::quit()
{
   am_running = false;
}

// Convert an SDL button constant to a MouseButton
MouseButton SDLWindow::from_sdl_button(Uint8 button) const
{
   switch (button) {
   case SDL_BUTTON_LEFT: return MOUSE_LEFT;
   case SDL_BUTTON_MIDDLE: return MOUSE_MIDDLE;
   case SDL_BUTTON_RIGHT: return MOUSE_RIGHT;
   case SDL_BUTTON_WHEELUP: return MOUSE_WHEEL_UP;
   case SDL_BUTTON_WHEELDOWN: return MOUSE_WHEEL_DOWN;
   default:
      return MOUSE_UNKNOWN;
   }
}

// Check for SDL input events
void SDLWindow::process_input()
{
   SDL_Event e;

   // Send only one mouse motion event per frame
   bool have_sent_mouseMotion = false;

   while (SDL_PollEvent(&e)) {
      switch (e.type) {
      case SDL_QUIT:
         // End the game
         quit();
         log() << "Window closed";
         break;
         
      case SDL_KEYDOWN:
         screen->on_key_down(e.key.keysym.sym);
         break;

      case SDL_KEYUP:
         screen->on_key_up(e.key.keysym.sym);
         break;

      case SDL_MOUSEMOTION:
         if (!have_sent_mouseMotion) {
            screen->on_mouse_move(shared_from_this(),
                                  e.motion.x, e.motion.y,
                                  e.motion.xrel, e.motion.yrel);
            have_sent_mouseMotion = true;
         }
         break;
         
      case SDL_MOUSEBUTTONDOWN:
         screen->on_mouse_click(shared_from_this(),
                                e.button.x, e.button.y,
                                from_sdl_button(e.button.button));
         break;

      case SDL_MOUSEBUTTONUP:
         screen->on_mouse_release(shared_from_this(),
                                  e.button.x, e.button.y,
                                  from_sdl_button(e.button.button));
         break;

      case SDL_VIDEORESIZE:
         width_ = e.resize.w;
         height_ = e.resize.h;
         
         resizeGLScene(shared_from_this());
         break;
      }
   }
}

// Set up OpenGL to pick out objects
IGraphicsPtr SDLWindow::begin_pick(int x, int y)
{
   ::begin_pick(shared_from_this(), my_select_buffer, x, y);
   return shared_from_this();
}

// Finish picking and return the name of the clicked object or zero
// It's *very* important that this is called exactly once for every
// begin_pick or things will get very messed up
unsigned SDLWindow::end_pick()
{
   return ::end_pick(my_select_buffer);
}

// Called to set the camera position
void SDLWindow::set_camera(const Vector<float>& a_pos,
                          const Vector<float>& a_rotation)
{
   glRotatef(a_rotation.x, 1.0f, 0.0f, 0.0f);
   glRotatef(a_rotation.y, 0.0f, 1.0f, 0.0f);
   glRotatef(a_rotation.z, 0.0f, 0.0f, 1.0f);
   glTranslatef(a_pos.x, a_pos.y, a_pos.z);

   view_frustum = get_view_frustum();
}

// A wrapper around glu_look_at
void SDLWindow::look_at(const Vector<float> an_eye_point,
                        const Vector<float> a_target_point)
{
   gluLookAt(an_eye_point.x, an_eye_point.y, an_eye_point.z,
             a_target_point.x, a_target_point.y, a_target_point.z,
             0, 1, 0);

   view_frustum = get_view_frustum();
}

// Intersect a cuboid with the current view frustum
bool SDLWindow::cuboid_in_view_frustum(float x, float y, float z,
                                       float sizeX, float sizeY, float sizeZ)
{
   return view_frustum.cuboid_in_frustum(x, y, z, sizeX, sizeY, sizeZ);
}

// Intersect a cube with the current view frustum
bool SDLWindow::cube_in_view_frustum(float x, float y, float z, float size)
{
   return view_frustum.cube_in_frustum(x, y, z, size);
}

// True if the point is contained within the view frustum
bool SDLWindow::point_in_view_frustum(float x, float y, float z)
{
   return view_frustum.point_in_frustum(x, y, z);
}

// Capture the OpenGL pixels and save them to a file
void SDLWindow::capture_frame() const
{
   static int file_number = 1;
   
   const string file_name
      ("screenshot" + lexical_cast<string>(file_number++) + ".bmp");

   SDL_Surface* temp = SDL_CreateRGBSurface
      (SDL_SWSURFACE, width_, height_, 24,
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
       0x000000FF, 0x0000FF00, 0x00FF0000, 0
#else
       0x00FF0000, 0x0000FF00, 0x000000FF, 0
#endif
       );
   assert(temp);

   const int w = width_;
   const int h = height_;
   unsigned char* pixels = new unsigned char[3 * w * h];

   glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, pixels);

   for (int i = 0; i < h; i++)
      memcpy(((char*)temp->pixels) + temp->pitch * i, pixels + 3*w * (h-i-1), w*3);
   delete[] pixels;

   SDL_SaveBMP(temp, file_name.c_str());
   SDL_FreeSurface(temp);

   log() << "Wrote screen shot to " << file_name;
}

int SDLWindow::get_fps() const
{
   return ::the_last_fps;
}

// Construct and initialise an OpenGL SDL window
IWindowPtr make_sdl_window()
{
   return std::tr1::shared_ptr<IWindow>(new SDLWindow);
}
