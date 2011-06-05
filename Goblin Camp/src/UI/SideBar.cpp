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

#include <libtcod.hpp>
#include <string>
#include <vector>
#include <boost/function.hpp>
#include <boost/lexical_cast.hpp>

#include "StatusEffect.hpp"
#include "Item.hpp"
#include "NPC.hpp"
#include "UI.hpp"
#include "Farmplot.hpp"
#include "UI/SideBar.hpp"
#include "UI/ScrollPanel.hpp"
#include "UI/Frame.hpp"
#include "UI/UIList.hpp"
#include "UI/Label.hpp"
#include "UI/Button.hpp"
#include "UI/ConstructionDialog.hpp"

SideBar::SideBar() :
width(19),
height(30),
topY(0),
npc(false),
construction(false)
{}

MenuResult SideBar::Update(int x, int y, bool clicked) {
	if (contents && x > Game::Inst()->ScreenWidth() - width) {
		MenuResult result = contents->Update(x - (leftX + 1), y - (topY + 14), clicked, NO_KEY);
		if(!(result & NOMENUHIT)) {
			return result;
		}
		if (y > topY && y < topY+height) return MENUHIT;
	}
	return NOMENUHIT;
}

void SideBar::Draw(TCODConsole* console) {
	if (entity.lock()) {
		console->setDefaultForeground(TCODColor::white);
		int edgeX = console->getWidth();
		leftX = edgeX - width;
		topY = std::max(0,(console->getHeight() - height) / 2);
		console->rect(edgeX - (width-1), topY+1, width-2, height-2, true);
		
		if(contents) {
			contents->Draw(edgeX - (width-1), topY+14, console);
		}
		
		console->setDefaultForeground(TCODColor::white);
		console->printFrame(edgeX - width, topY, width, height, false, TCOD_BKGND_DEFAULT, entity.lock()->Name().c_str());
		Game::Inst()->Draw(console, entity.lock()->Center().X() + 0.5f, entity.lock()->Center().Y() + 0.5f, false, edgeX - (width - 4), topY + 2, 11, 11);
		
		if (npc || construction) { //Draw health bar
			int health;
			if (npc) {
				boost::shared_ptr<NPC> creature = boost::static_pointer_cast<NPC>(entity.lock());
				health = (int)(((double)creature->GetHealth() / (double)std::max(1, creature->GetMaxHealth())) * 10);
			} else {
				boost::shared_ptr<Construction> construct = boost::static_pointer_cast<Construction>(entity.lock());
				health = (int)(((double)construct->Condition() / (double)std::max(1, construct->GetMaxCondition())) * 10);
			}
			for (int i = 0; i < health; ++i) {
				console->setChar(edgeX - (width-2), topY+12-i, 231);
				TCODColor color;
				if (health > 7) color = TCODColor::green;
				else if (health > 3) color = TCODColor::yellow;
				else color = TCODColor::red;
				console->setCharForeground(edgeX - (width-2), topY+12-i, color);
			}
		}
	}
	console->setDefaultForeground(TCODColor::white);
}

void SideBar::GetTooltip(int x, int y, Tooltip *tooltip, TCODConsole *console) {
	if(entity.lock()) {
		int edgeX = console->getWidth();
		int minimapX = edgeX - (width-4);
		int minimapY = topY + 2;
		if (x >= minimapX && x < minimapX + 11 &&
			y >= minimapY && y < minimapY + 11) {
			int actualX = (entity.lock()->Position()-5).X() + x - minimapX;
			int actualY = (entity.lock()->Position()-5).Y() + y - minimapY;
			std::list<boost::weak_ptr<Entity> > minimapUnderCursor = std::list<boost::weak_ptr<Entity> >();
			UI::Inst()->HandleUnderCursor(Coordinate(actualX, actualY), &minimapUnderCursor);
			if (!minimapUnderCursor.empty() && minimapUnderCursor.begin()->lock()) {
				for (std::list<boost::weak_ptr<Entity> >::iterator ucit = minimapUnderCursor.begin(); ucit != minimapUnderCursor.end(); ++ucit) {
					ucit->lock()->GetTooltip(actualX, actualY, tooltip);
				}
			}
		}
		if (contents) {
			contents->GetTooltip(x - (leftX + 1), y - (topY + 14), tooltip);
		}
	}
}

void SideBar::SetEntity(boost::weak_ptr<Entity> ent) {
	entity = ent;
	npc = construction = false;
	height = 15;
	contents.reset();
	if (boost::shared_ptr<NPC> npci = boost::dynamic_pointer_cast<NPC>(entity.lock())) {
		height = 30;
		npc = true;
		contents = boost::shared_ptr<Drawable>(new UIContainer(std::vector<Drawable *>(), 0, 0, width - 2, 15));
		boost::shared_ptr<UIContainer> container = boost::dynamic_pointer_cast<UIContainer>(contents);
		Frame *frame = new Frame("Effects", std::vector<Drawable *>(), 0, 0, width - 2, 12);
		frame->AddComponent(new UIList<StatusEffect, std::list<StatusEffect> >(boost::dynamic_pointer_cast<NPC>(entity.lock())->StatusEffects(), 1, 1, width - 4, 10,
																			   SideBar::DrawStatusEffect));
		container->AddComponent(frame);
		boost::function<std::string()> func = boost::bind(&SideBar::NPCSquadLabel, npci.get());
		container->AddComponent(new LiveLabel(boost::bind(&SideBar::NPCSquadLabel, npci.get()), 0, 12, TCOD_LEFT));
		container->AddComponent(new LiveLabel(boost::bind(&SideBar::NPCWeaponLabel, npci.get()), 0, 13, TCOD_LEFT));
		container->AddComponent(new LiveLabel(boost::bind(&SideBar::NPCArmorLabel, npci.get()), 0, 14, TCOD_LEFT));
	} else if (boost::shared_ptr<FarmPlot> fp = boost::dynamic_pointer_cast<FarmPlot>(entity.lock())) {
		height = 30;
		construction = true;
		contents = boost::shared_ptr<Drawable>(new UIContainer(std::vector<Drawable *>(), 0, 0, width - 2, 12));
		boost::shared_ptr<UIContainer> container = boost::dynamic_pointer_cast<UIContainer>(contents);
		container->AddComponent(new ScrollPanel(0, 0, width - 2, 15,
			new UIList<std::pair<ItemType, bool>, std::map<ItemType, bool> >(fp->AllowedSeeds(), 0, 0, width - 2, fp->AllowedSeeds()->size(),
												SideBar::DrawSeed,
												boost::bind(&FarmPlot::SwitchAllowed, fp.get(), _1))));
	} else if (boost::dynamic_pointer_cast<Construction>(entity.lock())) {
		boost::shared_ptr<Construction> construct(boost::static_pointer_cast<Construction>(entity.lock()));
		if (construct->HasTag(WORKSHOP)) {
			height = 30;
			contents = boost::shared_ptr<Drawable>(new UIContainer(std::vector<Drawable *>(), 0, 0, width - 2, 12));
			boost::shared_ptr<UIContainer> container = boost::dynamic_pointer_cast<UIContainer>(contents);
			Frame *frame = new Frame("Production", std::vector<Drawable *>(), 0, 0, width - 2, 12);
			frame->AddComponent(new UIList<ItemType, std::deque<ItemType> >(construct->JobList(), 1, 1, width - 4, 10,
																			ConstructionDialog::DrawJob));
			container->AddComponent(frame);			
		}
		construction = true;
	}
}

void SideBar::DrawStatusEffect(StatusEffect effect, int i, int x, int y, int width, bool selected, TCODConsole *console) {
	console->setDefaultForeground(effect.color);
	console->print(x, y, "%c%s", effect.graphic, effect.name.c_str());
	console->setDefaultForeground(TCODColor::white);
}

void SideBar::DrawSeed(std::pair<ItemType, bool> seed, int i, int x, int y, int width, bool selected, TCODConsole *console) {
	console->setDefaultForeground(seed.second ? TCODColor::green : TCODColor::red);
	console->print(x, y, "%c %s", seed.second ? 225 : 224, Item::Presets[seed.first].name.substr(0, width-3).c_str());
	console->setDefaultForeground(TCODColor::white);
}

std::string SideBar::NPCSquadLabel(NPC *npc) {
	if(npc->MemberOf().lock()) {
		return boost::str(boost::format("S: %s") % npc->MemberOf().lock()->Name());
	} else {
		return "";
	}
}

std::string SideBar::NPCWeaponLabel(NPC *npc) {
	if(npc->Wielding().lock()) {
		return boost::str(boost::format("W: %s") % npc->Wielding().lock()->Name());
	} else {
		return "";
	}
}

std::string SideBar::NPCArmorLabel(NPC *npc) {
	if(npc->Wearing().lock()) {
		return boost::str(boost::format("A: %s") % npc->Wearing().lock()->Name());
	} else {
		return "";
	}
}
