/* Copyright 2010-2011 Ilkka Halila
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

#include <boost/python/detail/wrap_python.hpp>
#include <boost/python.hpp>

#include "stdafx.hpp"
namespace py = boost::python;

#include "scripting/_gcampapi/LoggerStream.hpp"
#include "Logger.hpp"

namespace Script { namespace API {
	void LoggerStream::write(const char *str) {
		Logger::log << str;
	}
	
	void ExposeLoggerStream() {
		py::class_<LoggerStream>("LoggerStream")
			.def("write", &LoggerStream::write)
		;
	}
}}
