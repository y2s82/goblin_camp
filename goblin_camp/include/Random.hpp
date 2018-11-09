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

#include <boost/random.hpp>
#include <libtcod.hpp>
#include <Coordinate.hpp>

namespace Random {
	typedef boost::rand48 GeneratorImpl;

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
		Coordinate ChooseInExtent(const Coordinate& origin, const Coordinate& extent);
		Coordinate ChooseInExtent(const Coordinate& extent);
		Coordinate ChooseInRadius(const Coordinate& center, int radius);
		Coordinate ChooseInRadius(int radius);
		Coordinate ChooseInRectangle(const Coordinate& low, const Coordinate& high);
	private:
		GeneratorImpl generator;
		unsigned int seed;
	};
	
	void Init();
	int Generate(int, int);
	int Generate(int);
	double Generate();
	short Sign();
	bool GenerateBool();
	
	template <typename T>
	inline unsigned ChooseIndex(const T& container) {
		return static_cast<unsigned>(Generate(0, container.size() - 1));
	}
	
	template <typename T>
	inline const typename T::value_type& ChooseElement(const T& container) {
		return container[ChooseIndex(container)];
	}
	
	template <typename T>
	inline typename T::value_type& ChooseElement(T& container) {
		return container[ChooseIndex(container)];
	}
	
	template <typename T>
	inline T Sign(const T& expr) {
		return expr * Sign();
	}

	Coordinate ChooseInExtent(const Coordinate& origin, const Coordinate& extent);
	Coordinate ChooseInExtent(const Coordinate& extent);
	Coordinate ChooseInRadius(const Coordinate& center, const int radius);
	Coordinate ChooseInRadius(int radius);
	Coordinate ChooseInRectangle(const Coordinate& low, const Coordinate& high);
}
