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

using namespace std;

// Concrete logger implementation
class LoggerImpl : public ILogger {
   friend class PrintLine;
public:
   LoggerImpl();
   
   PrintLinePtr writeMsg(LogMsg::Type type);
};

namespace {
   bool isStdoutTTY;
}

LoggerImpl::LoggerImpl()
{
   isStdoutTTY = isatty(fileno(stdout));
}

PrintLinePtr LoggerImpl::writeMsg(LogMsg::Type type)
{
   if (isStdoutTTY)
         cout << "\x1B[1m";
   
   switch (type) {
   case LogMsg::NORMAL:
      cout << "[INFO ] ";
      break;
   case LogMsg::DEBUG:
      if (isStdoutTTY)
         cout << "\x1B[36m";
      cout << "[DEBUG] ";
      break;
   case LogMsg::WARN:
      if (isStdoutTTY)
         cout << "\x1B[33m";
      cout << "[WARN ] ";
      break;
   case LogMsg::ERROR:
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
