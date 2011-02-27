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

// std::basic_string serialising
#include <stdexcept>

namespace Serializer {
	/**
		Writes a string into the destination stream. It is encoded as a
		8-bit integer \c C denoting character size, followed by a
		32-bit integer \c N denoting string length in characters, followed by
		\c (C*N) characters of the raw string data. There is no explicit
		string terminator (i.e. string @c abc is encoded into 8 bytes).
		The 32-bit limit on the string's length is deliberate, as we primarily
		build as 32-bit code. It may change in the future.
		
		\tparam    T      Character type.
		\param[in] stream Destination stream.
		\param[in] string String to serialise.
	*/
	template <typename T>
	void WriteString(std::ostream& stream, const std::basic_string<T>& string) {
		// convert the character type into the equivalent unsigned integer type
		typedef typename _Type<sizeof(T)>::uint U;
		typedef typename std::basic_string<T>::const_iterator Iter;
		
		// character size is encoded to ensure type safety when decoding
		WriteUInt<boost::uint8_t> (stream, sizeof(T));
		WriteUInt<boost::uint32_t>(stream, string.size());
		
		for (Iter it = string.begin(); it != string.end(); ++it) {
			WriteUInt<U>(stream, static_cast<U>(*it));
		}
	}
	
	/**
		Reads a string from the source stream. For encoding details see
		\ref WriteString<T>. Will throw if the encoded character size is
		different than that of the provided character type.
		
		\throws std::runtime_error When sizeof(T) != first decoded byte of the stream.
		
		\tparam    T      Character type.
		\param[in] stream Source stream.
		\returns          Decoded string.
	*/
	template <typename T>
	std::basic_string<T> ReadString(std::istream& stream) {
		typedef typename _Type<sizeof(T)>::uint U;
		
		boost::uint8_t charSize = ReadUInt<boost::uint8_t>(stream);
		if (charSize != sizeof(T)) {
			throw std::runtime_error("Encoded character size is different than provided one.");
		}
		
		std::basic_string<T> string;
		boost::uint32_t size = ReadUInt<boost::uint32_t>(stream);
		
		string.reserve(size);
		while (size--) {
			string.push_back(static_cast<T>(ReadUInt<U>(stream)));
		}
		
		return string;
	}
}
