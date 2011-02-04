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

#pragma once

#include "tileRenderer/TileSetRenderer.hpp"
#include "tileRenderer/TileSet.hpp"

#include "Farmplot.hpp"
#include "Stockpile.hpp"
#include "Door.hpp"
#include "SpawningPool.hpp"

class DrawConstructionVisitor : public ConstructionVisitor  {
private:
	const TileSetRenderer * tileSetRenderer;
	const TileSet * tileSet;
	Map * map;
	SDL_Surface * dst;
	SDL_Rect * dstRect;
	const Coordinate& coordinate;
public:
	DrawConstructionVisitor(const TileSetRenderer * tileSetRenderer, const TileSet * tileSet, Map * map, SDL_Surface * destination, SDL_Rect *destinationRect, const Coordinate& pos);
	~DrawConstructionVisitor();

	void Visit(FarmPlot * farmPlot);
	void Visit(Stockpile * stockpile);
	void Visit(Construction * construction);
	void Visit(SpawningPool * spawningPool);
	void Visit(Door * door);
	
};