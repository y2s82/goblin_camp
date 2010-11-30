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

#include <boost/random/mersenne_twister.hpp>
#include <libtcod.hpp>

namespace Random {
	struct Dice {
		Dice(unsigned int, unsigned int = 1, float = 1.f, float = 0.f);
		Dice(const TCOD_dice_t&);
		int Roll();
		int Max();
		int Min();
	private:
		unsigned int dices;
		unsigned int faces;
		float multiplier;
		float offset;
	};
	
	struct Generator {
		Generator(unsigned int = 0);
		void SetSeed(unsigned int = 0);
		unsigned int GetSeed() const;
		int Generate(int, int);
		int Generate(int);
		double Generate();
		short Sign();
		bool GenerateBool();
	private:
		boost::mt19937 generator;
		unsigned int seed;
	};
	
	void Init();
	int Generate(int, int);
	int Generate(int);
	double Generate();
	short Sign();
	bool GenerateBool();
	
	template <typename T>
	inline unsigned Choose(const T& container) {
		return static_cast<unsigned>(Generate(0, container.size() - 1));
	}
	
	template <typename T>
	inline T Sign(const T& expr) {
		return expr * Sign();
	}
}

#include <cstdlib>
#define rand() ___error_stray_rand_call___
