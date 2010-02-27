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

#include <boost/optional.hpp>

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
   
   boost::optional<Message> active;
   gui::IFontPtr font;

   static const int FADE_OUT_TIME = 1000;
};

MessageArea::MessageArea()
{
   font = gui::loadFont("fonts/Vera.ttf", 18);
}

void MessageArea::render() const
{
   if (active) {
      const string& text = active.get().text;

      const int screenH = getGameWindow()->height();
      const int screenW = getGameWindow()->width();
      const int len = font->text_width(text);

      float alpha = 1.0f;
      const int& delay = active.get().delay;
      if (delay < 0) {
         assert(delay >= -FADE_OUT_TIME);

         alpha = 1.0f - (static_cast<float>(-delay) / FADE_OUT_TIME);
      }

      const Colour col = makeColour(1.0f, 1.0f, 1.0f, alpha);
      
      font->print((screenW - len)/2, screenH - 50, col, text);
   }
}

void MessageArea::update(int delta)
{
   if (active) {
      int& delay = active.get().delay;

      delay -= delta;
      if (delay < -FADE_OUT_TIME)
         active = boost::optional<Message>();
   }
}

void MessageArea::post(const string& mess, int priority, int delay)
{
   assert(priority >= 0 && priority <= 100);
   
   Message m = { mess, priority, delay };
   if (!active || active.get().priority <= priority)
      active = m;
}

IMessageAreaPtr makeMessageArea()
{
   return IMessageAreaPtr(new MessageArea);
}