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

#include "Announce.hpp"
#include "Events.hpp"
#include "Game.hpp"
#include "GCamp.hpp"

Events::Events() {}

void Events::Update() {
	if (rand() % (UPDATES_PER_SECOND * 60) == 0) {
		Announce::Inst()->AddMsg("AAAAHHH WOLF ATTACK", TCODColor::peach);
		Game::Inst()->CreateNPC(Coordinate(102,100), 3);
		Game::Inst()->CreateNPC(Coordinate(100,103), 3);
		Game::Inst()->CreateNPC(Coordinate(101,104), 3);
		Game::Inst()->CreateNPC(Coordinate(105,99), 3);
	}
}