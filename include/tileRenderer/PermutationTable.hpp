/* Copyright 2011 Ilkka Halila
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

#include <libtcod.hpp>
#include <vector>

class PermutationTable {
public:
	explicit PermutationTable(int power);
	explicit PermutationTable(int power, uint32_t seed);
	~PermutationTable();

	inline int Hash(int val) const {
		return table[bitMask & val];
	}

	inline int ExtHash(int val) const {
		return table[(table[bitMask & val] + (val >> power)) & bitMask];
	}
private:
	std::vector<int> table;
	int power;
	int bitMask;
};
