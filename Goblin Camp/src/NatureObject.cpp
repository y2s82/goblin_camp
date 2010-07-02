#include "NatureObject.hpp"
#include "Game.hpp"
#include "Logger.hpp"
#include "StockManager.hpp"

NatureObjectPreset::NatureObjectPreset() :
    name("NATUREOBJECT PRESET"),
    graphic('?'),
    color(TCODColor::pink),
    components(std::list<ItemType>()),
    rarity(100),
    cluster(1),
    condition(1),
    tree(false),
    harvestable(false),
    walkable(false)
{}

std::vector<NatureObjectPreset> NatureObject::Presets = std::vector<NatureObjectPreset>();

NatureObject::NatureObject(Coordinate pos, NatureObjectType typeVal) : Entity(),
    type(typeVal),
    marked(false)
{
    Position(pos);

    name = Presets[type].name;
    graphic = Presets[type].graphic;
    color = Presets[type].color;
    condition = Presets[type].condition;
    tree = Presets[type].tree;
    harvestable = Presets[type].harvestable;
}

NatureObject::~NatureObject() {}

void NatureObject::Draw(Coordinate upleft, TCODConsole* console) {
	int screenx = _x - upleft.x();
	int screeny = _y - upleft.y();
	if (screenx >= 0 && screenx < console->getWidth() && screeny >= 0 && screeny < console->getHeight()) {
        console->putCharEx(screenx, screeny, graphic, color, marked ? TCODColor::white : TCODColor::black);
	}
}

void NatureObject::Update() {}

void NatureObject::LoadPresets(ticpp::Document doc) {
    std::string strVal;
    int intVal = 0;
    ticpp::Element* parent = doc.FirstChildElement();

    Logger::Inst()->output<<"Reading wildplants.xml\n";
    try {
        ticpp::Iterator<ticpp::Node> node;
        for (node = node.begin(parent); node != node.end(); ++node) {
            if (node->Value() == "plant") {
#ifdef DEBUG
				std::cout<<"Plant\n";
#endif
                Presets.push_back(NatureObjectPreset());
                ticpp::Iterator<ticpp::Node> child;
                for (child = child.begin(node->ToElement()); child != child.end(); ++child) {
#ifdef DEBUG
					std::cout<<"Children\n";
#endif
                    if (child->Value() == "name") {
                        Presets.back().name = child->ToElement()->GetText();
                    } else if (child->Value() == "graphic") {
                        child->ToElement()->GetText(&intVal);
                        Presets.back().graphic = intVal;
                    } else if (child->Value() == "components") {
#ifdef DEBUG
                        std::cout<<"Components\n";
#endif
                        ticpp::Iterator<ticpp::Node> comps;
                        for (comps = comps.begin(child->ToElement()); comps != comps.end(); ++comps) {
                                Presets.back().components.push_back(Item::StringToItemType(comps->ToElement()->GetText()));
                        }
                    } else if (child->Value() == "color") {
                        int r = -1,g = -1,b = -1;
                        ticpp::Iterator<ticpp::Node> c;
                        for (c = c.begin(child->ToElement()); c != c.end(); ++c) {
                            c->ToElement()->GetTextOrDefault(&intVal, 0);
                            if (r == -1) r = intVal;
                            else if (g == -1) g = intVal;
                            else b = intVal;
                        }
                        Presets.back().color = TCODColor(r,g,b);
                    } else if (child->Value() == "rarity") {
                        child->ToElement()->GetText(&intVal);
                        Presets.back().rarity = intVal;
                    } else if (child->Value() == "cluster") {
                        child->ToElement()->GetText(&intVal);
                        Presets.back().cluster = intVal;
                    } else if (child->Value() == "condition") {
                        child->ToElement()->GetText(&intVal);
                        Presets.back().condition = intVal;
                    } else if (child->Value() == "tree") {
                        Presets.back().tree = true;
                    } else if (child->Value() == "harvestable") {
                        Presets.back().harvestable = true;
                    } else if (child->Value() == "walkable") {
                        Presets.back().walkable = true;
                    }
                }
            }
        }
    } catch (ticpp::Exception& ex) {
        Logger::Inst()->output<<"Failed reading wildplants.xml!\n";
        Logger::Inst()->output<<ex.what()<<'\n';
        Game::Inst()->Exit();
    }

    Logger::Inst()->output<<"Finished reading wildplants.xml\nPlants: "<<Presets.size()<<'\n';
}

void NatureObject::Mark() { marked = true; }
bool NatureObject::Marked() { return marked; }

void NatureObject::CancelJob(int) { 
	marked = false; 
}

int NatureObject::Fell() { return --condition; }
int NatureObject::Harvest() { return --condition; }

int NatureObject::Type() { return type; }
bool NatureObject::Tree() { return tree; }
bool NatureObject::Harvestable() { return harvestable; }
