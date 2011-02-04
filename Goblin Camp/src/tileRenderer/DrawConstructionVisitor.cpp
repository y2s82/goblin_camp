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
#include "tileRenderer/DrawConstructionVisitor.hpp"

DrawConstructionVisitor::DrawConstructionVisitor(const TileSetRenderer * tileSetRenderer, const TileSet * tileSet, Map * map, SDL_Surface * destination, SDL_Rect *destinationRect, const Coordinate& pos)
		: tileSetRenderer(tileSetRenderer),
		  tileSet(tileSet),
		  map(map),
		  dst(destination),
		  dstRect(destinationRect),
		  coordinate(pos)
	{}

DrawConstructionVisitor::~DrawConstructionVisitor() {}

void DrawConstructionVisitor::Visit(FarmPlot * farmplot) { 
	tileSet->DrawBaseConstruction(farmplot, coordinate, dst, dstRect);
	tileSetRenderer->DrawFilth(map, coordinate.X(), coordinate.Y(), dstRect);
	tileSet->DrawStockpileContents(farmplot, coordinate, dst, dstRect);
}

void DrawConstructionVisitor::Visit(Stockpile * stockpile) {
	tileSet->DrawBaseConstruction(stockpile, coordinate, dst, dstRect);
	tileSetRenderer->DrawFilth(map, coordinate.X(), coordinate.Y(), dstRect);
	tileSet->DrawStockpileContents(stockpile, coordinate, dst, dstRect);
}

void DrawConstructionVisitor::Visit(Construction * construction) {
	Coordinate internal_pos = coordinate - construction->Position();
	int pos = internal_pos.X() + internal_pos.Y() * Construction::Blueprint(construction->Type()).X();
	int maxPos = Construction::Blueprint(construction->Type()).X() * Construction::Blueprint(construction->Type()).Y();
	if ((construction->GetMaxCondition() + construction->Condition()) * maxPos > (pos + 1) * construction->GetMaxCondition()) {
		tileSet->DrawBaseConstruction(construction, coordinate, dst, dstRect);
	} else {
		tileSet->DrawUnderConstruction(construction, coordinate, dst, dstRect);
	}
	tileSetRenderer->DrawFilth(map, coordinate.X(), coordinate.Y(), dstRect);
}

void DrawConstructionVisitor::Visit(SpawningPool * spawningPool) {
	if (spawningPool->Condition() < 0) {
		tileSet->DrawUnderConstruction(spawningPool, coordinate, dst, dstRect);
	} else { 
		tileSet->DrawBaseConstruction(spawningPool, coordinate, dst, dstRect);
	}
	tileSetRenderer->DrawFilth(map, coordinate.X(), coordinate.Y(), dstRect);
}

void DrawConstructionVisitor::Visit(Door * door) {
	if (door->Open()) {
		tileSet->DrawOpenDoor(door, coordinate, dst, dstRect);
	} else {
		tileSet->DrawBaseConstruction(door, coordinate, dst, dstRect);
	}
	tileSetRenderer->DrawFilth(map, coordinate.X(), coordinate.Y(), dstRect);
}