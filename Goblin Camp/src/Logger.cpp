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
#include "stdafx.hpp"

#include <iostream>
#include <fstream>
#include <cassert>

#include <boost/date_time/local_time/local_time.hpp>

#include "Logger.hpp"

// TODO: rework logging to be more useful and friendly

Logger::Logger(const std::string& logFile) {
	output.open(logFile.c_str());
	#ifdef DEBUG
	output.rdbuf()->pubsetbuf(0, 0); // make log unbuffered for debug mode
	#endif
	output<<"Logging to " << logFile << " begun @ "<<boost::posix_time::second_clock::local_time()<<"\n";
}

Logger::~Logger() {
	output<<"Logging ended @ "<<boost::posix_time::second_clock::local_time()<<"\n";
	output.flush();
}

Logger* Logger::instance = NULL;

void Logger::Create(const std::string& logFile) {
	assert(instance == NULL);
	instance = new Logger(logFile);
}

Logger *Logger::Inst() {
	assert(instance != NULL);
	return instance;
}

void Logger::End() { delete(instance); }
