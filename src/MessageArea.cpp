//
//  Copyright (C) 2010  Nick Gasson
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

#include "IMessageArea.hpp"
#include "gui/IFont.hpp"
#include "GameScreens.hpp"
#include "ILogger.hpp"

class MessageArea : public IMessageArea {
public:
   MessageArea();

   // IMessageArea interface
   void update(int delta);
   void render() const;
   void post(const string& mess, int priority, int delay);
   
private:
   
   struct Message {
      string text;
      int priority;
      int delay;
   };
   
   Message active;
   bool have_active;
   gui::IFontPtr font;

   static const int FADE_OUT_TIME = 1000;
};

MessageArea::MessageArea()
   : have_active(false)
{
   font = gui::load_font("fonts/Vera.ttf", 18);
}

void MessageArea::render() const
{
   if (have_active) {
      const string& text = active.text;

      const int screenH = get_game_window()->height();
      const int screenW = get_game_window()->width();
      const int len = font->text_width(text);

      float alpha = 1.0f;
      const int& delay = active.delay;
      if (delay < 0) {
         assert(delay >= -FADE_OUT_TIME);

         alpha = 1.0f - (static_cast<float>(-delay) / FADE_OUT_TIME);
      }

      const Colour col = make_colour(1.0f, 1.0f, 1.0f, alpha);
      
      font->print((screenW - len)/2, screenH - 50, col, text);
   }
}

void MessageArea::update(int delta)
{
   if (have_active) {
      int& delay = active.delay;

      delay -= delta;
      if (delay < -FADE_OUT_TIME)
         have_active = false;
   }
}

void MessageArea::post(const string& mess, int priority, int delay)
{
   assert(priority >= 0 && priority <= 100);
   
   Message m = { mess, priority, delay };
   if (!have_active || active.priority <= priority) {
      active = m;
      have_active = true;
   }
}

IMessageAreaPtr make_message_area()
{
   return IMessageAreaPtr(new MessageArea);
}
