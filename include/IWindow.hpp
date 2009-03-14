#ifndef INC_IWINDOW_HPP
#define INC_IWINDOW_HPP

#include <tr1/memory>

// Interface to the game window
class IWindow {
public:
   virtual void run() = 0;
   virtual void quit() = 0;
};

typedef std::tr1::shared_ptr<IWindow> IWindowPtr;

// Implementors
IWindowPtr makeSDLWindow();

#endif
