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
#pragma once

#include <cstdint>
#include <ctime>

namespace Data {
	// saves
	struct Save {
		std::string filename, size, date;
		time_t timestamp; // for sorting
		
		Save(const std::string&, std::uintmax_t, time_t);
	};
	
	// http://www.sgi.com/tech/stl/LessThanComparable.html
	inline bool operator<(const Save& a, const Save& b) {
		return a.timestamp < b.timestamp;
	}
	inline bool operator>(const Save& a, const Save& b) {
		return b < a;
	}
	inline bool operator<=(const Save& a, const Save& b) {
		return !(b < a);
	}
	inline bool operator>=(const Save& a, const Save& b) {
		return !(a < b);
	}
	
	void GetSavedGames(std::vector<Save>&);
	unsigned CountSavedGames();
	bool LoadGame(const std::string&);
	bool SaveGame(const std::string&, bool=true);
	
	// font, config
	void LoadConfig();
	void LoadFont();
	
	// misc
	void SaveScreenshot();
}
