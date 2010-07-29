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
#pragma once

#include <string>
#include <vector>

#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/weak_ptr.hpp>
#include <libtcod.hpp>

#include "Game.hpp"
#include "UIComponents.hpp"

class MenuChoice {
public:
	MenuChoice(std::string = "", boost::function<void()> = boost::bind(Game::DoNothing));
	std::string label;
	boost::function<void()> callback;
};

class Menu: public Panel {
private:
    static std::map<std::string, Menu *> constructionCategoryMenus;
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

	static void YesNoDialog(std::string text, boost::function<void()> leftAction, boost::function<void()> rightAction,
                            std::string leftButton = "Yes", std::string rightButton = "No");
	static ItemCategory WeaponChoiceDialog();
};

class JobMenu : public Scrollable {
public:
	JobMenu() {}
	void Draw(int, int, int, int, int, TCODConsole*);
    int TotalHeight();
	static Dialog* jobListingMenu;
	static Dialog* JobListingMenu();
};

class AnnounceMenu : public Scrollable {
public:
	AnnounceMenu() {}
	void Draw(int, int, int, int, int, TCODConsole*);
    int TotalHeight();
	static Dialog* announcementsMenu;
	static Dialog* AnnouncementsMenu();
};

class NPCMenu : public Scrollable {
public:
	NPCMenu() {}
	void Draw(int, int, int, int, int, TCODConsole*);
    int TotalHeight();
	static Dialog* npcListMenu;
	static Dialog* NPCListMenu();
};

class ConstructionMenu : public Menu {
private:
	Construction* construct;
	int scroll;
	std::vector<int> productPlacement;
	bool firstTimeDraw;
public:
	ConstructionMenu();
	void Draw(int, int, TCODConsole*);
    MenuResult Update(int, int, bool, TCOD_key_t);
	static ConstructionMenu* constructionInfoMenu;
	static ConstructionMenu* ConstructionInfoMenu(Construction*);
	void Construct(Construction*);
	void ScrollDown();
	void ScrollUp();
	void ClearProductPlacement();
};

class StockManagerMenu : public Menu {
private:
	int scroll;
	std::string filter;
public:
	StockManagerMenu();
	void Draw(int, int, TCODConsole*);
    MenuResult Update(int, int, bool, TCOD_key_t);
	static StockManagerMenu* stocksMenu;
	static StockManagerMenu* StocksMenu();
	void ScrollDown();
	void ScrollUp();
	void Open();
};

class SquadsMenu : public Menu {
private:
	std::string squadName;
	int squadMembers;
	int squadPriority;
	boost::weak_ptr<Squad> chosenSquad;
public:
	SquadsMenu();
	void Draw(int, int, TCODConsole*);
    MenuResult Update(int, int, bool, TCOD_key_t);
	static SquadsMenu* squadMenu;
	static SquadsMenu* SquadMenu();
	void Open();
};