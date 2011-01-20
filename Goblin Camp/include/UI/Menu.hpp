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
#pragma once

#include <string>
#include <vector>

#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/weak_ptr.hpp>
#include <libtcod.hpp>

#include "Game.hpp"
#include "UIComponents.hpp"
#include "Grid.hpp"
#include "Dialog.hpp"
#include "UIList.hpp"
#include "Frame.hpp"

class MenuChoice {
public:
	MenuChoice(std::string label = "", boost::function<void()> = boost::bind(Game::DoNothing), bool = true,
		std::string tooltip = "");
	std::string label;
	boost::function<void()> callback;
	bool enabled;
	std::string tooltip;
};

class Menu: public Panel {
private:
	static std::map<std::string, Menu *> constructionCategoryMenus;
	static int menuTier;
protected:
	std::vector<MenuChoice> choices;
	int _selected;
	std::string title;
	void CalculateSize();
public:
	Menu(std::vector<MenuChoice>, std::string="");
	virtual ~Menu();
	virtual void Draw(int, int, TCODConsole*);
	virtual MenuResult Update(int, int, bool, TCOD_key_t);
	void selected(int);
	void AddChoice(MenuChoice);
	void Callback(unsigned int);

	static Menu* mainMenu;
	static Menu* MainMenu();
	static Menu* constructionMenu;
	static Menu* ConstructionMenu();
	static Menu* basicsMenu;
	static Menu* BasicsMenu();
	static Menu* WorkshopsMenu();
	static Menu* ordersMenu;
	static Menu* OrdersMenu();
	static Menu* FurnitureMenu();
	static Menu* ConstructionCategoryMenu(std::string);
	static Menu* devMenu;
	static Menu* DevMenu();
	static Menu* territoryMenu;
	static Menu* TerritoryMenu();

	static ItemCategory WeaponChoiceDialog();
	void GetTooltip(int, int, Tooltip *);
};
