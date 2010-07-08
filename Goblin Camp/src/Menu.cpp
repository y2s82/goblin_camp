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

#include <libtcod.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/scope_exit.hpp>
#include <boost/algorithm/string.hpp>
#include <string>

#include "Menu.hpp"
#include "UI.hpp"
#include "Announce.hpp"
#include "StockManager.hpp"
#include "JobManager.hpp"

MenuChoice::MenuChoice(std::string ntext, boost::function<void()> cb) {
	label = ntext;
	callback = cb;
}

Menu::Menu(std::vector<MenuChoice> newChoices) {
	_selected = -1;
	choices = newChoices;
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
	//Draw the box
	if (x + width >= console->getWidth()) x = console->getWidth() - width - 1;
	if (y + height >= console->getHeight()) y = console->getHeight() - height - 1;
	topX = x; topY = y; //Save coordinates of menu top-left corner
	console->printFrame(x, y, width, height, true, TCOD_BKGND_SET, 0);
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

MenuResult Menu::Update(int x, int y, bool clicked) {
	if (x > 0 && y > 0) {
		if (x > topX && x < topX + width) {
			y -= topY;
			if (y > 0 && y < height) {
				--y;
				if (y > 0) y /= 2;
				_selected = y;
				if (clicked) choices[y].callback();
				return MENUHIT; //Mouse was inside menu when clicked
			}
		}
	}
	return NOMENUHIT; //Mouse was not inside menu when clicked
}

void Menu::selected(int newSel) { _selected = newSel; }
void Menu::AddChoice(MenuChoice newChoice) { choices.push_back(newChoice); CalculateSize(); }

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
		mainMenu->AddChoice(MenuChoice("Stock Manager", boost::bind(UI::ChangeMenu, StockManagerMenu::StocksMenu())));
#ifdef DEBUG
		mainMenu->AddChoice(MenuChoice("Jobs", boost::bind(UI::ChangeMenu, JobMenu::JobListingMenu())));
		mainMenu->AddChoice(MenuChoice("Announcements", boost::bind(UI::ChangeMenu, AnnounceMenu::AnnouncementsMenu())));
		mainMenu->AddChoice(MenuChoice("NPC List", boost::bind(UI::ChangeMenu, NPCMenu::NPCListMenu())));
#endif
		mainMenu->AddChoice(MenuChoice("Squads", boost::bind(UI::ChangeMenu, SquadsMenu::SquadMenu())));
		mainMenu->AddChoice(MenuChoice("Main Menu", boost::bind(Game::ToMainMenu)));
		mainMenu->AddChoice(MenuChoice("Quit", boost::bind(Game::Exit)));
	}
	return mainMenu;
}

Menu* Menu::constructionMenu = 0;
Menu* Menu::ConstructionMenu() {
	if (!constructionMenu) {
		constructionMenu = new Menu(std::vector<MenuChoice>());
		constructionMenu->AddChoice(MenuChoice(std::string("Basics"), boost::bind(UI::ChangeMenu, Menu::BasicsMenu())));
		constructionMenu->AddChoice(MenuChoice(std::string("Workshops"), boost::bind(UI::ChangeMenu, Menu::WorkshopsMenu())));
	}
	return constructionMenu;
}

Menu* Menu::basicsMenu = 0;
Menu* Menu::BasicsMenu() {
	if (!basicsMenu) {
		basicsMenu = new Menu(std::vector<MenuChoice>());
		for (int i = 0; i < (signed int)Construction::Presets.size(); ++i) {
		    if (Construction::Presets[i].stockpile) {
                basicsMenu->AddChoice(MenuChoice(Construction::Presets[i].name, boost::bind(UI::ChooseStockpile, i)));
		    }
		}
		for (int i = 0; i < (signed int)Construction::Presets.size(); ++i) {
		    if (Construction::Presets[i].farmPlot) {
                basicsMenu->AddChoice(MenuChoice(Construction::Presets[i].name, boost::bind(UI::ChooseStockpile, i)));
		    }
		}
		for (int i = 0; i < (signed int)Construction::Presets.size(); ++i) {
		    if (Construction::Presets[i].wall) {
                basicsMenu->AddChoice(MenuChoice(Construction::Presets[i].name, boost::bind(UI::ChooseConstruct, i, UIABPLACEMENT)));
		    }
		}
	}
	return basicsMenu;
}

Menu* Menu::workshopsMenu = 0;
Menu* Menu::WorkshopsMenu() {
	if (!workshopsMenu) {
		workshopsMenu = new Menu(std::vector<MenuChoice>());
		for (int i = 0; i < (signed int)Construction::Presets.size(); ++i) {
		    if (!Construction::Presets[i].wall && !Construction::Presets[i].stockpile && !Construction::Presets[i].farmPlot) {
                workshopsMenu->AddChoice(MenuChoice(Construction::Presets[i].name, boost::bind(UI::ChooseConstruct, i, UIPLACEMENT)));
		    }
		}
	}
	return workshopsMenu;
}

Menu* Menu::ordersMenu = 0;
Menu* Menu::OrdersMenu() {
    if (!ordersMenu) {
        ordersMenu = new Menu(std::vector<MenuChoice>());
        ordersMenu->AddChoice(MenuChoice("Fell trees", boost::bind(UI::ChooseTreeFelling)));
        ordersMenu->AddChoice(MenuChoice("Designate trees", boost::bind(UI::ChooseDesignateTree)));
		ordersMenu->AddChoice(MenuChoice("Harvest wild plants", boost::bind(UI::ChoosePlantHarvest)));
    }
    return ordersMenu;
}

JobMenu::JobMenu() : Menu(std::vector<MenuChoice>()),
	scroll(0)
{
	width = Game::Inst()->ScreenWidth() - 20;
	height = Game::Inst()->ScreenHeight() - 20;
	topX = 10; topY = 10;
}

void JobMenu::Draw(int x, int y, TCODConsole* console) {
	if (scroll + height - 2 > JobManager::Inst()->JobAmount()) scroll = std::max(0, JobManager::Inst()->JobAmount() - height - 2);

	int scrollBar = 0;
	scrollBar = (int)((height-3) * ((double)scroll / (double)std::max(1, JobManager::Inst()->JobAmount() - height-2)));
	scrollBar += topY+2;
	scrollBar = std::min(scrollBar, topY+height-4);

	console->printFrame(topX, topY, width, height, true, TCOD_BKGND_SET, "Jobs");

	JobManager::Inst()->Draw(Coordinate(topX+1,topY+1), scroll, height-2, console);
	console->putChar(topX+width-2, topY+1, TCOD_CHAR_ARROW_N, TCOD_BKGND_SET);
	console->putChar(topX+width-2, topY+height-2, TCOD_CHAR_ARROW_S, TCOD_BKGND_SET);
	console->putChar(topX+width-2, scrollBar, 219, TCOD_BKGND_SET);

	for (int y = topY+1; y <= topY+height-2; ++y) {
		boost::weak_ptr<Job> job(JobManager::Inst()->GetJobByListIndex(y - (topY+1 + scroll)));
		if (job.lock()) {
            if (job.lock()->Paused()) {
                console->print(topX+width-15, y, "P");
            }
                console->print(topX+width-18, y, "A-> %d", job.lock()->Assigned());
		}
	}
}

MenuResult JobMenu::Update(int x, int y, bool clicked) {
	if (x > topX && x < Game::Inst()->ScreenWidth()-10 && y > topY && y < Game::Inst()->ScreenHeight()-10) {
		if (clicked) {
			if (x == topX+width-2 && y == topY+1) ScrollUp();
			if (x == topX+width-2 && y == topY+height-2) ScrollDown();
		}
		return MENUHIT;
	}
	return NOMENUHIT;
}

JobMenu* JobMenu::jobListingMenu = 0;
JobMenu* JobMenu::JobListingMenu() {
	if (!jobListingMenu) {
		jobListingMenu = new JobMenu();
	}
	return jobListingMenu;
}

void JobMenu::ScrollUp() { if (scroll > 0) --scroll; }
void JobMenu::ScrollDown() { ++scroll; }

AnnounceMenu::AnnounceMenu() : Menu(std::vector<MenuChoice>()),
	scroll(0)
{
	width = Game::Inst()->ScreenWidth() - 20;
	height = Game::Inst()->ScreenHeight() - 20;
	topX = 10; topY = 10;
}

void AnnounceMenu::Draw(int x, int y, TCODConsole* console) {
	if (scroll + height - 2 > Announce::Inst()->AnnounceAmount()) scroll = std::max(0, Announce::Inst()->AnnounceAmount() - height - 2);

	int scrollBar = 0;
	scrollBar = (int)((height-3) * ((double)scroll / (double)std::max(1, Announce::Inst()->AnnounceAmount() - height-2)));
	scrollBar += topY+2;
	scrollBar = std::min(scrollBar, topY+height-4);

	console->printFrame(topX, topY, width, height, true, TCOD_BKGND_SET, "Announcements");

	Announce::Inst()->Draw(Coordinate(topX+1,topY+1), scroll, height-2, console);
	console->putChar(topX+width-2, topY+1, TCOD_CHAR_ARROW_N, TCOD_BKGND_SET);
	console->putChar(topX+width-2, topY+height-2, TCOD_CHAR_ARROW_S, TCOD_BKGND_SET);
	console->putChar(topX+width-2, scrollBar, 219, TCOD_BKGND_SET);
}

MenuResult AnnounceMenu::Update(int x, int y, bool clicked) {
	if (x > topX && x < Game::Inst()->ScreenWidth()-10 && y > topY && y < Game::Inst()->ScreenHeight()-10) {
		if (x == topX+width-2 && y == topY+1 && clicked) ScrollUp();
		if (x == topX+width-2 && y == topY+height-2 && clicked) ScrollDown();
		return MENUHIT;
	}
	return NOMENUHIT;
}

AnnounceMenu* AnnounceMenu::announcementsMenu = 0;
AnnounceMenu* AnnounceMenu::AnnouncementsMenu() {
	if (!announcementsMenu) {
		announcementsMenu = new AnnounceMenu();
	}
	return announcementsMenu;
}

void AnnounceMenu::ScrollUp() { if (scroll > 0) --scroll; }
void AnnounceMenu::ScrollDown() { ++scroll; }

NPCMenu::NPCMenu() : Menu(std::vector<MenuChoice>()),
	scroll(0)
{
	width = Game::Inst()->ScreenWidth() - 20;
	height = Game::Inst()->ScreenHeight() - 20;
	topX = 10; topY = 10;
}

void NPCMenu::Draw(int x, int y, TCODConsole* console) {
	if (scroll + height - 2 > (signed int)Game::Inst()->npcList.size()) scroll = std::max(0, (signed int)Game::Inst()->npcList.size() - height - 2);

	int scrollBar = 0;
	scrollBar = (int)((height-3) * ((double)scroll / (double)std::max(1, (signed int)Game::Inst()->npcList.size() - height-2)));
	scrollBar += topY+2;
	scrollBar = std::min(scrollBar, topY+height-4);

	console->printFrame(topX, topY, width, height, true, TCOD_BKGND_SET, "NPC List");

    int count = 0;
	for (std::map<int,boost::shared_ptr<NPC> >::iterator npci = Game::Inst()->npcList.begin(); npci != Game::Inst()->npcList.end(); ++npci) {
	    if (count++ >= scroll) {
	        console->print(topX+1, topY+1+(count-scroll), "NPC: %d", npci->second->Uid());
	        console->print(topX+10, topY+1+(count-scroll), "%s: %s",
                npci->second->currentJob().lock() ? npci->second->currentJob().lock()->name.c_str() : "No job",
                npci->second->currentTask() ? Job::ActionToString(npci->second->currentTask()->action).c_str() : "No task");
	    }
	}

	console->putChar(topX+width-2, topY+1, TCOD_CHAR_ARROW_N, TCOD_BKGND_SET);
	console->putChar(topX+width-2, topY+height-2, TCOD_CHAR_ARROW_S, TCOD_BKGND_SET);
	console->putChar(topX+width-2, scrollBar, 219, TCOD_BKGND_SET);
}

MenuResult NPCMenu::Update(int x, int y, bool clicked) {
	if (x > topX && x < Game::Inst()->ScreenWidth()-10 && y > topY && y < Game::Inst()->ScreenHeight()-10) {
		if (x == topX+width-2 && y == topY+1 && clicked) ScrollUp();
		if (x == topX+width-2 && y == topY+height-2 && clicked) ScrollDown();
		return MENUHIT;
	}
	return NOMENUHIT;
}

NPCMenu* NPCMenu::npcListMenu = 0;
NPCMenu* NPCMenu::NPCListMenu() {
	if (!npcListMenu) {
		npcListMenu = new NPCMenu();
	}
	return npcListMenu;
}

void NPCMenu::ScrollUp() { if (scroll > 0) --scroll; }
void NPCMenu::ScrollDown() { ++scroll; }

ConstructionMenu* ConstructionMenu::constructionInfoMenu = 0;

ConstructionMenu::ConstructionMenu() : Menu(std::vector<MenuChoice>()),
    construct(0),
    scroll(0),
    firstTimeDraw(true)
{
    width = 50; height = 50;
    topX = (Game::Inst()->ScreenWidth() - width) / 2;
    topY = (Game::Inst()->ScreenHeight() - height) / 2;
}

void ConstructionMenu::Draw(int, int, TCODConsole* console) {
    console->setForegroundColor(TCODColor::white);
    console->printFrame(topX, topY, 50, 5, true, TCOD_BKGND_SET, construct->Name().c_str());
    console->setForegroundColor(TCODColor::green);
    console->print(topX + 3, topY + 2, "Rename");
    console->setForegroundColor(TCODColor::red);
    console->print(topX + 13, topY + 2, "Dismantle");
    console->setForegroundColor(TCODColor::white);

    //Only draw the production queue and product list if construction is a producer
    if (construct->Producer()) {
        console->printFrame(topX+25, topY+5, 25, 50, true, TCOD_BKGND_SET, "Product list");
        console->printFrame(topX, topY+5, 25, 50, true, TCOD_BKGND_SET, "Production queue");
        console->putChar(topX+25, topY+10, TCOD_CHAR_ARROW2_W, TCOD_BKGND_SET);
        console->putChar(topX+25, topY+25, TCOD_CHAR_ARROW2_W, TCOD_BKGND_SET);
        console->putChar(topX+25, topY+40, TCOD_CHAR_ARROW2_W, TCOD_BKGND_SET);
        console->putChar(topX+24, topY+10, TCOD_CHAR_ARROW2_W, TCOD_BKGND_SET);
        console->putChar(topX+24, topY+25, TCOD_CHAR_ARROW2_W, TCOD_BKGND_SET);
        console->putChar(topX+24, topY+40, TCOD_CHAR_ARROW2_W, TCOD_BKGND_SET);

        console->putChar(topX+48, topY+6, TCOD_CHAR_ARROW_N, TCOD_BKGND_SET);
        console->putChar(topX+48, topY+53, TCOD_CHAR_ARROW_S, TCOD_BKGND_SET);

        int y = 0;
        for (int prodi = scroll; prodi < (signed int)construct->Products()->size() && y < topY+50; ++prodi) {
            console->setForegroundColor(TCODColor::white);
            console->print(topX+26, topY+6+y, "%s x%d", Item::ItemTypeToString(construct->Products(prodi)).c_str(), Item::Presets[construct->Products(prodi)].multiplier);
            if (firstTimeDraw) productPlacement.push_back(topY+6+y);
            ++y;
            for (int compi = 0; compi < (signed int)Item::Components(construct->Products(prodi)).size() && y < topY+50; ++compi) {
                console->setForegroundColor(TCODColor::white);
                console->putChar(topX+27, topY+6+y, compi+1 < (signed int)Item::Components(construct->Products(prodi)).size() ? TCOD_CHAR_TEEE : TCOD_CHAR_SW, TCOD_BKGND_SET);
                console->setForegroundColor(TCODColor::grey);
                console->print(topX+28, topY+6+y, Item::ItemCategoryToString(Item::Components(construct->Products(prodi), compi)).c_str());
                ++y;
            }
            ++y;
        }

        for (int jobi = 0; jobi < (signed int)construct->JobList()->size(); ++jobi) {
            console->setForegroundColor(jobi == 0 ? TCODColor::white : TCODColor::grey);
            console->print(topX+2, topY+6+jobi, Item::ItemTypeToString(construct->JobList(jobi)).c_str());
        }

        firstTimeDraw = false;
	} else if (construct->IsFarmplot()) { //It's a farmplot

	} else if (construct->IsStockpile()) { //A stockpile, but not a farmplot
        Stockpile* sp = static_cast<Stockpile*>(construct);
        console->setForegroundColor(TCODColor::white);
        console->printFrame(topX, topY+5, 50, 50, true, TCOD_BKGND_SET, "Item categories allowed");
        int x = topX+2;
        int y = topY+6;
        for (unsigned int i = 0; i < Item::Categories.size(); ++i) {
            console->setForegroundColor(sp->Allowed(i) ? TCODColor::green : TCODColor::red);
            console->print(x,y, Item::Categories[i].name.c_str());
            ++y;
            if (i != 0 && i % 49 == 0) {
                x += 20;
                y = topY+6;
            }
        }
    }

    console->setForegroundColor(TCODColor::white);

}

MenuResult ConstructionMenu::Update(int x, int y, bool clicked) {
    if (x >= topX + 3 && x < topX + 3 + 6 && y == topY + 2) { /*Rename*/ }
    if (x >= topX + 13 && x < topX + 13 + 9 && y == topY + 2) { /*Dismantle*/ }

    if (construct->Producer()) {
        if (x > topX+2 && x < topX+25) {
            //Cancel production jobs
            if (y > topY+5 && y < topY+17) {
                construct->CancelJob(y-(topY+6));
                return MENUHIT;
            }
        }
        if (x > topX+25 && x < topX+48) {
            //Add production jobs
            for (int i = 0; i < (signed int)productPlacement.size(); ++i) {
                if (y == productPlacement[i]) {
                    construct->AddJob(construct->Products(i+scroll));
                    return MENUHIT;
                }
            }
        }

        if (x == topX+48 && y == topY+6) { ScrollUp(); return MENUHIT; }
        if (x == topX+48 && y == topY+53) { ScrollDown(); return MENUHIT; }
	} else if (construct->IsFarmplot()) { //It's a farmplot

	} else if (construct->IsStockpile()) { //A stockpile, but not a farmplot
        int i = ((x-topX+2) / 20)*50;
        i += (y - (topY+6));
        if (i >= 0 && i < (signed int)Item::Categories.size()) {
            static_cast<Stockpile*>(construct)->SwitchAllowed(i);
        }
        return MENUHIT;
    }
    return NOMENUHIT;
}

ConstructionMenu* ConstructionMenu::ConstructionInfoMenu(Construction* cons) {
    if (!constructionInfoMenu) constructionInfoMenu = new ConstructionMenu();
    constructionInfoMenu->Construct(cons);
    constructionInfoMenu->ClearProductPlacement();
    return constructionInfoMenu;
}

void ConstructionMenu::Construct(Construction* cons) { construct = cons; }

void ConstructionMenu::ClearProductPlacement() { productPlacement.clear(); firstTimeDraw = true; }

void ConstructionMenu::ScrollDown() { ++scroll; }
void ConstructionMenu::ScrollUp() { if (--scroll < 0) scroll = 0; }

StockManagerMenu* StockManagerMenu::stocksMenu = 0;

StockManagerMenu::StockManagerMenu() : Menu(std::vector<MenuChoice>()),
    scroll(0),
	filter("")
{
    width = 50; height = 50;
    topX = (Game::Inst()->ScreenWidth() - width) / 2;
    topY = (Game::Inst()->ScreenHeight() - height) / 2;
}

void StockManagerMenu::Draw(int, int, TCODConsole* console) {
    console->setForegroundColor(TCODColor::white);
    console->printFrame(topX, topY, 50, 50, true, TCOD_BKGND_SET, "Stock Manager");
	console->putChar(topX+48, topY+1, TCOD_CHAR_ARROW_N, TCOD_BKGND_SET);
	console->putChar(topX+48, topY+48, TCOD_CHAR_ARROW_S, TCOD_BKGND_SET);
	console->printFrame(topX+10, topY+1, 30, 3, true);
	console->print(topX+11, topY+2, filter.c_str());

	int x = topX + 8;
	int y = topY + 4;

	console->setAlignment(TCOD_CENTER);

	for (std::set<ItemType>::iterator itemi = boost::next(StockManager::Inst()->Producables()->begin(), scroll*3);
		itemi != StockManager::Inst()->Producables()->end(); ++itemi) {
			if (StockManager::Inst()->TypeQuantity(*itemi) > -1 &&
				boost::icontains(Item::Presets[*itemi].name, filter)) { //Hide unavailable products
				console->setForegroundColor(Item::Presets[*itemi].color);
				console->print(x,y, "%c %s", Item::Presets[*itemi].graphic, Item::Presets[*itemi].name.c_str());
				console->setForegroundColor(TCODColor::white);
				console->print(x,y+1, "%d", StockManager::Inst()->TypeQuantity(*itemi));

				console->print(x,y+2, "- %d +", StockManager::Inst()->Minimum(*itemi));

				x += 16;
				if (x > topX + (width - 10)) {x = topX + 8; y += 5;}
				if (y > topY + (height - 4)) break;
			}
	}



	console->setAlignment(TCOD_LEFT);
}

MenuResult StockManagerMenu::Update(int x, int y, bool clicked) {
	UI::Inst()->SetTextMode(true, 28);
	filter = UI::Inst()->InputString();
	if (x >= 0 && y >= 0 && clicked) {
		int ch = TCODConsole::root->getChar(x,y);

		if (ch == '-' || ch == '+') {
			x -= (topX + 4); //If it's the first choice, x is now ~0
			x /= 16; //Now x = the column
			y -= (topY + 3 + 2); //+2 because +/- are under the text
			y /= 5;
			int choice = x + (y*3);
			//Because choice = index based on the visible items, we need to translate that into
			//an actual ItemType, which might be anything. So just go through the items as in
			//Draw() to find which index equals which itemtype.

			int itemIndex = 0;
			for (std::set<ItemType>::iterator itemi = boost::next(StockManager::Inst()->Producables()->begin(), scroll*3);
			itemi != StockManager::Inst()->Producables()->end(); ++itemi) {
				if (StockManager::Inst()->TypeQuantity(*itemi) > -1 &&
					boost::icontains(Item::Presets[*itemi].name, filter)) {
					if (itemIndex++ == choice) {
						StockManager::Inst()->AdjustMinimum(*itemi, (ch == '-') ? -1 : 1);
						break;
					}
				}
			}
		} else if (x == topX+48 && y == topY+1) {
			if (scroll > 0) --scroll;
		} else if (x == topX+48 && y == topY+48) {
			if (scroll < (signed int)(StockManager::Inst()->Producables()->size() / 3)-1) ++scroll;
		}
		return MENUHIT;
	}
    return NOMENUHIT;
}

StockManagerMenu* StockManagerMenu::StocksMenu() {
    if (!stocksMenu) stocksMenu = new StockManagerMenu();
    return stocksMenu;
}

void StockManagerMenu::ScrollDown() { ++scroll; }
void StockManagerMenu::ScrollUp() { if (--scroll < 0) scroll = 0; }

SquadsMenu* SquadsMenu::squadMenu = 0;
SquadsMenu* SquadsMenu::SquadMenu() {
	if (!squadMenu) squadMenu = new SquadsMenu();
	return squadMenu;
}

SquadsMenu::SquadsMenu() : Menu(std::vector<MenuChoice>()),
	squadName(""),
	squadMembers(1),
	squadPriority(0),
	chosenSquad(boost::weak_ptr<Squad>())
{
	width = 50;
	height = 20;
	topX = (Game::Inst()->ScreenWidth() - width) / 2;
	topY = (Game::Inst()->ScreenHeight() - height) / 2;

}

void SquadsMenu::Draw(int x, int y, TCODConsole* console) {
	console->printFrame(topX, topY, width, height, true, TCOD_BKGND_SET, "Squads");

	console->printFrame(topX+1, topY+1, width / 2 - 1, height - 2, false, TCOD_BKGND_SET, "Existing");
	y = topY+2;
	for (std::map<std::string, boost::shared_ptr<Squad> >::iterator squadi = Game::Inst()->squadList.begin(); squadi != Game::Inst()->squadList.end(); ++squadi) {
		console->print(topX+2, y++, "%s (%d/%d)", squadi->first.c_str(), squadi->second->MemberCount(),
			squadi->second->MemberLimit());
	}

	x = topX+(width/2);
	y = topY+2;
	console->printFrame(x, topY+1, width / 2 - 1, height-2, false, TCOD_BKGND_SET, (chosenSquad.lock()) ? "Modify Squad" : "New Squad");
	console->setAlignment(TCOD_CENTER);
	++x;
	console->print(x+(width/4)-2, y, "Name");
	console->print(x+(width/4)-2, y+1, squadName.c_str());
	y += 3;
	console->printFrame(x+3, y, 3, 3, false, TCOD_BKGND_SET);
	console->printFrame(x+17, y++, 3, 3, false, TCOD_BKGND_SET);
	console->setForegroundColor(TCODColor::red);
	console->print(x+4, y, "-");
	console->setForegroundColor(TCODColor::white);
	console->print(x+(width/4)-1, y, "Members");
	console->print(x+(width/4)-1, y+1, "%d", squadMembers);
	console->setForegroundColor(TCODColor::green);
	console->print(x+18, y, "+");
	console->setForegroundColor(TCODColor::white);
	y += 3;
	console->printFrame(x+3, ++y, 3, 3, false, TCOD_BKGND_SET);
	console->printFrame(x+17, y++, 3, 3, false, TCOD_BKGND_SET);
	console->setForegroundColor(TCODColor::red);
	console->print(x+4, y, "-");
	console->setForegroundColor(TCODColor::white);
	console->print(x+(width/4)-1, y, "Priority");
	console->print(x+(width/4)-1, y+1, "%d", squadPriority);
	console->setForegroundColor(TCODColor::green);
	console->print(x+18, y++, "+");
	console->setForegroundColor(TCODColor::white);

	console->printFrame(x+(width/4)-11, y+2, 10, 3, false);
	console->print(x+(width/4)-6, y+3, (chosenSquad.lock()) ? "Modify" : "Create");
	if (chosenSquad.lock()) {
		console->printFrame(x+(width/4)-1, y+2, 10, 3, false);
		console->print(x+(width/4)+4, y+3, "Delete");
	}

	if (chosenSquad.lock()) {
		x = topX;
		y = topY+19;
		console->printFrame(x, y, width, 7, true, TCOD_BKGND_SET, "Orders for %s", chosenSquad.lock()->Name().c_str());
		console->setBackgroundColor((chosenSquad.lock()->Order() == GUARD) ? TCODColor::blue : TCODColor::black);
		console->printFrame(x+2, y+2, 7, 3, false);
		console->print(x+5, y+3, "Guard");
		console->setBackgroundColor((chosenSquad.lock()->Order() == ESCORT) ? TCODColor::blue : TCODColor::black);
		console->printFrame(x+12, y+2, 8, 3, false);
		console->print(x+16, y+3, "Escort");
		console->setBackgroundColor(TCODColor::black);
	}

	console->setAlignment(TCOD_LEFT);
}

MenuResult SquadsMenu::Update(int x, int y, bool clicked) {
	UI::Inst()->SetTextMode(true, 21);
	squadName = UI::Inst()->InputString();

	if (clicked) {
		if (y < topY+19) {
			if (x > topX + (width/2)) {
				if (x > topX + (width/2) + 3 && x < topX + (width/2) + 6) {
					if (y > topY+2+3 && y < topY+2+6) if (squadMembers > 0) {--squadMembers; return MENUHIT; }
					else if (y > topY+2+8 && y < topY+2+11) if (squadPriority > 0) {--squadPriority; return MENUHIT; }
				} else if (x > topX + (width/2) + 17 && x < topX + (width/2) + 20) {
					if (y > topY+2+3 && y < topY+2+6) {++squadMembers; return MENUHIT; }
					else if (y > topY+2+8 && y < topY+2+11) {++squadPriority; return MENUHIT; }
				} else if (x > topX + (width/2) + 1 && x < topX + (width/2) + 11
					&& y > topY+2+12 && y < topY+2+15) {
						if (squadName != "" && !chosenSquad.lock()) { //Create
							Game::Inst()->squadList.insert(std::pair<std::string, boost::shared_ptr<Squad> >
								(squadName, new Squad(squadName, squadMembers, squadPriority)));
	//						chosenSquad = Game::Inst()->squadList[squadName];
							squadName = "";
							UI::Inst()->InputString("");
							return MENUHIT;					
						} else if (chosenSquad.lock()) { //Modify
							boost::shared_ptr<Squad> tempSquad = chosenSquad.lock();
							Game::Inst()->squadList.erase(tempSquad->Name());
							tempSquad->Name(squadName);
							Game::Inst()->squadList.insert(std::pair<std::string, 
								boost::shared_ptr<Squad> >(squadName, tempSquad));
							tempSquad->MemberLimit(squadMembers);
							tempSquad->Priority(squadPriority);
							return MENUHIT;
						}
				} else if (x > topX + (width/2) + 12 && x < topX + (width/2) + 22
					&& y > topY+2+12 && y < topY+2+15) {
						if (chosenSquad.lock()) {
							chosenSquad.lock()->RemoveAllMembers();
							Game::Inst()->squadList.erase(chosenSquad.lock()->Name());
							chosenSquad = boost::weak_ptr<Squad>();
							return MENUHIT;
						}
				}

			} else if (x > topX) {
				y -= (topY+2);
				if (y < (signed int)Game::Inst()->squadList.size()) {
					std::map<std::string, boost::shared_ptr<Squad> >::iterator squadi = Game::Inst()->squadList.begin();
					//while (y-- > 0 && squadi != Game::Inst()->squadList.end()) ++squadi;
					squadi = boost::next(squadi, y);
					if (squadi != Game::Inst()->squadList.end()) {
						chosenSquad = squadi->second;
						squadMembers = squadi->second->MemberLimit();
						squadPriority = squadi->second->Priority();
						UI::Inst()->InputString(squadi->first);
					}
					else {
						chosenSquad = boost::weak_ptr<Squad>();
						squadMembers = 1;
						squadPriority = 0;
						squadName = "";
						UI::Inst()->InputString("");
					}
					return MENUHIT;
				} else { 
					chosenSquad = boost::weak_ptr<Squad>(); 
					squadMembers = 1;
					squadPriority = 0;
					squadName = "";
					UI::Inst()->InputString("");
					return MENUHIT;
				}
			}
		} else {
			if (y > topY+20 && y < topY+27) {
				if (x > topX+1 && x < topX+7) { //Guard
					chosenSquad.lock()->Order(GUARD);
					UI::ChooseOrderTargetCoordinate(chosenSquad.lock());
					UI::Inst()->HideMenu();
					return MENUHIT;
				} else if (x > topX+11 && x < topX+20) { //Escort
					chosenSquad.lock()->Order(ESCORT);
					UI::ChooseOrderTargetEntity(chosenSquad.lock());
					UI::Inst()->HideMenu();
					return MENUHIT;
				}
			}
		}
	}
	return NOMENUHIT;
}
