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

#include "UI.hpp"
#include "Announce.hpp"
#include "Game.hpp"
#include "GCamp.hpp"
#include "Logger.hpp"
#include "Map.hpp"
#include "Job.hpp"
#include "Stockpile.hpp"
#include "Farmplot.hpp"
#include "Data.hpp"
#include "UI/StockManagerDialog.hpp"
#include "UI/SquadsDialog.hpp"
#include "UI/ConstructionDialog.hpp"
#include "UI/Tooltip.hpp"
#include "UI/ScrollPanel.hpp"
#include "UI/Frame.hpp"
#include "UI/UIList.hpp"
#include "UI/Label.hpp"
#include "UI/Button.hpp"

static TCOD_key_t NO_KEY = {
    TCODK_NONE, 0, false, false, false, false, false, false
};

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
	//TODO: This isn't pretty, but it works.
    key = TCODConsole::checkForKeypress(TCOD_KEY_PRESSED);
    if (!currentMenu || currentMenu->Update(-1, -1, false, key) != KEYRESPOND) {
        if (!textMode) {
            if (key.c == 'q') Game::Exit();
            else if (key.c == 'b') {
                menuX = mouseInput.cx;
                menuY = mouseInput.cy;
                ChangeMenu(Menu::BasicsMenu());
            } else if (key.c == 'w') {
                menuX = mouseInput.cx;
                menuY = mouseInput.cy;
                ChangeMenu(Menu::WorkshopsMenu());
            } else if (key.c == 'o') {
                menuX = mouseInput.cx;
                menuY = mouseInput.cy;
                ChangeMenu(Menu::OrdersMenu());
            } else if (key.c == 's') {
                menuX = mouseInput.cx;
                menuY = mouseInput.cy;
                ChangeMenu(StockManagerDialog::StocksDialog());
            } else if (key.c == 'f') {
                menuX = mouseInput.cx;
                menuY = mouseInput.cy;
                ChangeMenu(Menu::FurnitureMenu());
            } else if (key.c == 'm') {
                menuX = mouseInput.cx;
                menuY = mouseInput.cy;
                ChangeMenu(SquadsDialog::SquadDialog());
            } else if (key.c == 'h') {
                keyHelpTextColor = 255;
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
            } else if (key.vk == TCODK_SPACE) { Game::Inst()->Pause();
            } else if (key.vk == TCODK_PRINTSCREEN) { 
                Data::SaveScreenshot();
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
                if (menuResult == NOMENUHIT) {
                    if (menuOpen) {menuResult = currentMenu->Update(mouseInput.cx, mouseInput.cy, true, NO_KEY); lbuttonPressed = false; }
                }                    
            }
            if (menuResult == NOMENUHIT) {
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
        if (menuResult == NOMENUHIT) {
            if (menuOpen) {
                menuResult = currentMenu->Update(mouseInput.cx, mouseInput.cy, false, NO_KEY);
            }
            if (menuResult == NOMENUHIT) {
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
            currentMenu = (*underCursor.begin()).lock()->GetContextMenu();
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
	if (_state == UINORMAL && currentMenu->Update(mouseInput.cx, mouseInput.cy, false, NO_KEY) == NOMENUHIT 
		&& sideBar.Update(mouseInput.cx, mouseInput.cy, false) == NOMENUHIT && !underCursor.empty() && underCursor.begin()->lock()) {
		for (std::list<boost::weak_ptr<Entity> >::iterator ucit = underCursor.begin(); ucit != underCursor.end(); ++ucit) {
			ucit->lock()->GetTooltip(Game::Inst()->upleft.X() + mouseInput.cx, Game::Inst()->upleft.Y() + mouseInput.cy, tooltip);
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
}

void UI::DrawTopBar(TCODConsole* console) {
	console->setAlignment(TCOD_CENTER);
	console->setForegroundColor(TCODColor::white);
	console->print(console->getWidth() / 2, 0, "Orcs: %d   Goblins: %d  -  %s", Game::Inst()->OrcCount(),
		Game::Inst()->GoblinCount(), Game::Inst()->SeasonToString(Game::Inst()->CurrentSeason()).c_str());

	if (Game::Inst()->Paused()) {
		console->setForegroundColor(TCODColor::red);
		console->print(Game::Inst()->ScreenWidth() / 2, 1, "- - - - PAUSED - - - -");
	}
	console->setAlignment(TCOD_LEFT);

	if (keyHelpTextColor > 0) {
		int x = 10;
		console->setForegroundColor(TCODColor(0,keyHelpTextColor,0));
		console->print(x++, 3, "Q");
		console->setForegroundColor(TCODColor(keyHelpTextColor,keyHelpTextColor,keyHelpTextColor));
		console->print(x, 3, "uit");
		console->print(x, 4, "Space to pause");
		x += 3 + 2;
		console->setForegroundColor(TCODColor(0,keyHelpTextColor,0));
		console->print(x++, 3, "B");
		console->setForegroundColor(TCODColor(keyHelpTextColor,keyHelpTextColor,keyHelpTextColor));
		console->print(x, 3, "asics");
		x += 5 + 2;
		console->setForegroundColor(TCODColor(0,keyHelpTextColor,0));
		console->print(x++, 3, "W");
		console->setForegroundColor(TCODColor(keyHelpTextColor,keyHelpTextColor,keyHelpTextColor));
		console->print(x, 3, "orkshops");
		x += 8 + 2;
		console->setForegroundColor(TCODColor(0,keyHelpTextColor,0));
		console->print(x++, 3, "O");
		console->setForegroundColor(TCODColor(keyHelpTextColor,keyHelpTextColor,keyHelpTextColor));
		console->print(x, 3, "rders");
		x += 5 + 2;
		console->setForegroundColor(TCODColor(0,keyHelpTextColor,0));
		console->print(x++, 3, "F");
		console->setForegroundColor(TCODColor(keyHelpTextColor,keyHelpTextColor,keyHelpTextColor));
		console->print(x, 3, "urniture");
		x += 8 + 2;
		console->setForegroundColor(TCODColor(0,keyHelpTextColor,0));
		console->print(x++, 3, "S");
		console->setForegroundColor(TCODColor(keyHelpTextColor,keyHelpTextColor,keyHelpTextColor));
		console->print(x, 3, "tockmanager");
		x += 11 + 2;
		console->setForegroundColor(TCODColor(0,keyHelpTextColor,0));
		console->print(x++, 3, "M");
		console->setForegroundColor(TCODColor(keyHelpTextColor,keyHelpTextColor,keyHelpTextColor));
		console->print(x, 3, "ilitary");
	}

	console->setForegroundColor(TCODColor::white);
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
			std::set<boost::weak_ptr<Item> >::iterator itemi = Game::Inst()->freeItems.find(Game::Inst()->itemList[*itemList->begin()]);
			if (itemi != Game::Inst()->freeItems.end()) {
				result->push_back(*itemi);
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

SideBar::SideBar() :
width(19),
	height(30),
	topY(0),
	npc(false),
	construction(false)
{}

MenuResult SideBar::Update(int x, int y, bool clicked) {
	if (contents && x > Game::Inst()->ScreenWidth() - width) {
		MenuResult result = contents->Update(x - (leftX + 1), y - (topY + 14), clicked, NO_KEY);
		if(result != NOMENUHIT) {
			return result;
		}
		if (y > topY && y < topY+height) return MENUHIT;
	}
	return NOMENUHIT;
}

void SideBar::Draw(TCODConsole* console) {
	if (entity.lock()) {
		int edgeX = console->getWidth();
		leftX = edgeX - width;
		topY = std::max(0,(console->getHeight() - height) / 2);
		TCODConsole minimap(11,11);

		console->rect(edgeX - (width-1), topY+1, width-2, height-2, true);

		if(contents) {
			contents->Draw(edgeX - (width-1), topY+14, console);
		}

		Game::Inst()->Draw(entity.lock()->Position()-5, &minimap, false);
		console->setForegroundColor(TCODColor::white);
		console->printFrame(edgeX - width, topY, width, height, false, TCOD_BKGND_DEFAULT, entity.lock()->Name().c_str());
		minimap.flush();
		TCODConsole::blit(&minimap, 0, 0, 11, 11, console, edgeX - (width-4), topY + 2);
	}
	console->setForegroundColor(TCODColor::white);
}

void SideBar::GetTooltip(int x, int y, Tooltip *tooltip, TCODConsole *console) {
	if(entity.lock()) {
		int edgeX = console->getWidth();
		int minimapX = edgeX - (width-4);
		int minimapY = topY + 2;
		if (x >= minimapX && x < minimapX + 11 &&
			y >= minimapY && y < minimapY + 11) {
			int actualX = (entity.lock()->Position()-5).X() + x - minimapX;
			int actualY = (entity.lock()->Position()-5).Y() + y - minimapY;
			std::list<boost::weak_ptr<Entity> > minimapUnderCursor = std::list<boost::weak_ptr<Entity> >();
			UI::Inst()->HandleUnderCursor(Coordinate(actualX, actualY), &minimapUnderCursor);
			if (!minimapUnderCursor.empty() && minimapUnderCursor.begin()->lock()) {
				for (std::list<boost::weak_ptr<Entity> >::iterator ucit = minimapUnderCursor.begin(); ucit != minimapUnderCursor.end(); ++ucit) {
					ucit->lock()->GetTooltip(actualX, actualY, tooltip);
				}
			}
		}
		if (contents) {
			contents->GetTooltip(x - (leftX + 1), y - (topY + 14), tooltip);
		}
	}
}

void SideBar::SetEntity(boost::weak_ptr<Entity> ent) {
	entity = ent;
	npc = construction = false;
	height = 15;
	contents.reset();
	if (boost::shared_ptr<NPC> npci = boost::dynamic_pointer_cast<NPC>(entity.lock())) {
		height = 30;
		npc = true;
		contents = boost::shared_ptr<Drawable>(new UIContainer(std::vector<Drawable *>(), 0, 0, width - 2, 15));
		boost::shared_ptr<UIContainer> container = boost::dynamic_pointer_cast<UIContainer>(contents);
		Frame *frame = new Frame("Effects", std::vector<Drawable *>(), 0, 0, width - 2, 12);
		frame->AddComponent(new UIList<StatusEffect, std::list<StatusEffect> >(boost::dynamic_pointer_cast<NPC>(entity.lock())->StatusEffects(), 1, 1, width - 4, 10,
													 SideBar::DrawStatusEffect));
		container->AddComponent(frame);
		boost::function<std::string()> func = boost::bind(&SideBar::NPCSquadLabel, npci.get());
		container->AddComponent(new LiveLabel(boost::bind(&SideBar::NPCSquadLabel, npci.get()), 0, 12, TCOD_LEFT));
		container->AddComponent(new LiveLabel(boost::bind(&SideBar::NPCWeaponLabel, npci.get()), 0, 13, TCOD_LEFT));
		container->AddComponent(new LiveLabel(boost::bind(&SideBar::NPCArmorLabel, npci.get()), 0, 14, TCOD_LEFT));
	} else if (boost::shared_ptr<FarmPlot> fp = boost::dynamic_pointer_cast<FarmPlot>(entity.lock())) {
		height = 30;
		construction = true;
		contents = boost::shared_ptr<Drawable>(new UIContainer(std::vector<Drawable *>(), 0, 0, width - 2, 12));
		boost::shared_ptr<UIContainer> container = boost::dynamic_pointer_cast<UIContainer>(contents);
		Frame *frame = new Frame("Seeds", std::vector<Drawable *>(), 0, 0, width - 2, 12);
		frame->AddComponent(new UIList<std::pair<ItemType, bool>, std::map<ItemType, bool> >(fp->AllowedSeeds(), 1, 1, width - 4, 10,
																							 SideBar::DrawSeed,
																							 boost::bind(&FarmPlot::SwitchAllowed, fp, _1)));
		container->AddComponent(frame);			
	} else if (boost::shared_ptr<Stockpile> sp = boost::dynamic_pointer_cast<Stockpile>(entity.lock())) {
		height = 51;
		construction = true;
		contents = boost::shared_ptr<Drawable>(new UIContainer(std::vector<Drawable *>(), 0, 0, width - 2, 12));
		boost::shared_ptr<UIContainer> container = boost::dynamic_pointer_cast<UIContainer>(contents);
		container->AddComponent(new Button("All", boost::bind(&Stockpile::SetAllAllowed, sp, true), 0, 0, 8));
		container->AddComponent(new Button("None", boost::bind(&Stockpile::SetAllAllowed, sp, false), 9, 0, 8));
		container->AddComponent(new ScrollPanel(0, 3, width - 2, 33,
												new UIList<ItemCat>(&Item::Categories, 0, 0, width, Item::Categories.size(),
																	boost::bind(&ConstructionDialog::DrawCategory, boost::dynamic_pointer_cast<Construction>(entity.lock()).get(), _1, _2, _3, _4, _5, _6, _7),
																	boost::bind(&Stockpile::SwitchAllowed, boost::dynamic_pointer_cast<Stockpile>(entity.lock()), _1, boost::bind(&UI::ShiftPressed, UI::Inst())))));
	} else if (boost::dynamic_pointer_cast<Construction>(entity.lock())) {
		boost::shared_ptr<Construction> construct(boost::static_pointer_cast<Construction>(entity.lock()));
		if (construct->HasTag(WORKSHOP)) {
			height = 30;
			contents = boost::shared_ptr<Drawable>(new UIContainer(std::vector<Drawable *>(), 0, 0, width - 2, 12));
			boost::shared_ptr<UIContainer> container = boost::dynamic_pointer_cast<UIContainer>(contents);
			Frame *frame = new Frame("Production", std::vector<Drawable *>(), 0, 0, width - 2, 12);
			frame->AddComponent(new UIList<ItemType, std::deque<ItemType> >(construct->JobList(), 1, 1, width - 4, 10,
																				   ConstructionDialog::DrawJob));
			container->AddComponent(frame);			
		}
		construction = true;
	}
}

void SideBar::DrawStatusEffect(StatusEffect effect, int i, int x, int y, int width, bool selected, TCODConsole *console) {
	console->setForegroundColor(effect.color);
	console->print(x, y, "%c%s", effect.graphic, effect.name.c_str());
	console->setForegroundColor(TCODColor::white);
}

void SideBar::DrawSeed(std::pair<ItemType, bool> seed, int i, int x, int y, int width, bool selected, TCODConsole *console) {
	console->setForegroundColor(seed.second ? TCODColor::green : TCODColor::red);
	console->print(x, y, "%c %s", seed.second ? 225 : 224, Item::Presets[seed.first].name.substr(0, width-3).c_str());
	console->setForegroundColor(TCODColor::white);
}

std::string SideBar::NPCSquadLabel(NPC *npc) {
	if(npc->MemberOf().lock()) {
		return boost::str(boost::format("S: %s") % npc->MemberOf().lock()->Name());
	} else {
		return "";
	}
}

std::string SideBar::NPCWeaponLabel(NPC *npc) {
	if(npc->Wielding().lock()) {
		return boost::str(boost::format("W: %s") % npc->Wielding().lock()->Name());
	} else {
		return "";
	}
}

std::string SideBar::NPCArmorLabel(NPC *npc) {
	if(npc->Wearing().lock()) {
		return boost::str(boost::format("A: %s") % npc->Wearing().lock()->Name());
	} else {
		return "";
	}
}

bool UI::ShiftPressed() { return TCODConsole::isKeyPressed(TCODK_SHIFT); }
TCOD_key_t UI::getKey() { return key; }
