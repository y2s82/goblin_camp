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

#include <boost/numeric/conversion/cast.hpp>

typedef boost::numeric::converter<
	int, double, boost::numeric::conversion_traits<int, double>, boost::numeric::def_overflow_handler, boost::numeric::Floor<double>
> FloorToInt;
typedef boost::numeric::converter<
	int, double, boost::numeric::conversion_traits<int, double>, boost::numeric::def_overflow_handler, boost::numeric::Ceil<double>
> CeilToInt;

namespace MathEx {
	inline int NextPowerOfTwo(int val)
	{
		val--;
		val = (val >> 1) | val;
		val = (val >> 2) | val;
		val = (val >> 4) | val;
		val = (val >> 8) | val;
		val = (val >> 16) | val;
		val++;
		return val;
	}
}