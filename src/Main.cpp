#include "IWindow.hpp"
#include "ILogger.hpp"

#include <iostream>

int main(int argc, char** argv)
{
   log() << "Program started";
   
   IWindowPtr window = makeSDLWindow();

   window->run();

   log() << "Finished";   
   return 0;
}
