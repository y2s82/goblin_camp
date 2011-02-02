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

#include "stdafx.hpp"
#include "tileRenderer/TileSetUtil.hpp"

int TilesetUtil::CalcConnectionMapIndex(bool connectNorth, bool connectEast, bool connectSouth, bool connectWest) {
	int index = 0;
	if (!connectSouth) index += 8;
	if (connectSouth == connectNorth) index +=4;
	if (connectWest) index += 2;
	if (connectEast != connectWest) index += 1;
	return index;
}
