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

#include "IScreen.hpp"
#include "gui/ILayout.hpp"
#include "gui/Label.hpp"
#include "ILogger.hpp"

#include <boost/lexical_cast.hpp>

class UIDemo : public IScreen {
public:
    UIDemo();

    // IScreen interface
    void display(IGraphicsPtr a_context) const {}
    void overlay() const;
    void update(IPickBufferPtr a_pick_buffer, int a_delta) {}
    void on_key_down(SDLKey a_key) {}
    void on_key_up(SDLKey a_key) {}
    void on_mouse_move(IPickBufferPtr a_pick_buffer, int x, int y,
	int xrel, int yrel) {}
    void on_mouse_click(IPickBufferPtr pick_buffer, int x, int y,
	MouseButton button);
    void on_mouse_release(IPickBufferPtr pick_buffer, int x, int y,
	MouseButton button) {}
   
private:
    void btn1Click(gui::Widget& w);
   
    gui::ILayoutPtr layout;
};

UIDemo::UIDemo()
{
    using namespace placeholders;
   
    layout = gui::make_layout("layouts/demo.xml");

    layout->get("/wnd1/btn1").connect(gui::Widget::SIG_CLICK,
	bind(&UIDemo::btn1Click, this, _1));
}

void UIDemo::btn1Click(gui::Widget& w)
{
    static int cnt = 0;
    debug() << "Clicked button 1!";

    layout->cast<gui::Label>("/wnd1/cntlabel").text(
	boost::lexical_cast<string>(++cnt));
}

void UIDemo::overlay() const
{
    layout->render();
}

void UIDemo::on_mouse_click(IPickBufferPtr pick_buffer, int x, int y,
    MouseButton button)
{
    layout->click(x, y);
}

IScreenPtr makeUIDemo()
{
    return IScreenPtr(new UIDemo);
}
