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

#include <tr1/memory>
#include <iosfwd>

// Stream surrogate for writing log data to
struct LogStream {
   LogStream(std::ostream& aStream);
   ~LogStream();
   std::ostream& stream;
};

typedef std::tr1::shared_ptr<LogStream> LogStreamPtr;

template <typename T>
inline LogStreamPtr operator<<(LogStreamPtr aLogStream, const T& aThing)
{
   using namespace std;
   aLogStream->stream << aThing;
   return aLogStream;
}

// Types of log levels
namespace LogMsg {
   enum Type {
      NORMAL, DEBUG
   };
}

// Interface to logger class
class ILogger {
public:
   virtual LogStreamPtr writeMsg(LogMsg::Type type = LogMsg::NORMAL) = 0;
};

typedef std::tr1::shared_ptr<ILogger> ILoggerPtr;

ILoggerPtr getLogger();

inline LogStreamPtr log(LogMsg::Type type = LogMsg::NORMAL)
{
   return getLogger()->writeMsg(type);
}

#endif
