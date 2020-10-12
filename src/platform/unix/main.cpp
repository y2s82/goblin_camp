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
#include "stdafx.hpp"

#include <vector>
#include <string>
#include <SDL2/SDL_log.h>

int GCMain(std::vector<std::string>&);

int main(int argc, char **argv) {
    SDL_LogSetAllPriority(SDL_LOG_PRIORITY_VERBOSE);
	std::vector<std::string> args(argv, argv + argc);
        puts("hello");
	return GCMain(args);
}
