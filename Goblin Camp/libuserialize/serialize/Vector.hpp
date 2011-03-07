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

namespace Serializer {
	template <typename T>
	void WriteVector(std::ostream&, const std::vector<T>&, boost::function<void(std::ostream&, const T&)>);
	template <typename T>
	std::vector<T> ReadVector(std::istream&, boost::function<void(std::istream&, T&)>);
	
	template <typename T>
	void WriteVectorInt(std::ostream& stream, const T& value) {
		BOOST_STATIC_ASSERT(boost::is_integral<T>::value);
		
		if (boost::is_unsigned<T>::value) {
			WriteUInt<T>(stream, value);
		} else {
			WriteSInt<T>(stream, value);
		}
	}
	
	template <typename U>
	void WriteVectorString(std::ostream& stream, const std::basic_string<U>& value) {
		WriteString<U>(stream, value);
	}
	
	/*template <typename U>
	void WriteVectorVector(std::ostream& stream, const std::vector<U>& value) {
		// for nested vectors
		WriteVector< std::vector<U> >(stream, value);
	}*/
	
	/**
		Writes a vector into the destination stream. Vector is encoded
		as a 32-bit unsigned integer \c N denoting number of
		vector elements, followed by \c N elements which are encoded
		using user-provided functor. Refer to \ref WriteString<T> for
		information about why 32-bit types are used for sizes.
		
		NOTE: No versioning is provided by the framework — if the
		element type changes, provided functor must account for
		that itself.
		
		\tparam    T      Element type.
		\param[in] stream Destination stream.
		\param[in] vec    Vector to encode.
	*/
	template <typename T>
	void WriteVector(std::ostream& stream, const std::vector<T>& vec, boost::function<void(std::ostream&, const T&)> functor) {
		WriteUInt<boost::uint32_t>(stream, vec.size());
		for (std::vector<T>::const_iterator it = vec.begin(); it != vec.end(); ++it) {
			functor(stream, *it);
		}
	}
	
	template <typename T>
	void ReadVectorInt(std::istream& stream, T& value) {
		BOOST_STATIC_ASSERT(boost::is_integral<T>::value);
		if (boost::is_unsigned<T>::value) {
			value = ReadUInt<T>(stream);
		} else {
			value = ReadSInt<T>(stream);
		}
	}
	
	template <typename U>
	void ReadVectorString(std::istream& stream, std::basic_string<U>& value) {
		value = ReadString<U>(stream);
	}
	
	/*template <typename U>
	void ReadVectorVector(std::istream& stream, std::vector<U>& vec) {
		vec = ReadVector< std::vector<U> >(stream);
	}*/
	
	/**
		Reads a vector from the source stream. Refer to \ref WriteVector<T>
		for details about encoding.
		
		\throws std::logic_error \ref ReadVectorElement<T>.
		
		\tparam    T      Element type.
		\param[in] stream Source stream.
		\returns          Decoded vector.
	*/
	template <typename T>
	std::vector<T> ReadVector(std::istream& stream, boost::function<void(std::istream&, T&)> functor) {
		std::vector<T> vec;
		boost::uint32_t size = ReadUInt<boost::uint32_t>(stream);
		vec.resize(size);
		
		for (boost::uint32_t idx = 0; idx < size; ++idx) {
			functor(stream, vec[idx]);
		}
		
		return vec;
	}
}
