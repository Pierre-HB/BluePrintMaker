#pragma once

#include "imnodes.h"
#include "json11.hpp"
#include <string>
#include "rat.hpp"

enum MACHINE_TYPE {
	MACHINE_REGULAR,
	MACHINE_INPUT,
	MACHINE_OUTPUT,
	MACHINE_SPLITTER,
	MACHINE_MERGER,
	MACHINE_BLACKBOX,
};

MACHINE_TYPE int2MachineType(int i);

struct Item {//1
	std::string name;
	int iconeId;
	std::string iconeString;

	Item(const json11::Json& json);
	Item();
};

struct Consumable {//2
	int itemId;
	Rat inputConsumtion;
	Rat outputConsumtion;

	Consumable(const json11::Json& json, const std::map<std::string, int>& itemIdMap);
};

struct Modifier {//3
	std::string name;
	int iconeId;
	std::string iconeString;
	Rat speedModifier = 1.0f;
	Rat outputModifier = 1.0f;
	Rat idlePower = 0.0f;
	Rat workingPower = 0.0f;
	Rat powerModifier = 1.0f;
	std::vector<Consumable> consumables; //consumables ar items

	Modifier(const json11::Json& json, const std::map<std::string, int>& itemIdMap);
	Modifier();
};

//MUST BE initialized AFTER modifiers
struct ModiferCategory {//4
	std::string name;
	std::vector<int> modifiersId;
	//no need for icone, the icone of the selected modifier will be used

	ModiferCategory(const json11::Json& json, const std::map<std::string, int>& modifierIdMap);
	ModiferCategory();
};

//MUST BE initialized AFTER items and modifiers
struct Recipe {//5
	std::string name;
	int iconeId;
	std::string iconeString;
	std::vector<std::pair<int, Rat>> inputsId;
	std::vector<std::pair<int, Rat>> outputsId;
	Rat time;
	std::vector<int> modifierCategoriesId;
	std::vector<std::vector<std::string>> modifierNames; // precomputed modifier icones for ImGui combo

	Recipe(const json11::Json& json, const std::map<std::string, int>& itemIdMap, const std::map<std::string, int>& modifierCategoryIdMap);
	Recipe();
};

//MUST BE initialized AFTER recipies
struct Machine {//6
	std::string name;
	int iconeId;
	std::string iconeString;
	std::vector<int> recipiesId;
	std::vector<std::string> recipeNames; //precomputed for ImGui display
	MACHINE_TYPE type;

	Machine(const json11::Json& json, const std::map<std::string, int>& recipyIdMap);
	Machine();
	Machine(std::string name, int iconeId, std::vector<int> recipiesId, MACHINE_TYPE type);
};

template<typename T>
std::map<std::string, int> createIdMap(std::vector<T> objects) {
	std::map<std::string, int> idMap;
	for (int i = 0; i < objects.size(); i++) {
		if (idMap.count(objects[i].name) == 0)
			idMap[objects[i].name] = i;
	}
	return idMap;
}

//const ImVec2 iconeSize = ImVec2(32, 32); //size of one icone in the spreadsheet

class DataBase {

	std::vector<Item> items;
	std::vector<Modifier> modifiers;
	std::vector<ModiferCategory> modifierCategories;
	std::vector<Recipe> recipes;
	std::vector<Machine> machines;

	int nbSpecialMachine;

	void addSpecialMachines(); //create MAM, Merger, Splitter, Input and Output machines
	void loadPlaceHolders(); //create empty item, machine, etc... to return when asking for out of bound objects
	
	std::string filename;
	std::string spreadsheetFilename;
	int iconeWidth;
	int iconeHeight;
	ImVector<std::string> iconeStrings;	

	void precomputeComboText();

public:
	DataBase();
	DataBase(const std::string& filename);
	~DataBase();

	void CreateIcones(ImFont* font);

	const Item& getItem(int itemId) const;
	const Machine& getMachine(int machineId) const;
	const Modifier& getModifier(int modifierId) const;
	const ModiferCategory& getModifierCategory(int modifierCategoryId) const;
	const Recipe& getRecipe(int recipeId) const;

	int getNbMachine() const;
	int getNbSpecialMachine() const;
	int getNbRecipe() const;
	int GetUnkownItemId() const;
	int GetIncorrectItemId() const;
	std::string getFileName() const;
};

inline const Item& DataBase::getItem(int itemId) const {
	if (itemId >= items.size())
		itemId = items.size() - 1;
	return items[itemId];
}

inline const Machine& DataBase::getMachine(int machineId) const {
	if (machineId >= machines.size())
		machineId = machines.size() - 1;
	return machines[machineId];
}

inline int DataBase::getNbMachine() const {
	return machines.size()-nbSpecialMachine;
}
inline int DataBase::getNbSpecialMachine() const {
	return nbSpecialMachine;
}
inline int DataBase::getNbRecipe() const {
	return recipes.size();
}
inline int DataBase::GetUnkownItemId() const {
	return items.size() - 2;
}
inline int DataBase::GetIncorrectItemId() const {
	return items.size() - 1;
}

inline std::string DataBase::getFileName() const {
	return filename;
}

inline const Modifier& DataBase::getModifier(int modifierId) const {
	if (modifierId >= modifiers.size())
		modifierId = modifiers.size() - 1;
	return modifiers[modifierId];
}

inline const ModiferCategory& DataBase::getModifierCategory(int modifierCategoryId) const {
	if (modifierCategoryId >= modifierCategories.size())
		modifierCategoryId = modifierCategories.size() - 1;
	return modifierCategories[modifierCategoryId];
}

inline const Recipe& DataBase::getRecipe(int recipeId) const {
	if (recipeId >= recipes.size())
		recipeId = recipes.size() - 1;
	return recipes[recipeId];
}

inline bool isRat(const json11::Json& json) {
	return json.is_number() || json.is_string();
}

int readInt(const json11::Json& json, const std::string& key, const std::string& struct_name, int default_value = -1);

float readFloat(const json11::Json& json, const std::string& key, const std::string& struct_name, float default_value = 1.0f);

Rat readRat(const json11::Json& json);

Rat readRat(const json11::Json& json, const std::string& key, const std::string& struct_name, Rat default_value = 1);

std::string readString(const json11::Json& json, const std::string& key, const std::string& struct_name, std::string default_value = "");

std::vector<int> readMap(const json11::Json& json, const std::map<std::string, int>& idMap, const std::string& key, const std::string& struct_name, const std::string& mapName);

std::vector<std::pair<int, Rat>> readList(const json11::Json& json, const std::map<std::string, int>& idMap, const std::string& key, const std::string& struct_name, const std::string& mapName);

std::vector<Consumable> readConsumable(const json11::Json& json, const std::map<std::string, int>& itemIdMap);