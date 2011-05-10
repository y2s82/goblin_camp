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

// ReadUInt<boost::uint64_t> calls ReadUInt<boost::uint32_t> and ReadUInt<boost::uint32_t>
// ReadUInt<boost::uint32_t> calls ReadUInt<boost::uint16_t> and ReadUInt<boost::uint16_t>
// ReadUInt<boost::uint16_t> calls ReadUInt<boost::uint8_t>  and ReadUInt<boost::uint8_t>
// ReadUInt<boost::uint8_t> reads single byte from the stream
//
// The result is then bitshifted and ORed to reconstruct the value.
//
// To serialize signed integers, cast them into unsigned (and vice versa for unserialization).
// This will work, because it doesn't change the representation in the memory.
//
// WriteUInt is a recursive call, just like ReadUInt.

namespace Serializer {
	/**
		Reads unsigned integer from the source stream. Internally,
		calls itself recursively, splitting the value into two halves until
		half size reaches 1 byte.
		
		\tparam    T      Target type to unserialize.
		\param[in] stream Source stream to read from.
		\returns          Unserialized value.
	*/
	template <typename T>
	T ReadUInt(std::istream& stream) {
		typedef typename _Type<sizeof(T) / 2>::uint U;
		const boost::uint32_t bits = sizeof(U) * 8;
		
		U a = 0, b = 0;
		a = ReadUInt<U>(stream);
		b = ReadUInt<U>(stream);
		
		return (static_cast<T>(a) << bits) | static_cast<T>(b);
	}
	
	template <>
	boost::uint8_t ReadUInt<boost::uint8_t>(std::istream& stream) {
		return static_cast<boost::uint8_t>(stream.get());
	}
	
	/**
		Writes unsigned integer into the destination stream. Internally,
		calls itself recursively, splitting the value into two halves until
		half size reaches 1 byte.
		
		\tparam    T      Target type to serialize.
		\param[in] stream Destination stream to write to.
		\param[in] value  Value to serialize.
	*/
	template <typename T>
	void WriteUInt(std::ostream& stream, T value) {
		typedef typename _Type<sizeof(T) / 2>::uint U;
		const boost::uint32_t bits = sizeof(U) * 8;
		
		// All types here are unsigned.
		const U max = static_cast<U>(-1);
		
		WriteUInt<U>(stream, (value >> bits) & max);
		WriteUInt<U>(stream, value & max);
	}
	
	template <>
	void WriteUInt<boost::uint8_t>(std::ostream& stream, boost::uint8_t value) {
		stream.put(static_cast<char>(value));
	}
	
	/**
		Reads signed integer from the source stream. Internally, calls
		\ref ReadUInt<T>.
		
		\tparam    T      Target type to unserialize.
		\param[in] stream Source stream to read from.
		\returns          Unserialized value.
	*/
	template <typename T>
	T ReadSInt(std::istream& stream) {
		typedef typename _Type<sizeof(T)>::uint U;
		return static_cast<T>(ReadUInt<U>(stream));
	}
	
	/**
		Writes signed integer into the destination stream. Internally, calls
		\ref WriteUInt<T>.
		
		\tparam    T      Target type to serialize.
		\param[in] stream Destination stream to write to.
		\param[in] value  Value to serialize.
	*/
	template <typename T>
	void WriteSInt(std::ostream& stream, T value) {
		typedef typename _Type<sizeof(T)>::uint U;
		WriteUInt<U>(stream, static_cast<U>(value));
	}
};
