#pragma once

#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/weak_ptr.hpp>
#include <libtcod.hpp>
#include <string>
#include <vector>

#include "Game.hpp"

enum MenuResult {
	MENUHIT,
	NOMENUHIT
};

class MenuChoice {
	public:
		MenuChoice(std::string = "", boost::function<void()> = boost::bind(Game::DoNothing));
		std::string label;
		boost::function<void()> callback;
};

class Menu {
	protected:
		std::vector<MenuChoice> choices;
		int topX, topY, width, height;
		int _selected;
		void CalculateSize();
	public:
		Menu(std::vector<MenuChoice>);
		virtual ~Menu();
		virtual void Draw(int, int, TCODConsole*);
		virtual MenuResult Update(int = -1, int = -1);
		void selected(int);
		void AddChoice(MenuChoice);
		void Callback(unsigned int);

		static Menu* mainMenu;
		static Menu* MainMenu();
		static Menu* constructionMenu;
		static Menu* ConstructionMenu();
		static Menu* basicsMenu;
		static Menu* BasicsMenu();
		static Menu* workshopsMenu;
		static Menu* WorkshopsMenu();
		static Menu* ordersMenu;
		static Menu* OrdersMenu();
};

class JobMenu : public Menu {
	private:
		int scroll;
	public:
		JobMenu();
		void Draw(int, int, TCODConsole*);
		MenuResult Update(int = -1, int = -1);
		static JobMenu* jobListingMenu;
		static JobMenu* JobListingMenu();
		void ScrollDown();
		void ScrollUp();
};

class AnnounceMenu : public Menu {
	private:
		int scroll;
	public:
		AnnounceMenu();
		void Draw(int, int, TCODConsole*);
		MenuResult Update(int = -1, int = -1);
		static AnnounceMenu* announcementsMenu;
		static AnnounceMenu* AnnouncementsMenu();
		void ScrollDown();
		void ScrollUp();
};

class NPCMenu : public Menu {
	private:
		int scroll;
	public:
		NPCMenu();
		void Draw(int, int, TCODConsole*);
		MenuResult Update(int = -1, int = -1);
		static NPCMenu* npcListMenu;
		static NPCMenu* NPCListMenu();
		void ScrollDown();
		void ScrollUp();
};

class ConstructionMenu : public Menu {
    private:
        Construction* construct;
        int scroll;
        std::vector<int> productPlacement;
        bool firstTimeDraw;
 	public:
		ConstructionMenu();
		void Draw(int, int, TCODConsole*);
		MenuResult Update(int = -1, int = -1);
		static ConstructionMenu* constructionInfoMenu;
		static ConstructionMenu* ConstructionInfoMenu(Construction*);
		void Construct(Construction*);
		void ScrollDown();
		void ScrollUp();
		void ClearProductPlacement();
};

class StockManagerMenu : public Menu {
    private:
        int scroll;
		std::string filter;
 	public:
		StockManagerMenu();
		void Draw(int, int, TCODConsole*);
		MenuResult Update(int = -1, int = -1);
		static StockManagerMenu* stocksMenu;
		static StockManagerMenu* StocksMenu();
		void ScrollDown();
		void ScrollUp();
};

class SquadsMenu : public Menu {
private:
	std::string squadName;
	int squadMembers;
	int squadPriority;
	boost::weak_ptr<Squad> chosenSquad;
public:
	SquadsMenu();
	void Draw(int, int, TCODConsole*);
	MenuResult Update(int = -1, int = -1);
	static SquadsMenu* squadMenu;
	static SquadsMenu* SquadMenu();
};