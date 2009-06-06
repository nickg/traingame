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

#include "ILogger.hpp"

#include <iostream>
#include <cstdio>

#ifdef WIN32
#include <io.h>   // For _isatty
#endif

using namespace std;

// Concrete logger implementation
class LoggerImpl : public ILogger {
   friend struct PrintLine;
public:
   LoggerImpl();
   
   PrintLinePtr writeMsg(LogMsgType type);
};

namespace {
   bool isStdoutTTY;
}

LoggerImpl::LoggerImpl()
{
#ifdef WIN32
   isStdoutTTY = (_isatty(_fileno(stdout)) != 0);
#else
   isStdoutTTY = (isatty(fileno(stdout)) != 0);
#endif
}

PrintLinePtr LoggerImpl::writeMsg(LogMsgType type)
{
   if (isStdoutTTY)
         cout << "\x1B[1m";
   
   switch (type) {
   case LOG_NORMAL:
      cout << "[INFO ] ";
      break;
   case LOG_DEBUG:
      if (isStdoutTTY)
         cout << "\x1B[36m";
      cout << "[DEBUG] ";
      break;
   case LOG_WARN:
      if (isStdoutTTY)
         cout << "\x1B[33m";
      cout << "[WARN ] ";
      break;
   case LOG_ERROR:
      if (isStdoutTTY)
         cout << "\x1B[31m";
      cout << "[ERROR] ";
      break;
   }
   return PrintLinePtr(new PrintLine(cout));
}

PrintLine::PrintLine(ostream& aStream)
   : stream(aStream)
{
   
}

PrintLine::~PrintLine()
{
   if (isStdoutTTY)
      cout << "\x1B[0m";
   
   stream << endl;
}

// Return the single instance of Logger
ILoggerPtr getLogger()
{
   static ILoggerPtr logger(new LoggerImpl);
   return logger;
}
