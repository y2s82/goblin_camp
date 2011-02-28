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

#include <boost/type_traits.hpp>
#include <boost/cstdint.hpp>
#include <string>
#include <vector>

namespace Serializer {
	// These exist to determine two smaller types that can compose a bigger type.
	// N is type size in bytes.
	template <size_t N> struct _Type { };
	
	#define DEFINE_TYPE(T) template <> struct _Type<sizeof(T)> { typedef T uint; }
	DEFINE_TYPE(boost::uint8_t);
	DEFINE_TYPE(boost::uint16_t);
	DEFINE_TYPE(boost::uint32_t);
	DEFINE_TYPE(boost::uint64_t);
	#undef DEFINE_TYPE
};
