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
#include "Announce.hpp"
#include "StockManager.hpp"
#include "JobManager.hpp"

void Label::Draw(int x, int y, TCODConsole *console) {
    console->setAlignment(TCOD_CENTER);
    console->setForegroundColor(TCODColor::white);
    console->print(x + _x, y + _y, text.c_str());
}

void Button::Draw(int x, int y, TCODConsole *console) {
    if(selected) {
        console->setForegroundColor(TCODColor::black);
        console->setBackgroundColor(TCODColor::white);
    } else {
        console->setForegroundColor(TCODColor::white);
        console->setBackgroundColor(TCODColor::black);
    }
    console->setAlignment(TCOD_CENTER);
    console->printFrame(x + _x, y + _y, width, 3);
    console->print(x + _x + width/2, y + _y + 1, text.c_str());
}

MenuResult Button::Update(int x, int y, bool clicked, TCOD_key_t key) {
    if(key.c == shortcut) {
        callback();
        return MENUHIT;
    }
    if(x >= _x && x < _x + width && y >= _y && y < _y + 3) {
        selected = true;
        if(clicked && callback) {
            callback();
        }
        return MENUHIT;
    } else {
        selected = false;
        return NOMENUHIT;
    }

}

void ScrollPanel::Draw(int x, int y, TCODConsole *console) {
    if (scroll < 0) {
        scroll = 0;
    }
	if (scroll + height - 2 > contents->TotalHeight()) {
        scroll = std::max(0, contents->TotalHeight() - (height - 2));
    }
	scrollBar = (int)((height-3) * ((double)scroll / (double)std::max(1, contents->TotalHeight() - (height-2)))) + 2;
	scrollBar = std::min(scrollBar, height - 4);
    
	console->printFrame(x + _x, y + _y, width, height);
    
	contents->Draw(x + _x + 1, y + _y + 1, scroll, width - 2, height - 2, console);
	console->putChar(x + _x + width - 2, y + _y + 1, TCOD_CHAR_ARROW_N, TCOD_BKGND_SET);
	console->putChar(x + _x + width - 2, y + _y + height - 2, TCOD_CHAR_ARROW_S, TCOD_BKGND_SET);
	console->putChar(x + _x + width - 2, y + _y + scrollBar, 219, TCOD_BKGND_SET);
}

MenuResult ScrollPanel::Update(int x, int y, bool clicked, TCOD_key_t key) {
    if (x >= _x && x < _x + width && y >= _y && y < _y + height) {
        if (clicked) {
            if (x == _x + width - 2) {
                if (y == _y + 1) {
                    scroll--;
                } else if (y == _y + height - 2) {
                    scroll++;
                } else if (y < scrollBar - _y) {
                    scroll -= height;
                } else if (y > scrollBar - _y) {
                    scroll += height; 
                }
            } else {
                contents->Update(x, y, clicked, key);
            }
        } else {
            contents->Update(x, y, clicked, key);
        }
        return MENUHIT;
    }
    return NOMENUHIT;
}

void Panel::selected(int newSel) {}
void Panel::Open() {}
void Panel::Close() {
	UI::Inst()->SetTextMode(false);
}

void Panel::ShowModal() {
	TCODConsole *background = new TCODConsole(Game::Inst()->ScreenWidth(), Game::Inst()->ScreenHeight());
	TCODConsole::blit (TCODConsole::root, 0, 0, Game::Inst()->ScreenWidth(), Game::Inst()->ScreenHeight(),
                       background, 0, 0);
    
	int _x = (Game::Inst()->ScreenWidth() - width) / 2;
	int _y = (Game::Inst()->ScreenHeight() - height) / 2;
	TCOD_key_t key;
	TCOD_mouse_t mouseStatus;
    
	while (true) {
		TCODConsole::root->clear();
		TCODConsole::root->setForegroundColor(TCODColor::white);
		TCODConsole::root->setBackgroundColor(TCODColor::black);
		TCODConsole::blit(background, 0, 0, Game::Inst()->ScreenWidth(), Game::Inst()->ScreenHeight(),
                          TCODConsole::root, 0, 0, 0.7, 1.0);
        
		Draw(_x, _y, TCODConsole::root);
		TCODConsole::root->flush();
        
		key = TCODConsole::checkForKeypress();
		mouseStatus = TCODMouse::getStatus();
        
		if((Update(mouseStatus.cx, mouseStatus.cy, mouseStatus.lbutton_pressed, key) == MENUHIT && mouseStatus.lbutton_pressed) ||
           mouseStatus.rbutton_pressed || key.vk == TCODK_ESCAPE) {
            delete this;
            return;
        }
	}    
}

Dialog::Dialog(std::vector<Drawable *> ncomponents, std::string ntitle, int nwidth, int nheight):
    components(ncomponents), title(ntitle), Panel(nwidth, nheight) {
    _x = (Game::Inst()->ScreenWidth() - width) / 2;
    _y = (Game::Inst()->ScreenHeight() - height) / 2;
}

void Dialog::AddComponent(Drawable *component) {
    components.push_back(component);
}

void Dialog::Draw(int x, int y, TCODConsole *console) {
    console->printFrame(_x, _y, width, height, true, TCOD_BKGND_SET, title.empty() ? 0 : title.c_str());
    for(std::vector<Drawable *>::iterator it = components.begin(); it != components.end(); it++) {
        Drawable *component = *it;
        component->Draw(_x, _y, console);
    }
}

Dialog::~Dialog() {
    for(std::vector<Drawable *>::iterator it = components.begin(); it != components.end(); it++) {
        delete *it;
    }
}

MenuResult Dialog::Update(int x, int y, bool clicked, TCOD_key_t key) {
    MenuResult rtn = NOMENUHIT;
    for(std::vector<Drawable *>::iterator it = components.begin(); it != components.end(); it++) {
        Drawable *component = *it;
        if(component->Update(x - _x, y - _y, clicked, key) == MENUHIT) {
            rtn = MENUHIT;
        }
    }
    return rtn;
}

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
    Dialog *dialog = new Dialog(std::vector<Drawable *>(), "", 50, 10);
    dialog->AddComponent(new Label(text, 25, 2));
    dialog->AddComponent(new Button(leftButton, leftAction, 10, 4, 10, 'y'));
    dialog->AddComponent(new Button(rightButton, rightAction, 30, 4, 10, 'n'));
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
        jobListingMenu = new Dialog(std::vector<Drawable *>(), "Jobs", width, height);
        jobListingMenu->AddComponent(new ScrollPanel(0, 0, width, height, new JobMenu()));
	}
	return jobListingMenu;
}

AnnounceMenu::AnnounceMenu() : Menu(std::vector<MenuChoice>()),
	scroll(0)
{
	width = Game::Inst()->ScreenWidth() - 20;
	height = Game::Inst()->ScreenHeight() - 20;
	_x = 10; _y = 10;
}

void AnnounceMenu::Draw(int x, int y, TCODConsole* console) {
	if (scroll + height - 2 > Announce::Inst()->AnnounceAmount()) scroll = std::max(0, Announce::Inst()->AnnounceAmount() - height - 2);

	int scrollBar = 0;
	scrollBar = (int)((height-3) * ((double)scroll / (double)std::max(1, Announce::Inst()->AnnounceAmount() - height-2)));
	scrollBar += _y+2;
	scrollBar = std::min(scrollBar, _y+height-4);

	console->printFrame(_x, _y, width, height, true, TCOD_BKGND_SET, "Announcements");

	Announce::Inst()->Draw(Coordinate(_x+1,_y+1), scroll, height-2, console);
	console->putChar(_x+width-2, _y+1, TCOD_CHAR_ARROW_N, TCOD_BKGND_SET);
	console->putChar(_x+width-2, _y+height-2, TCOD_CHAR_ARROW_S, TCOD_BKGND_SET);
	console->putChar(_x+width-2, scrollBar, 219, TCOD_BKGND_SET);
}

MenuResult AnnounceMenu::Update(int x, int y, bool clicked, TCOD_key_t key) {
	if (x > _x && x < Game::Inst()->ScreenWidth()-10 && y > _y && y < Game::Inst()->ScreenHeight()-10) {
		if (x == _x+width-2 && y == _y+1 && clicked) ScrollUp();
		if (x == _x+width-2 && y == _y+height-2 && clicked) ScrollDown();
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
	_x = 10; _y = 10;
}

void NPCMenu::Draw(int x, int y, TCODConsole* console) {
	if (scroll + height - 2 > (signed int)Game::Inst()->npcList.size()) scroll = std::max(0, (signed int)Game::Inst()->npcList.size() - height - 2);

	int scrollBar = 0;
	scrollBar = (int)((height-3) * ((double)scroll / (double)std::max(1, (signed int)Game::Inst()->npcList.size() - height-2)));
	scrollBar += _y+2;
	scrollBar = std::min(scrollBar, _y+height-4);

	console->printFrame(_x, _y, width, height, true, TCOD_BKGND_SET, "NPC List");

	int count = 0;
	for (std::map<int,boost::shared_ptr<NPC> >::iterator npci = Game::Inst()->npcList.begin(); npci != Game::Inst()->npcList.end(); ++npci) {
		if (count++ >= scroll) {
			console->print(_x+1, _y+1+(count-scroll), "NPC: %d", npci->second->Uid());
			console->print(_x+10, _y+1+(count-scroll), "%s: %s",
				npci->second->currentJob().lock() ? npci->second->currentJob().lock()->name.c_str() : "No job",
				npci->second->currentTask() ? Job::ActionToString(npci->second->currentTask()->action).c_str() : "No task");
		}
	}

	console->putChar(_x+width-2, _y+1, TCOD_CHAR_ARROW_N, TCOD_BKGND_SET);
	console->putChar(_x+width-2, _y+height-2, TCOD_CHAR_ARROW_S, TCOD_BKGND_SET);
	console->putChar(_x+width-2, scrollBar, 219, TCOD_BKGND_SET);
}

MenuResult NPCMenu::Update(int x, int y, bool clicked, TCOD_key_t key) {
	if (x > _x && x < Game::Inst()->ScreenWidth()-10 && y > _y && y < Game::Inst()->ScreenHeight()-10) {
		if (x == _x+width-2 && y == _y+1 && clicked) ScrollUp();
		if (x == _x+width-2 && y == _y+height-2 && clicked) ScrollDown();
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
	_x = (Game::Inst()->ScreenWidth() - width) / 2;
	_y = (Game::Inst()->ScreenHeight() - height) / 2;
}

void ConstructionMenu::Draw(int, int, TCODConsole* console) {
	console->setForegroundColor(TCODColor::white);
	console->printFrame(_x, _y, 50, 5, true, TCOD_BKGND_SET, construct->Name().c_str());
	console->setForegroundColor(TCODColor::green);
	console->print(_x + 3, _y + 2, "Rename");
	console->setForegroundColor(TCODColor::red);
	console->print(_x + 13, _y + 2, "Dismantle");
	console->setForegroundColor(TCODColor::white);

	//Only draw the production queue and product list if construction is a producer
	if (construct->Producer()) {
		console->printFrame(_x+25, _y+5, 25, 50, true, TCOD_BKGND_SET, "Product list");
		console->printFrame(_x, _y+5, 25, 50, true, TCOD_BKGND_SET, "Production queue");
		console->putChar(_x+25, _y+10, TCOD_CHAR_ARROW2_W, TCOD_BKGND_SET);
		console->putChar(_x+25, _y+25, TCOD_CHAR_ARROW2_W, TCOD_BKGND_SET);
		console->putChar(_x+25, _y+40, TCOD_CHAR_ARROW2_W, TCOD_BKGND_SET);
		console->putChar(_x+24, _y+10, TCOD_CHAR_ARROW2_W, TCOD_BKGND_SET);
		console->putChar(_x+24, _y+25, TCOD_CHAR_ARROW2_W, TCOD_BKGND_SET);
		console->putChar(_x+24, _y+40, TCOD_CHAR_ARROW2_W, TCOD_BKGND_SET);

		console->putChar(_x+48, _y+6, TCOD_CHAR_ARROW_N, TCOD_BKGND_SET);
		console->putChar(_x+48, _y+53, TCOD_CHAR_ARROW_S, TCOD_BKGND_SET);

		int y = 0;
		for (int prodi = scroll; prodi < (signed int)construct->Products()->size() && y < _y+50; ++prodi) {
			console->setForegroundColor(TCODColor::white);
			console->print(_x+26, _y+6+y, "%s x%d", Item::ItemTypeToString(construct->Products(prodi)).c_str(), Item::Presets[construct->Products(prodi)].multiplier);
			if (firstTimeDraw) productPlacement.push_back(_y+6+y);
			++y;
			for (int compi = 0; compi < (signed int)Item::Components(construct->Products(prodi)).size() && y < _y+50; ++compi) {
				console->setForegroundColor(TCODColor::white);
				console->putChar(_x+27, _y+6+y, compi+1 < (signed int)Item::Components(construct->Products(prodi)).size() ? TCOD_CHAR_TEEE : TCOD_CHAR_SW, TCOD_BKGND_SET);
				console->setForegroundColor(TCODColor::grey);
				console->print(_x+28, _y+6+y, Item::ItemCategoryToString(Item::Components(construct->Products(prodi), compi)).c_str());
				++y;
			}
			++y;
		}

		for (int jobi = 0; jobi < (signed int)construct->JobList()->size(); ++jobi) {
			console->setForegroundColor(jobi == 0 ? TCODColor::white : TCODColor::grey);
			console->print(_x+2, _y+6+jobi, Item::ItemTypeToString(construct->JobList(jobi)).c_str());
		}

		firstTimeDraw = false;
	} else if (construct->HasTag(FARMPLOT)) { //It's a farmplot

	} else if (construct->HasTag(STOCKPILE)) { //A stockpile, but not a farmplot
		Stockpile* sp = static_cast<Stockpile*>(construct);
		console->setForegroundColor(TCODColor::white);
		console->printFrame(_x, _y+5, 50, 50, true, TCOD_BKGND_SET, "Item categories allowed");
		int x = _x+2;
		int y = _y+6;
		for (unsigned int i = 0; i < Item::Categories.size(); ++i) {
			console->setForegroundColor(sp->Allowed(i) ? TCODColor::green : TCODColor::red);
			console->print(x,y, Item::Categories[i].name.c_str());
			++y;
			if (i != 0 && i % 49 == 0) {
				x += 20;
				y = _y+6;
			}
		}
	}

	console->setForegroundColor(TCODColor::white);

}

MenuResult ConstructionMenu::Update(int x, int y, bool clicked, TCOD_key_t key) {
	if (x >= _x + 3 && x < _x + 3 + 6 && y == _y + 2) { /*Rename*/ }
	if (x >= _x + 13 && x < _x + 13 + 9 && y == _y + 2) { /*Dismantle*/ }

	if (construct->Producer()) {
		if (x > _x+2 && x < _x+25) {
			//Cancel production jobs
			if (y > _y+5 && y < _y+17) {
				construct->CancelJob(y-(_y+6));
				return MENUHIT;
			}
		}
		if (x > _x+25 && x < _x+48) {
			//Add production jobs
			for (int i = 0; i < (signed int)productPlacement.size(); ++i) {
				if (y == productPlacement[i]) {
					construct->AddJob(construct->Products(i+scroll));
					return MENUHIT;
				}
			}
		}

		if (x == _x+48 && y == _y+6) { ScrollUp(); return MENUHIT; }
		if (x == _x+48 && y == _y+53) { ScrollDown(); return MENUHIT; }
	} else if (construct->HasTag(FARMPLOT)) { //It's a farmplot

	} else if (construct->HasTag(STOCKPILE)) { //A stockpile, but not a farmplot
		int i = ((x-_x+2) / 20)*50;
		i += (y - (_y+6));
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
	_x = (Game::Inst()->ScreenWidth() - width) / 2;
	_y = (Game::Inst()->ScreenHeight() - height) / 2;
}

void StockManagerMenu::Draw(int, int, TCODConsole* console) {
	console->setForegroundColor(TCODColor::white);
	console->printFrame(_x, _y, 50, 50, true, TCOD_BKGND_SET, "Stock Manager");
	console->putChar(_x+48, _y+1, TCOD_CHAR_ARROW_N, TCOD_BKGND_SET);
	console->putChar(_x+48, _y+48, TCOD_CHAR_ARROW_S, TCOD_BKGND_SET);
	console->printFrame(_x+10, _y+1, 30, 3, true);
	console->setBackgroundColor(TCODColor::darkGrey);
	console->rect(_x+11, _y+2, width-22, 1, true);
	console->setBackgroundColor(TCODColor::black);
	console->print(_x+11, _y+2, filter.c_str());

	int x = _x + 8;
	int y = _y + 4;

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
					if (x > _x + (width - 10)) {x = _x + 8; y += 5;}
					if (y > _y + (height - 4)) break;
			}
	}
	console->setAlignment(TCOD_LEFT);
}

MenuResult StockManagerMenu::Update(int x, int y, bool clicked, TCOD_key_t key) {
	filter = UI::Inst()->InputString();
	if (x >= 0 && y >= 0) {
		int ch = TCODConsole::root->getChar(x,y);

		x -= (_x + 4); //If it's the first choice, x is now ~0
		x /= 16; //Now x = the column
		y -= (_y + 3 + 2); //+2 because +/- are under the text
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

						int adjustAmount = UI::Inst()->ShiftPressed() ? 10 : 1;

						if (itemIndex == 0) { //TODO: You can only in/decrement the first item with the keyboard, for now.
							TCOD_key_t key = UI::Inst()->getKey();
							if (key.c == '+') {
								StockManager::Inst()->AdjustMinimum(*itemi, adjustAmount);
							} else if (key.c == '-') {
								StockManager::Inst()->AdjustMinimum(*itemi, -adjustAmount);
							}
						}

						if (clicked && (ch == '-' || ch == '+') && itemIndex == choice) {
							StockManager::Inst()->AdjustMinimum(*itemi, (ch == '-') ? -adjustAmount : adjustAmount);
							break;
						}
						++itemIndex;
				}
		}
		if (clicked && x == _x+48 && y == _y+1) {
			if (scroll > 0) --scroll;
		} else if (clicked && x == _x+48 && y == _y+48) {
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

void StockManagerMenu::Open() {
	UI::Inst()->SetTextMode(true, 28);
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
