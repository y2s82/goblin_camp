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

#include "Menu.hpp"
#include "UI.hpp"
#include "UIComponents.hpp"
#include "Label.hpp"
#include "Dialog.hpp"
#include "Button.hpp"
#include "ScrollPanel.hpp"
#include "Grid.hpp"
#include "UIList.hpp"
#include "Spinner.hpp"
#include "Announce.hpp"
#include "StockManager.hpp"
#include "JobManager.hpp"

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
				if (clicked) choices[y].callback();
				return MENUHIT; //Mouse was inside menu
			}
		}
	}
	return NOMENUHIT; //Mouse was not inside menu
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
		mainMenu->AddChoice(MenuChoice("Stock Manager", boost::bind(UI::ChangeMenu, StockManagerMenu::StocksMenu())));
#ifdef DEBUG
		mainMenu->AddChoice(MenuChoice("Jobs", boost::bind(UI::ChangeMenu, JobMenu::JobListingMenu())));
		mainMenu->AddChoice(MenuChoice("Announcements", boost::bind(UI::ChangeMenu, AnnounceMenu::AnnouncementsMenu())));
		mainMenu->AddChoice(MenuChoice("NPC List", boost::bind(UI::ChangeMenu, NPCMenu::NPCListMenu())));
#endif
		mainMenu->AddChoice(MenuChoice("Squads", boost::bind(UI::ChangeMenu, SquadsMenu::SquadMenu())));
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
        Menu *basicsMenu = ConstructionCategoryMenu("basics");
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
	}
	return ordersMenu;
}

void Menu::YesNoDialog(std::string text, boost::function<void()> leftAction, boost::function<void()> rightAction, std::string leftButton, std::string rightButton) {
    UIContainer *contents = new UIContainer(std::vector<Drawable *>(), 0, 0, 50, 10);
    Dialog *dialog = new Dialog(contents, "", 50, 10);
    contents->AddComponent(new Label(text, 25, 2));
    contents->AddComponent(new Button(leftButton, leftAction, 10, 4, 10, 'y'));
    contents->AddComponent(new Button(rightButton, rightAction, 30, 4, 10, 'n'));
    dialog->ShowModal();
}

void JobMenu::Draw(int _x, int _y, int scroll, int width, int height, TCODConsole* console) {
	JobManager::Inst()->Draw(Coordinate(_x + 1, _y), scroll, height, console);

	for (int y = _y; y < _y+height; ++y) {
		boost::weak_ptr<Job> job(JobManager::Inst()->GetJobByListIndex(y - _y + scroll));
		if (job.lock()) {
			if (job.lock()->Paused()) {
				console->print(_x+width-13, y, "P");
			}
			console->print(_x+width-16, y, "A-> %d", job.lock()->Assigned());
		}
	}
}

int JobMenu::TotalHeight() {
    return JobManager::Inst()->JobAmount();
}

Dialog* JobMenu::jobListingMenu = 0;
Dialog* JobMenu::JobListingMenu() {
	if (!jobListingMenu) {
        int width = Game::Inst()->ScreenWidth() - 20;
        int height = Game::Inst()->ScreenHeight() - 20;
        jobListingMenu = new Dialog(new ScrollPanel(0, 0, width, height, new JobMenu(), false), "Jobs", width, height);
	}
	return jobListingMenu;
}

void AnnounceMenu::Draw(int x, int y, int scroll, int width, int height, TCODConsole* console) {
	Announce::Inst()->Draw(Coordinate(x + 1, y), scroll, height, console);
}

int AnnounceMenu::TotalHeight() {
	return Announce::Inst()->AnnounceAmount();
}

Dialog* AnnounceMenu::announcementsMenu = 0;
Dialog* AnnounceMenu::AnnouncementsMenu() {
	if (!announcementsMenu) {
        int width = Game::Inst()->ScreenWidth() - 20;
        int height = Game::Inst()->ScreenHeight() - 20;
        announcementsMenu = new Dialog(new ScrollPanel(0, 0, width, height, new AnnounceMenu(), false), "Announcements", width, height);
	}
	return announcementsMenu;
}

void NPCMenu::DrawNPC(std::pair<int, boost::shared_ptr<NPC> > npci, int i, int x, int y, bool selected, TCODConsole* console) {
    console->print(x, y, "NPC: %d", npci.second->Uid());
    console->print(x+11, y, "%s: %s",
        npci.second->currentJob().lock() ? npci.second->currentJob().lock()->name.c_str() : "No job",
        npci.second->currentTask() ? Job::ActionToString(npci.second->currentTask()->action).c_str() : "No task");
}

Dialog* NPCMenu::npcListMenu = 0;
Dialog* NPCMenu::NPCListMenu() {
	if (!npcListMenu) {
        npcListMenu = new Dialog(new NPCMenu(), "NPCs", Game::Inst()->ScreenWidth() - 20, Game::Inst()->ScreenHeight() - 20);
	}
	return npcListMenu;
}

NPCMenu::NPCMenu(): UIContainer(std::vector<Drawable*>(), 0, 0, Game::Inst()->ScreenWidth() - 20, Game::Inst()->ScreenHeight() - 20) {
    AddComponent(new ScrollPanel(0, 0, width, height, 
                                 new UIList<std::pair<int, boost::shared_ptr<NPC> >, std::map<int, boost::shared_ptr<NPC> > >(&(Game::Inst()->npcList), 0, 0, width - 2, height, NPCMenu::DrawNPC), false));
}

Dialog* ConstructionMenu::constructionInfoMenu = 0;
Construction* ConstructionMenu::cachedConstruct = 0;
Dialog* ConstructionMenu::ConstructionInfoMenu(Construction* cons) {
    if (constructionInfoMenu && cons != cachedConstruct) {
        delete constructionInfoMenu;
        constructionInfoMenu = 0;
    }
	if (!constructionInfoMenu) {
        cachedConstruct = cons;
        ConstructionMenu *menu = new ConstructionMenu(50, 5);
        constructionInfoMenu = new Dialog(menu, "", 50, 5);
        menu->AddComponent(new Button("Rename", boost::bind(&ConstructionMenu::Rename, menu), 12, 1, 10));
        menu->AddComponent(new Button("Dismantle", boost::bind(&ConstructionMenu::Dismantle, menu), 28, 1, 13));
        if(cons->HasTag(STOCKPILE)) {
            constructionInfoMenu->SetHeight(40);
            menu->AddComponent(new UIList<ItemCat>(&Item::Categories, 2, 5, 46, Item::Categories.size(),
                                                                   boost::bind(&ConstructionMenu::DrawCategory, menu, _1, _2, _3, _4, _5, _6),
                                                                   boost::bind(&Stockpile::SwitchAllowed, static_cast<Stockpile *>(cons), _1, boost::bind(&UI::ShiftPressed, UI::Inst()))));
        } else if(cons->Producer()) {
            constructionInfoMenu->SetHeight(40);
            menu->AddComponent(new Label("Job Queue", 2, 5, TCOD_LEFT));
            menu->AddComponent(new ScrollPanel(2, 6, 23, 34, 
                                                               new UIList<ItemType, std::deque<ItemType> >(cons->JobList(), 0, 0, 20, 34, 
                                                                                    boost::bind(&ConstructionMenu::DrawJob, menu, _1, _2, _3, _4, _5, _6),
                                                                                    boost::bind(&Construction::CancelJob, cons, _1)),
                                                               false));
            menu->AddComponent(new Label("Product List", 26, 5, TCOD_LEFT));
            ProductList *productList = new ProductList(cons);
            for (int prodi = 0; prodi < (signed int)cons->Products()->size(); ++prodi) {
                productList->productPlacement.push_back(productList->height);
                productList->height += 2 + Item::Components(cons->Products(prodi)).size();
            }
            menu->AddComponent(new ScrollPanel(26, 6, 23, 34, 
                                                               productList,
                                                               false));
        }
        constructionInfoMenu->SetTitle(cons->Name());
        menu->Construct(cons);
    }
	return constructionInfoMenu;
}

void ConstructionMenu::Construct(Construction* cons) { construct = cons; }

void ConstructionMenu::Rename() {
    
}

void ConstructionMenu::Dismantle() {
    
}

void ConstructionMenu::DrawCategory(ItemCat category, int i, int x, int y, bool selected, TCODConsole *console) {
    Stockpile *sp = static_cast<Stockpile*>(construct);
    console->setForegroundColor(sp->Allowed(i) ? TCODColor::green : TCODColor::red);
    if (!category.parent) {
        console->print(x, y, "%c %s", sp->Allowed(i) ? 225 : 224, Item::Categories[i].name.substr(0,width-6).c_str());
    } else {
        if (i+1 < Item::Categories.size() && Item::Categories[i+1].parent == category.parent) {
            console->print(x, y, "%c%c %s", 195, sp->Allowed(i) ? 225 : 224, category.name.substr(0,width-7).c_str());
        } else {
            console->print(x, y, "%c%c %s", 192, sp->Allowed(i) ? 225 : 224, category.name.substr(0,width-7).c_str());
        }
    }
    console->setForegroundColor(TCODColor::white);
}

void ConstructionMenu::DrawJob(ItemType category, int i, int x, int y, bool selected, TCODConsole *console) {
    console->setForegroundColor(i == 0 ? TCODColor::white : TCODColor::grey);
    console->print(x, y, Item::ItemTypeToString(category).c_str());
    console->setForegroundColor(TCODColor::white);
}

void ConstructionMenu::ProductList::Draw(int x, int _y, int scroll, int width, int _height, TCODConsole *console) {
    int y = 0;
    for (int prodi = 0; prodi < (signed int)construct->Products()->size() && y < scroll + _height; ++prodi) {
        if (y >= scroll) {
            console->setForegroundColor(TCODColor::white);
            console->print(x, _y + y - scroll, "%s x%d", Item::ItemTypeToString(construct->Products(prodi)).c_str(), Item::Presets[construct->Products(prodi)].multiplier);
        }
        ++y;
        for (int compi = 0; compi < (signed int)Item::Components(construct->Products(prodi)).size() && y < scroll + _height; ++compi) {
            if (y >= scroll) {
                console->setForegroundColor(TCODColor::white);
                console->putChar(x + 1, _y + y - scroll, compi+1 < (signed int)Item::Components(construct->Products(prodi)).size() ? TCOD_CHAR_TEEE : TCOD_CHAR_SW, TCOD_BKGND_SET);
                console->setForegroundColor(TCODColor::grey);
                console->print(x + 2, _y + y - scroll, Item::ItemCategoryToString(Item::Components(construct->Products(prodi), compi)).c_str());
            }
            ++y;
        }
        ++y;
    }
    console->setForegroundColor(TCODColor::white);
}

int ConstructionMenu::ProductList::TotalHeight() {
    return height;
}

MenuResult ConstructionMenu::ProductList::Update(int x, int y, bool clicked, TCOD_key_t key) {
    for (int i = 0; i < (signed int)productPlacement.size(); ++i) {
        if (y == productPlacement[i]) {
            if(clicked) {
                construct->AddJob(construct->Products(i));
            }
            return MENUHIT;
        }
    }
    return NOMENUHIT;
}

Dialog* StockManagerMenu::stocksMenu = 0;

class StockPanel: public UIContainer {
private:
    ItemType itemType;
    StockManagerMenu *owner;
public:
    StockPanel(ItemType nItemType, StockManagerMenu *nowner): UIContainer(std::vector<Drawable *>(), 0, 0, 16, 4), itemType(nItemType), owner(nowner) {
        AddComponent(new Spinner(0, 2, 16, boost::bind(&StockManager::Minimum, StockManager::Inst(), itemType), 
                                 boost::bind(&StockManager::SetMinimum, StockManager::Inst(), itemType, _1)));
    }
    
    void Draw(int x, int y, TCODConsole *console) {
        console->setAlignment(TCOD_CENTER);
        console->setForegroundColor(Item::Presets[itemType].color);
        console->print(x + 8, y, "%c %s", Item::Presets[itemType].graphic, Item::Presets[itemType].name.c_str());
        console->setForegroundColor(TCODColor::white);
        console->print(x + 8, y+1, "%d", StockManager::Inst()->TypeQuantity(itemType));
        UIContainer::Draw(x, y, console);
    }
    
    bool Visible() {
        // TODO filter
        return StockManager::Inst()->TypeQuantity(itemType) > -1;
    }
};
    
    
StockManagerMenu::StockManagerMenu(): Dialog(0, "Stock Manager", 50, 50), filter("")
{
    Grid *grid = new Grid(std::vector<Drawable *>(), 3, 0, 0, 48, 48);
	for (std::set<ItemType>::iterator it = StockManager::Inst()->Producables()->begin(); it != StockManager::Inst()->Producables()->end(); it++) {
        grid->AddComponent(new StockPanel(*it, this));
    }
    contents = new ScrollPanel(0, 1, 50, 20, grid, false, 4);
}

Dialog* StockManagerMenu::StocksMenu() {
	if (!stocksMenu) stocksMenu = new StockManagerMenu();
	return stocksMenu;
}

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
	_x = (Game::Inst()->ScreenWidth() - width) / 2;
	_y = (Game::Inst()->ScreenHeight() - height) / 2;

}

void SquadsMenu::Draw(int x, int y, TCODConsole* console) {
	console->printFrame(_x, _y, width, height, true, TCOD_BKGND_SET, "Squads");

	console->printFrame(_x+1, _y+1, width / 2 - 1, height - 2, false, TCOD_BKGND_SET, "Existing");
	y = _y+2;
	for (std::map<std::string, boost::shared_ptr<Squad> >::iterator squadi = Game::Inst()->squadList.begin(); squadi != Game::Inst()->squadList.end(); ++squadi) {
		console->setBackgroundColor((chosenSquad.lock() == squadi->second) ? TCODColor::blue : TCODColor::black);
		console->print(_x+2, y++, "%s (%d/%d)", squadi->first.c_str(), squadi->second->MemberCount(),
			squadi->second->MemberLimit());
	}
    console->setBackgroundColor(TCODColor::black);

	x = _x+(width/2);
	y = _y+2;
	console->printFrame(x, _y+1, width / 2 - 1, height-2, false, TCOD_BKGND_SET, (chosenSquad.lock()) ? "Modify Squad" : "New Squad");
	console->setAlignment(TCOD_CENTER);
	++x;
	console->print(x+(width/4)-2, y, "Name (required)");
	console->setBackgroundColor(TCODColor::darkGrey);
	console->rect(x,y+1,(width/2)-3,1,true,TCOD_BKGND_SET);
	console->setBackgroundColor(TCODColor::black);
	console->print(x+(width/4)-1, y+1, squadName.c_str());
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
		x = _x;
		y = _y+19;
		console->printFrame(x, y, width, 7, true, TCOD_BKGND_SET, "Orders for %s", chosenSquad.lock()->Name().c_str());
		console->setBackgroundColor((chosenSquad.lock()->Order() == GUARD) ? TCODColor::blue : TCODColor::black);
		console->printFrame(x+2, y+2, 7, 3, false);
		console->print(x+5, y+3, "Guard");
		console->setBackgroundColor((chosenSquad.lock()->Order() == ESCORT) ? TCODColor::blue : TCODColor::black);
		console->printFrame(x+12, y+2, 8, 3, false);
		console->print(x+16, y+3, "Escort");
		console->setBackgroundColor(TCODColor::black);

		console->printFrame(x, y+7, 23, 5, true, TCOD_BKGND_SET, "Weapons");
		console->printFrame(x+1, y+8, 21, 3);
		console->print(x+11, y+9, chosenSquad.lock()->Weapon() >= 0 ? Item::Categories[chosenSquad.lock()->Weapon()].name.c_str() : "None");
		console->printFrame(x, y+11, 7, 3);
		console->print(x+3, y+12, "Rearm");
	}

	console->setAlignment(TCOD_LEFT);
}

MenuResult SquadsMenu::Update(int x, int y, bool clicked, TCOD_key_t key) {
	squadName = UI::Inst()->InputString();
	if (clicked) {
		if (y < _y+19) {
			if (x > _x + (width/2)) {
				if (x > _x + (width/2) + 3 && x < _x + (width/2) + 6) {
					if (y > _y+2+3 && y < _y+2+6) {
						if (squadMembers > 0) --squadMembers; 
						return MENUHIT;
					}
					else if (y > _y+2+8 && y < _y+2+11) {
						if (squadPriority > 0) --squadPriority; 
						return MENUHIT; 
					}
				} else if (x > _x + (width/2) + 17 && x < _x + (width/2) + 20) {
					if (y > _y+2+3 && y < _y+2+6) {
						++squadMembers; 
						return MENUHIT; 
					}
					else if (y > _y+2+8 && y < _y+2+11) {
						++squadPriority; 
						return MENUHIT; 
					}
				} else if (x > _x + (width/2) + 1 && x < _x + (width/2) + 11
					&& y > _y+2+12 && y < _y+2+15) {
						if (squadName != "" && !chosenSquad.lock()) { //Create
							Game::Inst()->squadList.insert(std::pair<std::string, boost::shared_ptr<Squad> >
								(squadName, boost::shared_ptr<Squad>(new Squad(squadName, squadMembers, squadPriority))));
							chosenSquad = Game::Inst()->squadList[squadName];
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
				} else if (x > _x + (width/2) + 12 && x < _x + (width/2) + 22
					&& y > _y+2+12 && y < _y+2+15) {
						if (chosenSquad.lock()) {
							chosenSquad.lock()->RemoveAllMembers();
							Game::Inst()->squadList.erase(chosenSquad.lock()->Name());
							chosenSquad = boost::weak_ptr<Squad>();
							return MENUHIT;
						}
				}

			} else if (x > _x && y >= _y+2 && y < _y + height-2) {
				y -= (_y+2);
				if (y >= 0 && y < (signed int)Game::Inst()->squadList.size()) {
					std::map<std::string, boost::shared_ptr<Squad> >::iterator squadi = Game::Inst()->squadList.begin();
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
			if (y > _y+20 && y < _y+24) {
				if (x > _x+1 && x < _x+9) { //Guard
					chosenSquad.lock()->Order(GUARD);
					UI::ChooseOrderTargetCoordinate(chosenSquad.lock());
					UI::Inst()->HideMenu();
					return MENUHIT;
				} else if (x > _x+11 && x < _x+20) { //Escort
					chosenSquad.lock()->Order(ESCORT);
					UI::ChooseOrderTargetEntity(chosenSquad.lock());
					UI::Inst()->HideMenu();
					return MENUHIT;
				}
			} else if (y > _y+26 && y < _y+30) {
				if (x > _x && x < _x+22) {
					Menu *weaponChoiceDialog = new Menu(std::vector<MenuChoice>(), "Weapons");
                    weaponChoiceDialog->AddChoice(MenuChoice("None", boost::bind(&Squad::Weapon, chosenSquad.lock(), -1)));
                    for (unsigned int i = 0; i < Item::Categories.size(); ++i) {
                        if (Item::Categories[i].parent && boost::iequals(Item::Categories[i].parent->name, "Weapon")) {
                            weaponChoiceDialog->AddChoice(MenuChoice(Item::Categories[i].name.c_str(), boost::bind(&Squad::Weapon, chosenSquad.lock(), i)));
                        }
                    }
					weaponChoiceDialog->ShowModal();
					return MENUHIT;
				}
			} else if (y >= _y+30 && y < _y+33) {
				if (x >= _x && x < _x+7) {
					chosenSquad.lock()->Rearm();
					Announce::Inst()->AddMsg(chosenSquad.lock()->Name() + " rearming.");
					return MENUHIT;
				}
			}
		}
		if (x > _x && x < _x+width && y > _y && y < _y+height) return MENUHIT;
	}
	return NOMENUHIT;
}

void SquadsMenu::Open() {
	UI::Inst()->SetTextMode(true, 22);
	if (boost::shared_ptr<Squad> squad = chosenSquad.lock()) {
		squadName = squad->Name();
		UI::Inst()->InputString(squadName);
	}
}
