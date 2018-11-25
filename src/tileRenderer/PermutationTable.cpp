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

#include <cstdint>
#include "stdafx.hpp"
#include "tileRenderer/PermutationTable.hpp"

PermutationTable::PermutationTable(int pow)
 : 	table(),
	power(pow),
	bitMask((1 << pow) - 1) 
{
	int size = 1 << pow;
	for (int i = 0; i < size; ++i) {
		table.push_back(i);
	}
	TCODRandom * random = TCODRandom::getInstance();
	for (int i = size - 1; i > 0; --i) {
		int j = random->get(0, i);
		int temp = table[i];
		table[i] = table[j];
		table[j] = temp;
	}
}

PermutationTable::PermutationTable(int pow, std::uint32_t seed)
 : 	table(),
	power(pow),
	bitMask((1 << pow) - 1) 
{
	TCODRandom random(seed, TCOD_RNG_CMWC);
	int size = 1 << pow;
	for (int i = 0; i < size; ++i) {
		table.push_back(i);
	}
	
	for (int i = size - 1; i > 0; --i) {
		int j = random.get(0, i);
		int temp = table[i];
		table[i] = table[j];
		table[j] = temp;
	}
}

PermutationTable::~PermutationTable() {}
