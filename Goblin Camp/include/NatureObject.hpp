#ifndef NATURE_OBJECT_HEADER
#define NATURE_OBJECT_HEADER

#include <string>
#include <vector>
#include <ticpp.h>

#include "GameEntity.hpp"
#include "Coordinate.hpp"
#include "Item.hpp"

typedef int NatureObjectType;

class NatureObjectPreset {
    public:
        NatureObjectPreset();
        std::string name;
        int graphic;
        TCODColor color;
        std::list<ItemType> components;
        int rarity;
        int cluster;
        int condition;
        bool tree, harvestable, walkable;
};

class NatureObject : public GameEntity
{
    friend class Game;
    private:
        NatureObject(Coordinate, NatureObjectType);
        NatureObjectType type;
        int graphic;
        TCODColor color;
        bool marked;
        int condition;
        bool tree, harvestable;
    public:
        static std::vector<NatureObjectPreset> Presets;
        static void LoadPresets(ticpp::Document);

        int Type();

        void Draw(Coordinate);
        void Update();
        virtual void CancelJob(int=0);
        void Mark();
        bool Marked();
        int Fell();
        int Harvest();
        bool Tree();
        bool Harvestable();
};

#endif
