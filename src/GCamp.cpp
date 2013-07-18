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

#include "Random.hpp"
#include "Camp.hpp"
#include "GCamp.hpp"
#include "Game.hpp"
#include "Map.hpp"
#include "Logger.hpp"
#include "Coordinate.hpp"
#include "Announce.hpp"
#include "UI.hpp"
#include "UI/MessageBox.hpp"
#include "data/Paths.hpp"
#include "data/Config.hpp"
#include "data/Data.hpp"
#include "data/Mods.hpp"
#include "data/Tilesets.hpp"
#include "NPC.hpp"
#include "Item.hpp"
#include "scripting/Engine.hpp"
#include "scripting/Event.hpp"
#include "Weather.hpp"
#include "StockManager.hpp"
#include "JobManager.hpp"

#include "Version.hpp"

int MainMenu();
void LoadMenu();
void SaveMenu();
void SettingsMenu();
void KeysMenu();
void ModsMenu();
void TilesetsMenu();

namespace Globals {
	bool noDumpMode;
}

extern "C" void TCOD_sys_startup(void);

int GCMain(std::vector<std::string>& args) {
	int exitcode = 0;
	
	//
	// Bootstrap phase.
	//
	Paths::Init();
	Random::Init();
	Config::Init();
	Script::Init(args);
	
	//
	// Load phase.
	//
	TCOD_sys_startup();
	Data::LoadConfig();
	Data::LoadFont();
	
	Game::LoadingScreen(&Mods::Load);
	
	#ifdef MACOSX
	Data::LoadFont();
	#endif
	
	//
	// Parse command line.
	//
	LOG("args.size() = " << args.size());
	
	bool bootTest = false;
	Globals::noDumpMode = false;
	
	BOOST_FOREACH(std::string arg, args) {
		if (arg == "-boottest") {
			bootTest = true;
		} else if (arg == "-dev") {
			Game::Inst()->EnableDevMode();
		} else if (arg == "-nodumps") {
			Globals::noDumpMode = true;
		}
	}
	
	if (!bootTest) {
		exitcode = MainMenu();
	} else {
		LOG("Bootstrap test, going into shutdown.");
	}
	
	//
	// Shutdown.
	//
	Script::Shutdown();
	
	#ifdef CHK_MEMORY_LEAKS
		// Pull down the singletons. Unnecessary but eases memory leak detection
		delete Game::Inst();
		delete Tooltip::Inst();
		delete UI::Inst();
		delete Camp::Inst();
		delete Announce::Inst();
		delete StockManager::Inst();
		delete JobManager::Inst();
		delete Map::Inst();
	#endif
	
	return exitcode;
}

void MainLoop() {
	Game* game = Game::Inst();
	if (!game->Running()) {
		Announce::Inst()->AddMsg("Press 'h' for keyboard shortcuts", TCODColor::cyan);
	}
	game->Running(true);

	int update = -1;
	if (Config::GetCVar<int>("halfRendering")) update = 0;

	int elapsedMilli;
	int targetMilli = 1000 / (UPDATES_PER_SECOND);
	int startMilli = TCODSystem::getElapsedMilli();
	while (game->Running()) {
		if (Game::ToMainMenu()) {
			Game::ToMainMenu(false);
			return;
		}

		UI::Inst()->Update();
		if (!game->Paused()) {
			game->Update();
			Announce::Inst()->Update();
		}

		if (update <= 0) {
			game->Draw();
			game->FlipBuffer();
			if (update == 0) update = 1;
		} else if (update == 1) update = 0;

		elapsedMilli = TCODSystem::getElapsedMilli() - startMilli;
		startMilli = TCODSystem::getElapsedMilli();
		if (elapsedMilli < targetMilli) TCODSystem::sleepMilli(targetMilli - elapsedMilli);
	}
	
	Script::Event::GameEnd();
}

void StartNewGame() {
	Game::Reset();
	Game* game = Game::Inst();

	game->GenerateMap(time(0));
	game->SetSeason(EarlySpring);

	std::priority_queue<std::pair<int, Coordinate> > spawnCenterCandidates;

	for (int tries = 0; tries < 20; ++tries) {
		std::pair<int,Coordinate> candidate(0, Random::ChooseInExtent(zero+100, Map::Inst()->Extent() - 100));

		int riverDistance = 1000, hillDistance = 1000;
		for (int i = 0; i < 4; ++i) {
			Direction dirs[4] = { WEST, EAST, NORTH, SOUTH };
			int distance = 200;
			Coordinate p = candidate.second + Coordinate::DirectionToCoordinate(dirs[i]) * distance;
			TCODLine::init(p.X(), p.Y(), candidate.second.X(), candidate.second.Y());
			do {
				if (Map::Inst()->IsInside(p)) {
					if (Map::Inst()->GetType(p) == TILEDITCH || Map::Inst()->GetType(p) == TILERIVERBED) {
						if (distance < riverDistance) riverDistance = distance;
						if (distance < 25) riverDistance = 2000;
					} else if (Map::Inst()->GetType(p) == TILEROCK) {
						if (distance < hillDistance) hillDistance = distance;
					}
				}
				--distance;
			} while (!TCODLine::step(p.Xptr(), p.Yptr()));
		}

		candidate.first = -hillDistance - riverDistance;
		if (Map::Inst()->GetType(candidate.second) != TILEGRASS) candidate.first -= 10000;
		spawnCenterCandidates.push(candidate);
	}

	Coordinate spawnTopCorner = spawnCenterCandidates.top().second - 20;
	Coordinate spawnBottomCorner = spawnCenterCandidates.top().second + 20;

	//Clear starting area
	for (int x = spawnTopCorner.X(); x < spawnBottomCorner.X(); ++x) {
		for (int y = spawnTopCorner.Y(); y < spawnBottomCorner.Y(); ++y) {
			Coordinate p(x,y);
			if (Map::Inst()->GetNatureObject(p) >= 0 && Random::Generate(2) < 2) {
				game->RemoveNatureObject(game->natureList[Map::Inst()->GetNatureObject(p)]);
			}
		}
	}

	//we use Top+15, Bottom-15 to restrict the spawning zone of goblin&orc to the very center, instead of spilled over the whole camp
	game->CreateNPCs(15, NPC::StringToNPCType("goblin"), spawnTopCorner+15, spawnBottomCorner-15);
	game->CreateNPCs(6, NPC::StringToNPCType("orc"), spawnTopCorner+15, spawnBottomCorner-15);

	game->CreateItems(30, Item::StringToItemType("Bloodberry seed"), spawnTopCorner, spawnBottomCorner);
	game->CreateItems(5, Item::StringToItemType("Blueleaf seed"), spawnTopCorner, spawnBottomCorner);
	game->CreateItems(30, Item::StringToItemType("Nightbloom seed"), spawnTopCorner, spawnBottomCorner);
	game->CreateItems(20, Item::StringToItemType("Bread"), spawnTopCorner, spawnBottomCorner);

	//we place two corpses on the map
	Coordinate corpseLoc[2];
	
	//find suitable location
	for (int c = 0; c < 2; ++c) {
		Coordinate p;
		do {
			p = Random::ChooseInRectangle(spawnTopCorner, spawnBottomCorner);
		} while(!Map::Inst()->IsWalkable(p));
		corpseLoc[c] = p;
	}

	//initialize corpses
	for (int c = 0; c < 2; ++c) {
		game->CreateItem(corpseLoc[c], Item::StringToItemType("stone axe"));
		game->CreateItem(corpseLoc[c], Item::StringToItemType("shovel"));
		int corpseuid = game->CreateItem(corpseLoc[c], Item::StringToItemType("corpse"));
		boost::shared_ptr<Item> corpse = game->itemList[corpseuid];
		corpse->Name("Corpse(Human woodsman)");
		corpse->Color(TCODColor::white);
		for (int i = 0; i < 6; ++i)
			game->CreateBlood(Random::ChooseInRadius(corpseLoc[c], 2));
	}

	Camp::Inst()->SetCenter(spawnCenterCandidates.top().second);
	game->CenterOn(spawnCenterCandidates.top().second);

	Map::Inst()->SetTerritoryRectangle(spawnTopCorner, spawnBottomCorner, true);

	Map::Inst()->weather->ApplySeasonalEffects();

	for (int i = 0; i < 10; ++i)
		Game::Inst()->events->SpawnBenignFauna();
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

void ConfirmStartNewGame() {
	boost::function<void(void)> run(boost::bind(&Game::LoadingScreen, &StartNewGame));
	
	if (Game::Inst()->Running()) {
		MessageBox::ShowMessageBox(
			"A game is already running, are you sure you want  to start a new one?",
			run, "Yes", NULL, "No"
		);
	} else {
		run();
	}
	
	Script::Event::GameStart();
	MainLoop();
}

int MainMenu() {
	MainMenuEntry entries[] = {
		{"New Game", 'n', &ActiveAlways,     &ConfirmStartNewGame},
		{"Continue", 'c', &ActiveIfRunning,  &MainLoop},
		{"Load",     'l', &ActiveIfHasSaves, &LoadMenu},
		{"Save",     's', &ActiveIfRunning,  &SaveMenu},
		{"Settings", 'o', &ActiveAlways,     &SettingsMenu},
		{"Keys",     'k', &ActiveAlways,     &KeysMenu},
		{"Mods",     'm', &ActiveAlways,     &ModsMenu},
		{"Tilesets", 't', &ActiveAlways,     &TilesetsMenu},
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
	bool endCredits = false;
	bool lButtonDown = false;

	TCOD_key_t key;
	TCOD_mouse_t mouse;
	TCOD_event_t event;

	while (!exit) {
		// FIXME: event checking before console flushing, should be other way around
		event = TCODSystem::checkForEvent(TCOD_EVENT_ANY, &key, &mouse);

		TCODConsole::root->setDefaultForeground(TCODColor::white);
		TCODConsole::root->setDefaultBackground(TCODColor::black);
		TCODConsole::root->clear();

		TCODConsole::root->printFrame(edgex, edgey, width, height, true, TCOD_BKGND_DEFAULT, "Main Menu");

		TCODConsole::root->setAlignment(TCOD_CENTER);
		TCODConsole::root->setBackgroundFlag(TCOD_BKGND_SET);

		TCODConsole::root->setDefaultForeground(TCODColor::celadon);
		TCODConsole::root->print(edgex+width/2, edgey-3, Globals::gameVersion);
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

			// FIXME: checking here because of seemingly poor menu drawing procedure
			if (event & TCOD_EVENT_KEY_PRESS) {
				if (key.c == entry.shortcut && entry.isActive()) {
					exit     = (entry.function == NULL);
					function = entry.function;
				}
			}
		}

		if (!endCredits) {
			endCredits = TCODConsole::renderCredits(edgex + 5, edgey + 25, true);
		}

		TCODConsole::root->flush();

		if (TCODConsole::isWindowClosed()) break;

		// FIXME: probably redundant
		if (event & TCOD_EVENT_MOUSE) {
		    mouse = TCODMouse::getStatus();
		    if (mouse.lbutton) {
			lButtonDown = true;
		    }
		}

		// FIXME: don't put this into an event clause until the whole method is properly rewritten with libtcod events
		if (function != NULL) {
			function();
		} else {
			if (mouse.cx > edgex && mouse.cx < edgex+width) {
				selected = mouse.cy - (edgey+2);
			} else selected = -1;

			if (!mouse.lbutton && lButtonDown) {
				lButtonDown = false;
				int entry = static_cast<int>(floor(selected / 2.));

				if (entry >= 0 && entry < static_cast<int>(entryCount) && entries[entry].isActive()) {
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
	std::vector<Data::Save> list;
	Data::GetSavedGames(list);
	
	// sort by last modification, newest on top
	std::sort(list.begin(), list.end(), std::greater<Data::Save>());

	int width = 59; // 2 for borders, 20 for filename, 2 spacer, 20 for date, 2 spacer, 13 for filesize
	int edgex = Game::Inst()->ScreenWidth()/2 - width/2;
	int height = list.size() + 4;
	int edgey = Game::Inst()->ScreenHeight()/2 - height/2;
	int selected = -1;

	TCODConsole::root->setAlignment(TCOD_LEFT);

	TCOD_key_t key;
	TCOD_mouse_t mouse;
	TCOD_event_t event;
	
	while (true) {
		TCODConsole::root->clear();

		TCODConsole::root->printFrame(edgex, edgey, width, height, true, TCOD_BKGND_SET, "Saved games");
		
		TCODConsole::root->setAlignment(TCOD_CENTER);
		TCODConsole::root->print(edgex + (width / 2), edgey + 1, "ESC to cancel.");
		TCODConsole::root->setAlignment(TCOD_LEFT);

		for (int i = 0; i < static_cast<int>(list.size()); ++i) {
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

		event = TCODSystem::checkForEvent(TCOD_EVENT_ANY, &key, &mouse);

		if (event & TCOD_EVENT_KEY_PRESS) {
		    if (key.vk == TCODK_ESCAPE) {
			break;
		    }
		}

		if (event & TCOD_EVENT_MOUSE) {
		    mouse = TCODMouse::getStatus();
		}

		if (event & TCOD_EVENT_MOUSE_MOVE) {
		    if (mouse.cx > edgex && mouse.cx < edgex+width) {
			selected = mouse.cy - (edgey+3);
		    } else selected = -1;
		}

		if (event & TCOD_EVENT_MOUSE_PRESS) {
		    if (mouse.lbutton) {
			if (selected < static_cast<int>(list.size()) && selected >= 0) {
			    if (!Data::LoadGame(list[selected].filename)) {
				TCODConsole::root->setDefaultForeground(TCODColor::white);
				TCODConsole::root->setDefaultBackground(TCODColor::black);
				TCODConsole::root->setAlignment(TCOD_CENTER);
				TCODConsole::root->clear();
				
				TCODConsole::root->print(
				    Game::Inst()->ScreenWidth() / 2, Game::Inst()->ScreenHeight() / 2,
				    "Could not load the game. Refer to the logfile."
				    );
				
				TCODConsole::root->print(
				    Game::Inst()->ScreenWidth() / 2, Game::Inst()->ScreenHeight() / 2 + 1,
				    "Press any key to return to the main menu."
				    );
				
				TCODConsole::root->flush();
				TCODConsole::waitForKeypress(true);
				return;
			    }
			    
			    MainLoop();
			    break;
			}
		    }
		}
	}
}


void SaveMenu() {
    if (!Game::Inst()->Running()) return;
    std::string saveName;
    TCODConsole::root->setDefaultForeground(TCODColor::white);
    TCODConsole::root->setDefaultBackground(TCODColor::black);

    // TODO: allow picking from list of previous saves (and choosing it with the mouse!)
    TCOD_key_t key;
    TCOD_mouse_t mouse;
    TCOD_event_t event;

    while (true) {
	TCODConsole::root->clear();
	TCODConsole::root->printFrame(Game::Inst()->ScreenWidth()/2-15,
				      Game::Inst()->ScreenHeight()/2-3, 30, 3, true, TCOD_BKGND_SET, "Save name");
	TCODConsole::root->setDefaultBackground(TCODColor::darkGrey);
	TCODConsole::root->rect(Game::Inst()->ScreenWidth()/2-14, Game::Inst()->ScreenHeight()/2-2, 28, 1, true);
	TCODConsole::root->setDefaultBackground(TCODColor::black);
	TCODConsole::root->print(Game::Inst()->ScreenWidth()/2,
				 Game::Inst()->ScreenHeight()/2-2, "%s", saveName.c_str());
	TCODConsole::root->flush();

	event = TCODSystem::checkForEvent(TCOD_EVENT_ANY, &key, &mouse);

	if (event & TCOD_EVENT_KEY_PRESS) {
	    if (key.c >= ' ' && key.c <= '}' && saveName.size() < 28) {
		saveName.push_back(key.c);
	    } else if (key.vk == TCODK_BACKSPACE && saveName.size() > 0) {
		saveName.erase(saveName.end() - 1);
	    }

	    if (key.vk == TCODK_ESCAPE) {
		break;
	    }
	    else if (key.vk == TCODK_ENTER || key.vk == TCODK_KPENTER) {
		savesCount = -1;
		if (!Data::SaveGame(saveName)) {
		    TCODConsole::root->setDefaultForeground(TCODColor::white);
		    TCODConsole::root->setDefaultBackground(TCODColor::black);
		    TCODConsole::root->setAlignment(TCOD_CENTER);
		    TCODConsole::root->clear();

		    TCODConsole::root->print(
			Game::Inst()->ScreenWidth() / 2, Game::Inst()->ScreenHeight() / 2,
			"Could not save the game. Refer to the logfile."
			);

		    TCODConsole::root->print(
			Game::Inst()->ScreenWidth() / 2, Game::Inst()->ScreenHeight() / 2 + 1,
			"Press any key to return to the main menu."
			);

		    TCODConsole::root->flush();
		    TCODConsole::waitForKeypress(true);
		    return;
		}
		break;
	    }
	}
    }
    return;
}

// XXX: No, really.
namespace {
	struct SettingRenderer {
		const char *label;
		TCOD_renderer_t renderer;
		bool useTileset;
	};

	struct SettingField {
		const char  *label;
		std::string *value;
	};
}

// TODO: split into smaller functions?
void SettingsMenu() {
	std::string width        = Config::GetStringCVar("resolutionX");
	std::string height       = Config::GetStringCVar("resolutionY");
	TCOD_renderer_t renderer = static_cast<TCOD_renderer_t>(Config::GetCVar<int>("renderer"));
	bool useTileset          = Config::GetCVar<bool>("useTileset");
	bool fullscreen          = Config::GetCVar<bool>("fullscreen");
	bool tutorial            = Config::GetCVar<bool>("tutorial");
	bool translucentUI       = Config::GetCVar<bool>("translucentUI");
	bool compressSaves       = Config::GetCVar<bool>("compressSaves");
	bool autosave            = Config::GetCVar<bool>("autosave");
	bool pauseOnDanger       = Config::GetCVar<bool>("pauseOnDanger");

	TCODConsole::root->setAlignment(TCOD_LEFT);

	const int w = 40;
	const int h = 29;
	const int x = Game::Inst()->ScreenWidth()/2 - (w / 2);
	const int y = Game::Inst()->ScreenHeight()/2 - (h / 2);

	SettingRenderer renderers[] = {
		{ "GLSL Tileset",	TCOD_RENDERER_GLSL   , true},
		{ "OpenGL Tileset",	TCOD_RENDERER_OPENGL , true},
		{ "Tileset",		TCOD_RENDERER_SDL    , true},
		{ "GLSL",			TCOD_RENDERER_GLSL   , false},
		{ "OpenGL",			TCOD_RENDERER_OPENGL , false},
		{ "SDL",			TCOD_RENDERER_SDL    , false}
	};

	SettingField fields[] = {
		{ "Resolution (width)",  &width  },
		{ "Resolution (height)", &height },
	};

	SettingField *focus = &fields[0];
	const unsigned int rendererCount = sizeof(renderers) / sizeof(SettingRenderer);
	const unsigned int fieldCount    = sizeof(fields) / sizeof(SettingField);

	TCOD_key_t   key;
	TCOD_mouse_t mouse;
	TCOD_event_t event;

	bool         clicked(false);

	while (true) {
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
		TCODConsole::root->setDefaultForeground((tutorial ? TCODColor::green : TCODColor::grey));
		TCODConsole::root->print(x + 1, currentY, "Tutorial");

		currentY += 2;
		TCODConsole::root->setDefaultForeground((translucentUI ? TCODColor::green : TCODColor::grey));
		TCODConsole::root->print(x + 1, currentY, "Translucent UI");

		currentY += 2;
		TCODConsole::root->setDefaultForeground((compressSaves ? TCODColor::green : TCODColor::grey));
		TCODConsole::root->print(x + 1, currentY, "Compress saves");

		currentY += 2;
		TCODConsole::root->setDefaultForeground((autosave ? TCODColor::green : TCODColor::grey));
		TCODConsole::root->print(x + 1, currentY, "Autosave");

		currentY += 2;
		TCODConsole::root->setDefaultForeground((pauseOnDanger ? TCODColor::green : TCODColor::grey));
		TCODConsole::root->print(x + 1, currentY, "Pause on danger");

		currentY += 2;
		TCODConsole::root->setDefaultForeground(TCODColor::white);
		TCODConsole::root->print(x + 1, currentY, "Renderer");

		for (unsigned int idx = 0; idx < rendererCount; ++idx) {
			if (renderer == renderers[idx].renderer && useTileset == renderers[idx].useTileset) {
				TCODConsole::root->setDefaultForeground(TCODColor::green);
			} else {
				TCODConsole::root->setDefaultForeground(TCODColor::grey);
			}
			TCODConsole::root->print(x + 3, currentY + idx + 1, renderers[idx].label);
		}

		TCODConsole::root->flush();

		event = TCODSystem::checkForEvent(TCOD_EVENT_ANY, &key, &mouse);

		if (event & TCOD_EVENT_KEY_PRESS) {
		    if (key.vk == TCODK_ESCAPE) return;
		    else if (key.vk == TCODK_ENTER || key.vk == TCODK_KPENTER) break;

		    // TODO: if field had 'mask' property it would be bit more generic..
		    if (focus != NULL) {
			std::string& str = *focus->value;
			
			if (key.c >= '0' && key.c <= '9' && str.size() < (w - 7)) {
			    str.push_back(key.c);
			} else if (key.vk == TCODK_BACKSPACE && str.size() > 0) {
			    str.erase(str.end() - 1);
			}
		    }
		}
		
		// FIXME: don't use 'clicked', use events
		if (event & TCOD_EVENT_MOUSE) {
		    mouse = TCODMouse::getStatus();
		    if (mouse.lbutton) {
			clicked = true;
		    }

		    if (clicked && !mouse.lbutton && mouse.cx > x && mouse.cx < x + w && mouse.cy > y && mouse.cy < y + h) {
			clicked = false;
			int whereY      = mouse.cy - y - 1;
			int rendererY   = currentY - y - 1;
			int fullscreenY = rendererY - 12;
			int tutorialY = rendererY - 10;
			int translucentUIY = rendererY - 8;
			int compressSavesY = rendererY - 6;
			int autosaveY = rendererY - 4;
			int pauseOnDangerY = rendererY - 2;
			
			if (whereY > 1 && whereY < fullscreenY) {
			    int whereFocus = static_cast<int>(floor((whereY - 2) / 3.));
			    if (whereFocus >= 0 && whereFocus < static_cast<int>(fieldCount)) {
				focus = &fields[whereFocus];
			    }
			} else if (whereY == fullscreenY) {
			    fullscreen = !fullscreen;
			} else if (whereY == tutorialY) {
			    tutorial = !tutorial;
			} else if (whereY == translucentUIY) {
			    translucentUI = !translucentUI;
			} else if (whereY == compressSavesY) {
			    compressSaves = !compressSaves;
			} else if (whereY == autosaveY) {
			    autosave = !autosave;
			} else if (whereY == pauseOnDangerY) {
			    pauseOnDanger = !pauseOnDanger;
			} else if (whereY > rendererY) {
			    int whereRenderer = whereY - rendererY - 1;
			    if (whereRenderer >= 0 && whereRenderer < static_cast<int>(rendererCount)) {
				renderer = renderers[whereRenderer].renderer;
				useTileset = renderers[whereRenderer].useTileset;
			    }
			}
		    }
		}
	}
	
	Config::SetStringCVar("resolutionX", width);
	Config::SetStringCVar("resolutionY", height);
	Config::SetCVar("renderer", renderer);
	Config::SetCVar("useTileset", useTileset);
	Config::SetCVar("fullscreen", fullscreen);
	Config::SetCVar("tutorial", tutorial);
	Config::SetCVar("translucentUI", translucentUI);
	Config::SetCVar("compressSaves", compressSaves);
	Config::SetCVar("autosave", autosave);
	Config::SetCVar("pauseOnDanger", pauseOnDanger);

	try {
		Config::Save();
	} catch (const std::exception& e) {
		LOG("Could not save configuration! " << e.what());
	}
	if (Game::Inst()->Renderer()) {
		Game::Inst()->Renderer()->SetTranslucentUI(translucentUI);
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
	const int subH = static_cast<int>(modList.size()) * 9;
	TCODConsole sub(w - 2, std::max(1, subH));

	int currentY = 0;
	
	BOOST_FOREACH(Mods::Metadata mod, modList) {
		sub.setDefaultBackground(TCODColor::black);
		
		sub.setAlignment(TCOD_CENTER);
		sub.setDefaultForeground(TCODColor::azure);
		sub.print(w / 2, currentY, "%s", mod.mod.c_str());
		
		sub.setAlignment(TCOD_LEFT);
		sub.setDefaultForeground(TCODColor::white);
		sub.print(3, currentY + 2, "Name:    %s", mod.name.c_str());
		sub.print(3, currentY + 4, "Author:  %s", mod.author.c_str());
		sub.print(3, currentY + 6, "Version: %s", mod.version.c_str());
		
		currentY += 9;
	}

	TCOD_key_t   key;
	TCOD_mouse_t mouse;
	TCOD_event_t event;

	int scroll = 0;
	bool clicked = false;

	while (true) {
		TCODConsole::root->clear();

		TCODConsole::root->setDefaultForeground(TCODColor::white);
		TCODConsole::root->setDefaultBackground(TCODColor::black);

		TCODConsole::root->printFrame(x, y, w, h, true, TCOD_BKGND_SET, "Loaded mods");
		TCODConsole::blit(&sub, 0, scroll, w - 2, h - 3, TCODConsole::root, x + 1, y + 2);

		TCODConsole::root->putChar(x + w - 2, y + 1,     TCOD_CHAR_ARROW_N, TCOD_BKGND_SET);
		TCODConsole::root->putChar(x + w - 2, y + h - 2, TCOD_CHAR_ARROW_S, TCOD_BKGND_SET);

		TCODConsole::root->flush();

		event = TCODSystem::checkForEvent(TCOD_EVENT_ANY, &key, &mouse);

		if (event & TCOD_EVENT_KEY_PRESS) {
		    if (key.vk == TCODK_ESCAPE) return;
		}

		if (event & TCOD_EVENT_MOUSE) {
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
		}
	}
}


void TilesetsMenu() {
	TCODConsole::root->setAlignment(TCOD_LEFT);

	int screenWidth = TCODConsole::root->getWidth();
	int screenHeight = TCODConsole::root->getHeight();

	int listWidth = screenWidth / 3;

	const std::vector<TileSetMetadata> tilesetsList = Tilesets::LoadTilesetMetadata();
	
	const int subH = static_cast<int>(tilesetsList.size());
	TCODConsole sub(listWidth - 2, std::max(1, subH));

	int currentY = 0;
	
	BOOST_FOREACH(TileSetMetadata tileset, tilesetsList) {
		sub.setDefaultBackground(TCODColor::black);
		
		sub.setAlignment(TCOD_LEFT);
		sub.setDefaultForeground(TCODColor::azure);
		sub.print(0, currentY, "%s", tileset.name.c_str());
		currentY += 1;
	}

	TCOD_key_t   key;
	TCOD_mouse_t mouse;
	TCOD_event_t event;

	int originalSelection = -1;
	int selection = 0;
	std::string tilesetDir = Config::GetStringCVar("tileset");
	if (tilesetDir.size() == 0) {
		tilesetDir = "default";
	}
	for (size_t i = 0; i < tilesetsList.size(); ++i) {
		if (boost::iequals(tilesetDir, tilesetsList.at(i).path.filename().string())) {
			selection = static_cast<int>(i);
			originalSelection = static_cast<int>(i);
			break;
		}
	}

	int scroll = 0;
	bool clicked = false;

	while (true) {
		TCODConsole::root->clear();

		TCODConsole::root->setDefaultForeground(TCODColor::white);
		TCODConsole::root->setDefaultBackground(TCODColor::black);

		// Left frame
		TCODConsole::root->printFrame(0, 0, listWidth, screenHeight, true, TCOD_BKGND_SET, "Tilesets");
		TCODConsole::blit(&sub, 0, scroll, listWidth - 2, screenHeight - 4, TCODConsole::root, 1, 2);

		if (scroll > 0)
		{
			TCODConsole::root->putChar(listWidth - 2,     1,     TCOD_CHAR_ARROW_N, TCOD_BKGND_SET);
		}
		if (scroll < subH - screenHeight + 3) 
		{
			TCODConsole::root->putChar(listWidth - 2, screenHeight - 2, TCOD_CHAR_ARROW_S, TCOD_BKGND_SET);
		}

		// Right frame
		TCODConsole::root->printFrame(listWidth, 0, screenWidth - listWidth, screenHeight, true, TCOD_BKGND_SET, "Details");

		if (selection < static_cast<int>(tilesetsList.size()))
		{
			TCODConsole::root->print(listWidth + 3, 2,      "Name:    %s", tilesetsList.at(selection).name.c_str());
			TCODConsole::root->print(listWidth + 3, 4,      "Size:    %dx%d", tilesetsList.at(selection).width, tilesetsList.at(selection).height);
			TCODConsole::root->print(listWidth + 3, 6,      "Author:  %s", tilesetsList.at(selection).author.c_str());
			TCODConsole::root->print(listWidth + 3, 8,      "Version: %s", tilesetsList.at(selection).version.c_str());
			TCODConsole::root->print(listWidth + 3, 10,     "Description:");
			TCODConsole::root->printRect(listWidth + 3, 12, screenWidth - listWidth - 6, screenHeight - 19, "%s", tilesetsList.at(selection).description.c_str());
		}
				
		// Buttons
		int buttonDist = (screenWidth - listWidth) / 3;
		TCODConsole::root->printFrame(listWidth + buttonDist - 4, screenHeight - 6, 8, 3);
		TCODConsole::root->print(listWidth + buttonDist - 1, screenHeight - 5, "Ok");
		TCODConsole::root->printFrame(listWidth + 2 * buttonDist - 4, screenHeight - 6, 8, 3);
		TCODConsole::root->print(listWidth + 2 * buttonDist - 3, screenHeight - 5, "Cancel");

		TCODConsole::root->flush();

		event = TCODSystem::checkForEvent(TCOD_EVENT_ANY, &key, &mouse);

		if (event & TCOD_EVENT_KEY_PRESS) {
		    if (key.vk == TCODK_ESCAPE) return;
		}

		if (event & TCOD_EVENT_MOUSE) {
		    mouse = TCODMouse::getStatus();
		    if (mouse.lbutton) {
			clicked = true;
		    }

		    if (clicked && !mouse.lbutton) {
			// Left frame click checks
			if (mouse.cx == listWidth - 2) {
			    if (mouse.cy == 1) {
				scroll = std::max(0, scroll - 1);
			    } else if (mouse.cy == screenHeight - 2) {
				scroll = std::max(0, std::min(subH - screenHeight + 3, scroll + 1));
			    }
			}
			else if (mouse.cx > 1 && mouse.cx < listWidth - 2 && mouse.cy > 1 && mouse.cy < screenHeight - 2
				 && mouse.cy - 2 + scroll < static_cast<int>(tilesetsList.size())) {
			    selection = scroll + mouse.cy - 2;
			}

			// Button clicks
			else if (mouse.cy >= screenHeight - 6 && mouse.cy < screenHeight - 3) {
			    if (mouse.cx >= listWidth + buttonDist - 4 && mouse.cx < listWidth + buttonDist + 4) {
				if (selection != originalSelection) {
				    Config::SetStringCVar("tileset", tilesetsList.at(selection).path.filename().string());
				}
				Game::Inst()->ResetRenderer();
				return;
			    } else if (mouse.cx >= listWidth + 2 * buttonDist - 4 && mouse.cx < listWidth + 2 * buttonDist + 4) {
				return;
			    }
			}
			clicked = false;
		    }
		}
	}
}

void KeysMenu() {
	Config::KeyMap& keyMap = Config::GetKeyMap();
	std::vector<std::string> labels;
	labels.reserve(keyMap.size());
	
	TCODConsole::root->setAlignment(TCOD_LEFT);
	
	int w = 40;
	const int h = static_cast<int>(keyMap.size()) + 4;
	
	BOOST_FOREACH(Config::KeyMap::value_type pair, keyMap) {
		w = std::max(w, (int)pair.first.size() + 7); // 2 for borders, 5 for [ X ]
		labels.push_back(pair.first);
	}
	
	const int x = Game::Inst()->ScreenWidth()/2 - (w / 2);
	const int y = Game::Inst()->ScreenHeight()/2 - (h / 2);
	
	TCOD_key_t   key;
	TCOD_mouse_t mouse;
	TCOD_event_t event;
	
	int focus = 0;
	
	while (true) {		
		TCODConsole::root->clear();
		
		TCODConsole::root->setDefaultForeground(TCODColor::white);
		TCODConsole::root->setDefaultBackground(TCODColor::black);
		
		TCODConsole::root->printFrame(x, y, w, h, true, TCOD_BKGND_SET, "Keys");
		TCODConsole::root->print(x + 1, y + 1, "ENTER to save changes, ESC to discard.");
		
		for (int idx = 0; idx < static_cast<int>(labels.size()); ++idx) {
			if (focus == idx) {
				TCODConsole::root->setDefaultForeground(TCODColor::green);
			}
			TCODConsole::root->print(x + 1, y + idx + 3, labels[idx].c_str());
			
			char key = keyMap[labels[idx]];
			TCODConsole::root->print(x + w - 6, y + idx + 3, (key == ' ' ? "[SPC]" : "[ %c ]"), key);
			
			TCODConsole::root->setDefaultForeground(TCODColor::white);
		}

		TCODConsole::root->flush();

		event = TCODSystem::checkForEvent(TCOD_EVENT_ANY, &key, &mouse);

		if (event & TCOD_EVENT_KEY_PRESS) {		
		    if (key.vk == TCODK_ESCAPE) return;
		    else if (key.vk == TCODK_ENTER || key.vk == TCODK_KPENTER) break;
		    
		    if (key.c >= ' ' && key.c <= '~') {
			keyMap[labels[focus]] = key.c;
		    }
		}

		if (event & TCOD_EVENT_MOUSE_MOVE) {
		    mouse = TCODMouse::getStatus();
		    
		    if (mouse.lbutton && mouse.cx > x && mouse.cx < x + w && mouse.cy >= y + 3 && mouse.cy < y + h - 1) {
			focus = mouse.cy - y - 3;
		    }
		}
	}
	
	try {
		Config::Save();
	} catch (const std::exception& e) {
		LOG("Could not save keymap! " << e.what());
	}
}
