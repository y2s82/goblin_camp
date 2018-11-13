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

#include <boost/serialization/split_member.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/version.hpp>

/**
	This macro exists to enfoce consistent signatures across all serialisable
	classes. I removed templateness to speed up the build (since we only use binary
	archives anyway) — save/load can now be implemented in separate translation units.
	
	A complete definition of a serialisable class/struct looks like this:
	
	<code>
		class T {
			GC_SERIALIZABLE_CLASS
		};
		
		BOOST_CLASS_VERSION(T, 0)
	</code>
	
	\ref InputArchive and \ref OutputArchive typedefs have been created
	so that we don't completely lose ability to change the archive type
	in the future (even if it's not likely to happen).
*/
#define GC_SERIALIZABLE_CLASS \
	friend class boost::serialization::access; \
	void save(OutputArchive&, const unsigned) const; \
	void load(InputArchive&, const unsigned); \
	BOOST_SERIALIZATION_SPLIT_MEMBER()

typedef boost::archive::binary_oarchive OutputArchive;
typedef boost::archive::binary_iarchive InputArchive;
