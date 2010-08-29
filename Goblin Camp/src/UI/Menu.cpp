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

MenuChoice::MenuChoice(std::string ntext, boost::function<void()> cb) {
	label = ntext;
	callback = cb;
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
	if (x + width >= console->getWidth()) x = console->getWidth() - width - 1;
	if (y + height >= console->getHeight()) y = console->getHeight() - height - 1;
	_x = x; _y = y; //Save coordinates of menu top-left corner
	console->printFrame(x, y, width, height, true, TCOD_BKGND_SET, title.empty() ? 0 : title.c_str());
	console->setBackgroundFlag(TCOD_BKGND_SET);
	//Draw the menu entries
	for (int i = 0; i < (signed int)choices.size(); ++i) {
		console->setBackgroundColor(TCODColor::black);
		if (UI::Inst()->KeyHelpTextColor() > 0) {
			console->setForegroundColor(TCODColor(0,UI::Inst()->KeyHelpTextColor(),0));
			console->print(x, y+1+(i*2), boost::lexical_cast<std::string>(i+1).c_str());
		}
		console->setForegroundColor(TCODColor::white);
		if (_selected == i) {
			console->setBackgroundColor(TCODColor::white);
			console->setForegroundColor(TCODColor::darkerGrey);
		}
		console->print(x+1, y+1+(i*2), choices[i].label.c_str());
	}
	console->setForegroundColor(TCODColor::white);
	console->setBackgroundColor(TCODColor::black);
}

MenuResult Menu::Update(int x, int y, bool clicked, TCOD_key_t key) {
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
				if (clicked) {
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

Menu* Menu::mainMenu = 0;
Menu* Menu::MainMenu() {
	if (!mainMenu) {
		mainMenu = new Menu(std::vector<MenuChoice>());
		mainMenu->AddChoice(MenuChoice("Construction", boost::bind(UI::ChangeMenu, Menu::ConstructionMenu())));
		mainMenu->AddChoice(MenuChoice("Orders", boost::bind(UI::ChangeMenu, Menu::OrdersMenu())));
		mainMenu->AddChoice(MenuChoice("Stock Manager", boost::bind(UI::ChangeMenu, StockManagerDialog::StocksDialog())));
#ifdef DEBUG
		mainMenu->AddChoice(MenuChoice("Jobs", boost::bind(UI::ChangeMenu, JobDialog::JobListingDialog())));
		mainMenu->AddChoice(MenuChoice("NPC List", boost::bind(UI::ChangeMenu, NPCDialog::NPCListDialog())));
#endif
		mainMenu->AddChoice(MenuChoice("Announcements", boost::bind(UI::ChangeMenu, AnnounceDialog::AnnouncementsDialog())));
		mainMenu->AddChoice(MenuChoice("Squads", boost::bind(UI::ChangeMenu, SquadsDialog::SquadDialog())));
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
        Menu *basicsMenu = ConstructionCategoryMenu("Basics");
        if(basicsMenu) {
            basicsMenu->AddChoice(MenuChoice("Dismantle", boost::bind(UI::ChooseDismantle)));
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
            if (boost::iequals(preset.category, category)) {
                if(preset.tags[STOCKPILE] || preset.tags[FARMPLOT]) {
                    menu->AddChoice(MenuChoice(preset.name, boost::bind(UI::ChooseStockpile, i)));
                } else {
                    UIState placementType = UIPLACEMENT;
                    if(preset.placementType > 0 && preset.placementType < UICOUNT) {
                        placementType = (UIState)preset.placementType;
                    }
                    menu->AddChoice(MenuChoice(preset.name, boost::bind(UI::ChooseConstruct, i, placementType)));
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
	return ConstructionCategoryMenu("basics");
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
		ordersMenu->AddChoice(MenuChoice("Fell trees", boost::bind(UI::ChooseTreeFelling)));
		ordersMenu->AddChoice(MenuChoice("Designate trees", boost::bind(UI::ChooseDesignateTree)));
		ordersMenu->AddChoice(MenuChoice("Harvest wild plants", boost::bind(UI::ChoosePlantHarvest)));
		ordersMenu->AddChoice(MenuChoice("Designate bog for iron", boost::bind(UI::ChooseDesignateBog)));
		ordersMenu->AddChoice(MenuChoice("Undesignate", boost::bind(UI::ChooseUndesignate)));
	}
	return ordersMenu;
}

