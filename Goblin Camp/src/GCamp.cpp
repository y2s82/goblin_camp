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
#include <windows.h>
#include <boost/multi_array.hpp>
#include <boost/thread/thread.hpp>
#include <boost/format.hpp>
#include <sstream>
#include <list>
#include <set>
#include <queue>
#include <iostream>
#include <cctype>
#include <cstdlib>

#include "GCamp.hpp"
#include "Game.hpp"
#include "Tile.hpp"
#include "Map.hpp"
#include "NPC.hpp"
#include "Logger.hpp"
#include "Coordinate.hpp"
#include "Announce.hpp"
#include "UI.hpp"
#include "JobManager.hpp"


int main(std::string cmdLine) {
	int width = -1, height = -1;
    bool fullscreen = false;

	TCODList<char*> list =  TCODSystem::getDirectoryContent("./", "config.ini");
	if (!list.isEmpty()) {
		ticpp::Document iniFile("config.ini");
		try {
			iniFile.LoadFile();
		} catch (ticpp::Exception& ex) {
			Logger::Inst()->output<<"Failed loading config.ini\n"<<ex.what();
			Logger::Inst()->output<<"Using default values";
			goto CONTINUEMAIN;
		}

		try {
			int intVal;
			ticpp::Element* parent = iniFile.FirstChildElement();
			ticpp::Iterator<ticpp::Node> node;
			for (node = node.begin(parent); node != node.end(); ++node) {
				if (node->Value() == "fullscreen") {
					fullscreen = (node->ToElement()->GetText() == "true") ? true : false;
				} else if (node->Value() == "width") {
					node->ToElement()->GetText(&intVal);
					width = intVal;
				} else if (node->Value() == "height") {
					node->ToElement()->GetText(&intVal);
					height = intVal;
				}
			}
		} catch (ticpp::Exception& ex) {
			Logger::Inst()->output<<"Failed reading config.ini\n"<<ex.what();
			Logger::Inst()->output<<"Using default values";
			width = -1; height = -1; fullscreen = false;
		}
	}
	CONTINUEMAIN:

	Game::Inst()->Init(width,height,fullscreen);
	return MainMenu();
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR cmdLine, int)
{
    return main(cmdLine);
}

void MainLoop() {
	for (int npcs = 0; npcs < 10; ++npcs) {
		Game::Inst()->CreateNPC(Coordinate(rand() % 20 + 200, rand() % 20 + 200), 1);
	}
	for (int npcs = 0; npcs < 15; ++npcs) {
		Game::Inst()->CreateNPC(Coordinate(rand() % 20 + 200, rand() % 20 + 200), 0);
		Game::Inst()->CreateNPC(Coordinate(rand() % 20 + 250, rand() % 20 + 200), 2);
	}

	for (int seeds = 0; seeds < 20; ++seeds) {
		Game::Inst()->CreateItem(Coordinate(220, 220), Item::StringToItemType("Bloodberry seed"), true);
	}

	Announce::Inst()->AddMsg("Press 'h' for keyboard shortcuts", TCODColor::cyan);

	while(true) {

		if (Game::toMainMenu) {
			Game::toMainMenu = false;
			return;
		}
		UI::Inst()->Update();
		if (!Game::Inst()->Paused()) {
			Game::Inst()->Update();
			Announce::Inst()->Update();
			JobManager::Inst()->Update();
		}

		Game::Inst()->buffer->flush();
        Game::Inst()->Draw();
		Game::Inst()->FlipBuffer();
	}
}

int Distance(int x0, int y0, int x1, int y1) {
	return (abs(y1 - y0) + abs(x1 - x0));
}

int Distance(Coordinate a, Coordinate b) {
	return Distance(a.x(), a.y(), b.x(), b.y());
}

int MainMenu() {
	bool exit = false;
	int width = 20;
	int edgex = Game::Inst()->ScreenWidth()/2 - width/2;
	int height = 13;
	int edgey = Game::Inst()->ScreenHeight()/2 - height/2;
	int selected = -1;
	TCOD_mouse_t mouseStatus;
	TCOD_key_t key;
	bool endCredits = false;
	while (!exit) {
		TCODConsole::root->setForegroundColor(TCODColor::white);
		TCODConsole::root->setBackgroundColor(TCODColor::black);
		TCODConsole::root->clear();

		TCODConsole::root->printFrame(edgex, edgey, width, height, true, TCOD_BKGND_DEFAULT,
			"Main Menu");

		TCODConsole::root->setAlignment(TCOD_CENTER);

		mouseStatus = TCODMouse::getStatus();
		key = TCODConsole::checkForKeypress(TCOD_KEY_PRESSED);

		if (mouseStatus.cx > edgex && mouseStatus.cx < edgex+width) {
			selected = mouseStatus.cy - (edgey+2);
		} else selected = -1;

		if (mouseStatus.lbutton_pressed) {
			if (selected == 0) MainLoop();
			else if (selected == 2) MainLoop();
			else if (selected == 4) LoadMenu();
			else if (selected == 6) SaveMenu();
			else if (selected == 8) exit = true;
		}
		TCODConsole::root->setBackgroundFlag(TCOD_BKGND_SET);
		if (selected == 0) {
			TCODConsole::root->setForegroundColor(TCODColor::black);
			TCODConsole::root->setBackgroundColor(TCODColor::white);
		} else {
			TCODConsole::root->setForegroundColor(TCODColor::white);
			TCODConsole::root->setBackgroundColor(TCODColor::black);
		}
		TCODConsole::root->print(edgex+width/2, edgey+2, "New Game");

		if (selected == 2) {
			TCODConsole::root->setForegroundColor(TCODColor::black);
			TCODConsole::root->setBackgroundColor(TCODColor::white);
		} else {
			TCODConsole::root->setForegroundColor(TCODColor::white);
			TCODConsole::root->setBackgroundColor(TCODColor::black);
		}
		TCODConsole::root->print(edgex+width/2, edgey+4, "Continue");

		if (selected == 4) {
			TCODConsole::root->setForegroundColor(TCODColor::black);
			TCODConsole::root->setBackgroundColor(TCODColor::white);
		} else {
			TCODConsole::root->setForegroundColor(TCODColor::white);
			TCODConsole::root->setBackgroundColor(TCODColor::black);
		}
		TCODConsole::root->print(edgex+width/2, edgey+6, "Load");

		if (selected == 6) {
			TCODConsole::root->setForegroundColor(TCODColor::black);
			TCODConsole::root->setBackgroundColor(TCODColor::white);
		} else {
			TCODConsole::root->setForegroundColor(TCODColor::white);
			TCODConsole::root->setBackgroundColor(TCODColor::black);
		}

		TCODConsole::root->print(edgex+width/2, edgey+8, "Save");

		if (selected == 8) {
			TCODConsole::root->setForegroundColor(TCODColor::black);
			TCODConsole::root->setBackgroundColor(TCODColor::white);
		} else {
			TCODConsole::root->setForegroundColor(TCODColor::white);
			TCODConsole::root->setBackgroundColor(TCODColor::black);
		}
		TCODConsole::root->print(edgex+width/2, edgey+10, "Exit");

		if (key.c == 'q' || key.vk == TCODK_ESCAPE) exit = true;
		if (key.c == 'n') MainLoop();

		if (!endCredits) endCredits = TCODConsole::renderCredits(Game::Inst()->ScreenWidth()-20, 
			Game::Inst()->ScreenHeight()-3, true);

		TCODConsole::root->flush();

	}
	Game::Inst()->Exit();
	return 0;
}

void LoadMenu() {
	bool exit = false;
	int width = 20;
	int edgex = Game::Inst()->ScreenWidth()/2 - width/2;
	int selected = -1;
	TCOD_mouse_t mouseStatus;
	TCOD_key_t key;
	TCODList<char*> list = TCODSystem::getDirectoryContent("./saves/", "*.sav");
	int height = list.size()+4;
	int edgey = Game::Inst()->ScreenHeight()/2 - height/2;

	TCODConsole::root->setAlignment(TCOD_LEFT);

	while (!exit) {
		TCODConsole::root->clear();
		mouseStatus = TCODMouse::getStatus();
		if (mouseStatus.cx > edgex && mouseStatus.cx < edgex+width) {
			selected = mouseStatus.cy - (edgey+2);
		} else selected = -1;

		if (selected < list.size() && selected >= 0 && mouseStatus.lbutton) {
			Game::Inst()->LoadGame((boost::format("./saves/%s") % (list.get(selected))).str().c_str());
			MainLoop();
			return;
		}

		TCODConsole::root->printFrame(edgex, edgey, width, height, true, TCOD_BKGND_SET, "Saved games");

		for (int i = 0; i < list.size(); ++i) {
			if (selected == i) {
				TCODConsole::root->setForegroundColor(TCODColor::black);
				TCODConsole::root->setBackgroundColor(TCODColor::white);
			} else {
				TCODConsole::root->setForegroundColor(TCODColor::white);
				TCODConsole::root->setBackgroundColor(TCODColor::black);
			}
			TCODConsole::root->print(edgex+1, edgey+2+i, list.get(i));
		}
		TCODConsole::root->setForegroundColor(TCODColor::white);
		TCODConsole::root->setBackgroundColor(TCODColor::black);

		TCODConsole::root->flush();
	}
}

void SaveMenu() {
	UI::Inst()->SetTextMode(true);
	std::string saveName;
	UI::Inst()->Update();
	Game::Inst()->SaveGame("./saves/testsave.sav");
	return;
}