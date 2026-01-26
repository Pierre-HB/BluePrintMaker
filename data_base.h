#pragma once

#include "imnodes.h"
#include "json11.hpp"
#include <string>

struct Item {//1
	std::string name;
	int iconeId;

	Item(const json11::Json& json);
	Item();
};

struct Consumable {//2
	int itemId;
	float inputConsumtion;
	float outputConsumtion;

	Consumable(const json11::Json& json, const std::map<std::string, int>& itemIdMap);
};

struct Modifier {//3
	std::string name;
	int iconeId;
	float speedModifier = 1.0f;
	float outputModifier = 1.0f;
	float idlePower = 0.0f;
	float workingPower = 0.0f;
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
	std::vector<std::pair<int, int>> inputsId;
	std::vector<std::pair<int, int>> outputsId;
	float time;
	std::vector<int> modifierCategoriesId;

	Recipe(const json11::Json& json, const std::map<std::string, int>& itemIdMap, const std::map<std::string, int>& modifierCategoryIdMap);
	Recipe();
};

//MUST BE initialized AFTER recipies
struct Machine {//6
	std::string name;
	int iconeId;
	std::vector<int> recipiesId;

	Machine(const json11::Json& json, const std::map<std::string, int>& recipyIdMap);
	Machine();
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

const ImVec2 iconeSize = ImVec2(32, 32); //size of one icone in the spreadsheet

class DataBase {

	std::vector<Item> items;
	std::vector<Modifier> modifiers;
	std::vector<ModiferCategory> modifierCategories;
	std::vector<Recipe> recipes;
	std::vector<Machine> machines;

	void loadIcones(std::string filename);
	void loadPlaceHolders(); //create empty item, machine, etc... to return when asking for out of bound objects

public:
	ImTextureID textureId;
	ImVec2 textureSize;

public:
	DataBase();
	DataBase(const std::string& filename);
	~DataBase();

	const Item& getItem(int itemId) const;
	const Machine& getMachine(int machineId) const;
	const Modifier& getModifier(int modifierId) const;
	const ModiferCategory& getModifierCategory(int modifierCategoryId) const;
	const Recipe& getRecipe(int recipeId) const;
};

void drawDataBaseIcone(int ressourceId, const DataBase* data_base, ImVec2 size=ImVec2(16, 16));

inline const Item& DataBase::getItem(int itemId) const {
	if (itemId >= items.size())
		itemId = items.size()-1;
	return items[itemId];
}

inline const Machine& DataBase::getMachine(int machineId) const {
	if (machineId >= machines.size())
		machineId = machines.size() - 1;
	return machines[machineId];
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