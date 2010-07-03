#pragma once

#include "Stockpile.hpp"

class FarmPlot : public Stockpile {
    friend class Game;

    private:
        FarmPlot(ConstructionType, int symbol, Coordinate);
        bool tilled;
        std::map<ItemType, bool> allowedSeeds;
		std::map<Coordinate, int> growth;

    public:
        void Update();
        virtual void Draw(Coordinate, TCODConsole*);
        virtual int Use();
        void AllowSeed(ItemType, bool);
        bool SeedAllowed(ItemType);
};
