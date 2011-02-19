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

#include "Trap.hpp"
#include "Game.hpp"
#include "NPC.hpp"
#include "GCamp.hpp"
#include "JobManager.hpp"
#include "Faction.hpp"

Trap::Trap(ConstructionType vtype, Coordinate pos) : Construction(vtype, pos),
ready(true){
	readyGraphic = graphic[1];
}

void Trap::Update() {
	if (built) {
		if (ready && !Map::Inst()->NPCList(x, y)->empty()) {
			ready = false;
			graphic[1] = 62;
			boost::shared_ptr<NPC> npc = Game::Inst()->npcList[*Map::Inst()->NPCList(x, y)->begin()];
			npc->AddEffect(Construction::Presets[type].trapAttack.StatusEffects()->front().first);
			npc->Damage(&Construction::Presets[type].trapAttack);
			Faction::factions[npc->GetFaction()]->TrapDiscovered(Position());
		}
	}
}

int Trap::GetMoveCostModifier(bool visible) {
	return visible ? 100 : -(Map::Inst()->GetTerrainMoveCost(x,y)-1); //-1 because a movecost of 0 = unwalkable
}

int Trap::Use() {
	if (!ready) {
		if (++progress == 75) {
			ready = true;
			graphic[1] = readyGraphic;
			progress = 0;
			//Hide the trap from everyone but the player faction
			for (int i = 0; i < Faction::factions.size(); ++i) {
				Faction::factions[i]->TrapSet(Position(), i == PLAYERFACTION);
			}
			return 100;
		}
		return progress;
	}
	return -1;
}

void Trap::SpawnRepairJob() {
	Construction::SpawnRepairJob();
	if (!ready && !reloadJob.lock()) { //Spawn reload job if one doesn't already exist
		boost::shared_ptr<Job> reload(new Job("Reset "+name));
		reload->tasks.push_back(Task(MOVEADJACENT, Position(), shared_from_this()));
		reload->tasks.push_back(Task(USE, Position(), shared_from_this()));
		reload->DisregardTerritory();
		JobManager::Inst()->AddJob(reload);
		reloadJob = reload;
	}
}

void Trap::AcceptVisitor(ConstructionVisitor& visitor) {
	visitor.Visit(this);
}

bool Trap::IsReady() const {
	return ready;
}