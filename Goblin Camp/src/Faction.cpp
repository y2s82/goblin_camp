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

#include "stdafx.hpp"

#include "Faction.hpp"
#include "NPC.hpp"

Faction::Faction() {}

void Faction::AddMember(boost::weak_ptr<NPC> newMember) {
	members.push_back(newMember);
}

void Faction::RemoveMember(boost::weak_ptr<NPC> member) {
	for (std::list<boost::weak_ptr<NPC> >::iterator membi = members.begin(); membi != members.end();) {
		if (membi->lock()) {
			if (member.lock() && member.lock() == membi->lock()) {
				members.erase(membi);
				return;
			}
			++membi;
		} else membi = members.erase(membi);
	}
}

void Faction::TrapDiscovered(Coordinate trapLocation) {
	trapVisible[trapLocation] = true;
}

bool Faction::IsTrapVisible(Coordinate trapLocation) {
	return trapVisible[trapLocation];
}

void Faction::TrapSet(Coordinate trapLocation, bool visible) {
	trapVisible[trapLocation] = visible;
}
