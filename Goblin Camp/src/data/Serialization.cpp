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
#include <boost/algorithm/string.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/version.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/set.hpp>
#include <boost/serialization/weak_ptr.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/deque.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/array.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/multi_array.hpp>
#include <map>
#include <fstream>
#include <boost/cstdint.hpp>

#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/zlib.hpp>

namespace io = boost::iostreams;

#include "Logger.hpp"
#include "data/Config.hpp"
#include "Game.hpp"
#include "Tile.hpp"
#include "Coordinate.hpp"
#include "JobManager.hpp"
#include "MapMarker.hpp"
#include "Map.hpp"
#include "StockManager.hpp"
#include "StatusEffect.hpp"
#include "Stockpile.hpp"
#include "Farmplot.hpp"
#include "Door.hpp"
#include "Camp.hpp"
#include "Container.hpp"
#include "Blood.hpp"
#include "Entity.hpp"
#include "Attack.hpp"
#include "SpawningPool.hpp"
#include "Faction.hpp"
#include "Trap.hpp"
#include "Weather.hpp"

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
const boost::uint32_t saveMagicConst = 0x47434d50;

// File format version (8-bit, because it should not change too often).
const boost::uint8_t fileFormatConst = 0x01;

//





// Save/load entry points
//

namespace {
	template <typename T>
	void WritePayload(T& ofs) {
		boost::archive::binary_oarchive oarch(ofs);
		oarch << Entity::uids;
		oarch << *Game::Inst();
		oarch << *JobManager::Inst();
		oarch << *Camp::Inst();
		oarch << *StockManager::Inst();
		oarch << *Map::Inst();
	}
	
	template <typename T>
	void ReadPayload(T& ifs) {
		boost::archive::binary_iarchive iarch(ifs);
		iarch >> Entity::uids;
		iarch >> *Game::Inst();
		iarch >> *JobManager::Inst();
		iarch >> *Camp::Inst();
		iarch >> *StockManager::Inst();
		iarch >> *Map::Inst();
	}
}

#include "uSerialize.hpp"
using Serializer::WriteUInt;
using Serializer::ReadUInt;

bool Game::SaveGame(const std::string& filename) {
	try {
		std::ofstream ofs(filename.c_str(), std::ios::binary);
		
		// Write the file header
		WriteUInt<boost::uint32_t>(ofs, saveMagicConst);
		WriteUInt<boost::uint8_t> (ofs, fileFormatConst);
		
		bool compress = Config::GetCVar<bool>("compressSaves");
		// compression flag
		WriteUInt<boost::uint8_t>(ofs, (compress ? 0x01 : 0x00));
		
		// reserved
		WriteUInt<boost::uint8_t> (ofs, 0x00U);
		WriteUInt<boost::uint16_t>(ofs, 0x00U);
		WriteUInt<boost::uint32_t>(ofs, 0x00UL);
		WriteUInt<boost::uint64_t>(ofs, 0x00ULL);
		WriteUInt<boost::uint64_t>(ofs, 0x00ULL);
		WriteUInt<boost::uint64_t>(ofs, 0x00ULL);
		
		// Write the payload
		if (compress) {
			io::filtering_ostream cfs;
			io::zlib_params params(6); // level
			cfs.push(io::zlib_compressor(params));
			cfs.push(ofs);
			WritePayload(cfs);
		} else {
			WritePayload(ofs);
		}
		
		return true;
	} catch (const std::exception& e) {
		LOG("std::exception while trying to save the game: " << e.what());
		return false;
	}
}

bool Game::LoadGame(const std::string& filename) {
	try {
		std::ifstream ifs(filename.c_str(), std::ios::binary);
		
		// Read and verify the file header
		if (ReadUInt<boost::uint32_t>(ifs) != saveMagicConst) {
			throw std::runtime_error("Invalid magic value.");
		}
		
		if (ReadUInt<boost::uint8_t>(ifs) != fileFormatConst) {
			throw std::runtime_error("Invalid file format value.");
		}
		
		// compression
		boost::uint8_t compressed = ReadUInt<boost::uint8_t>(ifs);
		
		if (compressed > 1) {
			throw std::runtime_error("Invalid compression algorithm.");
		}
		
		// reserved values
		if (ReadUInt<boost::uint8_t>(ifs) != 0) {
			throw std::runtime_error("Forward compatibility: reserved value #1 not 0x00.");
		}
		if (ReadUInt<boost::uint16_t>(ifs) != 0) {
			throw std::runtime_error("Forward compatibility: reserved value #2 not 0x0000.");
		}
		if (ReadUInt<boost::uint32_t>(ifs) != 0) {
			throw std::runtime_error("Forward compatibility: reserved value #3 not 0x00000000.");
		}
		if (ReadUInt<boost::uint64_t>(ifs) != 0) {
			throw std::runtime_error("Forward compatibility: reserved value #4 not 0x0000000000000000.");
		}
		if (ReadUInt<boost::uint64_t>(ifs) != 0) {
			throw std::runtime_error("Forward compatibility: reserved value #5 not 0x0000000000000000.");
		}
		if (ReadUInt<boost::uint64_t>(ifs) != 0) {
			throw std::runtime_error("Forward compatibility: reserved value #6 not 0x0000000000000000.");
		}
		
		Game::Inst()->Reset();
		Game::Inst()->LoadingScreen();
		
		// Read the payload
		if (compressed) {
			io::filtering_istream cfs;
			cfs.push(io::zlib_decompressor());
			cfs.push(ifs);
			ReadPayload(cfs);
		} else {
			ReadPayload(ifs);
		}
		
		Game::Inst()->TranslateContainerListeners();
		
		return true;
	} catch (const std::exception& e) {
		LOG("std::exception while trying to load the game: " << e.what());
		return false;
	}
}

#pragma warning(pop)
