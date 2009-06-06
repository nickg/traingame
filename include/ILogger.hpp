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

#ifndef INC_ILOGGER_HPP
#define INC_ILOGGER_HPP

#include "Platform.hpp"

#include <ostream>

// Stream surrogate for writing log data to
struct PrintLine {
   PrintLine(std::ostream& aStream);
   ~PrintLine();
   std::ostream& stream;
};

typedef std::tr1::shared_ptr<PrintLine> PrintLinePtr;

template <typename T>
inline PrintLinePtr operator<<(PrintLinePtr aPrintLine, const T& aThing)
{
   using namespace std;
   aPrintLine->stream << aThing;
   return aPrintLine;
}

// Types of log levels
enum LogMsgType {
   LOG_NORMAL, LOG_DEBUG, LOG_WARN, LOG_ERROR
};

// Interface to logger class
struct ILogger {
   virtual ~ILogger() {}
   
   virtual PrintLinePtr writeMsg(LogMsgType type = LOG_NORMAL) = 0;
};

typedef std::tr1::shared_ptr<ILogger> ILoggerPtr;

ILoggerPtr getLogger();

inline PrintLinePtr log(LogMsgType type = LOG_NORMAL)
{
   return getLogger()->writeMsg(type);
}

inline PrintLinePtr warn() { return log(LOG_WARN); }
inline PrintLinePtr debug() { return log(LOG_DEBUG); }
inline PrintLinePtr error() { return log(LOG_ERROR); }

#endif
