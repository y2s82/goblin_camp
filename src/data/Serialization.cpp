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

/* I've placed all the serialization here, because I was unable to get
Goblin Camp to compile when each class' serialization function was in
the class' .cpp file. I assume that there is some magical Boost macro
that would fix that problem, but I unfortunately have very limited time
and I couldn't come up with a coherent answer just by googling. */
#include "stdafx.hpp"

#pragma warning(push, 2) //Boost::serialization generates a few very long warnings

#include <boost/bind.hpp>
#include <boost/format.hpp>
#include <fstream>
#include <cstdint>

#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/zlib.hpp>

namespace io = boost::iostreams;

#include "Logger.hpp"
#include "data/Config.hpp"
#include "data/Serialization.hpp"

#include "Entity.hpp"
#include "Game.hpp"
#include "JobManager.hpp"
#include "Camp.hpp"
#include "StockManager.hpp"
#include "Map.hpp"

// IMPORTANT
// Implementing class versioning properly is an effort towards backward compatibility for saves,
// or at least eliminating those pesky crashes. This means some discipline in editing following
// functions. General rules:
//   - If you change ::save, you change ::load accordingly, and vice versa.
//   - If you change any of them, therefore changing the file structure,
//     (and this is vital) *increment the appropriate class' version*.
//   - No reordering of old fields, Boost.Serialization is very order-dependent.
//     Append new fields at the end, and enclose them with the correct version check.
//   - If you need to remove any old field, remove it from the saving code, but
//     *preserve loading code for the previous version* (use dummy variable of
//     the same type) -- no point in having zeroed fields in saved game with newer class version,
//     but old saves will always have those fields present.
//   - Do not change the magic constant. It's introduced to ease recognition of the save
//     files, especially after implementing these rules.
//
//  NB: When saving, we always use newest version, so version checks have to be
//      present only in loading code.
//
// File format after this revision:
//
//  These two fields MUST always be present:
//    - magic constant (uint32_t, little endian):
//        This value should never change and must always be equal to saveMagicConst.
//        If it's not, the file is not Goblin Camp save, and MUST be rejected.
//    - file format version (uint8_t, little endian):
//        This value represents overall file format, as defined here.
//        It should be incremented when file format changes so much that maintaining backward 
//        compatibility is not possible or feasible. Parser MUST NOT attempt any further decoding
//        if file format version is different than build's fileFormatConst.
//        
//        File format version of 0xFF is reserved for experimental file formats,
//        and should never be used in production branches.
//
//  These fields are specific to current file format version:
//    - compression flag (uint8_t, little endian)
//        0x00 = uncompressed save (backwards compatible)
//        If other than 0x00, then specifies the algorithm.
//           0x01 = zlib deflate
//        Other values are invalid and MUST be rejected.
//    - 0x00 (uint8_t,  reserved, little endian)
//    - 0x00 (uint32_t, reserved, little endian)
//    - 0x00 (uint64_t, reserved, little endian)
//    - 0x00 (uint64_t, reserved, little endian)
//    - 0x00 (uint64_t, reserved, little endian)
//    - actual serialised payload, defined and processed by Boost.Serialization machinery

// Magic constant: reversed fourcc 'GCMP'
// (so you can see it actually spelled like this when hex-viewing the save).
const std::uint32_t saveMagicConst = 0x47434d50;

// File format version (8-bit, because it should not change too often).
const std::uint8_t fileFormatConst = 0x01;

//
// Save/load entry points
//

namespace {
	// XXX implement the archives
	template <size_t N> struct Type { };

	#define DEFINE_TYPE(T) template <> struct Type<sizeof(T)> { typedef T uint; }
	DEFINE_TYPE(std::uint8_t);
	DEFINE_TYPE(std::uint16_t);
	DEFINE_TYPE(std::uint32_t);
	DEFINE_TYPE(std::uint64_t);
	#undef DEFINE_TYPE
	
	template <typename T>
	T ReadUInt(std::istream& stream) {
		typedef typename Type<sizeof(T) / 2>::uint U;
		const std::uint32_t bits = sizeof(U) * 8;
		
		U a = 0, b = 0;
		a = ReadUInt<U>(stream);
		b = ReadUInt<U>(stream);
		
		return (static_cast<T>(a) << bits) | static_cast<T>(b);
	}
	
	template <>
	std::uint8_t ReadUInt<std::uint8_t>(std::istream& stream) {
		return static_cast<std::uint8_t>(stream.get());
	}
	
	template <typename T>
	void WriteUInt(std::ostream& stream, T value) {
		typedef typename Type<sizeof(T) / 2>::uint U;
		const std::uint32_t bits = sizeof(U) * 8;
		
		// All types here are unsigned.
		const U max = static_cast<U>(-1);
		
		WriteUInt<U>(stream, (value >> bits) & max);
		WriteUInt<U>(stream, value & max);
	}
	
	template <>
	void WriteUInt<std::uint8_t>(std::ostream& stream, std::uint8_t value) {
		stream.put(static_cast<char>(value));
	}
	
	void WritePayload(io::filtering_ostream& ofs) {
		boost::archive::binary_oarchive oarch(ofs);
		oarch << Entity::uids;
		oarch << *Game::Inst();
		oarch << *JobManager::Inst();
		oarch << *Camp::Inst();
		oarch << *StockManager::Inst();
		oarch << *Map::Inst();
	}
	
	void ReadPayload(io::filtering_istream& ifs) {
		boost::archive::binary_iarchive iarch(ifs);
		iarch >> Entity::uids;
		iarch >> *Game::Inst();
		iarch >> *JobManager::Inst();
		iarch >> *Camp::Inst();
		iarch >> *StockManager::Inst();
		iarch >> *Map::Inst();
	}
}

bool Game::SaveGame(const std::string& filename) {
	try {
		std::ofstream rawStream(filename.c_str(), std::ios::binary);
		io::filtering_ostream stream;
		
		// Write the file header
		WriteUInt<std::uint32_t>(rawStream, saveMagicConst);
		WriteUInt<std::uint8_t> (rawStream, fileFormatConst);
		
		bool compress = Config::GetCVar<bool>("compressSaves");
		// compression flag
		WriteUInt<std::uint8_t>(rawStream, (compress ? 0x01 : 0x00));
		
		// reserved
		WriteUInt<std::uint8_t> (rawStream, 0x00U);
		WriteUInt<std::uint16_t>(rawStream, 0x00U);
		WriteUInt<std::uint32_t>(rawStream, 0x00UL);
		WriteUInt<std::uint64_t>(rawStream, 0x00ULL);
		WriteUInt<std::uint64_t>(rawStream, 0x00ULL);
		WriteUInt<std::uint64_t>(rawStream, 0x00ULL);
		
		// Write the payload
		if (compress) {
			io::zlib_params params(6); // level
			stream.push(io::zlib_compressor(params));
		}
		
		stream.push(rawStream);
		Game::SavingScreen(std::bind(&WritePayload, std::ref(stream)));
		
		return true;
	} catch (const std::exception& e) {
		LOG("std::exception while trying to save the game: " << e.what());
		return false;
	}
}

bool Game::LoadGame(const std::string& filename) {
	try {
		std::ifstream rawStream(filename.c_str(), std::ios::binary);
		io::filtering_istream stream;
		
		// Read and verify the file header
		if (ReadUInt<std::uint32_t>(rawStream) != saveMagicConst) {
			throw std::runtime_error("Invalid magic value.");
		}
		
		if (ReadUInt<std::uint8_t>(rawStream) != fileFormatConst) {
			throw std::runtime_error("Invalid file format value.");
		}
		
		// compression
		std::uint8_t compressed = ReadUInt<std::uint8_t>(rawStream);
		
		if (compressed > 1) {
			throw std::runtime_error("Invalid compression algorithm.");
		}
		
		// reserved values
		if (ReadUInt<std::uint8_t>(rawStream) != 0) {
			throw std::runtime_error("Forward compatibility: reserved value #1 not 0x00.");
		}
		if (ReadUInt<std::uint16_t>(rawStream) != 0) {
			throw std::runtime_error("Forward compatibility: reserved value #2 not 0x0000.");
		}
		if (ReadUInt<std::uint32_t>(rawStream) != 0) {
			throw std::runtime_error("Forward compatibility: reserved value #3 not 0x00000000.");
		}
		if (ReadUInt<std::uint64_t>(rawStream) != 0) {
			throw std::runtime_error("Forward compatibility: reserved value #4 not 0x0000000000000000.");
		}
		if (ReadUInt<std::uint64_t>(rawStream) != 0) {
			throw std::runtime_error("Forward compatibility: reserved value #5 not 0x0000000000000000.");
		}
		if (ReadUInt<std::uint64_t>(rawStream) != 0) {
			throw std::runtime_error("Forward compatibility: reserved value #6 not 0x0000000000000000.");
		}
		
		Game::Inst()->Reset();
		
		// Read the payload
		if (compressed) {
			stream.push(io::zlib_decompressor());
		}
		
		stream.push(rawStream);
		Game::LoadingScreen(boost::bind(&ReadPayload, boost::ref(stream)));
		Game::Inst()->TranslateContainerListeners();
		Game::Inst()->ProvideMap();
		Game::Inst()->Pause();
		
		return true;
	} catch (const std::exception& e) {
		LOG("std::exception while trying to load the game: " << e.what());
		return false;
	}
}

#pragma warning(pop)
