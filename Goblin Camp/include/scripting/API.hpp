/* Copyright 2010 Ilkka Halila
This file is part of Goblin Camp.

Goblin Camp is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Goblin Camp is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with Goblin Camp. If not, see <http://www.gnu.org/licenses/>.*/
#pragma once

class Logger;

namespace Script {
	namespace API {
		// Implements minimal file-like interface for use with logging.
		class LoggerStream {
			Logger *logger;
		public:
			LoggerStream();
			void close();
			void write(const char*);
			void flush();
		};
		
		// Announcer.
		void announce(const char*);
		
		namespace py = boost::python;
		extern py::object pyLoggerStream;
		extern py::object pyCoordinate;
	}
	
	void ExposeAPI();
	void AppendListener(PyObject*);
	void InvokeListeners(char*, char*, ...);
	void InvokeListeners(char*, PyObject* = NULL);
	void ReleaseListeners();
}
