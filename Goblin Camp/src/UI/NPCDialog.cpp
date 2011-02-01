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

#include <string>

#include <libtcod.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

#include "UI/NPCDialog.hpp"
#include "UI/ScrollPanel.hpp"
#include "UI/UIList.hpp"

Dialog* NPCDialog::npcListDialog = 0;
Dialog* NPCDialog::NPCListDialog() {
	if (!npcListDialog) {
		npcListDialog = new Dialog(new NPCDialog(), "NPCs", Game::Inst()->ScreenWidth() - 20, Game::Inst()->ScreenHeight() - 20);
	}
	return npcListDialog;
}

NPCDialog::NPCDialog(): UIContainer(std::vector<Drawable*>(), 0, 0, Game::Inst()->ScreenWidth() - 20, Game::Inst()->ScreenHeight() - 20) {
	AddComponent(new ScrollPanel(0, 0, width, height, 
								 new UIList<std::pair<int, boost::shared_ptr<NPC> >, std::map<int, boost::shared_ptr<NPC> > >(&(Game::Inst()->npcList), 0, 0, width - 2, height, NPCDialog::DrawNPC), false));
}

void NPCDialog::DrawNPC(std::pair<int, boost::shared_ptr<NPC> > npci, int i, int x, int y, int width, bool selected, TCODConsole* console) {
	console->print(x, y, "NPC: %d", npci.second->Uid());
	console->print(x+11, y, "%s: %s",
				   npci.second->currentJob().lock() ? npci.second->currentJob().lock()->name.c_str() : "No job",
				   npci.second->currentTask() ? Job::ActionToString(npci.second->currentTask()->action).c_str() : "No task");
}
