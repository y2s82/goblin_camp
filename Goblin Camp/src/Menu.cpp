#include "Menu.hpp"
#include "UI.hpp"
#include "Announce.hpp"

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

void Menu::Draw(int x, int y) {
	//Draw the box
	if (x + width >= Game::Inst()->ScreenWidth()) x = Game::Inst()->ScreenWidth() - width - 1;
	if (y + height >= Game::Inst()->ScreenHeight()) y = Game::Inst()->ScreenHeight() - height - 1;
	topX = x; topY = y; //Save coordinates of menu top-left corner
	TCODConsole::root->printFrame(x, y, width, height, true, TCOD_BKGND_SET, 0);
    TCODConsole::root->setBackgroundFlag(TCOD_BKGND_SET);
	//Draw the menu entries
	for (int i = 0; i < (signed int)choices.size(); ++i) {
		TCODConsole::root->setBackgroundColor(TCODColor::black);
		TCODConsole::root->setForegroundColor(TCODColor::white);
		if (_selected == i) {
			TCODConsole::root->setBackgroundColor(TCODColor::white);
			TCODConsole::root->setForegroundColor(TCODColor::darkerGrey);
		}
		TCODConsole::root->print(x+1, y+1+(i*2), choices[i].label.c_str());
	}
	TCODConsole::root->setForegroundColor(TCODColor::white);
	TCODConsole::root->setBackgroundColor(TCODColor::black);
}

MenuResult Menu::Update(int x, int y) {
	if (x > 0 && y > 0) {
		if (x > topX && x < topX + width) {
			y -= topY;
			if (y > 0 && y < height) {
				--y;
				if (y > 0) y /= 2;
				_selected = y;
				choices[y].callback();
				return MENUHIT; //Mouse was inside menu when clicked
			}
		}
	}
	return NOMENUHIT; //Mouse was not inside menu when clicked
}

void Menu::selected(int newSel) { _selected = newSel; }
void Menu::AddChoice(MenuChoice newChoice) { choices.push_back(newChoice); CalculateSize(); }

Menu* Menu::mainMenu = 0;
Menu* Menu::MainMenu() {
	if (!mainMenu) {
		mainMenu = new Menu(std::vector<MenuChoice>());
		mainMenu->AddChoice(MenuChoice("Construction", boost::bind(UI::ChangeMenu, Menu::ConstructionMenu())));
		mainMenu->AddChoice(MenuChoice("Orders", boost::bind(UI::ChangeMenu, Menu::OrdersMenu())));
		mainMenu->AddChoice(MenuChoice("Jobs", boost::bind(UI::ChangeMenu, JobMenu::JobListingMenu())));
		mainMenu->AddChoice(MenuChoice("Announcements", boost::bind(UI::ChangeMenu, AnnounceMenu::AnnouncementsMenu())));
		mainMenu->AddChoice(MenuChoice("NPC List", boost::bind(UI::ChangeMenu, NPCMenu::NPCListMenu())));
		mainMenu->AddChoice(MenuChoice("Exit", boost::bind(Game::Exit)));
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

void JobMenu::Draw(int x, int y) {
	if (scroll + height - 2 > JobManager::Inst()->JobAmount()) scroll = std::max(0, JobManager::Inst()->JobAmount() - height - 2);

	int scrollBar = 0;
	scrollBar = (int)((height-3) * ((double)scroll / (double)std::max(1, JobManager::Inst()->JobAmount() - height-2)));
	scrollBar += topY+2;
	scrollBar = std::min(scrollBar, topY+height-4);

	TCODConsole::root->printFrame(topX, topY, width, height, true, TCOD_BKGND_SET, "Jobs");

	JobManager::Inst()->Draw(Coordinate(topX+1,topY+1), scroll, height-2);
	TCODConsole::root->putChar(topX+width-2, topY+1, TCOD_CHAR_ARROW_N, TCOD_BKGND_SET);
	TCODConsole::root->putChar(topX+width-2, topY+height-2, TCOD_CHAR_ARROW_S, TCOD_BKGND_SET);
	TCODConsole::root->putChar(topX+width-2, scrollBar, 219, TCOD_BKGND_SET);

	for (int y = topY+1; y <= topY+height-2; ++y) {
		boost::weak_ptr<Job> job(JobManager::Inst()->GetJobByListIndex(y - (topY+1 + scroll)));
		if (job.lock()) {
            if (job.lock()->Paused()) {
                TCODConsole::root->print(topX+width-15, y, "P");
            }
                TCODConsole::root->print(topX+width-18, y, "A-> %d", job.lock()->Assigned());
		}
	}
}

MenuResult JobMenu::Update(int x, int y) {
	if (x > topX && x < Game::Inst()->ScreenWidth()-10 && y > topY && y < Game::Inst()->ScreenHeight()-10) {
		if (x == topX+width-2 && y == topY+1) ScrollUp();
		if (x == topX+width-2 && y == topY+height-2) ScrollDown();
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

void AnnounceMenu::Draw(int x, int y) {
	if (scroll + height - 2 > Announce::Inst()->AnnounceAmount()) scroll = std::max(0, Announce::Inst()->AnnounceAmount() - height - 2);

	int scrollBar = 0;
	scrollBar = (int)((height-3) * ((double)scroll / (double)std::max(1, Announce::Inst()->AnnounceAmount() - height-2)));
	scrollBar += topY+2;
	scrollBar = std::min(scrollBar, topY+height-4);

	TCODConsole::root->printFrame(topX, topY, width, height, true, TCOD_BKGND_SET, "Announcements");

	Announce::Inst()->Draw(Coordinate(topX+1,topY+1), scroll, height-2);
	TCODConsole::root->putChar(topX+width-2, topY+1, TCOD_CHAR_ARROW_N, TCOD_BKGND_SET);
	TCODConsole::root->putChar(topX+width-2, topY+height-2, TCOD_CHAR_ARROW_S, TCOD_BKGND_SET);
	TCODConsole::root->putChar(topX+width-2, scrollBar, 219, TCOD_BKGND_SET);
}

MenuResult AnnounceMenu::Update(int x, int y) {
	if (x > topX && x < Game::Inst()->ScreenWidth()-10 && y > topY && y < Game::Inst()->ScreenHeight()-10) {
		if (x == topX+width-2 && y == topY+1) ScrollUp();
		if (x == topX+width-2 && y == topY+height-2) ScrollDown();
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

void NPCMenu::Draw(int x, int y) {
	if (scroll + height - 2 > (signed int)Game::Inst()->npcList.size()) scroll = std::max(0, (signed int)Game::Inst()->npcList.size() - height - 2);

	int scrollBar = 0;
	scrollBar = (int)((height-3) * ((double)scroll / (double)std::max(1, (signed int)Game::Inst()->npcList.size() - height-2)));
	scrollBar += topY+2;
	scrollBar = std::min(scrollBar, topY+height-4);

	TCODConsole::root->printFrame(topX, topY, width, height, true, TCOD_BKGND_SET, "NPC List");

    int count = 0;
	for (std::map<int,boost::shared_ptr<NPC> >::iterator npci = Game::Inst()->npcList.begin(); npci != Game::Inst()->npcList.end(); ++npci) {
	    if (count++ >= scroll) {
	        TCODConsole::root->print(topX+1, topY+1+(count-scroll), "NPC: %d", npci->second->Uid());
	        TCODConsole::root->print(topX+10, topY+1+(count-scroll), "%s: %s",
                npci->second->currentJob().lock() ? npci->second->currentJob().lock()->name.c_str() : "No job",
                npci->second->currentTask() ? Job::ActionToString(npci->second->currentTask()->action).c_str() : "No task");
	    }
	}

	TCODConsole::root->putChar(topX+width-2, topY+1, TCOD_CHAR_ARROW_N, TCOD_BKGND_SET);
	TCODConsole::root->putChar(topX+width-2, topY+height-2, TCOD_CHAR_ARROW_S, TCOD_BKGND_SET);
	TCODConsole::root->putChar(topX+width-2, scrollBar, 219, TCOD_BKGND_SET);
}

MenuResult NPCMenu::Update(int x, int y) {
	if (x > topX && x < Game::Inst()->ScreenWidth()-10 && y > topY && y < Game::Inst()->ScreenHeight()-10) {
		if (x == topX+width-2 && y == topY+1) ScrollUp();
		if (x == topX+width-2 && y == topY+height-2) ScrollDown();
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

void ConstructionMenu::Draw(int, int) {
    TCODConsole::root->setForegroundColor(TCODColor::white);
    TCODConsole::root->printFrame(topX, topY, 50, 5, true, TCOD_BKGND_SET, construct->Name().c_str());
    TCODConsole::root->setForegroundColor(TCODColor::green);
    TCODConsole::root->print(topX + 3, topY + 2, "Rename");
    TCODConsole::root->setForegroundColor(TCODColor::red);
    TCODConsole::root->print(topX + 13, topY + 2, "Dismantle");
    TCODConsole::root->setForegroundColor(TCODColor::white);

    //Only draw the production queue and product list if construction is a producer
    if (construct->Producer()) {
        TCODConsole::root->printFrame(topX+25, topY+5, 25, 50, true, TCOD_BKGND_SET, "Product list");
        TCODConsole::root->printFrame(topX, topY+5, 25, 50, true, TCOD_BKGND_SET, "Production queue");
        TCODConsole::root->putChar(topX+25, topY+10, TCOD_CHAR_ARROW2_W, TCOD_BKGND_SET);
        TCODConsole::root->putChar(topX+25, topY+25, TCOD_CHAR_ARROW2_W, TCOD_BKGND_SET);
        TCODConsole::root->putChar(topX+25, topY+40, TCOD_CHAR_ARROW2_W, TCOD_BKGND_SET);
        TCODConsole::root->putChar(topX+24, topY+10, TCOD_CHAR_ARROW2_W, TCOD_BKGND_SET);
        TCODConsole::root->putChar(topX+24, topY+25, TCOD_CHAR_ARROW2_W, TCOD_BKGND_SET);
        TCODConsole::root->putChar(topX+24, topY+40, TCOD_CHAR_ARROW2_W, TCOD_BKGND_SET);

        TCODConsole::root->putChar(topX+48, topY+6, TCOD_CHAR_ARROW_N, TCOD_BKGND_SET);
        TCODConsole::root->putChar(topX+48, topY+53, TCOD_CHAR_ARROW_S, TCOD_BKGND_SET);

        int y = 0;
        for (int prodi = scroll; prodi < (signed int)construct->Products()->size() && y < topY+50; ++prodi) {
            TCODConsole::root->setForegroundColor(TCODColor::white);
            TCODConsole::root->print(topX+26, topY+6+y, "%s x%d", Item::ItemTypeToString(construct->Products(prodi)).c_str(), Item::Presets[construct->Products(prodi)].multiplier);
            if (firstTimeDraw) productPlacement.push_back(topY+6+y);
            ++y;
            for (int compi = 0; compi < (signed int)Item::Components(construct->Products(prodi)).size() && y < topY+50; ++compi) {
                TCODConsole::root->setForegroundColor(TCODColor::white);
                TCODConsole::root->putChar(topX+27, topY+6+y, compi+1 < (signed int)Item::Components(construct->Products(prodi)).size() ? TCOD_CHAR_TEEE : TCOD_CHAR_SW, TCOD_BKGND_SET);
                TCODConsole::root->setForegroundColor(TCODColor::grey);
                TCODConsole::root->print(topX+28, topY+6+y, Item::ItemCategoryToString(Item::Components(construct->Products(prodi), compi)).c_str());
                ++y;
            }
            ++y;
        }

        for (int jobi = 0; jobi < (signed int)construct->JobList()->size(); ++jobi) {
            TCODConsole::root->setForegroundColor(jobi == 0 ? TCODColor::white : TCODColor::grey);
            TCODConsole::root->print(topX+2, topY+6+jobi, Item::ItemTypeToString(construct->JobList(jobi)).c_str());
        }

        firstTimeDraw = false;
	} else if (construct->IsFarmplot()) { //It's a farmplot

	} else if (construct->IsStockpile()) { //A stockpile, but not a farmplot
        Stockpile* sp = static_cast<Stockpile*>(construct);
        TCODConsole::root->setForegroundColor(TCODColor::white);
        TCODConsole::root->printFrame(topX, topY+5, 50, 50, true, TCOD_BKGND_SET, "Item categories allowed");
        int x = topX+2;
        int y = topY+6;
        for (unsigned int i = 0; i < Item::Categories.size(); ++i) {
            TCODConsole::root->setForegroundColor(sp->Allowed(i) ? TCODColor::green : TCODColor::red);
            TCODConsole::root->print(x,y, Item::Categories[i].name.c_str());
            ++y;
            if (i != 0 && i % 49 == 0) {
                x += 20;
                y = topY+6;
            }
        }
    }

    TCODConsole::root->setForegroundColor(TCODColor::white);

}

MenuResult ConstructionMenu::Update(int x, int y) {
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
