#include <libtcod.hpp>
#include <windows.h>
#include <boost/multi_array.hpp>
#include <boost/thread/thread.hpp>
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
#include "Graphics.hpp"


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
	mainLoop();
	return 0;
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR cmdLine, int)
{
    main(cmdLine);
    return 0;
}

void mainLoop() {
	LARGE_INTEGER timeStart, timeNow, freq;
	int logicTimer = 0, timerAddition;

	TCOD_key_t key;

	std::stringstream sstream;

	QueryPerformanceFrequency(&freq);

	for (int npcs = 0; npcs < 100; ++npcs) {
		Game::Inst()->CreateNPC(Coordinate(rand() % 20 + 200, rand() % 20 + 200), 0);
	}
	for (int npcsiks = 0; npcsiks < 10; ++npcsiks) {
		Game::Inst()->CreateNPC(Coordinate(rand() % 20 + 200, rand() % 20 + 200), 1);
		Game::Inst()->CreateNPC(Coordinate(rand() % 20 + 250, rand() % 20 + 200), 2);
	}

	QueryPerformanceCounter(&timeStart);

    Game::Inst()->center = Coordinate(250,250);

	while(true) {
		while (logicTimer >= (1000 / UPDATES_PER_SECOND)) {
			key = TCODConsole::checkForKeypress(TCOD_KEY_PRESSED);
			logicTimer -= (1000 / UPDATES_PER_SECOND);
			if (key.c == 'w') { if (Game::Inst()->center.y(Game::Inst()->center.y()-1) < (Game::Inst()->ScreenHeight() / 2)-1) Game::Inst()->center.y((Game::Inst()->ScreenHeight() / 2)-1); }
			if (key.c == 's') { if (Game::Inst()->center.y(Game::Inst()->center.y()+1) > 1+ GameMap::Inst()->Height() - (Game::Inst()->ScreenHeight() / 2)) Game::Inst()->center.y(1+ GameMap::Inst()->Height() - (Game::Inst()->ScreenHeight() / 2)); }
			if (key.c == 'a') { if (Game::Inst()->center.x(Game::Inst()->center.x()-1) < (Game::Inst()->ScreenWidth() / 2)-1) Game::Inst()->center.x((Game::Inst()->ScreenWidth() / 2)-1); }
			if (key.c == 'd') { if (Game::Inst()->center.x(Game::Inst()->center.x()+1) > 1+ GameMap::Inst()->Width() - (Game::Inst()->ScreenWidth() / 2)) Game::Inst()->center.x(1+ GameMap::Inst()->Width() - (Game::Inst()->ScreenWidth() / 2)); }
			if (key.c == 'q') Game::Exit();
			if (key.vk == TCODK_PRINTSCREEN) TCODSystem::saveScreenshot(0);
			if (key.c == 'v') {
				for (int i = 0; i < 3; ++i) {
					for (int e = 0; e < 3; ++e) {
						Game::Inst()->CreateWater(Coordinate(200+i,200+e), 10000);
					}
				}
			}
			if (key.c == 'g') Game::Inst()->CreateItem(Coordinate(200,200), BERRYSEED, true);
			if (key.c == 'k') Game::Inst()->CreateItem(Coordinate(200,200), 16, true);

			UI::Inst()->Update(key, Game::Inst()->center);
			Game::Inst()->Update();
			Announce::Inst()->Update();
			JobManager::Inst()->Update();
		}

        TCODConsole::root->clear();

		GameMap::Inst()->Draw(Game::Inst()->center);
        Game::Inst()->Draw(Game::Inst()->center);
		UI::Inst()->Draw(Game::Inst()->center);

		Announce::Inst()->Draw(Coordinate(Game::Inst()->ScreenWidth()/2, Game::Inst()->ScreenHeight()-1), 5);

		TCODConsole::flush();

		//This "weirdness" is required because the loop is not guaranteed to take longer than 1ms to execute
		QueryPerformanceCounter(&timeNow);
		timerAddition = (int)((timeNow.QuadPart - timeStart.QuadPart) * 1000 / freq.QuadPart);
		logicTimer += timerAddition;
		if (timerAddition >= 1)	QueryPerformanceCounter(&timeStart);
		if (timerAddition < 25) TCODSystem::sleepMilli(25-timerAddition); //Stops gcamp from maxing CPU for no reason
	}
}

int distance(int x0, int y0, int x1, int y1) {
	return (abs(y1 - y0) + abs(x1 - x0));
}
