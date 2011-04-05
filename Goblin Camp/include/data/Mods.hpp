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

// Data refactoring: mods.
#include <list>
#include "tileRenderer/TileSetLoader.hpp"

namespace Mods {
	struct Metadata {
		std::string mod, name, author, version;
		short apiVersion;
		
		Metadata(const std::string&, const std::string&, const std::string&, const std::string&, short);
	};
		
	const std::list<Metadata>& GetLoaded();
	const std::list<TilesetModMetadata>& GetAvailableTilesetMods();
	
	void Load();
}
