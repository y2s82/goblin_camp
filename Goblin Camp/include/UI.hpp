#ifndef UI_HEADER
#define UI_HEADER

#include <libtcod.hpp>
#include <vector>
#include <string>
#include <boost/function.hpp>

#include "Menu.hpp"
#include "GameEntity.hpp"
#include "Game.hpp"

enum UIState {
	UINORMAL,
	UIPLACEMENT,
	UIABPLACEMENT,
	UIRECTPLACEMENT
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
		std::list<boost::weak_ptr<GameEntity> > underCursor;
		void HandleUnderCursor(Coordinate);
        boost::weak_ptr<GameEntity> GetEntity(Coordinate);

        void DrawTopBar();

	public:
		static UI* Inst();
		void Update(TCOD_key_t, Coordinate);
		void Draw(Coordinate);
		void blueprint(Coordinate);
		void state(UIState);
		static void ChangeMenu(Menu*);
		static void ChooseConstruct(ConstructionType, UIState);
		static void ChooseStockpile(ConstructionType);
		static void ChooseTreeFelling();
		static void ChoosePlantHarvest();
		void SetCallback(boost::function<void(Coordinate)>);
		void SetRectCallback(boost::function<void(Coordinate,Coordinate)>);
		void SetPlacementCallback(boost::function<bool(Coordinate,Coordinate)>);
		Menu* CurrentMenu();
		void CurrentMenu(Menu*);
		void AddToHistory(Menu*);
};

#endif
