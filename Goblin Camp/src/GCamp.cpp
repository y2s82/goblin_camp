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

#include <libtcod.hpp>
#ifdef WINDOWS
#	define WIN32_LEAN_AND_MEAN
#	include <windows.h>
#endif
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <cstdlib>
#include <sys/types.h>
#include <sys/stat.h>

#include "GCamp.hpp"
#include "Game.hpp"
#include "Map.hpp"
#include "NPC.hpp"
#include "Logger.hpp"
#include "Coordinate.hpp"
#include "Announce.hpp"
#include "UI.hpp"
#include "JobManager.hpp"

#ifndef GC_VERSION
#	define GC_VERSION "<version undefined>"
#endif

#ifdef WINDOWS
// Win32CrashHandler.cpp
void InstallExceptionHandler();
#endif

int main() {
#	ifdef WINDOWS
	InstallExceptionHandler();
	std::string config = "./config.ini";
#	else
	std::string config = std::string(getenv("HOME")) + "/.goblincamp/config.ini";
#	endif
	
	struct stat buffer;
	if (stat(config.c_str(), &buffer) == 0) {
		Game::Inst()->LoadConfig(config);
	}
	
	Game::Inst()->Init();
	return MainMenu();
}

#ifdef WINDOWS
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR cmdLine, int)
{ return main(); }
#endif

void MainLoop() {
	Game* game = Game::Inst();
	if (!game->Running()) Announce::Inst()->AddMsg("Press 'h' for keyboard shortcuts", TCODColor::cyan);
	game->Running(true);

	bool update = false;
	int elapsedMilli;
	int targetMilli = 1000 / (UPDATES_PER_SECOND*2);
	int startMilli = TCODSystem::getElapsedMilli();
	while(true) {

		if (Game::ToMainMenu()) {
			Game::ToMainMenu(false);
			return;
		}

		UI::Inst()->Update();
		if (update) {
			if (!game->Paused()) {
				game->Update();
				Announce::Inst()->Update();
				JobManager::Inst()->Update();
			}

			game->buffer->flush();
			game->Draw();
			game->FlipBuffer();
		}
		update = !update;

		elapsedMilli = TCODSystem::getElapsedMilli() - startMilli;
		startMilli = TCODSystem::getElapsedMilli();
		if (elapsedMilli < targetMilli) TCODSystem::sleepMilli(targetMilli - elapsedMilli);
	}
}

void StartNewGame() {
	Game* game = Game::Inst();

	for (int npcs = 0; npcs < 15; ++npcs) {
		game->CreateNPC(Coordinate(rand() % 20 + 200, rand() % 20 + 200), NPC::StringToNPCType("goblin"));
	}
	for (int npcs = 0; npcs < 6; ++npcs) {
		game->CreateNPC(Coordinate(rand() % 20 + 200, rand() % 20 + 200), NPC::StringToNPCType("orc"));
	}

	for (int seeds = 0; seeds < 20; ++seeds) {
		game->CreateItem(Coordinate(220+rand()%10, 220+rand()%10), Item::StringToItemType("Bloodberry seed"), true);
	}
	MainLoop();
}

int Distance(int x0, int y0, int x1, int y1) {
	return (abs(y1 - y0) + abs(x1 - x0));
}

int Distance(Coordinate a, Coordinate b) {
	return Distance(a.X(), a.Y(), b.X(), b.Y());
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

	bool lButtonDown = false;
	while (!exit) {
		TCODConsole::root->setForegroundColor(TCODColor::white);
		TCODConsole::root->setBackgroundColor(TCODColor::black);
		TCODConsole::root->clear();

		TCODConsole::root->printFrame(edgex, edgey, width, height, true, TCOD_BKGND_DEFAULT,
			"Main Menu");

		TCODConsole::root->setAlignment(TCOD_CENTER);

		mouseStatus = TCODMouse::getStatus();
		if (mouseStatus.lbutton) {
			lButtonDown = true;
		}
		key = TCODConsole::checkForKeypress(TCOD_KEY_RELEASED);


		if (mouseStatus.cx > edgex && mouseStatus.cx < edgex+width) {
			selected = mouseStatus.cy - (edgey+2);
		} else selected = -1;

		if (!mouseStatus.lbutton && lButtonDown) {
			lButtonDown = false;
			if (selected == 0) StartNewGame();
			else if (selected == 2 && Game::Inst()->Running()) MainLoop();
			else if (selected == 4) LoadMenu();
			else if (selected == 6 && Game::Inst()->Running()) SaveMenu();
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
		if (!Game::Inst()->Running()) TCODConsole::root->setForegroundColor(TCODColor::grey);
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
		if (!Game::Inst()->Running()) TCODConsole::root->setForegroundColor(TCODColor::grey);
		TCODConsole::root->print(edgex+width/2, edgey+8, "Save");

		if (selected == 8) {
			TCODConsole::root->setForegroundColor(TCODColor::black);
			TCODConsole::root->setBackgroundColor(TCODColor::white);
		} else {
			TCODConsole::root->setForegroundColor(TCODColor::white);
			TCODConsole::root->setBackgroundColor(TCODColor::black);
		}
		TCODConsole::root->print(edgex+width/2, edgey+10, "Exit");

		if (key.c == 'q') exit = true;
		else if (key.c == 'n') StartNewGame();

		TCODConsole::root->setForegroundColor(TCODColor::celadon);
		TCODConsole::root->setBackgroundColor(TCODColor::black);
		TCODConsole::root->print(edgex+width/2, edgey-3, "Goblin Camp " GC_VERSION);
		if (!endCredits) endCredits = TCODConsole::renderCredits(edgex+5, 
			edgey+25, true);

		TCODConsole::root->flush();

	}
	Game::Inst()->Exit();
	return 0;
}

#if defined(WINDOWS)
#	define SAVE_DIR  "./saves/"
#	define SAVE_FILE SAVE_DIR "%s"
#else
#	define SAVE_DIR  GetSaveDir().c_str()
#	define SAVE_FILE GetSaveFile()
inline std::string GetSaveDir() {
	// superfluous calls, but whatever
	std::string home = std::string(getenv("HOME"));
	TCODSystem::createDirectory((home + "/.goblincamp").c_str());
	TCODSystem::createDirectory((home + "/.goblincamp/saves").c_str());
	return home + std::string("/.goblincamp/saves/");
}
inline std::string GetSaveFile() {
	return getSaveDir() + std::string("%s");
}
#endif

void LoadMenu() {
	bool exit = false;
	int width = 30;
	int edgex = Game::Inst()->ScreenWidth()/2 - width/2;
	int selected = -1;
	TCOD_mouse_t mouseStatus;
	TCODList<char*> list = TCODSystem::getDirectoryContent(SAVE_DIR, "*.sav");
	int height = list.size()+5;
	int edgey = Game::Inst()->ScreenHeight()/2 - height/2;

	TCODConsole::root->setAlignment(TCOD_CENTER);

	bool lButtonDown = false;

	while (!exit) {

		TCODConsole::root->clear();
		mouseStatus = TCODMouse::getStatus();
		if (mouseStatus.lbutton) {
			lButtonDown = true;
		}

		if (mouseStatus.cx > edgex && mouseStatus.cx < edgex+width) {
			selected = mouseStatus.cy - (edgey+2);
		} else selected = -1;

		if (selected < list.size() && selected >= 0 && !mouseStatus.lbutton && lButtonDown) {
			Game::Inst()->LoadGame((boost::format(SAVE_FILE) % (list.get(selected))).str().c_str());
			MainLoop();
			lButtonDown = false;
			return;
		} else if (selected == list.size()+1 && !mouseStatus.lbutton && lButtonDown) {
			lButtonDown = false;
			return;
		}

		TCODConsole::root->printFrame(edgex, edgey, width, height, true, TCOD_BKGND_SET, "Saved games");

		for (int i = 0; i <= list.size()+1; ++i) {
			if (selected == i) {
				TCODConsole::root->setForegroundColor(TCODColor::black);
				TCODConsole::root->setBackgroundColor(TCODColor::white);
			} else {
				TCODConsole::root->setForegroundColor(TCODColor::white);
				TCODConsole::root->setBackgroundColor(TCODColor::black);
			}
			if (i < list.size()) TCODConsole::root->print(edgex+width/2, edgey+2+i, list.get(i));
			else if (i == list.size()+1) TCODConsole::root->print(edgex+width/2, edgey+2+i, "Cancel");
		}
		TCODConsole::root->setForegroundColor(TCODColor::white);
		TCODConsole::root->setBackgroundColor(TCODColor::black);

		TCODConsole::root->flush();
	}
}

void SaveMenu() {
	if (!Game::Inst()->Running()) return;
	std::string saveName;
	while (true) {
		TCOD_key_t key = TCODConsole::checkForKeypress(TCOD_KEY_RELEASED);
		if (key.c >= ' ' && key.c <= '}' && saveName.size() < 28) {
			saveName.push_back(key.c);
		} else if (key.vk == TCODK_BACKSPACE && saveName.size() > 0) {
			saveName.erase(saveName.end() - 1);
		}

		if (key.vk == TCODK_ESCAPE) return;
		else if (key.vk == TCODK_ENTER || key.vk == TCODK_KPENTER) {
			TCODConsole::root->clear();
			TCODConsole::root->printFrame(Game::Inst()->ScreenWidth()/2-5, 
				Game::Inst()->ScreenHeight()/2-3, 10, 2, true, TCOD_BKGND_SET, "SAVING");
			TCODConsole::root->flush();
			TCODSystem::createDirectory(SAVE_DIR);
			Game::Inst()->SaveGame((boost::format(SAVE_FILE) % saveName).str() + ".sav");
			break;
		}

		TCODConsole::root->clear();
		TCODConsole::root->printFrame(Game::Inst()->ScreenWidth()/2-15, 
			Game::Inst()->ScreenHeight()/2-3, 30, 3, true, TCOD_BKGND_SET, "Save name");
		TCODConsole::root->print(Game::Inst()->ScreenWidth()/2, 
			Game::Inst()->ScreenHeight()/2-2, "%s", saveName.c_str());
		TCODConsole::root->flush();

	}
	return;
}
