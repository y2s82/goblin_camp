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

#include "UI/Menu.hpp"
#include "UI.hpp"
#include "UI/UIComponents.hpp"
#include "UI/JobDialog.hpp"
#include "UI/AnnounceDialog.hpp"
#include "UI/StockManagerDialog.hpp"
#include "UI/SquadsDialog.hpp"
#include "UI/NPCDialog.hpp"
#include "Camp.hpp"
#include "Map.hpp"
#include "Weather.hpp"

MenuChoice::MenuChoice(std::string ntext, boost::function<void()> cb, bool nenabled, std::string ntooltip) {
	label = ntext;
	callback = cb;
	enabled = nenabled;
	tooltip = ntooltip;
}

Menu::Menu(std::vector<MenuChoice> newChoices, std::string ntitle): Panel(0, 0) {
	_selected = -1;
	choices = newChoices;
	title = ntitle;
	CalculateSize();
}

Menu::~Menu() {}

void Menu::CalculateSize() {
	height = choices.size()*2+1;
	width = 0;
	for (std::vector<MenuChoice>::iterator iter = choices.begin(); iter != choices.end(); ++iter) {
		if ((signed int)iter->label.length() > width) width = (signed int)iter->label.length();
	}
	width += 2;
}

void Menu::Draw(int x, int y, TCODConsole* console) {
	console->setAlignment(TCOD_LEFT);
	//Draw the box
	if (x + width >= console->getWidth()) x = std::max(console->getWidth() - width - 1,0);
	if (y + height >= console->getHeight()) y = std::max(console->getHeight() - height - 1,0);
	_x = x; _y = y; //Save coordinates of menu top-left corner
	console->printFrame(x, y, width, height, true, TCOD_BKGND_SET, title.empty() ? 0 : title.c_str());
	console->setBackgroundFlag(TCOD_BKGND_SET);
	//Draw the menu entries
	for (int i = 0; i < (signed int)choices.size(); ++i) {
		console->setDefaultBackground(TCODColor::black);
		if (UI::Inst()->KeyHelpTextColor() > 0) {
			console->setDefaultForeground(TCODColor(0,std::min(255, UI::Inst()->KeyHelpTextColor()),0));
			console->print(x, y+1+(i*2), boost::lexical_cast<std::string>(i+1).c_str());
		}
		if (choices[i].enabled) console->setDefaultForeground(TCODColor::white);
		else console->setDefaultForeground(TCODColor::grey);
		if (_selected == i) {
			console->setDefaultBackground(TCODColor::white);
			console->setDefaultForeground(TCODColor::darkerGrey);
		}
		console->print(x+1, y+1+(i*2), choices[i].label.c_str());
	}
	console->setDefaultForeground(TCODColor::white);
	console->setDefaultBackground(TCODColor::black);
}

MenuResult Menu::Update(int x, int y, bool clicked, const TCOD_key_t key) {
	if (key.c >= '0' && key.c <= '9') {
		selected(boost::lexical_cast<int>((char)key.c)-1);
		Callback(boost::lexical_cast<int>((char)key.c)-1);
	}
	if (x > 0 && y > 0) {
		if (x > _x && x < _x + width) {
			y -= _y;
			if (y > 0 && y < height) {
				--y;
				if (y > 0) y /= 2;
				_selected = y;
				if (clicked && choices[y].enabled) {
					choices[y].callback();
					return (MenuResult) (DISMISS | MENUHIT);
				} 
				return MENUHIT;
			}
		}
	}
	return NOMENUHIT;
}

void Menu::selected(int newSel) {
	_selected = newSel;
}

void Menu::AddChoice(MenuChoice newChoice) { 
	choices.push_back(newChoice); 
	CalculateSize();
}

void Menu::Callback(unsigned int choice) {
	if (choices.size() > choice) {
		choices[choice].callback();
	}
}

int Menu::menuTier = 0;
Menu* Menu::mainMenu = 0;
Menu* Menu::MainMenu() {
	if (!mainMenu || menuTier != Camp::Inst()->GetTier()) {
		if (mainMenu) { //Tier has changed
			delete mainMenu;
			mainMenu = 0;
			if (constructionMenu) { delete constructionMenu; constructionMenu = 0; }
			if (!constructionCategoryMenus.empty()) {
				for (std::map<std::string, Menu *>::iterator iterator = constructionCategoryMenus.begin();
					iterator != constructionCategoryMenus.end(); ++iterator) {
						if (iterator->second) delete iterator->second;
				}
				constructionCategoryMenus.clear();
			}
			menuTier = Camp::Inst()->GetTier();
		}
		mainMenu = new Menu(std::vector<MenuChoice>());
		if (Game::Inst()->DevMode()) mainMenu->AddChoice(MenuChoice("Dev", boost::bind(UI::ChangeMenu, Menu::DevMenu())));
		mainMenu->AddChoice(MenuChoice("Build", boost::bind(UI::ChangeMenu, Menu::ConstructionMenu())));
		mainMenu->AddChoice(MenuChoice("Dismantle", boost::bind(UI::ChooseDismantle)));		
		mainMenu->AddChoice(MenuChoice("Orders", boost::bind(UI::ChangeMenu, Menu::OrdersMenu())));
		mainMenu->AddChoice(MenuChoice("Stock Manager", boost::bind(UI::ChangeMenu, StockManagerDialog::StocksDialog())));
		mainMenu->AddChoice(MenuChoice("Jobs", boost::bind(UI::ChangeMenu, JobDialog::JobListingDialog())));
#ifdef DEBUG
		mainMenu->AddChoice(MenuChoice("NPC List", boost::bind(UI::ChangeMenu, NPCDialog::NPCListDialog())));
#endif
		mainMenu->AddChoice(MenuChoice("Announcements", boost::bind(UI::ChangeMenu, AnnounceDialog::AnnouncementsDialog())));
		mainMenu->AddChoice(MenuChoice("Squads", boost::bind(UI::ChangeMenu, SquadsDialog::SquadDialog())));
		mainMenu->AddChoice(MenuChoice("Territory", boost::bind(UI::ChangeMenu, Menu::TerritoryMenu())));
		mainMenu->AddChoice(MenuChoice("Stats", boost::bind(&Game::DisplayStats, Game::Inst())));
		mainMenu->AddChoice(MenuChoice("Main Menu", boost::bind(Game::ToMainMenu, true)));
		mainMenu->AddChoice(MenuChoice("Quit", boost::bind(Game::Exit, true)));
	}
	return mainMenu;
}

Menu* Menu::constructionMenu = 0;
Menu* Menu::ConstructionMenu() {
	if (!constructionMenu) {
		constructionMenu = new Menu(std::vector<MenuChoice>());
		for(std::set<std::string>::iterator it = Construction::Categories.begin(); it != Construction::Categories.end(); it++) {
			constructionMenu->AddChoice(MenuChoice(*it, boost::bind(UI::ChangeMenu, Menu::ConstructionCategoryMenu(*it))));
		}
	}
	return constructionMenu;
}

std::map<std::string, Menu *> Menu::constructionCategoryMenus = std::map<std::string, Menu *>();

Menu* Menu::ConstructionCategoryMenu(std::string category) {
	std::map<std::string, Menu *>::iterator found = constructionCategoryMenus.find(category);
	Menu *menu;
	if(found == constructionCategoryMenus.end()) {
		menu = new Menu(std::vector<MenuChoice>());
		for (int i = 0; i < (signed int)Construction::Presets.size(); ++i) {
			ConstructionPreset preset = Construction::Presets[i];
			if (boost::iequals(preset.category, category) && preset.tier <= Camp::Inst()->GetTier() + 1) {
				if(preset.tags[STOCKPILE] || preset.tags[FARMPLOT]) {
					menu->AddChoice(MenuChoice(preset.name, boost::bind(UI::ChooseStockpile, i), preset.tier <= Camp::Inst()->GetTier(), preset.description));
				} else {
					UIState placementType = UIPLACEMENT;
					if(preset.placementType > 0 && preset.placementType < UICOUNT) {
						placementType = (UIState)preset.placementType;
					}
					menu->AddChoice(MenuChoice(preset.name, boost::bind(UI::ChooseConstruct, i, placementType), preset.tier <= Camp::Inst()->GetTier(), preset.description));
				}
			}
		}
		constructionCategoryMenus[category] = menu;
	} else {
		menu = found->second;
	}
	return menu;
}

Menu* Menu::BasicsMenu() {
	return ConstructionCategoryMenu("Basics");
}

Menu* Menu::WorkshopsMenu() {
	return ConstructionCategoryMenu("Workshops");
}

Menu* Menu::FurnitureMenu() {
	return ConstructionCategoryMenu("Furniture");
}

Menu* Menu::ordersMenu = 0;
Menu* Menu::OrdersMenu() {
	if (!ordersMenu) {
		ordersMenu = new Menu(std::vector<MenuChoice>());

		boost::function<bool(Coordinate, Coordinate)> checkDitch = boost::bind(Game::CheckTileType, TILEDITCH, _1, _2);
		boost::function<void(Coordinate, Coordinate)> rectCall = boost::bind(Game::FillDitch, _1, _2);

		ordersMenu->AddChoice(MenuChoice("Fell trees", boost::bind(UI::ChooseTreeFelling)));
		ordersMenu->AddChoice(MenuChoice("Designate trees", boost::bind(UI::ChooseDesignateTree)));
		ordersMenu->AddChoice(MenuChoice("Harvest wild plants", boost::bind(UI::ChoosePlantHarvest)));
		ordersMenu->AddChoice(MenuChoice("Dig", boost::bind(UI::ChooseDig)));
		ordersMenu->AddChoice(MenuChoice("Fill ditches", boost::bind(UI::ChooseRectPlacement, rectCall, checkDitch, 178, "Fill ditches")));
		ordersMenu->AddChoice(MenuChoice("Designate bog for iron", boost::bind(UI::ChooseDesignateBog)));
		ordersMenu->AddChoice(MenuChoice("Gather items", boost::bind(UI::ChooseGatherItems)));

		boost::function<bool(Coordinate, Coordinate)> checkTree = boost::bind(Game::CheckTree, _1, Coordinate(1,1));
		rectCall = boost::bind(&Camp::AddWaterZone, Camp::Inst(), _1, _2);
		ordersMenu->AddChoice(MenuChoice("Pour water", boost::bind(UI::ChooseRectPlacement, rectCall, checkTree, 'W', "Pour water")));

		boost::function<void(Coordinate)> call = boost::bind(&Game::StartFire, Game::Inst(), _1);
		ordersMenu->AddChoice(MenuChoice("Start fire", boost::bind(UI::ChooseNormalPlacement, call, checkTree, 'F', "Start fire")));

		ordersMenu->AddChoice(MenuChoice("Undesignate", boost::bind(UI::ChooseUndesignate)));
	}
	return ordersMenu;
}

Menu* Menu::devMenu = 0;
Menu* Menu::DevMenu() {
	if (!devMenu) {
		devMenu = new Menu(std::vector<MenuChoice>());

		devMenu->AddChoice(MenuChoice("Create NPC", boost::bind(UI::ChooseCreateNPC)));
		devMenu->AddChoice(MenuChoice("Create item", boost::bind(UI::ChooseCreateItem)));

		boost::function<bool(Coordinate, Coordinate)> checkTree = boost::bind(Game::CheckTree, _1, Coordinate(1,1));
		boost::function<void(Coordinate)> call = boost::bind(&Game::CreateFilth, Game::Inst(), _1, 100);
		devMenu->AddChoice(MenuChoice("Create filth", boost::bind(UI::ChooseNormalPlacement, call, checkTree, '~', "Filth")));
		
		call = boost::bind(&Game::CreateWater, Game::Inst(), _1);
		devMenu->AddChoice(MenuChoice("Create water", boost::bind(UI::ChooseNormalPlacement, call, checkTree, '~', "Water")));
		
		call = boost::bind(&Map::Corrupt, Map::Inst(), _1, 500000);
		devMenu->AddChoice(MenuChoice("Corrupt", boost::bind(UI::ChooseNormalPlacement, call, checkTree, 'C', "Corrupt")));

		devMenu->AddChoice(MenuChoice("Naturify world", boost::bind(UI::ChooseNaturify)));

		boost::function<void(Coordinate, Coordinate)> rectCall = boost::bind(&Game::RemoveNatureObject, Game::Inst(), _1, _2);
		devMenu->AddChoice(MenuChoice("Remove NatureObjects", boost::bind(UI::ChooseRectPlacement, rectCall, checkTree, 'R', "Remove NatureObjects")));
		devMenu->AddChoice(MenuChoice("Trigger attack", boost::bind(&Game::TriggerAttack, Game::Inst())));
		devMenu->AddChoice(MenuChoice("Trigger migration", boost::bind(&Game::TriggerMigration, Game::Inst())));
		
		call = boost::bind(&Game::Damage, Game::Inst(), _1);
		devMenu->AddChoice(MenuChoice("Explode", boost::bind(UI::ChooseNormalPlacement, call, checkTree, 'E', "Explode")));
		
		call = boost::bind(&Game::Hungerize, Game::Inst(), _1);
		devMenu->AddChoice(MenuChoice("Hungerize", boost::bind(UI::ChooseNormalPlacement, call, checkTree, 'H', "Hunger")));

		call = boost::bind(&Game::Tire, Game::Inst(), _1);
		devMenu->AddChoice(MenuChoice("Tire", boost::bind(UI::ChooseNormalPlacement, call, checkTree, 'T', "Tire")));

		call = boost::bind(&Game::CreateFire, Game::Inst(), _1);
		devMenu->AddChoice(MenuChoice("Fire", boost::bind(UI::ChooseNormalPlacement, call, checkTree, '!', "Fire")));

		call = boost::bind(&Game::CreateDitch, Game::Inst(), _1);
		devMenu->AddChoice(MenuChoice("Dig", boost::bind(UI::ChooseABPlacement, call, checkTree, '_', "Dig")));

		call = boost::bind(&Game::Thirstify, Game::Inst(), _1);
		devMenu->AddChoice(MenuChoice("Thirstify", boost::bind(UI::ChooseNormalPlacement, call, checkTree, 'T', "Thirst")));
		call = boost::bind(&Game::Badsleepify, Game::Inst(), _1);
		devMenu->AddChoice(MenuChoice("Badsleepify", boost::bind(UI::ChooseNormalPlacement, call, checkTree, 'T', "Bad sleep")));

		call = boost::bind(&Game::Diseasify, Game::Inst(), _1);
		devMenu->AddChoice(MenuChoice("Diseasify", boost::bind(UI::ChooseNormalPlacement, call, checkTree, 'D', "Disease")));
	}
	return devMenu;
}

void Menu::GetTooltip(int x, int y, Tooltip *tooltip) {
	if (x > 0 && y > 0) {
		if (x > _x && x < _x + width) {
			y -= _y;
			if (y > 0 && y < height) {
				--y;
				if (y > 0) y /= 2;
				_selected = y;
				if (y < (signed int)choices.size() && choices[y].tooltip != "") {
					tooltip->OffsetPosition((_x + width) - x - 1, 0);
					for (unsigned int i = 0; i < choices[y].tooltip.length(); i += 25) {
						tooltip->AddEntry(TooltipEntry(choices[y].tooltip.substr(i, 25), TCODColor::white));
					}
				}
			}
		}
	}
}

Menu* Menu::territoryMenu = 0;
Menu* Menu::TerritoryMenu() {
	if (!territoryMenu) {
		territoryMenu = new Menu(std::vector<MenuChoice>());
		territoryMenu->AddChoice(MenuChoice("Toggle territory overlay", boost::bind(&Map::ToggleOverlay, Map::Inst(), TERRITORY_OVERLAY)));
		territoryMenu->AddChoice(MenuChoice("Expand territory", boost::bind(UI::ChooseChangeTerritory, true)));
		territoryMenu->AddChoice(MenuChoice("Shrink territory", boost::bind(UI::ChooseChangeTerritory, false)));
		territoryMenu->AddChoice(MenuChoice("Toggle automatic territory", boost::bind(&Camp::ToggleAutoTerritory, Camp::Inst())));
	}
	return territoryMenu;
}
