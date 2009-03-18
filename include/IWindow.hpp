#ifndef INC_IWINDOW_HPP
#define INC_IWINDOW_HPP

#include <tr1/memory>

#include "IScreen.hpp"

// Interface to the game window
class IWindow {
public:
   virtual ~IWindow() {}
   
   virtual void run(IScreenPtr aScreen) = 0;
   virtual void quit() = 0;
};

typedef std::tr1::shared_ptr<IWindow> IWindowPtr;

// Implementors
IWindowPtr makeSDLWindow();

#endif
