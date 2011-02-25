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

#include "Logger.hpp"
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
//    - 0x00 (uint64_t, reserved, little endian)
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

// XXX: Relying on the little-endianess of the platform. Are we supporting anything other than x86/x64?
// WARNING: Wild templates ahead.
namespace {
	// These exist to determine two smaller types that can compose a bigger type.
	// N is type size in bytes.
	template <size_t N> struct type { };
	
	#define DEFINE_TYPE(T) template <> struct type<sizeof(T)> { typedef T uint; }
	DEFINE_TYPE(boost::uint8_t);
	DEFINE_TYPE(boost::uint16_t);
	DEFINE_TYPE(boost::uint32_t);
	DEFINE_TYPE(boost::uint64_t);
	#undef DEFINE_TYPE
	
	// ReadUInt<boost::uint64_t> calls ReadUInt<boost::uint32_t> and ReadUInt<boost::uint32_t>
	// ReadUInt<boost::uint32_t> calls ReadUInt<boost::uint16_t> and ReadUInt<boost::uint16_t>
	// ReadUInt<boost::uint16_t> calls ReadUInt<boost::uint8_t>  and ReadUInt<boost::uint8_t>
	// ReadUInt<boost::uint8_t> reads single byte from the stream
	//
	// The result is then bitshifted and ORed to reconstruct the value.
	
	template <typename T>
	T ReadUInt(std::ifstream& stream) {
		typedef typename type<sizeof(T) / 2>::uint smaller;
		
		const boost::uint32_t smallerBits = sizeof(smaller) * 8;
		
		smaller a, b;
		a = ReadUInt<smaller>(stream);
		b = ReadUInt<smaller>(stream);
		
		return ((T)a << smallerBits) | (T)b;
	}
	
	template <>
	boost::uint8_t ReadUInt<boost::uint8_t>(std::ifstream& stream) {
		return (boost::uint8_t)stream.get();
	}
	
	// WriteUInt is a recursive call, just like ReadUInt.
	
	template <typename T>
	void WriteUInt(std::ofstream& stream, typename type<sizeof(T)>::uint value) {
		typedef typename type<sizeof(T) / 2>::uint smaller;
		
		const boost::uint32_t smallerBits = sizeof(smaller) * 8;
		// All types here are unsigned.
		const smaller maxValue = (smaller)-1;
		
		WriteUInt<smaller>(stream, (value >> smallerBits) & maxValue);
		WriteUInt<smaller>(stream, value & maxValue);
	}
	
	template <>
	void WriteUInt<boost::uint8_t>(std::ofstream& stream, boost::uint8_t value) {
		stream.put((char)value);
	}
}

bool Game::SaveGame(const std::string& filename) {
	try {
		std::ofstream ofs(filename.c_str(), std::ios::binary);
		
		// Write the file header
		WriteUInt<boost::uint32_t>(ofs, saveMagicConst);
		WriteUInt<boost::uint8_t> (ofs, fileFormatConst);
		WriteUInt<boost::uint64_t>(ofs, 0x00ULL);
		WriteUInt<boost::uint64_t>(ofs, 0x00ULL);
		WriteUInt<boost::uint64_t>(ofs, 0x00ULL);
		WriteUInt<boost::uint64_t>(ofs, 0x00ULL);
		
		// Write the payload
		boost::archive::binary_oarchive oarch(ofs);
		oarch << Entity::uids;
		oarch << *instance;
		oarch << *JobManager::Inst();
		oarch << *Camp::Inst();
		oarch << *StockManager::Inst();
		oarch << *Map::Inst();
		
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
		
		// reserved values
		ReadUInt<boost::uint64_t>(ifs);
		ReadUInt<boost::uint64_t>(ifs);
		ReadUInt<boost::uint64_t>(ifs);
		ReadUInt<boost::uint64_t>(ifs);
		
		Game::Inst()->Reset();
		Game::Inst()->LoadingScreen();
		
		// Read the payload
		boost::archive::binary_iarchive iarch(ifs);
		iarch >> Entity::uids;
		iarch >> *instance;
		iarch >> *JobManager::Inst();
		iarch >> *Camp::Inst();
		iarch >> *StockManager::Inst();
		iarch >> *Map::Inst();
		
		Game::Inst()->TranslateContainerListeners();
		
		return true;
	} catch (const std::exception& e) {
		LOG("std::exception while trying to load the game: " << e.what());
		return false;
	}
}

#pragma warning(pop)
