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
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>
#include <cstdlib>
#include <cmath>
#include <algorithm>
#include <functional>

#include "Camp.hpp"
#include "GCamp.hpp"
#include "Game.hpp"
#include "Map.hpp"
#include "Logger.hpp"
#include "Coordinate.hpp"
#include "Announce.hpp"
#include "UI.hpp"
#include "data/Paths.hpp"
#include "data/Config.hpp"
#include "data/Data.hpp"
#include "data/Mods.hpp"
#include "NPC.hpp"
#include "Item.hpp"
#include "scripting/Engine.hpp"
#include "scripting/Event.hpp"

#include "Version.hpp"

int MainMenu();
void LoadMenu();
void SaveMenu();
void SettingsMenu();
void KeysMenu();
void ModsMenu();

int GCMain(std::vector<std::string>& args) {
	int exitcode = 0;
	
	//
	// Bootstrap phase.
	//
	Paths::Init();
	Config::Init();
	Script::Init(args);
	
	//
	// Load phase.
	//
	Data::LoadConfig();
	Data::LoadFont();
	
	Game::Inst()->Init();
	#ifdef MACOSX
	Data::LoadFont();
	#endif
	
	Mods::Load();
	
	//
	// Parse command line.
	//
	LOG("args.size() = " << args.size());
	
	if (std::find(args.begin(), args.end(), "-boottest") == args.end()) {
		exitcode = MainMenu();
	} else {
		LOG("Bootstrap test, going into shutdown.");
	}
	
	//
	// Shutdown.
	//
	Script::Shutdown();
	return exitcode;
}

void MainLoop() {
	Game* game = Game::Inst();
	if (!game->Running()) {
		Announce::Inst()->AddMsg("Press 'h' for keyboard shortcuts", TCODColor::cyan);
	}
	game->Running(true);

	bool update = false;
	int elapsedMilli;
	int targetMilli = 1000 / (UPDATES_PER_SECOND*2);
	int startMilli = TCODSystem::getElapsedMilli();
	while (game->Running()) {

		if (Game::ToMainMenu()) {
			Game::ToMainMenu(false);
			return;
		}

		UI::Inst()->Update();
		if (update) {
			if (!game->Paused()) {
				game->Update();
				Announce::Inst()->Update();
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
	
	Script::Event::GameEnd();
}

void StartNewGame() {
	Game* game = Game::Inst();
	game->Reset();
	
	Script::Event::GameStart();
	game->GenerateMap();

	std::priority_queue<std::pair<int, Coordinate> > spawnCenterCandidates;

	for (unsigned int i = 0; i < 20; ++i) {
		std::pair<int,Coordinate> candidate(0, 
			Coordinate(100 + rand() % (Map::Inst()->Width()-200), 
			100 + rand() % (Map::Inst()->Height()-200)));

		int riverDistance = 1000, hillDistance = 1000;
		int lineX, lineY;

		for (int i = 0; i < 4; ++i) {
			switch (i) {
			case 0:
				lineX = candidate.second.X() - 200;
				lineY = candidate.second.Y();
				break;
			case 1:
				lineX = candidate.second.X() + 200;
				lineY = candidate.second.Y();
				break;
			case 2:
				lineX = candidate.second.X();
				lineY = candidate.second.Y() - 200;
				break;
			case 3:
				lineX = candidate.second.X();
				lineY = candidate.second.Y() + 200;
				break;
			}
			int distance = 200;
			TCODLine::init(lineX, lineY, candidate.second.X(), candidate.second.Y());
			do {
				if (lineX >= 0 && lineX < Map::Inst()->Width() && lineY >= 0 && lineY < Map::Inst()->Height()) {
					if (Map::Inst()->Type(lineX, lineY) == TILEDITCH || Map::Inst()->Type(lineX, lineY) == TILERIVERBED) {
						if (distance < riverDistance) riverDistance = distance;
						if (distance < 25) riverDistance = 2000;
					} else if (Map::Inst()->Type(lineX, lineY) == TILEROCK) {
						if (distance < hillDistance) hillDistance = distance;
					}
				}
				--distance;
			} while (!TCODLine::step(&lineX, &lineY));
		}

		candidate.first = -hillDistance - riverDistance;
		if (Map::Inst()->Type(candidate.second.X(), candidate.second.Y()) != TILEGRASS) candidate.first -= 10000;
		spawnCenterCandidates.push(candidate);
	}

	Coordinate spawnTopCorner(spawnCenterCandidates.top().second.X()-20,spawnCenterCandidates.top().second.Y()-20);
	Coordinate spawnBottomCorner(spawnCenterCandidates.top().second.X()+20,spawnCenterCandidates.top().second.Y()+20);

	//Clear starting area
	for (int x = spawnTopCorner.X(); x < spawnBottomCorner.X(); ++x) {
		for (int y = spawnTopCorner.Y(); y < spawnBottomCorner.Y(); ++y) {
			if (Map::Inst()->NatureObject(x,y) >= 0 && rand() % 3 < 2) {
				game->RemoveNatureObject(game->natureList[Map::Inst()->NatureObject(x,y)]);
			}
		}
	}

	game->CreateNPCs(15, NPC::StringToNPCType("goblin"), spawnTopCorner, spawnBottomCorner);
	game->CreateNPCs(6, NPC::StringToNPCType("orc"), spawnTopCorner, spawnBottomCorner);

	game->CreateItems(20, Item::StringToItemType("Bloodberry seed"), spawnTopCorner, spawnBottomCorner);
	game->CreateItems(20, Item::StringToItemType("Blueleaf seed"), spawnTopCorner, spawnBottomCorner);
	game->CreateItems(20, Item::StringToItemType("Bread"), spawnTopCorner, spawnBottomCorner);

	Camp::Inst()->SetCenter(spawnCenterCandidates.top().second);
	game->CenterOn(spawnCenterCandidates.top().second);

	MainLoop();
}

// XXX: This really needs serious refactoring.
namespace {
	struct MainMenuEntry {
		const char *label;
		char shortcut;
		bool (*isActive)();
		void (*function)();
	};
	
	bool ActiveAlways() {
		return true;
	}
	
	bool ActiveIfRunning() {
		return Game::Inst()->Running();
	}
	
	int savesCount = -1;
	
	bool ActiveIfHasSaves() {
		if (savesCount == -1) savesCount = Data::CountSavedGames();
		return savesCount > 0;
	}
}

int MainMenu() {
	MainMenuEntry entries[] = {
		{"New Game", 'n', &ActiveAlways,     &StartNewGame},
		{"Continue", 'c', &ActiveIfRunning,  &MainLoop},
		{"Load",     'l', &ActiveIfHasSaves, &LoadMenu},
		{"Save",     's', &ActiveIfRunning,  &SaveMenu},
		{"Settings", 'o', &ActiveAlways,     &SettingsMenu},
		{"Keys",     'k', &ActiveAlways,     &KeysMenu},
		{"Mods",     'm', &ActiveAlways,     &ModsMenu},
		{"Exit",     'q', &ActiveAlways,     NULL}
	};

	const unsigned int entryCount = sizeof(entries) / sizeof(MainMenuEntry);
	void (*function)() = NULL;
	
	bool exit = false;
	int width = 20;
	int edgex = Game::Inst()->ScreenWidth()/2 - width/2;
	int height = (entryCount * 2) + 2;
	int edgey = Game::Inst()->ScreenHeight()/2 - height/2;
	int selected = -1;
	TCOD_mouse_t mouseStatus;
	TCOD_key_t key;
	bool endCredits = false;
	bool lButtonDown = false;

	while (!exit) {
		key = TCODConsole::checkForKeypress(TCOD_KEY_RELEASED);

		TCODConsole::root->setDefaultForeground(TCODColor::white);
		TCODConsole::root->setDefaultBackground(TCODColor::black);
		TCODConsole::root->clear();

		TCODConsole::root->printFrame(edgex, edgey, width, height, true, TCOD_BKGND_DEFAULT, "Main Menu");

		TCODConsole::root->setAlignment(TCOD_CENTER);
		TCODConsole::root->setBackgroundFlag(TCOD_BKGND_SET);

		TCODConsole::root->setDefaultForeground(TCODColor::celadon);
		TCODConsole::root->print(edgex+width/2, edgey-3, GC_VERSION);
		TCODConsole::root->setDefaultForeground(TCODColor::white);

		for (unsigned int idx = 0; idx < entryCount; ++idx) {
			const MainMenuEntry& entry = entries[idx];

			if (selected == (idx * 2)) {
				TCODConsole::root->setDefaultForeground(TCODColor::black);
				TCODConsole::root->setDefaultBackground(TCODColor::white);
			} else {
				TCODConsole::root->setDefaultForeground(TCODColor::white);
				TCODConsole::root->setDefaultBackground(TCODColor::black);
			}

			if (!entry.isActive()) {
				TCODConsole::root->setDefaultForeground(TCODColor::grey);
			}

			TCODConsole::root->print(edgex + width / 2, edgey + ((idx + 1) * 2), entry.label);

			if (entry.shortcut != NULL && key.c == entry.shortcut && entry.isActive()) {
				exit     = (entry.function == NULL);
				function = entry.function;
			}
		}

		if (!endCredits) {
			endCredits = TCODConsole::renderCredits(edgex + 5, edgey + 25, true);
		}

		TCODConsole::root->flush();

		mouseStatus = TCODMouse::getStatus();
		if (mouseStatus.lbutton) {
			lButtonDown = true;
		}

		if (function != NULL) {
			function();
		} else {
			if (mouseStatus.cx > edgex && mouseStatus.cx < edgex+width) {
				selected = mouseStatus.cy - (edgey+2);
			} else selected = -1;

			if (!mouseStatus.lbutton && lButtonDown) {
				lButtonDown = false;
				int entry = static_cast<int>(floor(selected / 2.));

				if (entry < entryCount && entries[entry].isActive()) {
					if (entries[entry].function == NULL) {
						exit = true;
					} else {
						entries[entry].function();
					}
				}
			}
		}
	}
	
	return 0;
}

void LoadMenu() {
	int width = 59; // 2 for borders, 20 for filename, 2 spacer, 20 for date, 2 spacer, 13 for filesize
	int edgex = Game::Inst()->ScreenWidth()/2 - width/2;
	int selected = -1;
	TCOD_mouse_t mouseStatus;

	std::vector<Data::Save> list;
	Data::GetSavedGames(list);
	
	// sort by last modification, newest on top
	std::sort(list.begin(), list.end(), std::greater<Data::Save>());

	int height = list.size() + 4;
	int edgey = Game::Inst()->ScreenHeight()/2 - height/2;

	TCODConsole::root->setAlignment(TCOD_LEFT);

	bool lButtonDown = false;
	TCOD_key_t key;
	
	while (true) {
		key = TCODConsole::checkForKeypress(TCOD_KEY_RELEASED);
		if (key.vk == TCODK_ESCAPE) {
			break;
		}
		
		TCODConsole::root->clear();

		TCODConsole::root->printFrame(edgex, edgey, width, height, true, TCOD_BKGND_SET, "Saved games");
		
		TCODConsole::root->setAlignment(TCOD_CENTER);
		TCODConsole::root->print(edgex + (width / 2), edgey + 1, "ESC to cancel.");
		TCODConsole::root->setAlignment(TCOD_LEFT);

		for (unsigned int i = 0; i < list.size(); ++i) {
			if (selected == i) {
				TCODConsole::root->setDefaultForeground(TCODColor::black);
				TCODConsole::root->setDefaultBackground(TCODColor::white);
			} else {
				TCODConsole::root->setDefaultForeground(TCODColor::white);
				TCODConsole::root->setDefaultBackground(TCODColor::black);
			}
			
			std::string label = list[i].filename;
			if (label.size() > 20) {
				label = label.substr(0, 17) + "...";
			}
			TCODConsole::root->print(edgex + 1, edgey + 3 + i, "%-20s", label.c_str());
			TCODConsole::root->setDefaultForeground(TCODColor::azure);
			
			// last modification date
			TCODConsole::root->print(edgex + 1 + 20 + 2, edgey + 3 + i, "%-20s", list[i].date.c_str());
			
			// filesize
			TCODConsole::root->print(edgex + 1 + 20 + 2 + 20 + 2, edgey + 3 + i, "%s", list[i].size.c_str());
		}
		
		TCODConsole::root->setDefaultForeground(TCODColor::white);
		TCODConsole::root->setDefaultBackground(TCODColor::black);

		TCODConsole::root->flush();

		mouseStatus = TCODMouse::getStatus();
		if (mouseStatus.lbutton) {
			lButtonDown = true;
		}

		if (mouseStatus.cx > edgex && mouseStatus.cx < edgex+width) {
			selected = mouseStatus.cy - (edgey+3);
		} else selected = -1;

		if (!mouseStatus.lbutton && lButtonDown) {
			lButtonDown = false;
			
			if (selected < (signed int)list.size() && selected >= 0) {
				Data::LoadGame(list[selected].filename);
				MainLoop();
				break;
			}
		}
	}
}

void SaveMenu() {
	if (!Game::Inst()->Running()) return;
	std::string saveName;
	TCODConsole::root->setDefaultForeground(TCODColor::white);
	TCODConsole::root->setDefaultBackground(TCODColor::black);
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
			
			savesCount = -1;
			Data::SaveGame(saveName);
			break;
		}

		TCODConsole::root->clear();
		TCODConsole::root->printFrame(Game::Inst()->ScreenWidth()/2-15,
			Game::Inst()->ScreenHeight()/2-3, 30, 3, true, TCOD_BKGND_SET, "Save name");
		TCODConsole::root->setDefaultBackground(TCODColor::darkGrey);
		TCODConsole::root->rect(Game::Inst()->ScreenWidth()/2-14, Game::Inst()->ScreenHeight()/2-2, 28, 1, true);
		TCODConsole::root->setDefaultBackground(TCODColor::black);
		TCODConsole::root->print(Game::Inst()->ScreenWidth()/2,
			Game::Inst()->ScreenHeight()/2-2, "%s", saveName.c_str());
		TCODConsole::root->flush();

	}
	return;
}

// XXX: No, really.
namespace {
	struct SettingRenderer {
		const char *label;
		TCOD_renderer_t renderer;
	};

	struct SettingField {
		const char  *label;
		std::string *value;
	};
}

void SettingsMenu() {
	std::string width        = Config::GetStringCVar("resolutionX");
	std::string height       = Config::GetStringCVar("resolutionY");
	TCOD_renderer_t renderer = static_cast<TCOD_renderer_t>(Config::GetCVar<int>("renderer"));
	bool fullscreen          = Config::GetCVar<bool>("fullscreen");

	TCODConsole::root->setAlignment(TCOD_LEFT);

	const int w = 40;
	const int h = 16;
	const int x = Game::Inst()->ScreenWidth()/2 - (w / 2);
	const int y = Game::Inst()->ScreenHeight()/2 - (h / 2);

	SettingRenderer renderers[] = {
		{ "GLSL",   TCOD_RENDERER_GLSL   },
		{ "OpenGL", TCOD_RENDERER_OPENGL },
		{ "SDL",    TCOD_RENDERER_SDL    }
	};

	SettingField fields[] = {
		{ "Resolution (width)",  &width  },
		{ "Resolution (height)", &height },
	};

	SettingField *focus = &fields[0];
	const unsigned int rendererCount = sizeof(renderers) / sizeof(SettingRenderer);
	const unsigned int fieldCount    = sizeof(fields) / sizeof(SettingField);

	TCOD_mouse_t mouse;
	TCOD_key_t   key;
	bool         clicked;

	while (true) {
		key = TCODConsole::checkForKeypress(TCOD_KEY_RELEASED);
		if (key.vk == TCODK_ESCAPE) return;
		else if (key.vk == TCODK_ENTER || key.vk == TCODK_KPENTER) break;

		// if field had 'mask' property it would be bit more generic..
		if (focus != NULL) {
			std::string& str = *focus->value;

			if (key.c >= '0' && key.c <= '9' && str.size() < (w - 7)) {
				str.push_back(key.c);
			} else if (key.vk == TCODK_BACKSPACE && str.size() > 0) {
				str.erase(str.end() - 1);
			}
		}

		TCODConsole::root->clear();

		TCODConsole::root->setDefaultForeground(TCODColor::white);
		TCODConsole::root->setDefaultBackground(TCODColor::black);

		TCODConsole::root->printFrame(x, y, w, h, true, TCOD_BKGND_SET, "Settings");
		TCODConsole::root->print(x + 1, y + 1, "ENTER to save changes, ESC to discard.");

		int currentY = y + 3;

		for (unsigned int idx = 0; idx < fieldCount; ++idx) {
			if (focus == &fields[idx]) {
				TCODConsole::root->setDefaultForeground(TCODColor::green);
			}
			TCODConsole::root->print(x + 1, currentY, fields[idx].label);

			TCODConsole::root->setDefaultForeground(TCODColor::white);
			TCODConsole::root->setDefaultBackground(TCODColor::darkGrey);
			TCODConsole::root->rect(x + 3, currentY + 1, w - 7, 1, true);
			TCODConsole::root->print(x + 3, currentY + 1, "%s", fields[idx].value->c_str());
			TCODConsole::root->setDefaultBackground(TCODColor::black);

			currentY += 3;
		}

		TCODConsole::root->setDefaultForeground((fullscreen ? TCODColor::green : TCODColor::grey));
		TCODConsole::root->print(x + 1, currentY, "Fullscreen mode");

		currentY += 2;
		TCODConsole::root->setDefaultForeground(TCODColor::white);
		TCODConsole::root->print(x + 1, currentY, "Renderer");

		for (unsigned int idx = 0; idx < rendererCount; ++idx) {
			if (renderer == renderers[idx].renderer) {
				TCODConsole::root->setDefaultForeground(TCODColor::green);
			} else {
				TCODConsole::root->setDefaultForeground(TCODColor::grey);
			}
			TCODConsole::root->print(x + 3, currentY + idx + 1, renderers[idx].label);
		}

		TCODConsole::root->flush();

		mouse = TCODMouse::getStatus();
		if (mouse.lbutton) {
			clicked = true;
		}

		if (clicked && !mouse.lbutton && mouse.cx > x && mouse.cx < x + w && mouse.cy > y && mouse.cy < y + h) {
			clicked = false;
			int whereY      = mouse.cy - y - 1;
			int rendererY   = currentY - y - 1;
			int fullscreenY = rendererY - 2;

			if (whereY > 1 && whereY < rendererY && whereY != fullscreenY) {
				int whereFocus = static_cast<int>(floor((whereY - 2) / 3.));
				if (whereFocus >= 0 && whereFocus < fieldCount) {
					focus = &fields[whereFocus];
				}
			} else if (whereY == fullscreenY) {
				fullscreen = !fullscreen;
			} else if (whereY > rendererY) {
				int whereRenderer = whereY - rendererY - 1;
				if (whereRenderer >= 0 && whereRenderer < rendererCount) {
					renderer = renderers[whereRenderer].renderer;
				}
			}
		}
	}
	
	Config::SetStringCVar("resolutionX", width);
	Config::SetStringCVar("resolutionY", height);
	Config::SetCVar("renderer", renderer);
	Config::SetCVar("fullscreen", fullscreen);
	
	try {
		Config::Save();
	} catch (const std::exception& e) {
		LOG("Could not save configuration! " << e.what());
	}
}

// Possible TODO: toggle mods on and off.
void ModsMenu() {
	TCODConsole::root->setAlignment(TCOD_LEFT);

	const int w = 60;
	const int h = 20;
	const int x = Game::Inst()->ScreenWidth()/2 - (w / 2);
	const int y = Game::Inst()->ScreenHeight()/2 - (h / 2);
	
	const std::list<Mods::Metadata>& modList = Mods::GetLoaded();
	const int subH = modList.size() * 5;
	TCODConsole sub(w - 2, std::max(1, subH));

	int currentY = 0;
	
	BOOST_FOREACH(Mods::Metadata mod, modList) {
		sub.setDefaultBackground(TCODColor::black);
		
		sub.setAlignment(TCOD_CENTER);
		sub.setDefaultForeground(TCODColor::azure);
		sub.print(w / 2, currentY, "%s", mod.mod.c_str());
		
		sub.setAlignment(TCOD_LEFT);
		sub.setDefaultForeground(TCODColor::white);
		sub.print(3, currentY + 1, "Name:    %s", mod.name.c_str());
		sub.print(3, currentY + 2, "Author:  %s", mod.author.c_str());
		sub.print(3, currentY + 3, "Version: %s", mod.version.c_str());
		
		currentY += 5;
	}

	TCOD_key_t   key;
	TCOD_mouse_t mouse;

	int scroll = 0;
	bool clicked = false;

	while (true) {
		key = TCODConsole::checkForKeypress(TCOD_KEY_RELEASED);
		if (key.vk == TCODK_ESCAPE) return;

		TCODConsole::root->clear();

		TCODConsole::root->setDefaultForeground(TCODColor::white);
		TCODConsole::root->setDefaultBackground(TCODColor::black);

		TCODConsole::root->printFrame(x, y, w, h, true, TCOD_BKGND_SET, "Loaded mods");
		TCODConsole::blit(&sub, 0, scroll, w - 2, h - 3, TCODConsole::root, x + 1, y + 2);

		TCODConsole::root->putChar(x + w - 2, y + 1,     TCOD_CHAR_ARROW_N, TCOD_BKGND_SET);
		TCODConsole::root->putChar(x + w - 2, y + h - 2, TCOD_CHAR_ARROW_S, TCOD_BKGND_SET);

		mouse = TCODMouse::getStatus();
		if (mouse.lbutton) {
			clicked = true;
		}

		if (clicked && !mouse.lbutton) {
			if (mouse.cx == x + w - 2) {
				if (mouse.cy == y + 1) {
					scroll = std::max(0, scroll - 1);
				} else if (mouse.cy == y + h - 2) {
					scroll = std::min(subH - h + 3, scroll + 1);
				}
			}
			clicked = false;
		}

		TCODConsole::root->flush();
	}
}

void KeysMenu() {
	Config::KeyMap& keyMap = Config::GetKeyMap();
	std::vector<std::string> labels;
	labels.reserve(keyMap.size());
	
	TCODConsole::root->setAlignment(TCOD_LEFT);
	
	int w = 40;
	const int h = keyMap.size() + 4;
	
	BOOST_FOREACH(Config::KeyMap::value_type pair, keyMap) {
		w = std::max(w, (int)pair.first.size() + 7); // 2 for borders, 5 for [ X ]
		labels.push_back(pair.first);
	}
	
	const int x = Game::Inst()->ScreenWidth()/2 - (w / 2);
	const int y = Game::Inst()->ScreenHeight()/2 - (h / 2);
	
	TCOD_mouse_t mouse;
	TCOD_key_t   key;
	
	int focus = 0;
	
	while (true) {
		key = TCODConsole::checkForKeypress(TCOD_KEY_RELEASED);
		if (key.vk == TCODK_ESCAPE) return;
		else if (key.vk == TCODK_ENTER || key.vk == TCODK_KPENTER) break;
		
		if (key.c >= ' ' && key.c <= '~') {
			keyMap[labels[focus]] = key.c;
		}
		
		TCODConsole::root->clear();
		
		TCODConsole::root->setDefaultForeground(TCODColor::white);
		TCODConsole::root->setDefaultBackground(TCODColor::black);
		
		TCODConsole::root->printFrame(x, y, w, h, true, TCOD_BKGND_SET, "Keys");
		TCODConsole::root->print(x + 1, y + 1, "ENTER to save changes, ESC to discard.");
		
		for (unsigned int idx = 0; idx < labels.size(); ++idx) {
			if (focus == idx) {
				TCODConsole::root->setDefaultForeground(TCODColor::green);
			}
			TCODConsole::root->print(x + 1, y + idx + 3, labels[idx].c_str());
			
			char key = keyMap[labels[idx]];
			TCODConsole::root->print(x + w - 6, y + idx + 3, (key == ' ' ? "[SPC]" : "[ %c ]"), key);
			
			TCODConsole::root->setDefaultForeground(TCODColor::white);
		}
		
		mouse = TCODMouse::getStatus();
		
		if (mouse.lbutton && mouse.cx > x && mouse.cx < x + w && mouse.cy >= y + 3 && mouse.cy < y + h - 1) {
			focus = mouse.cy - y - 3;
		}
		
		TCODConsole::root->flush();
	}
	
	try {
		Config::Save();
	} catch (const std::exception& e) {
		LOG("Could not save keymap! " << e.what());
	}
}
