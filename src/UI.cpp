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
#include <string>
#include <vector>
#include <boost/function.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/lambda/lambda.hpp>

#include "Random.hpp"
#include "UI.hpp"
#include "Announce.hpp"
#include "Game.hpp"
#include "GCamp.hpp"
#include "Logger.hpp"
#include "Map.hpp"
#include "Job.hpp"
#include "Stockpile.hpp"
#include "Farmplot.hpp"
#include "data/Config.hpp"
#include "data/Data.hpp"
#include "Camp.hpp"
#include "UI/StockManagerDialog.hpp"
#include "UI/SquadsDialog.hpp"
#include "UI/ConstructionDialog.hpp"
#include "UI/AnnounceDialog.hpp"
#include "UI/Tooltip.hpp"
#include "UI/JobDialog.hpp"
#include "UI/DevConsole.hpp"

UI* UI::instance = 0;

UI::UI() :
menuOpen(false),
	_state(UINORMAL),
	_blueprint(1,1),
	placeable(false),
	underCursor(std::list<boost::weak_ptr<Entity> >()),
	drawCursor(false),
	lbuttonPressed(false),
	mbuttonPressed(false),
	rbuttonPressed(false),
	keyHelpTextColor(0),
	draggingViewport(false),
	draggingPlacement(false),
	textMode(false),
	inputString(std::string("")),
	currentStrobeTarget(boost::weak_ptr<Entity>())
{
	currentMenu = Menu::MainMenu();
	menuHistory.reserve(10);
	placementCallback = boost::bind(Game::CheckPlacement, _1, _2, std::set<TileType>());
	callback = boost::bind(Game::PlaceConstruction, _1, 0);
	rectCallback = boost::bind(Game::PlaceStockpile, _1, _2, 0, 0);
	event = (TCOD_event_t)0; // FIXME: bug Doryen to have TCOD_EVENT_NONE
	mouseInput = TCODMouse::getStatus();
	oldMouseInput = mouseInput;
}

UI* UI::Inst() {
	if (!instance) instance = new UI();
	return instance;
}

void UI::Update() {
	if (keyHelpTextColor > 0) keyHelpTextColor -= 2;
	if (keyHelpTextColor < 0) keyHelpTextColor = 0;

	// FIXME: should probably be elsewhere
	event = TCODSystem::checkForEvent(TCOD_EVENT_ANY, &key, &mouseInput);

	if (event & TCOD_EVENT_KEY_PRESS)
	    HandleKeyboard();
	if (event & TCOD_EVENT_MOUSE)
	    HandleMouse();
}

void UI::HandleKeyboard() {
	Config::KeyMap& keyMap = Config::GetKeyMap();

	float diffX = 0;
	float diffY = 0;

	//TODO: This isn't pretty, but it works.
	//key = TCODConsole::checkForKeypress(TCOD_KEY_PRESSED);
	if (!currentMenu || !(currentMenu->Update(-1, -1, false, key) & KEYRESPOND)) {
		if (!textMode) {
			if (key.c == keyMap["Exit"]) {
				Game::Exit();
			} else if (key.c == keyMap["Basics"]) {
				menuX = mouseInput.cx;
				menuY = mouseInput.cy;
				ChangeMenu(Menu::BasicsMenu());
			} else if (key.c == keyMap["Workshops"]) {
				menuX = mouseInput.cx;
				menuY = mouseInput.cy;
				ChangeMenu(Menu::WorkshopsMenu());
			} else if (key.c == keyMap["Permanent"]) {
				menuX = mouseInput.cx;
				menuY = mouseInput.cy;
				ChangeMenu(Menu::ConstructionCategoryMenu("Permanent"));
			} else if (key.c == keyMap["Orders"]) {
				menuX = mouseInput.cx;
				menuY = mouseInput.cy;
				ChangeMenu(Menu::OrdersMenu());
			} else if (key.c == keyMap["StockManager"]) {
				ChangeMenu(StockManagerDialog::StocksDialog());
			} else if (key.c == keyMap["Furniture"]) {
				menuX = mouseInput.cx;
				menuY = mouseInput.cy;
				ChangeMenu(Menu::FurnitureMenu());
			} else if (key.c == keyMap["Squads"]) {
				ChangeMenu(SquadsDialog::SquadDialog());
			} else if (key.c == keyMap["Announcements"]) {
				ChangeMenu(AnnounceDialog::AnnouncementsDialog());
			} else if (key.c == keyMap["Center"]) {
				Game::Inst()->CenterOn(Camp::Inst()->Center());
			} else if (key.c == keyMap["Help"]) {
				keyHelpTextColor = 855;
			} else if (key.c == keyMap["Pause"]) {
				Game::Inst()->Pause();
			} else if (key.c == keyMap["Jobs"]) {
				ChangeMenu(JobDialog::JobListingDialog());
			} else if (Game::Inst()->DevMode() && key.c == keyMap["DevConsole"]) {
				ShowDevConsole();
			} else if (key.c == keyMap["TerrainOverlay"]) {
				if (Map::Inst()->GetOverlayFlags() & TERRAIN_OVERLAY) Map::Inst()->RemoveOverlay(TERRAIN_OVERLAY);
				else Map::Inst()->AddOverlay(TERRAIN_OVERLAY);
			}

			int addition = 1;
			if (ShiftPressed()) addition *= 10;
			if (key.vk == TCODK_UP) {
				diffY -= addition;
			} else if (key.vk == TCODK_DOWN) {
				diffY += addition;
			} else if (key.vk == TCODK_LEFT) {
				diffX -= addition;
			} else if (key.vk == TCODK_RIGHT) {
				diffX += addition;
			} else if (key.vk == TCODK_KP1) {
				TCODMouse::showCursor(false);
				drawCursor = true;
				mouseInput.x -= Game::Inst()->CharWidth() * addition;
				mouseInput.cx -= addition;
				mouseInput.y += Game::Inst()->CharHeight() * addition;
				mouseInput.cy += addition;
			} else if (key.vk == TCODK_KP2) {
				TCODMouse::showCursor(false);
				drawCursor = true;
				mouseInput.y += Game::Inst()->CharHeight() * addition;
				mouseInput.cy += addition;
			} else if (key.vk == TCODK_KP3) {
				TCODMouse::showCursor(false);
				drawCursor = true;
				mouseInput.x += Game::Inst()->CharWidth() * addition;
				mouseInput.cx += addition;
				mouseInput.y += Game::Inst()->CharHeight() * addition;
				mouseInput.cy += addition;
			} else if (key.vk == TCODK_KP4) {
				TCODMouse::showCursor(false);
				drawCursor = true;
				mouseInput.x -= Game::Inst()->CharWidth() * addition;
				mouseInput.cx -= addition;
			} else if (key.vk == TCODK_KP6) {
				TCODMouse::showCursor(false);
				drawCursor = true;
				mouseInput.x += Game::Inst()->CharWidth() * addition;
				mouseInput.cx += addition;
			} else if (key.vk == TCODK_KP7) {
				TCODMouse::showCursor(false);
				drawCursor = true;
				mouseInput.x -= Game::Inst()->CharWidth() * addition;
				mouseInput.cx -= addition;
				mouseInput.y -= Game::Inst()->CharHeight() * addition;
				mouseInput.cy -= addition;
			} else if (key.vk == TCODK_KP8) {
				TCODMouse::showCursor(false);
				drawCursor = true;
				mouseInput.y -= Game::Inst()->CharHeight() * addition;
				mouseInput.cy -= addition;
			} else if (key.vk == TCODK_KP9) {
				TCODMouse::showCursor(false);
				drawCursor = true;
				mouseInput.x += Game::Inst()->CharWidth() * addition;
				mouseInput.cx += addition;
				mouseInput.y -= Game::Inst()->CharHeight() * addition;
				mouseInput.cy -= addition;
			} else if (key.vk == TCODK_ENTER || key.vk == TCODK_KPENTER) {
				lbuttonPressed = true;
			} else if (key.vk == TCODK_PRINTSCREEN) {
				Data::SaveScreenshot();
			}
			
			if (key.vk >= TCODK_F1 && key.vk <= TCODK_F12) {
				if(ShiftPressed()) {
					Game::Inst()->SetMark(key.vk - TCODK_F1);
				} else {
					Game::Inst()->ReturnToMark(key.vk - TCODK_F1);
				}
			}
		} else {
			if (key.c >= ' ' && key.c <= '}' && key.c != '+' && key.c != '-' && (signed int)inputString.size() < inputStringLimit) {
				inputString += key.c;
			} else if (key.vk == TCODK_BACKSPACE) {
				if (inputString.size() > 0) inputString.erase(inputString.end() - 1);
			}
		}
		if (key.vk == TCODK_ESCAPE) {
			extraTooltip = "";
			if (menuOpen) {
				rbuttonPressed = true;
			} else {
				TCODMouse::showCursor(true);
				Game::Inst()->ToMainMenu(true);
			}
		}
	}

	if (mouseInput.x < 0) {
		diffX += mouseInput.cx;
		mouseInput.x = 0;
		mouseInput.cx = 0;
	} else if (mouseInput.cx >= Game::Inst()->ScreenWidth()) {
		diffX += mouseInput.cx - (Game::Inst()->ScreenWidth()-1);
		mouseInput.cx = Game::Inst()->ScreenWidth() - 1;
		mouseInput.x = (Game::Inst()->ScreenWidth() - 1)* Game::Inst()->CharWidth();
	}
	if (mouseInput.y < 0) {
		diffY += mouseInput.cy;
		mouseInput.y = 0;
		mouseInput.cy = 0;
	} else if (mouseInput.cy >= Game::Inst()->ScreenHeight()) {
		diffY += mouseInput.cy - (Game::Inst()->ScreenHeight()-1);
		mouseInput.cy = Game::Inst()->ScreenHeight() - 1;
		mouseInput.y = (Game::Inst()->ScreenHeight() - 1) * Game::Inst()->CharHeight();
	}

	Game::Inst()->MoveCam(diffX, diffY);
}

void UI::HandleMouse() {
	int tmp;
	MenuResult menuResult = NOMENUHIT;
	bool xswap = false, yswap = false;

	if (TCODConsole::isWindowClosed()) Game::Inst()->Exit();

	TCOD_mouse_t newMouseInput = TCODMouse::getStatus();
	if (newMouseInput.x != oldMouseInput.x || newMouseInput.y != oldMouseInput.y) {
		mouseInput = newMouseInput;
		drawCursor = false;
		TCODMouse::showCursor(true);
	}

	if (newMouseInput.lbutton_pressed) lbuttonPressed = true;
	if (newMouseInput.mbutton_pressed) mbuttonPressed = true;
	if (newMouseInput.rbutton_pressed) rbuttonPressed = true;

	if (_state == UINORMAL) {
		HandleUnderCursor(Game::Inst()->TileAt(mouseInput.x, mouseInput.y), &underCursor);
	}

	if (_state == UIPLACEMENT || _state == UIABPLACEMENT || _state == UIRECTPLACEMENT) {
		placeable = placementCallback(Game::Inst()->TileAt(mouseInput.x, mouseInput.y), _blueprint);
	}

	if (_state == UIABPLACEMENT || _state == UIRECTPLACEMENT) {
		b = Game::Inst()->TileAt(mouseInput.x, mouseInput.y);
	}

	currentMenu->Update(mouseInput.cx, mouseInput.cy, false, NO_KEY);

	if (lbuttonPressed) {
		if (draggingViewport) {
			draggingViewport = false;
		} else {
			menuResult = NOMENUHIT;
			if(!draggingPlacement) {
				menuResult = sideBar.Update(mouseInput.cx, mouseInput.cy, true);
				if (menuResult & NOMENUHIT) {
					menuResult = Announce::Inst()->Update(mouseInput.cx, mouseInput.cy, true);
					if (menuResult & NOMENUHIT) {
						if (menuOpen) {
							menuResult = currentMenu->Update(mouseInput.cx, mouseInput.cy, true, NO_KEY); lbuttonPressed = false;
						}
					}
				}
			}
			if (menuResult & NOMENUHIT) {
				if (menuOpen && _state == UINORMAL) {
					CloseMenu();
				}
				if (_state == UIPLACEMENT && placeable) {
					callback(Game::Inst()->TileAt(mouseInput.x, mouseInput.y));
				} else if (_state == UIABPLACEMENT && placeable) {
					if (a.X() == 0) {
						a = Game::Inst()->TileAt(mouseInput.x, mouseInput.y);
					}
					else if(!draggingPlacement || a.X() != b.X() || a.Y() != b.Y()) {
						//Place construction from a->b
						if (a.X() > b.X()) {
							tmp = a.X();
							a.X(b.X());
							b.X(tmp);
							xswap = true;
						}
						if (a.Y() > b.Y()) {
							tmp = a.Y();
							a.Y(b.Y());
							b.Y(tmp);
							yswap = true;
						}
						for (int ix = a.X(); ix <= b.X(); ++ix) {
							if (!yswap) {
								if (placementCallback(Coordinate(ix, a.Y()), _blueprint))
									callback(Coordinate(ix, a.Y()));
							} else {
								if (placementCallback(Coordinate(ix, b.Y()), _blueprint))
									callback(Coordinate(ix, b.Y()));
							}
						}
						for (int iy = a.Y(); iy <= b.Y(); ++iy) {
							if (!xswap) {
								if (placementCallback(Coordinate(b.X(), iy), _blueprint))
									callback(Coordinate(b.X(), iy));
							} else {
								if (placementCallback(Coordinate(a.X(), iy), _blueprint))
									callback(Coordinate(a.X(), iy));
							}
						}
						a.X(0); a.Y(0); b.X(0); b.Y(0);
					}
				} else if (_state == UIRECTPLACEMENT && placeable) {
					if (a.X() == 0) {
						a = Game::Inst()->TileAt(mouseInput.x, mouseInput.y);
					}
					else if(!draggingPlacement || a.X() != b.X() || a.Y() != b.Y()) {
						//Place construction from a->b
						if (a.X() > b.X()) {
							tmp = a.X();
							a.X(b.X());
							b.X(tmp);
							xswap = true;
						}
						if (a.Y() > b.Y()) {
							tmp = a.Y();
							a.Y(b.Y());
							b.Y(tmp);
							yswap = true;
						}

						/*I removed the placement call from here, it causes unnecessary cancelations
						Callbacks should check tile validity anyway*/
						rectCallback(a,b);

						a.X(0); a.Y(0); b.X(0); b.Y(0);
					}
				} else { //Current state is not any kind of placement, so open construction/npc context menu if over one
					if (!underCursor.empty()) sideBar.SetEntity(*underCursor.begin());
					else sideBar.SetEntity(boost::weak_ptr<Entity>());
				}
			}
		}
		draggingPlacement = false;
	} else if (newMouseInput.lbutton && !oldMouseInput.lbutton) {
		menuResult = sideBar.Update(mouseInput.cx, mouseInput.cy, false);
		if (menuResult & NOMENUHIT) {
			if (menuOpen) {
				menuResult = currentMenu->Update(mouseInput.cx, mouseInput.cy, false, NO_KEY);
			}
			if (menuResult & NOMENUHIT) {
				if (_state == UIABPLACEMENT && placeable) {
					if (a.X() == 0) {
						a = Game::Inst()->TileAt(mouseInput.x, mouseInput.y);
						draggingPlacement = true;
					}
				} else if (_state == UIRECTPLACEMENT && placeable) {
					if (a.X() == 0) {
						a = Game::Inst()->TileAt(mouseInput.x, mouseInput.y);
						draggingPlacement = true;
					}
				}
			}
		}
	}

	if (rbuttonPressed) {
		menuX = mouseInput.cx;
		menuY = mouseInput.cy;
		menuOpen = !menuOpen;
		currentMenu->selected(-1);
		if (!menuOpen || _state != UINORMAL) {
			CloseMenu();
		}
		currentMenu = 0;
		if(!underCursor.empty()) {
			if (underCursor.begin()->lock()) {
				if (boost::dynamic_pointer_cast<Construction>(underCursor.begin()->lock())) {
					if (!boost::static_pointer_cast<Construction>(underCursor.begin()->lock())->DismantlingOrdered())
						currentMenu = underCursor.begin()->lock()->GetContextMenu();
				} else {
					currentMenu = underCursor.begin()->lock()->GetContextMenu();
				}
			}
		}
		if(!currentMenu) {
			currentMenu = Menu::MainMenu();
		}
		if (menuOpen) currentMenu->Open();
		menuHistory.clear();
		extraTooltip = "";
	}

	if (mbuttonPressed && menuOpen && !menuHistory.empty()) {
		currentMenu->selected(-1);
		_state = UINORMAL; a.X(0); a.Y(0);
		currentMenu->Close();
		currentMenu = menuHistory.back();
		currentMenu->Open();
		menuHistory.pop_back();
	}

	if (newMouseInput.lbutton && _state == UINORMAL) {
		Game::Inst()->MoveCam(-(newMouseInput.dx / 3.0f), -(newMouseInput.dy / 3.0f));
		if (newMouseInput.dx > 0 || newMouseInput.dy > 0) draggingViewport = true;
	}

	lbuttonPressed = false;
	mbuttonPressed = false;
	rbuttonPressed = false;
	oldMouseInput = newMouseInput;
}

void UI::Draw(TCODConsole* console) {
	int tmp;
	bool xswap = false, yswap = false;

	DrawTopBar(console);

	if (menuOpen) {
		currentMenu->Draw(menuX, menuY, console);
	}
	
	Coordinate mouseTile(Game::Inst()->TileAt(mouseInput.x, mouseInput.y));
	boost::shared_ptr<MapRenderer> renderer = Game::Inst()->Renderer();

	if (_state == UIPLACEMENT || ((_state == UIABPLACEMENT || _state == UIRECTPLACEMENT) && a.X() == 0)) {
		renderer->DrawCursor(mouseTile, Coordinate(mouseTile.X() + _blueprint.X() - 1, mouseTile.Y() + _blueprint.Y() - 1), placeable);
	}
	else if (_state == UIABPLACEMENT && a.X() > 0) {
		if (a.X() > b.X()) {
			tmp = a.X();
			a.X(b.X());
			b.X(tmp);
			xswap = true;
		}
		if (a.Y() > b.Y()) {
			tmp = a.Y();
			a.Y(b.Y());
			b.Y(tmp);
			yswap = true;
		}
		for (int ix = a.X(); ix <= b.X(); ++ix) {
			if (!yswap) {
				renderer->DrawCursor(Coordinate(ix, a.Y()), placeable);
			}
			else {
				renderer->DrawCursor(Coordinate(ix, b.Y()), placeable);
			}
		}
		for (int iy = a.Y(); iy <= b.Y(); ++iy) {
			if (!xswap) {
				renderer->DrawCursor(Coordinate(b.X(), iy), placeable);
			}
			else {
				renderer->DrawCursor(Coordinate(a.X(), iy), placeable);
			}
		}
		if (xswap) {
			tmp = a.X();
			a.X(b.X());
			b.X(tmp);
		}
		if (yswap) {
			tmp = a.Y();
			a.Y(b.Y());
			b.Y(tmp);
		}
	} else if (_state == UIRECTPLACEMENT && a.X() > 0) {
		if (a.X() > b.X()) {
			tmp = a.X();
			a.X(b.X());
			b.X(tmp);
			xswap = true;
		}
		if (a.Y() > b.Y()) {
			tmp = a.Y();
			a.Y(b.Y());
			b.Y(tmp);
			yswap = true;
		}
		renderer->DrawCursor(a, b, placeable);
		if (xswap) {
			tmp = a.X();
			a.X(b.X());
			b.X(tmp);
		}
		if (yswap) {
			tmp = a.Y();
			a.Y(b.Y());
			b.Y(tmp);
		}
	}

	if (drawCursor) renderer->DrawCursor(mouseTile, true);
	
	Announce::Inst()->Draw(console);
	sideBar.Draw(console);

	Tooltip *tooltip = Tooltip::Inst();
	tooltip->Clear();

	if (extraTooltip != "") tooltip->AddEntry(TooltipEntry(extraTooltip, TCODColor::white));
	if (menuOpen) {
		currentMenu->GetTooltip(mouseInput.cx, mouseInput.cy, tooltip);
	}

	sideBar.GetTooltip(mouseInput.cx, mouseInput.cy, tooltip, console);

	if (_state == UINORMAL && (!menuOpen || (currentMenu->Update(mouseInput.cx, mouseInput.cy, false, NO_KEY) & NOMENUHIT)) 
		&& (sideBar.Update(mouseInput.cx, mouseInput.cy, false) & NOMENUHIT)
		&& (Announce::Inst()->Update(mouseInput.cx, mouseInput.cy, false) & NOMENUHIT)
		&& !underCursor.empty()) {

			if (boost::shared_ptr<Entity> strobeEntity = currentStrobeTarget.lock()) { strobeEntity->Strobe(); }
			for (std::list<boost::weak_ptr<Entity> >::iterator ucit = underCursor.begin(); ucit != underCursor.end(); ++ucit) {
				if (boost::shared_ptr<Entity> entity = ucit->lock()) {
					Coordinate mouseLoc = Game::Inst()->TileAt(mouseInput.x, mouseInput.y);
					entity->GetTooltip(mouseLoc.X(), mouseLoc.Y(), tooltip);

					if (entity->CanStrobe()) {
						if (boost::shared_ptr<Entity> strobeTarget = currentStrobeTarget.lock()) {
							if (entity != strobeTarget) {
								strobeTarget->ResetStrobe();
							}
						}
						currentStrobeTarget = entity;
					}
							
				}
			}
	} else if (boost::shared_ptr<Entity> strobeEntity = currentStrobeTarget.lock()) {
		strobeEntity->ResetStrobe();
		currentStrobeTarget.reset();
	}


	tooltip->Draw(mouseInput.cx, mouseInput.cy, console);
}

int UI::DrawShortcutHelp(TCODConsole *console, int x, int y, std::string shortcut) {
	Config::KeyMap& keyMap = Config::GetKeyMap();
	std::string out = "";
	bool found = false;
	for(std::string::iterator it = shortcut.begin(); it != shortcut.end(); it++) {
		if(!found && tolower(*it) == keyMap[shortcut]) {
			out.push_back(TCOD_COLCTRL_1);
			out.push_back(*it);
			out.push_back(TCOD_COLCTRL_STOP);
			found = true;
		} else {
			out.push_back(*it);
		}
	}

	if(!found) {
		if(keyMap[shortcut] == ' ') {
			out.insert(0, (boost::format("%cSpace%c-") % (char)TCOD_COLCTRL_1 % (char)TCOD_COLCTRL_STOP).str());
		} else {
			out.insert(0, (boost::format("%c%c%c-") % (char)TCOD_COLCTRL_1 % (char)toupper(keyMap[shortcut]) % (char)TCOD_COLCTRL_STOP).str());
		}
	}

	out.push_back(' ');
	console->print(x, y, out.c_str());
	return out.length() - 2;
}

void UI::DrawTopBar(TCODConsole* console) {
	console->setAlignment(TCOD_CENTER);
	console->setDefaultForeground(TCODColor::white);
	console->print(console->getWidth() / 2, 0, "w(%s)  -  %s  -  Orcs: %d   Goblins: %d  -  Year %d, %s  FPS: %d", Map::Inst()->GetWindAbbreviation().c_str(),
		Camp::Inst()->GetName().c_str(),
		Game::Inst()->OrcCount(), Game::Inst()->GoblinCount(), 
		Game::Inst()->GetAge(),
		Game::Inst()->SeasonToString(Game::Inst()->CurrentSeason()).c_str(),
		TCODSystem::getFps());

	if (Game::Inst()->Paused()) {
		console->setDefaultForeground(TCODColor::red);
		console->print(Game::Inst()->ScreenWidth() / 2, 1, "- - - - PAUSED - - - -");
	}
	console->setAlignment(TCOD_LEFT);

	if (keyHelpTextColor > 0) {
		console->setDefaultForeground(TCODColor(std::min(255, keyHelpTextColor), std::min(255, keyHelpTextColor), std::min(255, keyHelpTextColor)));
		console->setColorControl(TCOD_COLCTRL_1, TCODColor(0, std::min(255, keyHelpTextColor), 0), TCODColor::black);
		int x = 10;
		x += DrawShortcutHelp(console, x, 3, "Exit");
		x += DrawShortcutHelp(console, x, 3, "Basics");
		x += DrawShortcutHelp(console, x, 3, "Workshops");
		x += DrawShortcutHelp(console, x, 3, "Orders");
		x += DrawShortcutHelp(console, x, 3, "Furniture");
		x += DrawShortcutHelp(console, x, 3, "StockManager");
		x += DrawShortcutHelp(console, x, 3, "Squads");
		x += DrawShortcutHelp(console, x, 3, "Announcements");
		x += DrawShortcutHelp(console, x, 3, "Jobs");
		x = 10;
		console->print(x, 5, "%cShift+F1-F12%c Set Mark  %cF1-F12%c Return To Mark  ", TCOD_COLCTRL_1,TCOD_COLCTRL_STOP, TCOD_COLCTRL_1,TCOD_COLCTRL_STOP, TCOD_COLCTRL_1,TCOD_COLCTRL_STOP);
		x = 56;
		x += DrawShortcutHelp(console, x, 5, "Center");
		console->print(x, 5, "Camp");
		x = 10;
		x += DrawShortcutHelp(console, x, 7, "Pause");
	}

	console->setDefaultForeground(TCODColor::white);
}

void UI::blueprint(const Coordinate& newBlue) {
	_blueprint = newBlue;
}
void UI::state(UIState newState) { _state = newState; }

void UI::ChangeMenu(Panel* menu) {
	if (UI::Inst()->CurrentMenu()) {
		UI::Inst()->CurrentMenu()->Close();
	}
	UI::Inst()->CurrentMenu()->selected(-1);
	UI::Inst()->AddToHistory(UI::Inst()->CurrentMenu());
	UI::Inst()->CurrentMenu(menu);
	UI::Inst()->menuOpen = true;
	menu->Open();
}

void UI::AddToHistory(Panel* menu) {menuHistory.push_back(menu);}
Panel* UI::CurrentMenu() {return currentMenu;}
void UI::CurrentMenu(Panel* menu) {currentMenu = menu;}

void UI::SetCallback(boost::function<void(Coordinate)> newCallback) {callback = newCallback;}
void UI::SetRectCallback(boost::function<void(Coordinate,Coordinate)> newCallback) {rectCallback = newCallback;}
void UI::SetPlacementCallback(boost::function<bool(Coordinate,Coordinate)> newCallback) {placementCallback = newCallback;}

void UI::ChooseConstruct(ConstructionType construct, UIState state) {
	UI::Inst()->SetCallback(boost::bind(Game::PlaceConstruction, _1, construct));
	UI::Inst()->SetPlacementCallback(boost::bind(Game::CheckPlacement, _1, _2, Construction::Presets[construct].tileReqs));
	UI::Inst()->blueprint(Construction::Blueprint(construct));
	UI::Inst()->state(state);
	Game::Inst()->Renderer()->SetCursorMode(Cursor_Construct);
	UI::Inst()->HideMenu();
	UI::Inst()->SetExtraTooltip(Construction::Presets[construct].name);
}

void UI::ChooseStockpile(ConstructionType stockpile) {
	int stockpileSymbol = '%';
	if (Construction::Presets[stockpile].tags[STOCKPILE]) {
		UI::Inst()->SetCallback(boost::bind(Game::PlaceStockpile, _1, _1, stockpile, stockpileSymbol));
		UI::Inst()->state(UIPLACEMENT);
	} else {
		UI::Inst()->SetRectCallback(boost::bind(Game::PlaceStockpile, _1, _2, stockpile, stockpileSymbol));
		UI::Inst()->state(UIRECTPLACEMENT);
	}
	UI::Inst()->SetPlacementCallback(boost::bind(Game::CheckPlacement, _1, _2, Construction::Presets[stockpile].tileReqs));
	UI::Inst()->blueprint(Construction::Blueprint(stockpile));
	Game::Inst()->Renderer()->SetCursorMode(Cursor_Stockpile);
	UI::Inst()->HideMenu();
	UI::Inst()->SetExtraTooltip(Construction::Presets[stockpile].name);
}

void UI::ChooseTreeFelling() {
	UI::Inst()->state(UIRECTPLACEMENT);
	UI::Inst()->SetRectCallback(boost::bind(Game::FellTree, _1, _2));
	UI::Inst()->SetPlacementCallback(boost::bind(Game::CheckTree, _1, _2));
	UI::Inst()->blueprint(Coordinate(1,1));
	Game::Inst()->Renderer()->SetCursorMode(Cursor_TreeFelling);
	UI::Inst()->HideMenu();
	UI::Inst()->SetExtraTooltip("Fell trees");
}

void UI::ChoosePlantHarvest() {
	UI::Inst()->state(UIRECTPLACEMENT);
	UI::Inst()->SetRectCallback(boost::bind(Game::HarvestWildPlant, _1, _2));
	UI::Inst()->SetPlacementCallback(boost::bind(Game::CheckTree, _1, _2));
	UI::Inst()->blueprint(Coordinate(1,1));
	Game::Inst()->Renderer()->SetCursorMode(Cursor_Harvest);
	UI::Inst()->HideMenu();
	UI::Inst()->SetExtraTooltip("Harvest plants");
}

void UI::ChooseOrderTargetCoordinate(boost::shared_ptr<Squad> squad, Order order) {
	UI::Inst()->state(UIPLACEMENT);
	bool autoClose = true;
	if (order == PATROL) {
		autoClose = false;
		order = GUARD;
	}
	UI::Inst()->SetCallback(boost::bind(Game::SetSquadTargetCoordinate, order, _1, squad, autoClose));
	UI::Inst()->SetPlacementCallback(boost::bind(Game::CheckTree, _1, Coordinate(1,1)));
	UI::Inst()->blueprint(Coordinate(1,1));
	Game::Inst()->Renderer()->SetCursorMode(Cursor_Order);
}

void UI::ChooseOrderTargetEntity(boost::shared_ptr<Squad> squad, Order order) {
	UI::Inst()->state(UIPLACEMENT);
	UI::Inst()->SetCallback(boost::bind(Game::SetSquadTargetEntity, order, _1, squad));
	UI::Inst()->SetPlacementCallback(boost::bind(Game::CheckTree, _1, Coordinate(1,1)));
	UI::Inst()->blueprint(Coordinate(1,1));
	Game::Inst()->Renderer()->SetCursorMode(Cursor_Order);
}

void UI::ChooseDesignateTree() {
	UI::Inst()->state(UIRECTPLACEMENT);
	UI::Inst()->SetRectCallback(boost::bind(Game::DesignateTree, _1, _2));
	UI::Inst()->SetPlacementCallback(boost::bind(Game::CheckTree, _1, _2));
	UI::Inst()->blueprint(Coordinate(1,1));
	Game::Inst()->Renderer()->SetCursorMode(Cursor_Tree);
	UI::Inst()->HideMenu();
	UI::Inst()->SetExtraTooltip("Designate trees");
}

void UI::ChooseDismantle() {
	UI::Inst()->state(UIRECTPLACEMENT);
	UI::Inst()->SetRectCallback(boost::bind(Game::DismantleConstruction, _1, _2));
	UI::Inst()->SetPlacementCallback(boost::bind(Game::CheckTree, _1, _2));
	UI::Inst()->blueprint(Coordinate(1,1));
	Game::Inst()->Renderer()->SetCursorMode(Cursor_Dismantle);
	UI::Inst()->HideMenu();
	UI::Inst()->SetExtraTooltip("Dismantle");
}

void UI::ChooseUndesignate() {
	UI::Inst()->state(UIRECTPLACEMENT);
	UI::Inst()->SetRectCallback(boost::bind(Game::Undesignate, _1, _2));
	UI::Inst()->SetPlacementCallback(boost::bind(Game::CheckTree, _1, _2));
	UI::Inst()->blueprint(Coordinate(1,1));
	Game::Inst()->Renderer()->SetCursorMode(Cursor_Undesignate);
	UI::Inst()->HideMenu();
	UI::Inst()->SetExtraTooltip("Undesignate");
}

void UI::ChooseDesignateBog() {
	UI::Inst()->state(UIRECTPLACEMENT);
	UI::Inst()->SetRectCallback(boost::bind(Game::DesignateBog, _1, _2));
	UI::Inst()->SetPlacementCallback(boost::bind(Game::CheckTileType, TILEBOG, _1, _2));
	UI::Inst()->blueprint(Coordinate(1,1));
	Game::Inst()->Renderer()->SetCursorMode(Cursor_Bog);
	UI::Inst()->HideMenu();
	UI::Inst()->SetExtraTooltip("Designate bog");
}


boost::weak_ptr<Entity> UI::GetEntity(const Coordinate& pos) {
	if (pos.X() >= 0 && pos.X() < Map::Inst()->Width() && pos.Y() >= 0 && pos.Y() < Map::Inst()->Height()) {
		std::set<int> *npcList = Map::Inst()->NPCList(pos);
		if (!npcList->empty()) return Game::Inst()->GetNPC((*npcList->begin()));

		std::set<int> *itemList = Map::Inst()->ItemList(pos);
		if (!itemList->empty()) {
			std::set<boost::weak_ptr<Item> >::iterator itemi = Game::Inst()->freeItems.find(Game::Inst()->itemList[*itemList->begin()]);
			if (itemi != Game::Inst()->freeItems.end()) {
				return *itemi;
			}
		}

		int entity = Map::Inst()->GetNatureObject(pos);
		if (entity > -1) return (Game::Inst()->natureList[entity]);

		entity = Map::Inst()->GetConstruction(pos);
		if (entity > -1) return Game::Inst()->GetConstruction(entity);
	}
	return boost::weak_ptr<Entity>();
}

void UI::HandleUnderCursor(const Coordinate& pos, std::list<boost::weak_ptr<Entity> >* result) {
	result->clear();

	if (Map::Inst()->IsInside(pos)) {
		std::set<int> *npcList = Map::Inst()->NPCList(pos);
		if (!npcList->empty()) {
			for (std::set<int>::iterator npci = npcList->begin(); npci != npcList->end(); ++npci) {
				result->push_back(Game::Inst()->GetNPC(*npci));
			}
		}

		std::set<int> *itemList = Map::Inst()->ItemList(pos);
		if (!itemList->empty()) {
			for (std::set<int>::iterator itemi = itemList->begin(); itemi != itemList->end(); ++itemi) {
				result->push_back(Game::Inst()->itemList[*itemi]);
			}
		}

		int entity = Map::Inst()->GetNatureObject(pos);
		if (entity > -1) {
			result->push_back(Game::Inst()->natureList[entity]);
		}

		entity = Map::Inst()->GetConstruction(pos);
		if (entity > -1) {
			result->push_back(Game::Inst()->GetConstruction(entity));
		}
	}
}

int UI::KeyHelpTextColor() const { return keyHelpTextColor; }

void UI::SetTextMode(bool val, int limit) { 
	textMode = val; 
	if (!textMode) inputString = "";
	inputStringLimit = limit; 
}

std::string UI::InputString() { return inputString;}
void UI::InputString(std::string value) { inputString = value; }

void UI::HideMenu() {
	menuOpen = false;
	textMode = false;
}

void UI::CloseMenu() {
	menuOpen = false;
	_state = UINORMAL;
	a.X(0); a.Y(0);
	textMode = false;
	if (currentMenu) currentMenu->Close();
	currentMenu = Menu::MainMenu();
	extraTooltip = "";
}

bool UI::ShiftPressed() { return TCODConsole::isKeyPressed(TCODK_SHIFT); }
TCOD_key_t UI::getKey() { return key; }

void UI::ChooseCreateNPC() {
	int npc;
	Menu *NPCChoiceMenu = new Menu(std::vector<MenuChoice>(), "NPC");
	NPCChoiceMenu->AddChoice(MenuChoice("None", boost::lambda::var(npc) = -1));
	for (unsigned int i = 0; i < NPC::Presets.size(); ++i) {
		NPCChoiceMenu->AddChoice(MenuChoice(NPC::Presets[i].name, boost::lambda::var(npc) = i));
	}
	NPCChoiceMenu->ShowModal();

	if (npc >= 0) {
		UI::Inst()->state(UIPLACEMENT);
		UI::Inst()->SetCallback(boost::bind(&Game::CreateNPC, Game::Inst(), _1, NPCType(npc)));
		UI::Inst()->SetPlacementCallback(boost::bind(Game::CheckTree, _1, Coordinate(1,1)));
		UI::Inst()->blueprint(Coordinate(1,1));
		Game::Inst()->Renderer()->SetCursorMode(NPC::Presets[npc]);
	}
}

void UI::ChooseCreateItem() {
	int item, category;
	Menu *ItemCategoryMenu = new Menu(std::vector<MenuChoice>(), "Categories");
	for (unsigned int i = 0; i < Item::Categories.size(); ++i) {
		if (Item::Categories[i].parent < 0)
			ItemCategoryMenu->AddChoice(MenuChoice(Item::Categories[i].name, boost::lambda::var(category) = i));
	}
	ItemCategoryMenu->ShowModal();

	Menu *ItemChoiceMenu = new Menu(std::vector<MenuChoice>(), "Item");
	ItemChoiceMenu->AddChoice(MenuChoice("None", boost::lambda::var(item) = -1));
	for (unsigned int i = 0; i < Item::Presets.size(); ++i) {
		if (Item::Presets[i].categories.find(category) != Item::Presets[i].categories.end())
			ItemChoiceMenu->AddChoice(MenuChoice(Item::Presets[i].name, boost::lambda::var(item) = i));
	}
	ItemChoiceMenu->ShowModal();

	if (item >= 0) {
		UI::Inst()->state(UIPLACEMENT);
		UI::Inst()->SetCallback(boost::bind(&Game::CreateItem, Game::Inst(), _1, ItemType(item), false,
			0, std::vector<boost::weak_ptr<Item> >(), boost::shared_ptr<Container>()));
		UI::Inst()->SetPlacementCallback(boost::bind(Game::CheckTree, _1, Coordinate(1,1)));
		UI::Inst()->blueprint(Coordinate(1,1));
		Game::Inst()->Renderer()->SetCursorMode(Item::Presets[item]);
	}

}

void UI::ChooseDig() {
	UI::Inst()->state(UIRECTPLACEMENT);
	UI::Inst()->SetRectCallback(boost::bind(Game::Dig, _1, _2));
	UI::Inst()->SetPlacementCallback(boost::bind(Game::CheckPlacement, _1, _2, std::set<TileType>()));
	UI::Inst()->blueprint(Coordinate(1,1));
	Game::Inst()->Renderer()->SetCursorMode(Cursor_Dig);
	UI::Inst()->HideMenu();
	UI::Inst()->SetExtraTooltip("Dig");
}

void UI::ChooseNaturify() {
	for (int i = 0; i < 10000; ++i) {
		Map::Inst()->Naturify(Random::ChooseInExtent(Map::Inst()->Extent()));
	}
}

void UI::ChooseChangeTerritory(bool add) {
	if (Camp::Inst()->IsAutoTerritoryEnabled() && !add) {
		Camp::Inst()->DisableAutoTerritory();
		Announce::Inst()->AddMsg("Automatic territory handling disabled", TCODColor::cyan);
	}
	UI::Inst()->state(UIRECTPLACEMENT);
	UI::Inst()->SetRectCallback(boost::bind(&Map::SetTerritoryRectangle, Map::Inst(), _1, _2, add));
	UI::Inst()->SetPlacementCallback(boost::bind(Game::CheckTree, _1, _2));
	UI::Inst()->blueprint(Coordinate(1,1));
	Game::Inst()->Renderer()->SetCursorMode(add ? Cursor_AddTerritory : Cursor_RemoveTerritory);
	UI::Inst()->HideMenu();
	UI::Inst()->SetExtraTooltip(add ? "Expand" : "Shrink");
}

void UI::ChooseGatherItems() {
	UI::Inst()->state(UIRECTPLACEMENT);
	UI::Inst()->SetRectCallback(boost::bind(&Game::GatherItems, Game::Inst(), _1, _2));
	UI::Inst()->SetPlacementCallback(boost::bind(Game::CheckTree, _1, _2));
	UI::Inst()->blueprint(Coordinate(1,1));
	Game::Inst()->Renderer()->SetCursorMode(Cursor_Gather);
	UI::Inst()->HideMenu();
	UI::Inst()->SetExtraTooltip("Gather items");
}

void UI::ChooseNormalPlacement(boost::function<void(Coordinate)> callback, boost::function<bool(Coordinate, Coordinate)> placement, int cursor, std::string optionalTooltip) {
	UI::Inst()->state(UIPLACEMENT);
	UI::Inst()->SetCallback(callback);
	UI::Inst()->SetPlacementCallback(placement);
	UI::Inst()->blueprint(Coordinate(1,1));
	Game::Inst()->Renderer()->SetCursorMode(cursor);
	if (optionalTooltip.length() > 0) {
		UI::Inst()->HideMenu();
		UI::Inst()->SetExtraTooltip(optionalTooltip);
	}
}

void UI::ChooseRectPlacement(boost::function<void(Coordinate, Coordinate)> rectCallback, boost::function<bool(Coordinate, Coordinate)> placement, int cursor, std::string optionalTooltip) {
	UI::Inst()->state(UIRECTPLACEMENT);
	UI::Inst()->SetRectCallback(rectCallback);
	UI::Inst()->SetPlacementCallback(placement);
	UI::Inst()->blueprint(Coordinate(1,1));
	Game::Inst()->Renderer()->SetCursorMode(cursor);
	if (optionalTooltip.length() > 0) {
		UI::Inst()->HideMenu();
		UI::Inst()->SetExtraTooltip(optionalTooltip);
	}
}

void UI::ChooseRectPlacementCursor(boost::function<void(Coordinate, Coordinate)> rectCallback, boost::function<bool(Coordinate, Coordinate)> placement, CursorType cursor) {
	UI::Inst()->state(UIRECTPLACEMENT);
	UI::Inst()->SetRectCallback(rectCallback);
	UI::Inst()->SetPlacementCallback(placement);
	UI::Inst()->blueprint(Coordinate(1,1));
	Game::Inst()->Renderer()->SetCursorMode(cursor);
}

void UI::ChooseABPlacement(boost::function<void(Coordinate)> callback, boost::function<bool(Coordinate, Coordinate)> placement, int cursor, std::string optionalTooltip) {
	UI::Inst()->state(UIABPLACEMENT);
	UI::Inst()->SetCallback(callback);
	UI::Inst()->SetPlacementCallback(placement);
	UI::Inst()->blueprint(Coordinate(1,1));
	Game::Inst()->Renderer()->SetCursorMode(cursor);
	if (optionalTooltip.length() > 0) {
		UI::Inst()->HideMenu();
		UI::Inst()->SetExtraTooltip(optionalTooltip);
	}
}

void UI::SetExtraTooltip(std::string tooltip) {
	extraTooltip = tooltip;
}

void UI::Reset() {
	delete instance;
	instance = 0;
}
