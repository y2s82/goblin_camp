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

#include <cmath>

namespace FloorToInt {
    inline int convert(const double val) { return std::floor(val); }
}
namespace CeilToInt {
    inline int convert(const double val) { return std::ceil(val); }
}

namespace MathEx {
#if defined(__GNUC__) || defined(__clang__)
	inline int NextPowerOfTwo(int val)
	{
            if (!val) {
                return val;
            }
            return uint32_t(2) << (31 ^ __builtin_clz(uint32_t(val)));
        }
#else
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
#endif
}
