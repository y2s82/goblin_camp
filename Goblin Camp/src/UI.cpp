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
#include <string>
#include <vector>
#include <boost/function.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/lambda/lambda.hpp>

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

#pragma mark UI

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
	cursorChar('X')
{
	currentMenu = Menu::MainMenu();
	menuHistory.reserve(10);
	placementCallback = boost::bind(Game::CheckPlacement, _1, _2);
	callback = boost::bind(Game::PlaceConstruction, _1, 0);
	rectCallback = boost::bind(Game::PlaceStockpile, _1, _2, 0, 0);
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
	HandleKeyboard();
	HandleMouse();
}

void UI::HandleKeyboard() {
	Config::KeyMap& keyMap = Config::GetKeyMap();

	//TODO: This isn't pretty, but it works.
	key = TCODConsole::checkForKeypress(TCOD_KEY_PRESSED);
	if (!currentMenu || !(currentMenu->Update(-1, -1, false, key) & KEYRESPOND)) {
		if (!textMode) {
			if (key.c == keyMap["Exit"]) Game::Exit();
			else if (key.c == keyMap["Basics"]) {
				menuX = mouseInput.cx;
				menuY = mouseInput.cy;
				ChangeMenu(Menu::BasicsMenu());
			} else if (key.c == keyMap["Workshops"]) {
				menuX = mouseInput.cx;
				menuY = mouseInput.cy;
				ChangeMenu(Menu::WorkshopsMenu());
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
			} else if (key.c == '.') {
				Game::Inst()->CreateNPC(Coordinate(100,100), NPC::StringToNPCType("giant"));
			} else if (key.c == keyMap["Jobs"]) { 
				ChangeMenu(JobDialog::JobListingDialog());
			}

			int addition = 1;
			if (ShiftPressed()) addition *= 10;
			if (key.vk == TCODK_UP) {
				if (Game::Inst()->upleft.Y(Game::Inst()->upleft.Y()-1) < -1) Game::Inst()->upleft.Y(-1);
			} else if (key.vk == TCODK_DOWN) {
				if (Game::Inst()->upleft.Y(Game::Inst()->upleft.Y()+1) > 1+ Map::Inst()->Height() - Game::Inst()->ScreenHeight()) Game::Inst()->upleft.Y(1+ Map::Inst()->Height() - Game::Inst()->ScreenHeight());
			} else if (key.vk == TCODK_LEFT) {
				if (Game::Inst()->upleft.X(Game::Inst()->upleft.X()-1) < -1) Game::Inst()->upleft.X(-1);
			} else if (key.vk == TCODK_RIGHT) {
				if (Game::Inst()->upleft.X(Game::Inst()->upleft.X()+1) > 1+ Map::Inst()->Width() - Game::Inst()->ScreenWidth()) Game::Inst()->upleft.X(1+ Map::Inst()->Width() - Game::Inst()->ScreenWidth());
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
		if (key.vk == TCODK_ESCAPE && menuOpen) {
			rbuttonPressed = true;
		}
	}

	if (mouseInput.x < 0) {
		Game::Inst()->upleft.X(Game::Inst()->upleft.X() + mouseInput.cx);
		mouseInput.x = 0;
		mouseInput.cx = 0;
	} else if (mouseInput.cx >= Game::Inst()->ScreenWidth()) {
		Game::Inst()->upleft.X(Game::Inst()->upleft.X() + (mouseInput.cx - (Game::Inst()->ScreenWidth()-1)));
		mouseInput.cx = Game::Inst()->ScreenWidth() - 1;
		mouseInput.x = (Game::Inst()->ScreenWidth() - 1)* Game::Inst()->CharWidth();
	}
	if (mouseInput.y < 0) {
		Game::Inst()->upleft.Y(Game::Inst()->upleft.Y() + mouseInput.cy);
		mouseInput.y = 0;
		mouseInput.cy = 0;
	} else if (mouseInput.cy >= Game::Inst()->ScreenHeight()) {
		Game::Inst()->upleft.Y(Game::Inst()->upleft.Y() + (mouseInput.cy - (Game::Inst()->ScreenHeight()-1)));
		mouseInput.cy = Game::Inst()->ScreenHeight() - 1;
		mouseInput.y = (Game::Inst()->ScreenHeight() - 1) * Game::Inst()->CharHeight();
	}

	if (Game::Inst()->upleft.Y(Game::Inst()->upleft.Y()-1) < -1) Game::Inst()->upleft.Y(-1);
	if (Game::Inst()->upleft.Y(Game::Inst()->upleft.Y()+1) > 1+ Map::Inst()->Height() - Game::Inst()->ScreenHeight()) Game::Inst()->upleft.Y(1+ Map::Inst()->Height() - Game::Inst()->ScreenHeight());
	if (Game::Inst()->upleft.X(Game::Inst()->upleft.X()-1) < -1) Game::Inst()->upleft.X(-1);
	if (Game::Inst()->upleft.X(Game::Inst()->upleft.X()+1) > 1+ Map::Inst()->Width() - Game::Inst()->ScreenWidth()) Game::Inst()->upleft.X(1+ Map::Inst()->Width() - Game::Inst()->ScreenWidth());
}

void UI::HandleMouse() {
	int tmp;
	MenuResult menuResult = NOMENUHIT;
	bool xswap = false, yswap = false;

	if (TCODConsole::isWindowClosed()) Game::Inst()->Exit();

	TCOD_mouse_t tempStatus = TCODMouse::getStatus();
	if (tempStatus.x != oldMouseInput.x || tempStatus.y != oldMouseInput.y) {
		mouseInput = tempStatus;
		drawCursor = false;
		TCODMouse::showCursor(true);
	}

	if (tempStatus.lbutton_pressed) lbuttonPressed = true;
	if (tempStatus.mbutton_pressed) mbuttonPressed = true;
	if (tempStatus.rbutton_pressed) rbuttonPressed = true;

	if (_state == UINORMAL) {
		HandleUnderCursor(Coordinate(mouseInput.cx + Game::Inst()->upleft.X(), mouseInput.cy + Game::Inst()->upleft.Y()), &underCursor);
	}

	if (_state == UIPLACEMENT || _state == UIABPLACEMENT || _state == UIRECTPLACEMENT) {
		placeable = placementCallback(Coordinate(mouseInput.cx + Game::Inst()->upleft.X(), mouseInput.cy + Game::Inst()->upleft.Y()), _blueprint);
	}

	if (_state == UIABPLACEMENT || _state == UIRECTPLACEMENT) {
		b.X(mouseInput.cx + Game::Inst()->upleft.X());
		b.Y(mouseInput.cy + Game::Inst()->upleft.Y());
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
					callback(Coordinate(mouseInput.cx + Game::Inst()->upleft.X(), mouseInput.cy + Game::Inst()->upleft.Y()));
				} else if (_state == UIABPLACEMENT && placeable) {
					if (a.X() == 0) {
						a.X(mouseInput.cx + Game::Inst()->upleft.X());
						a.Y(mouseInput.cy + Game::Inst()->upleft.Y());
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
						a.X(mouseInput.cx + Game::Inst()->upleft.X());
						a.Y(mouseInput.cy + Game::Inst()->upleft.Y());
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

						if (placementCallback(Coordinate(a.X(), a.Y()), _blueprint))
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
	} else if (tempStatus.lbutton && !oldMouseInput.lbutton) {
		menuResult = sideBar.Update(mouseInput.cx, mouseInput.cy, false);
		if (menuResult & NOMENUHIT) {
			if (menuOpen) {
				menuResult = currentMenu->Update(mouseInput.cx, mouseInput.cy, false, NO_KEY);
			}
			if (menuResult & NOMENUHIT) {
				if (_state == UIABPLACEMENT && placeable) {
					if (a.X() == 0) {
						a.X(mouseInput.cx + Game::Inst()->upleft.X());
						a.Y(mouseInput.cy + Game::Inst()->upleft.Y());
						draggingPlacement = true;
					}
				} else if (_state == UIRECTPLACEMENT && placeable) {
					if (a.X() == 0) {
						a.X(mouseInput.cx + Game::Inst()->upleft.X());
						a.Y(mouseInput.cy + Game::Inst()->upleft.Y());
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
		if (!menuOpen) {
			_state = UINORMAL; 
			a.X(0); 
			a.Y(0); 
			if (currentMenu) currentMenu->Close();
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
	}

	if (mbuttonPressed && menuOpen && !menuHistory.empty()) {
		currentMenu->selected(-1);
		_state = UINORMAL; a.X(0); a.Y(0);
		currentMenu->Close();
		currentMenu = menuHistory.back();
		currentMenu->Open();
		menuHistory.pop_back();
	}

	if (tempStatus.lbutton && _state == UINORMAL) {
		Game::Inst()->upleft.X(Game::Inst()->upleft.X() - (tempStatus.dx / 3));
		Game::Inst()->upleft.Y(Game::Inst()->upleft.Y() - (tempStatus.dy / 3));
		if (Game::Inst()->upleft.Y(Game::Inst()->upleft.Y()-1) < -1) Game::Inst()->upleft.Y(-1);
		if (Game::Inst()->upleft.Y(Game::Inst()->upleft.Y()+1) > 1+ Map::Inst()->Height() - Game::Inst()->ScreenHeight()) Game::Inst()->upleft.Y(1+ Map::Inst()->Height() - Game::Inst()->ScreenHeight());
		if (Game::Inst()->upleft.X(Game::Inst()->upleft.X()-1) < -1) Game::Inst()->upleft.X(-1);
		if (Game::Inst()->upleft.X(Game::Inst()->upleft.X()+1) > 1+ Map::Inst()->Width() - Game::Inst()->ScreenWidth()) Game::Inst()->upleft.X(1+ Map::Inst()->Width() - Game::Inst()->ScreenWidth());
		if (tempStatus.dx > 0 || tempStatus.dy > 0) draggingViewport = true;
	}

	lbuttonPressed = false;
	mbuttonPressed = false;
	rbuttonPressed = false;
	oldMouseInput = mouseInput;
}
void UI::Draw(Coordinate upleft, TCODConsole* console) {
	int tmp;
	bool xswap = false, yswap = false;

	DrawTopBar(console);
	sideBar.Draw(console);

	if (menuOpen) {
		currentMenu->Draw(menuX, menuY, console);
	}

	Tooltip *tooltip = Tooltip::Inst();
	tooltip->Clear();
	if (menuOpen) {
		currentMenu->GetTooltip(mouseInput.cx, mouseInput.cy, tooltip);
	}
	sideBar.GetTooltip(mouseInput.cx, mouseInput.cy, tooltip, console);
	if (_state == UINORMAL && (!menuOpen || (currentMenu->Update(mouseInput.cx, mouseInput.cy, false, NO_KEY) & NOMENUHIT)) 
		&& (sideBar.Update(mouseInput.cx, mouseInput.cy, false) & NOMENUHIT)
		&& (Announce::Inst()->Update(mouseInput.cx, mouseInput.cy, false) & NOMENUHIT)
		&& !underCursor.empty() && underCursor.begin()->lock()) {
			for (std::list<boost::weak_ptr<Entity> >::iterator ucit = underCursor.begin(); ucit != underCursor.end(); ++ucit) {
				if (ucit->lock()) {
					ucit->lock()->GetTooltip(Game::Inst()->upleft.X() + mouseInput.cx, Game::Inst()->upleft.Y() + mouseInput.cy, tooltip);
				}
			}
	}
	tooltip->Draw(mouseInput.cx, mouseInput.cy, console);

	if (_state == UIPLACEMENT || ((_state == UIABPLACEMENT || _state == UIRECTPLACEMENT) && a.X() == 0)) {
		for (int x = mouseInput.cx; x < mouseInput.cx + _blueprint.X() && x < console->getWidth(); ++x) {
			for (int y = mouseInput.cy; y < mouseInput.cy + _blueprint.Y() && y < console->getHeight(); ++y) {
				if (!placeable) console->putCharEx(x,y, cursorChar, TCODColor::red, TCODColor::black);
				else console->putCharEx(x,y, cursorChar, TCODColor::green, TCODColor::black);
			}
		}
	} else if (_state == UIABPLACEMENT && a.X() > 0) {
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
		for (int ix = a.X()-upleft.X(); ix <= b.X()-upleft.X() && ix < console->getWidth(); ++ix) {
			if (!yswap) {
				if (!placeable) console->putCharEx(ix,a.Y()-upleft.Y(), cursorChar, TCODColor::red, TCODColor::black);
				else console->putCharEx(ix,a.Y()-upleft.Y(), cursorChar, TCODColor::green, TCODColor::black);
			}
			else {
				if (!placeable) console->putCharEx(ix,b.Y()-upleft.Y(), cursorChar, TCODColor::red, TCODColor::black);
				else console->putCharEx(ix,b.Y()-upleft.Y(), cursorChar, TCODColor::green, TCODColor::black);
			}
		}
		for (int iy = a.Y()-upleft.Y(); iy <= b.Y()-upleft.Y() && iy < console->getHeight(); ++iy) {
			if (!xswap) {
				if (!placeable) console->putCharEx(b.X()-upleft.X(),iy, cursorChar, TCODColor::red, TCODColor::black);
				else console->putCharEx(b.X()-upleft.X(),iy, cursorChar, TCODColor::green, TCODColor::black);
			}
			else {
				if (!placeable) console->putCharEx(a.X()-upleft.X(),iy, cursorChar, TCODColor::red, TCODColor::black);
				else console->putCharEx(a.X()-upleft.X(),iy, cursorChar, TCODColor::green, TCODColor::black);
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
		for (int ix = a.X()-upleft.X(); ix <= b.X()-upleft.X() && ix < console->getWidth(); ++ix) {
			for (int iy = a.Y()-upleft.Y(); iy <= b.Y()-upleft.Y() && iy < console->getHeight(); ++iy) {
				if (!placeable) console->putCharEx(ix,iy,cursorChar, TCODColor::red, TCODColor::black);
				else console->putCharEx(ix,iy,cursorChar, TCODColor::green, TCODColor::black);
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
	}

	if (drawCursor) console->putCharEx(mouseInput.cx, mouseInput.cy, 'X', TCODColor::azure, TCODColor::black);

	Announce::Inst()->Draw(console);
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
	console->print(console->getWidth() / 2, 0, "Orcs: %d   Goblins: %d  -  %s", Game::Inst()->OrcCount(),
		Game::Inst()->GoblinCount(), Game::Inst()->SeasonToString(Game::Inst()->CurrentSeason()).c_str());

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

void UI::blueprint(Coordinate newBlue) { _blueprint = newBlue; }
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
	UI::Inst()->SetPlacementCallback(boost::bind(Game::CheckPlacement, _1, _2));
	UI::Inst()->blueprint(Construction::Blueprint(construct));
	UI::Inst()->state(state);
	UI::Inst()->SetCursor('C');
}

void UI::ChooseStockpile(ConstructionType stockpile) {
	int stockpileSymbol = '%';
	UI::Inst()->SetRectCallback(boost::bind(Game::PlaceStockpile, _1, _2, stockpile, stockpileSymbol));
	UI::Inst()->SetPlacementCallback(boost::bind(Game::CheckPlacement, _1, _2));
	UI::Inst()->blueprint(Construction::Blueprint(stockpile));
	UI::Inst()->state(UIRECTPLACEMENT);
	UI::Inst()->SetCursor('=');
}

void UI::ChooseTreeFelling() {
	UI::Inst()->state(UIRECTPLACEMENT);
	UI::Inst()->SetRectCallback(boost::bind(Game::FellTree, _1, _2));
	UI::Inst()->SetPlacementCallback(boost::bind(Game::CheckTree, _1, _2));
	UI::Inst()->blueprint(Coordinate(1,1));
	UI::Inst()->SetCursor('X');
}

void UI::ChoosePlantHarvest() {
	UI::Inst()->state(UIRECTPLACEMENT);
	UI::Inst()->SetRectCallback(boost::bind(Game::HarvestWildPlant, _1, _2));
	UI::Inst()->SetPlacementCallback(boost::bind(Game::CheckTree, _1, _2));
	UI::Inst()->blueprint(Coordinate(1,1));
	UI::Inst()->SetCursor('H');
}

void UI::ChooseOrderTargetCoordinate(boost::shared_ptr<Squad> squad) {
	UI::Inst()->state(UIPLACEMENT);
	UI::Inst()->SetCallback(boost::bind(Game::SetSquadTargetCoordinate, _1, squad));
	UI::Inst()->SetPlacementCallback(boost::bind(Game::CheckPlacement, _1, Coordinate(1,1)));
	UI::Inst()->blueprint(Coordinate(1,1));
	UI::Inst()->SetCursor('X');
}

void UI::ChooseOrderTargetEntity(boost::shared_ptr<Squad> squad) {
	UI::Inst()->state(UIPLACEMENT);
	UI::Inst()->SetCallback(boost::bind(Game::SetSquadTargetEntity, _1, squad));
	UI::Inst()->SetPlacementCallback(boost::bind(Game::CheckPlacement, _1, Coordinate(1,1)));
	UI::Inst()->blueprint(Coordinate(1,1));
	UI::Inst()->SetCursor('X');
}

void UI::ChooseDesignateTree() {
	UI::Inst()->state(UIRECTPLACEMENT);
	UI::Inst()->SetRectCallback(boost::bind(Game::DesignateTree, _1, _2));
	UI::Inst()->SetPlacementCallback(boost::bind(Game::CheckTree, _1, _2));
	UI::Inst()->blueprint(Coordinate(1,1));
	UI::Inst()->SetCursor('T');
}

void UI::ChooseDismantle() {
	UI::Inst()->state(UIRECTPLACEMENT);
	UI::Inst()->SetRectCallback(boost::bind(Game::DismantleConstruction, _1, _2));
	UI::Inst()->SetPlacementCallback(boost::bind(Game::CheckTree, _1, _2));
	UI::Inst()->blueprint(Coordinate(1,1));
	UI::Inst()->SetCursor('D');
}

void UI::ChooseUndesignate() {
	UI::Inst()->state(UIRECTPLACEMENT);
	UI::Inst()->SetRectCallback(boost::bind(Game::Undesignate, _1, _2));
	UI::Inst()->SetPlacementCallback(boost::bind(Game::CheckTree, _1, _2));
	UI::Inst()->blueprint(Coordinate(1,1));
	UI::Inst()->SetCursor('U');
}
void UI::ChooseDesignateBog() {
	UI::Inst()->state(UIRECTPLACEMENT);
	UI::Inst()->SetRectCallback(boost::bind(Game::DesignateBog, _1, _2));
	UI::Inst()->SetPlacementCallback(boost::bind(Game::CheckTileType, TILEBOG, _1, _2));
	UI::Inst()->blueprint(Coordinate(1,1));
	UI::Inst()->SetCursor('B');
}


boost::weak_ptr<Entity> UI::GetEntity(Coordinate pos) {
	if (pos.X() >= 0 && pos.X() < Map::Inst()->Width() && pos.Y() >= 0 && pos.Y() < Map::Inst()->Height()) {
		std::set<int> *npcList = Map::Inst()->NPCList(pos.X(), pos.Y());
		if (!npcList->empty()) return Game::Inst()->npcList[(*npcList->begin())];

		std::set<int> *itemList = Map::Inst()->ItemList(pos.X(), pos.Y());
		if (!itemList->empty()) {
			std::set<boost::weak_ptr<Item> >::iterator itemi = Game::Inst()->freeItems.find(Game::Inst()->itemList[*itemList->begin()]);
			if (itemi != Game::Inst()->freeItems.end()) {
				return *itemi;
			}
		}

		int entity = Map::Inst()->NatureObject(pos.X(), pos.Y());
		if (entity > -1) return (Game::Inst()->natureList[entity]);

		entity = Map::Inst()->GetConstruction(pos.X(), pos.Y());
		if (entity > -1) return Game::Inst()->GetConstruction(entity);
	}
	return boost::weak_ptr<Entity>();
}

void UI::HandleUnderCursor(Coordinate pos, std::list<boost::weak_ptr<Entity> >* result) {
	result->clear();

	if (pos.X() >= 0 && pos.X() < Map::Inst()->Width() && pos.Y() >= 0 && pos.Y() < Map::Inst()->Height()) {
		std::set<int> *npcList = Map::Inst()->NPCList(pos.X(), pos.Y());
		if (!npcList->empty()) {
			for (std::set<int>::iterator npci = npcList->begin(); npci != npcList->end(); ++npci) {
				result->push_back(Game::Inst()->npcList[*npci]);
			}
		}

		std::set<int> *itemList = Map::Inst()->ItemList(pos.X(), pos.Y());
		if (!itemList->empty()) {
			for (std::set<int>::iterator itemi = itemList->begin(); itemi != itemList->end(); ++itemi) {
				result->push_back(Game::Inst()->itemList[*itemi]);
			}
		}

		int entity = Map::Inst()->NatureObject(pos.X(), pos.Y());
		if (entity > -1) {
			result->push_back(Game::Inst()->natureList[entity]);
		}

		entity = Map::Inst()->GetConstruction(pos.X(), pos.Y());
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
}

void UI::SetCursor(int value) { cursorChar = value; }

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
		UI::Inst()->SetCursor(NPC::Presets[npc].graphic);
	}
}

void UI::ChooseCreateItem() {
	int item, category;
	Menu *ItemCategoryMenu = new Menu(std::vector<MenuChoice>(), "Categories");
	for (unsigned int i = 0; i < Item::Categories.size(); ++i) {
		if (!Item::Categories[i].parent)
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
		UI::Inst()->SetCallback(boost::bind(&Game::CreateItem, Game::Inst(), _1, ItemType(item), true,
			0, std::vector<boost::weak_ptr<Item> >(), boost::shared_ptr<Container>()));
		UI::Inst()->SetPlacementCallback(boost::bind(Game::CheckTree, _1, Coordinate(1,1)));
		UI::Inst()->blueprint(Coordinate(1,1));
		UI::Inst()->SetCursor(Item::Presets[item].graphic);
	}

}

void UI::ChooseDig() {
	UI::Inst()->state(UIRECTPLACEMENT);
	UI::Inst()->SetRectCallback(boost::bind(Game::Dig, _1, _2));
	UI::Inst()->SetPlacementCallback(boost::bind(Game::CheckPlacement, _1, _2));
	UI::Inst()->blueprint(Coordinate(1,1));
	UI::Inst()->SetCursor('_');
}

void UI::ChooseCreateFilth() {
	UI::Inst()->state(UIPLACEMENT);
	UI::Inst()->SetCallback(boost::bind(&Game::CreateFilth, Game::Inst(), _1));
	UI::Inst()->SetPlacementCallback(boost::bind(Game::CheckTree, _1, Coordinate(1,1)));
	UI::Inst()->blueprint(Coordinate(1,1));
	UI::Inst()->SetCursor('~');
}

void UI::ChooseCreateWater() {
	UI::Inst()->state(UIPLACEMENT);
	UI::Inst()->SetCallback(boost::bind(&Game::CreateWater, Game::Inst(), _1));
	UI::Inst()->SetPlacementCallback(boost::bind(Game::CheckTree, _1, Coordinate(1,1)));
	UI::Inst()->blueprint(Coordinate(1,1));
	UI::Inst()->SetCursor('~');
}

void UI::ChooseCorrupt() {
	UI::Inst()->state(UIPLACEMENT);
	UI::Inst()->SetCallback(boost::bind(&Map::Corrupt, Map::Inst(), _1, 200));
	UI::Inst()->SetPlacementCallback(boost::bind(Game::CheckTree, _1, Coordinate(1,1)));
	UI::Inst()->blueprint(Coordinate(1,1));
	UI::Inst()->SetCursor('c');
}