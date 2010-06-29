#pragma once

#include <libtcod.hpp>
#include <vector>
#include <string>
#include <boost/function.hpp>
#include <boost/weak_ptr.hpp>

#include "Menu.hpp"
#include "Entity.hpp"
#include "Game.hpp"

enum UIState {
	UINORMAL,
	UIPLACEMENT,
	UIABPLACEMENT,
	UIRECTPLACEMENT
};

class SideBar {
	boost::weak_ptr<Entity> entity;
	int width, height, topY;
	bool npc, construction, stockpile, farmplot;
public:
	SideBar();
	void SetEntity(boost::weak_ptr<Entity>);
	MenuResult Update(int, int);
	void Draw(TCODConsole*);
};

class UI {
private:
	UI();
	static UI* instance;
	bool menuOpen;
	int menuX, menuY;
	TCOD_mouse_t mouseInput;
	Menu* currentMenu;
	UIState _state;
	boost::function<void(Coordinate)> callback;
	boost::function<void(Coordinate,Coordinate)> rectCallback;
	boost::function<bool(Coordinate,Coordinate)> placementCallback;
	Coordinate _blueprint;
	bool placeable;
	Coordinate a,b;
	std::vector<Menu*> menuHistory;
	std::list<boost::weak_ptr<Entity> > underCursor;
	bool drawCursor;
	bool lbuttonPressed, mbuttonPressed, rbuttonPressed;
	TCOD_mouse_t oldMouseInput;
	int keyHelpTextColor;
	bool draggingViewport;
	SideBar sideBar;
	bool textMode;
	std::string inputString;
	int inputStringLimit;
	int cursorChar;

	void HandleUnderCursor(Coordinate);
	boost::weak_ptr<Entity> GetEntity(Coordinate);
	void DrawTopBar(TCODConsole*);
	void HandleKeyboard();
	void HandleMouse();
public:
	static UI* Inst();
	void Update();
	void Draw(Coordinate, TCODConsole*);
	void blueprint(Coordinate);
	void state(UIState);
	static void ChangeMenu(Menu*);
	static void ChooseConstruct(ConstructionType, UIState);
	static void ChooseStockpile(ConstructionType);
	static void ChooseTreeFelling();
	static void ChoosePlantHarvest();
	static void ChooseOrderTargetCoordinate(boost::shared_ptr<Squad>);
	static void ChooseOrderTargetEntity(boost::shared_ptr<Squad>);
	void SetCallback(boost::function<void(Coordinate)>);
	void SetRectCallback(boost::function<void(Coordinate,Coordinate)>);
	void SetPlacementCallback(boost::function<bool(Coordinate,Coordinate)>);
	Menu* CurrentMenu();
	void CurrentMenu(Menu*);
	void AddToHistory(Menu*);
	int KeyHelpTextColor() const;
	void SetTextMode(bool, int=50);
	std::string InputString();
	void HideMenu();
	void CloseMenu();
	void SetCursor(int);
};
