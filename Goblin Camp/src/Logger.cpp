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

#include <iostream>
#include <fstream>

#include <boost/date_time/local_time/local_time.hpp>

#include "Logger.hpp"

Logger::Logger() {
    output.open("Log.txt");
    output<<"Logging begun @ "<<boost::posix_time::second_clock::local_time()<<"\n";
	
}

Logger::~Logger() {
	output<<"Logging ended @ "<<boost::posix_time::second_clock::local_time();
    output.flush();
}

Logger* Logger::instance = NULL;

Logger *Logger::Inst() {
	if (!instance) instance = new Logger();
	return instance;
}

void Logger::End() { delete(instance); }
