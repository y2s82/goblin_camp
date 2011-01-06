/* Copyright 2010 Ilkka Halila
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

#include "MapRenderer.hpp"

class TCODMapRenderer : public MapRenderer
{
public:
	TCODMapRenderer();
	~TCODMapRenderer();

	void DrawMap(TCODConsole * console, Map* map, Coordinate upleft, int posX = 0, int posY = 0, int sizeX = 0, int sizeY = 0) ;
};