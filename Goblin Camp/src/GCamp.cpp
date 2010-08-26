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
#include <cstdlib>
#include <cmath>

#include "GCamp.hpp"
#include "Game.hpp"
#include "Map.hpp"
#include "Logger.hpp"
#include "Coordinate.hpp"
#include "Announce.hpp"
#include "UI.hpp"
#include "JobManager.hpp"
#include "Data.hpp"
#include "NPC.hpp"
#include "Item.hpp"
#ifdef GC_PYTHON
#	include "Python.hpp"
#endif

#if defined(GC_BOOST_BUILD)
// This variable is defined in buildsystem-generated _version.cpp.
	extern const char *_GOBLIN_CAMP_VERSION_;
#	define GC_VERSION _GOBLIN_CAMP_VERSION_
#elif !defined(GC_VERSION)
#	define GC_VERSION "Goblin Camp 0.12"
#endif

int GCMain() {
	Data::Init();
#ifdef GC_PYTHON
	Python::Init();
#endif
	Game::Inst()->Init();
	return MainMenu();
}

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
	game->Reset();
	game->GenerateMap();

	Coordinate spawnTopCorner(200,200);
	Coordinate middleCorner(220,220);
	Coordinate itemBottomCorner(230,230);
	
	game->CreateNPCs(15, NPC::StringToNPCType("goblin"), spawnTopCorner, middleCorner);
	game->CreateNPCs(6, NPC::StringToNPCType("orc"), spawnTopCorner, middleCorner);
	
	game->CreateItems(20, Item::StringToItemType("Bloodberry seed"), middleCorner, itemBottomCorner);
	game->CreateItems(20, Item::StringToItemType("Blueleaf seed"), middleCorner, itemBottomCorner);
	game->CreateItems(20, Item::StringToItemType("Bread"), middleCorner, itemBottomCorner);
	game->CreateItems(3, Item::StringToItemType("Leather armor"), middleCorner, itemBottomCorner);
	game->CreateItems(3, Item::StringToItemType("Chainmail"), middleCorner, itemBottomCorner);
	MainLoop();
}

int Distance(int x0, int y0, int x1, int y1) {
	return (abs(y1 - y0) + abs(x1 - x0));
}

int Distance(Coordinate a, Coordinate b) {
	return Distance(a.X(), a.Y(), b.X(), b.Y());
}

// XXX: This really needs serious refactoring.
namespace {
	struct MainMenuEntry {
		const char *label;
		char shortcut;
		bool setExit;
		bool ifRunning;
		void (*function)();
	};
}

int MainMenu() {
	MainMenuEntry entries[] = {
		{"New Game", 'n', false, false, StartNewGame},
		{"Continue", 0,   false, true,  MainLoop},
		{"Load",     0,   false, false, LoadMenu},
		{"Save",     0,   false, true,  SaveMenu},
		{"Settings", 0,   false, false, SettingsMenu},
		{"Mods",     0,   false, false, NULL},
		{"Exit",     'q', true,  false, NULL}
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
		TCODConsole::root->setForegroundColor(TCODColor::white);
		TCODConsole::root->setBackgroundColor(TCODColor::black);
		TCODConsole::root->clear();
		
		TCODConsole::root->printFrame(
			edgex, edgey, width, height, true, TCOD_BKGND_DEFAULT,
			"Main Menu"
		);
		
		TCODConsole::root->setAlignment(TCOD_CENTER);
		TCODConsole::root->setBackgroundFlag(TCOD_BKGND_SET);
		
		TCODConsole::root->print(edgex+width/2, edgey-3, GC_VERSION);
		
		key = TCODConsole::checkForKeypress(TCOD_KEY_RELEASED);
		for (unsigned int idx = 0; idx < entryCount; ++idx) {
			const MainMenuEntry& entry = entries[idx];
			
			if (selected == (idx * 2)) {
				TCODConsole::root->setForegroundColor(TCODColor::black);
				TCODConsole::root->setBackgroundColor(TCODColor::white);
			} else {
				TCODConsole::root->setForegroundColor(TCODColor::white);
				TCODConsole::root->setBackgroundColor(TCODColor::black);
			}
			
			if ((entry.function == NULL && !entry.setExit) || (entry.ifRunning && !Game::Inst()->Running())) {
				TCODConsole::root->setForegroundColor(TCODColor::grey);
			}
			
			TCODConsole::root->print(
				edgex + width / 2, edgey + ((idx + 1) * 2), entry.label
			);
			
			if (entry.shortcut != NULL && key.c == entry.shortcut) {
				exit     = entry.setExit;
				function = entry.function;
			}
		}
		
		if (!endCredits) {
			endCredits = TCODConsole::renderCredits(
				edgex + 5, edgey + 25, true
			);
		}
		
		TCODConsole::root->flush();
		
		mouseStatus = TCODMouse::getStatus();
		if (mouseStatus.lbutton) {
			lButtonDown = true;
		}
		
		if (function != NULL) {
			function();
		} else {
			function = NULL;
			
			if (mouseStatus.cx > edgex && mouseStatus.cx < edgex+width) {
				selected = mouseStatus.cy - (edgey+2);
			} else selected = -1;
			
			if (!mouseStatus.lbutton && lButtonDown) {
				lButtonDown = false;
				int entry = static_cast<int>(floor(selected / 2.));
				
				if (entry < entryCount) {
					exit = entries[entry].setExit;
					if (
						entries[entry].function != NULL &&
						(!entries[entry].ifRunning || (entries[entry].ifRunning && Game::Inst()->Running()))
					) {
						entries[entry].function();
					}
				}
			}
		}
	}
	Game::Inst()->Exit(false);
	return 0;
}

void LoadMenu() {
	bool exit = false;
	int width = 30;
	int edgex = Game::Inst()->ScreenWidth()/2 - width/2;
	int selected = -1;
	TCOD_mouse_t mouseStatus;
	
	TCODList<std::string> list;
	Data::GetSavedGames(list);
	
	int height = list.size()+5;
	int edgey = Game::Inst()->ScreenHeight()/2 - height/2;

	TCODConsole::root->setAlignment(TCOD_CENTER);

	bool lButtonDown = false;

	while (!exit) {

		TCODConsole::root->clear();

		TCODConsole::root->printFrame(edgex, edgey, width, height, true, TCOD_BKGND_SET, "Saved games");

		for (int i = 0; i <= list.size()+1; ++i) {
			if (selected == i) {
				TCODConsole::root->setForegroundColor(TCODColor::black);
				TCODConsole::root->setBackgroundColor(TCODColor::white);
			} else {
				TCODConsole::root->setForegroundColor(TCODColor::white);
				TCODConsole::root->setBackgroundColor(TCODColor::black);
			}
			if (i < list.size()) TCODConsole::root->print(edgex+width/2, edgey+2+i, list.get(i).c_str());
			else if (i == list.size()+1) TCODConsole::root->print(edgex+width/2, edgey+2+i, "Cancel");
		}
		TCODConsole::root->setForegroundColor(TCODColor::white);
		TCODConsole::root->setBackgroundColor(TCODColor::black);

		TCODConsole::root->flush();

		mouseStatus = TCODMouse::getStatus();
		if (mouseStatus.lbutton) {
			lButtonDown = true;
		}

		if (mouseStatus.cx > edgex && mouseStatus.cx < edgex+width) {
			selected = mouseStatus.cy - (edgey+2);
		} else selected = -1;

		if (selected < list.size() && selected >= 0 && !mouseStatus.lbutton && lButtonDown) {
			Data::LoadGame(list.get(selected));
			MainLoop();
			lButtonDown = false;
			return;
		} else if (selected == list.size()+1 && !mouseStatus.lbutton && lButtonDown) {
			lButtonDown = false;
			return;
		}

	}
}

void SaveMenu() {
	if (!Game::Inst()->Running()) return;
	std::string saveName;
	TCODConsole::root->setForegroundColor(TCODColor::white);
	TCODConsole::root->setBackgroundColor(TCODColor::black);
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
			
			Data::SaveGame(saveName);
			break;
		}

		TCODConsole::root->clear();
		TCODConsole::root->printFrame(Game::Inst()->ScreenWidth()/2-15, 
			Game::Inst()->ScreenHeight()/2-3, 30, 3, true, TCOD_BKGND_SET, "Save name");
		TCODConsole::root->setBackgroundColor(TCODColor::darkGrey);
		TCODConsole::root->rect(Game::Inst()->ScreenWidth()/2-14, Game::Inst()->ScreenHeight()/2-2, 28, 1, true);
		TCODConsole::root->setBackgroundColor(TCODColor::black);
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
	std::string width        = boost::lexical_cast<std::string>(Game::Inst()->resolutionWidth);
	std::string height       = boost::lexical_cast<std::string>(Game::Inst()->resolutionHeight);
	TCOD_renderer_t renderer = Game::Inst()->renderer;
	bool fullscreen          = Game::Inst()->fullscreen;
	
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
		
		TCODConsole::root->setForegroundColor(TCODColor::white);
		TCODConsole::root->setBackgroundColor(TCODColor::black);
		
		TCODConsole::root->printFrame(x, y, w, h, true, TCOD_BKGND_SET, "Settings");
		TCODConsole::root->print(x + 1, y + 1, "ENTER to save changes, ESC to discard.");
		
		int currentY = y + 3;
		
		for (unsigned int idx = 0; idx < fieldCount; ++idx) {
			if (focus == &fields[idx]) {
				TCODConsole::root->setForegroundColor(TCODColor::green);
			}
			TCODConsole::root->print(x + 1, currentY, fields[idx].label);
			
			TCODConsole::root->setForegroundColor(TCODColor::white);
			TCODConsole::root->setBackgroundColor(TCODColor::darkGrey);
			TCODConsole::root->rect(x + 3, currentY + 1, w - 7, 1, true);
			TCODConsole::root->print(x + 3, currentY + 1, "%s", fields[idx].value->c_str());
			TCODConsole::root->setBackgroundColor(TCODColor::black);
			
			currentY += 3;
		}
		
		TCODConsole::root->setForegroundColor((fullscreen ? TCODColor::green : TCODColor::grey));
		TCODConsole::root->print(x + 1, currentY, "Fullscreen mode");
		
		currentY += 2;
		TCODConsole::root->setForegroundColor(TCODColor::white);
		TCODConsole::root->print(x + 1, currentY, "Renderer");
		
		for (unsigned int idx = 0; idx < rendererCount; ++idx) {
			if (renderer == renderers[idx].renderer) {
				TCODConsole::root->setForegroundColor(TCODColor::green);
			} else {
				TCODConsole::root->setForegroundColor(TCODColor::grey);
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
			int whereX      = mouse.cx - x;
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
	
	unsigned int cfgWidth  = boost::lexical_cast<unsigned int>(width);
	unsigned int cfgHeight = boost::lexical_cast<unsigned int>(height);
	const char *cfgRenderer;
	// it happens that TCOD_RENDERER_GLSL = 0, TCOD_RENDERER_OPENGL = 1 and TCOD_RENDERER_SDL = 2
	// but I'd rather not rely on that
	for (unsigned int idx = 0; idx < rendererCount; ++idx) {
		if (renderers[idx].renderer == renderer) {
			cfgRenderer = renderers[idx].label;
		}
	}
	
	try {
		Data::SaveConfig(cfgWidth, cfgHeight, cfgRenderer, fullscreen);
	} catch (const std::exception& e) {
		Logger::Inst()->output << "Could not save configuration! " << e.what() << "\n";
	}
	
	// remember new settings
	// I tried to make it apply new settings immediately, but it didn't work reliably
	// this is only so the settings dialog will display updated values next time
	Game::Inst()->resolutionWidth  = cfgWidth;
	Game::Inst()->resolutionHeight = cfgHeight;
	Game::Inst()->renderer         = renderer;
	Game::Inst()->fullscreen       = fullscreen;
}
