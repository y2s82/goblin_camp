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

DrawConstructionVisitor::DrawConstructionVisitor(const TilesetRenderer * tileSetRenderer, const TileSet * tileSet, int screenX, int screenY, const Coordinate& pos)
		: tileSetRenderer(tileSetRenderer),
		  tileSet(tileSet),
		  screenX(screenX),
		  screenY(screenY),
		  coordinate(pos)
	{}

DrawConstructionVisitor::~DrawConstructionVisitor() {}

void DrawConstructionVisitor::Visit(FarmPlot * farmplot) { 
	tileSet->DrawBaseConstruction(screenX, screenY, farmplot, coordinate);
	tileSetRenderer->DrawFilth(screenX, screenY, coordinate);
	tileSet->DrawStockpileContents(screenX, screenY, farmplot, coordinate);
}

void DrawConstructionVisitor::Visit(Stockpile * stockpile) {
	tileSet->DrawBaseConstruction(screenX, screenY, stockpile, coordinate);
	tileSetRenderer->DrawFilth(screenX, screenY, coordinate);
	tileSet->DrawStockpileContents(screenX, screenY, stockpile, coordinate);
}

void DrawConstructionVisitor::Visit(Construction * construction) {
	tileSetRenderer->DrawFilth(screenX, screenY, coordinate);

	Coordinate internal_pos = coordinate - construction->Position();
	int pos = internal_pos.X() + internal_pos.Y() * Construction::Blueprint(construction->Type()).X();
	int maxPos = Construction::Blueprint(construction->Type()).X() * Construction::Blueprint(construction->Type()).Y();
	if ((construction->GetMaxCondition() + construction->Condition()) * maxPos > (pos + 1) * construction->GetMaxCondition()) {
		tileSet->DrawBaseConstruction(screenX, screenY, construction, coordinate);
	} else {
		tileSet->DrawUnderConstruction(screenX, screenY, construction, coordinate);
	}
}

void DrawConstructionVisitor::Visit(SpawningPool * spawningPool) {
	tileSetRenderer->DrawFilth(screenX, screenY, coordinate);
	if (spawningPool->Condition() < 0) {
		tileSet->DrawUnderConstruction(screenX, screenY, spawningPool, coordinate);
	} else { 
		tileSet->DrawBaseConstruction(screenX, screenY, spawningPool, coordinate);
	}
}

void DrawConstructionVisitor::Visit(Door * door) {
	tileSetRenderer->DrawFilth(screenX, screenY, coordinate);
	if (door->Condition() < 0) {
		tileSet->DrawUnderConstruction(screenX, screenY, door, coordinate);
	} else if (door->Open()) {
		tileSet->DrawOpenDoor(screenX, screenY, door, coordinate);
	} else {
		tileSet->DrawBaseConstruction(screenX, screenY, door, coordinate);
	}
	
}

void DrawConstructionVisitor::Visit(Trap * trap) {
	tileSetRenderer->DrawFilth(screenX, screenY, coordinate);
	Coordinate internal_pos = coordinate - trap->Position();

	int pos = internal_pos.X() + internal_pos.Y() * Construction::Blueprint(trap->Type()).X();
	int maxPos = Construction::Blueprint(trap->Type()).X() * Construction::Blueprint(trap->Type()).Y();
	if ((trap->GetMaxCondition() + trap->Condition()) * maxPos <= (pos + 1) * trap->GetMaxCondition()) {
		tileSet->DrawUnderConstruction(screenX, screenY, trap, coordinate);
	} else if (trap->IsReady()) {
		tileSet->DrawBaseConstruction(screenX, screenY, trap, coordinate);
	} else {
		tileSet->DrawUnreadyTrap(screenX, screenY, trap, coordinate);
	}
}