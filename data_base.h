#pragma once

#include "imnodes.h"
#include "json11.hpp"
#include <string>

struct Item {//1
	std::string name;
	int iconeId;

	Item(const json11::Json& json);
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
	float speedModifiers = 1.0f;
	float outputModifiers = 1.0f;
	float idlePower = 0.0f;
	float workingPower = 0.0f;
	std::vector<Consumable> consumables; //consumables ar items

	Modifier(const json11::Json& json, const std::map<std::string, int>& itemIdMap);
};

//MUST BE initialized AFTER modifiers
struct ModiferCategory {//4
	std::string name;
	std::vector<int> modifiersId;
	//no need for icone, the icone of the selected modifier will be used

	ModiferCategory(const json11::Json& json, const std::map<std::string, int>& modifierIdMap);
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
};

//MUST BE initialized AFTER recipies
struct Machine {//6
	std::string name;
	int iconeId;
	std::vector<int> recipiesId;

	Machine(const json11::Json& json, const std::map<std::string, int>& recipyIdMap);
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
	std::vector<Machine> machines;
	std::vector<Modifier> modifiers;
	std::vector<ModiferCategory> modifierCategories;
	std::vector<Recipe> recipes;

	void loadIcones(std::string filename);

public:
	ImTextureID textureId;
	ImVec2 textureSize;

public:
	DataBase();
	DataBase(const json11::Json& json);
	~DataBase();

	std::string getRessourceName(int ressourceId) const;

	const Item& getItem(int itemId) const;
	
	const Machine& getMachine(int machineId) const;
	const Modifier& getModifier(int modifierId) const;
	const ModiferCategory& getModifierCategory(int modifierCategoryId) const;
	const Recipe& getRecipe(int recipeId) const;
};

void drawDataBaseIcone(int ressourceId, const DataBase* data_base, ImVec2 size=ImVec2(16, 16));

inline const Item& DataBase::getItem(int itemId) const {
	return items[itemId];
}

inline const Machine& DataBase::getMachine(int machineId) const {
	return machines[machineId];
}

inline const Modifier& DataBase::getModifier(int modifierId) const {
	return modifiers[modifierId];
}

inline const ModiferCategory& DataBase::getModifierCategory(int modifierCategoryId) const {
	return modifierCategories[modifierCategoryId];
}

inline const Recipe& DataBase::getRecipe(int recipeId) const {
	return recipes[recipeId];
}