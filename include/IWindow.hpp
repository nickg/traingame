#ifndef INC_IWINDOW_HPP
#define INC_IWINDOW_HPP

#include <memory>

#include "IScreen.hpp"

// Interface to the game window
class IWindow {
public:
   virtual ~IWindow() {}
   
   virtual void run(IScreenPtr aScreen) = 0;
   virtual void switchScreen(IScreenPtr aScreen) = 0;
   virtual void quit() = 0;
   virtual void takeScreenShot() = 0;
};

typedef std::shared_ptr<IWindow> IWindowPtr;

// Implementors
IWindowPtr makeSDLWindow();

#endif
