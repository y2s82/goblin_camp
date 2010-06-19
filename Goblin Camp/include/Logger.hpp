#ifndef LOGGER_HEADER
#define LOGGER_HEADER

#include <fstream>

class Logger {
	private:
		Logger();
		~Logger();
		static Logger* instance;
	public:
		static Logger* Inst();
		std::ofstream output;
		static void End();
};

#endif
