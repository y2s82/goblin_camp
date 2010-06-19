#include <iostream>
#include <fstream>

#include "logger.hpp"

Logger::Logger() {
    output.open("Log.txt");
    output<<"Logging begun\n";
}

Logger::~Logger() {
    output<<"Logging ended.";
    output.flush();
}

Logger* Logger::instance = NULL;

Logger *Logger::Inst() {
	if (!instance) instance = new Logger();
	return instance;
}

void Logger::End() { delete(instance); }
