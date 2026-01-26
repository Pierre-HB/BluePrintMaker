#include "data_base.h"
#include <iostream>

//===================================================================

static int readInt(const json11::Json& json, const std::string& key, const std::string& struct_name, int default_value = -1) {
	if (json.object_items().count(key)) {
		if (json.object_items().at(key).is_number())
			return json.object_items().at(key).int_value();
		else {
			std::cout << "[WARNING] Wrong type for '" << key << "' (should be int) for " << struct_name << " : " << json.dump() << std::endl;
			return default_value;
		}
	}
	else {
		std::cout << "[WARNING] did not found '" << key << "' for " << struct_name << " : " << json.dump() << std::endl;
		return default_value;
	}
}

static float readFloat(const json11::Json& json, const std::string& key, const std::string& struct_name, float default_value = 1.0f) {
	if (json.object_items().count(key)) {
		if (json.object_items().at(key).is_number())
			return json.object_items().at(key).int_value();
		else {
			std::cout << "[WARNING] Wrong type for '" << key << "' (should be float) for " << struct_name << " : " << json.dump() << std::endl;
			return default_value;
		}
	}
	else {
		std::cout << "[WARNING] did not found '" << key << "' for " << struct_name << " : " << json.dump() << std::endl;
		return default_value;
	}
}

static std::string readString(const json11::Json& json, const std::string& key, const std::string& struct_name, std::string default_value = "") {
	if (json.object_items().count(key)) {
		if (json.object_items().at(key).is_string())
			return json.object_items().at(key).string_value();
		else {
			std::cout << "[WARNING] Wrong type for '" << key << "' (should be string) for " << struct_name << " : " << json.dump() << std::endl;
			return default_value;
		}
	}
	else {
		std::cout << "[WARNING] did not found '" << key << "' for " << struct_name << " : " << json.dump() << std::endl;
		return default_value;
	}
}

std::vector<int> readMap(const json11::Json& json, const std::map<std::string, int>& idMap, const std::string& key, const std::string& struct_name, const std::string& mapName) {

	if (json.object_items().count(key)) {
		if (json.object_items().at(key).is_array()) {
			std::vector<int> idList = std::vector<int>();
			for (const auto& va : json.object_items().at(key).array_items()) {

				if (va.is_string()) {
					if (idMap.count(va.string_value())) {
						idList.push_back(idMap.at(va.string_value()));
					}
					else {
						std::cout << "[WARNING] did not found '" << va.string_value() << "' in " << mapName << " list for " << struct_name << " : " << json.dump() << std::endl;
					}
				}
				else {
					std::cout << "[WARNING] Wrong type for '" << va.dump() << "' (should be string) in array '" << key << "' for " << struct_name << " : " << json.dump() << std::endl;
				}
			}
			return idList;
		}
		else {
			std::cout << "[WARNING] Wrong type for '" << key << "' (should be array) for " << struct_name << " : " << json.dump() << std::endl;
			return std::vector<int>();
		}
	}
	else {
		std::cout << "[WARNING] did not found '" << key << "' for " << struct_name << " : " << json.dump() << std::endl;
		return std::vector<int>();
	}
}

std::vector<std::pair<int, int>> readList(const json11::Json& json, const std::map<std::string, int>& idMap, const std::string& key, const std::string& struct_name, const std::string& mapName) {

	if (json.object_items().count(key)) {
		if (json.object_items().at(key).is_object()) {
			std::vector<std::pair<int, int>> idQuantityList = std::vector<std::pair<int, int>>();
			for (const auto& va : json.object_items().at(key).object_items()) {

				if (va.second.is_number()) {
					if (idMap.count(va.first)) {
						idQuantityList.push_back(std::pair<int, int>(idMap.at(va.first), va.second.int_value()));
					}
					else {
						std::cout << "[WARNING] did not found '" << va.first << "' in " << mapName << " list for " << struct_name << " : " << json.dump() << std::endl;
					}
				}
				else {
					std::cout << "[WARNING] Wrong type for '" << va.second.dump() << "' (should be int) at key : " << va.first << " in map '" << key << "' for " << struct_name << " : " << json.dump() << std::endl;
				}
			}
			return idQuantityList;
		}
		else {
			std::cout << "[WARNING] Wrong type for '" << key << "' (should be map) for " << struct_name << " : " << json.dump() << std::endl;
			return std::vector<std::pair<int, int>>();
		}
	}
	else {
		std::cout << "[WARNING] did not found '" << key << "' for " << struct_name << " : " << json.dump() << std::endl;
		return std::vector<std::pair<int, int>>();
	}
}


std::vector<Consumable> readConsumable(const json11::Json& json, const std::map<std::string, int>& itemIdMap) {
	const std::string& key = "consumables";
	const std::string& struct_name = "Modifier";
	const std::string& mapName = "items";

	if (json.object_items().count(key)) {
		if (json.object_items().at(key).is_array()) {
			std::vector<Consumable> consumableList = std::vector<Consumable>();
			for (const auto& va : json.object_items().at(key).array_items()) {

				if (va.is_object()) {
					Consumable consumable = Consumable(va, itemIdMap);
					if (consumable.itemId != -1)
						consumableList.push_back(consumable);
					else
						std::cout << "[WARNING] wrong consumable '" << va.dump() << "' in " << struct_name << " : " << json.dump() << std::endl;

				}
				else {
					std::cout << "[WARNING] Wrong type for '" << va.dump() << "' (should be map) in array '" << key << "' for " << struct_name << " : " << json.dump() << std::endl;
				}
			}
			return consumableList;
		}
		else {
			std::cout << "[WARNING] Wrong type for '" << key << "' (should be array) for " << struct_name << " : " << json.dump() << std::endl;
			return std::vector<Consumable>();
		}
	}
	else {
		std::cout << "[WARNING] did not found '" << key << "' for " << struct_name << " : " << json.dump() << std::endl;
		return std::vector<Consumable>();
	}
}


//===================================================================


DataBase::DataBase(const json11::Json& json) : textureId(0), textureSize(0, 0) {
	std::string filename = readString(json, "spreadsheet", "DataBase", "logo2.png");

	loadIcones(filename);

	for (const auto& va : json.object_items().at("items").array_items()) {
		items.push_back(Item(va));
	}
	std::map<std::string, int> itemIdMap = createIdMap(items);

	for (const auto& va : json.object_items().at("items").array_items()) {
		machines.push_back(Machine(va, itemIdMap));
	}

	for (const auto& va : json.object_items().at("items").array_items()) {
		modifiers.push_back(Modifier(va, itemIdMap));
	}
	std::map<std::string, int> modifierIdMap = createIdMap(modifiers);

	for (const auto& va : json.object_items().at("items").array_items()) {
		modifierCategories.push_back(ModiferCategory(va, modifierIdMap));
	}
	std::map<std::string, int> modifierCategoryIdMap = createIdMap(modifierCategories);

	for (const auto& va : json.object_items().at("items").array_items()) {
		recipes.push_back(Recipe(va, itemIdMap, modifierCategoryIdMap));
	}
}

DataBase::DataBase() : textureId(0), textureSize(0, 0) {
	loadIcones("logo2.png");
}

std::string DataBase::getRessourceName(int ressourceId) const {
	return "test";
}

void drawDataBaseIcone(int ressourceId, const DataBase* dataBase, ImVec2 size) {

	if (dataBase->textureSize.x == 0)
		return;

	int nb_icones_line = int(dataBase->textureSize.x) / int(iconeSize.x); //supposed to be power of 2
	int nb_icones_column =  int(dataBase->textureSize.y)/ int(iconeSize.y); //supposed to be power of 2

	int line = ressourceId / nb_icones_line;
	int column = ressourceId % nb_icones_line;

	ImVec2 iconeUV = ImVec2(1.0f / float(nb_icones_line), 1.0f / float(nb_icones_column));
	ImVec2 uv0 = ImVec2(iconeUV.x*line, iconeUV.y*column);
	ImVec2 uv1 = ImVec2(uv0.x + iconeUV.x, uv0.y + iconeUV.y);

	ImGui::Image(dataBase->textureId, size, uv0, uv1);
}



Item::Item(const json11::Json& json) {
	name = readString(json, "name", "Item", "Item");
	iconeId = readInt(json, "iconeId", "Item");
}

Machine::Machine(const json11::Json& json, const std::map<std::string, int>& recipyIdMap) {
	name = readString(json, "name", "Machine", "Machine");
	iconeId = readInt(json, "iconeId", "Machine");

	recipiesId = readMap(json, recipyIdMap, "recipies", "Machine", "recipies");
}

Consumable::Consumable(const json11::Json& json, const std::map<std::string, int>& itemIdMap) {
	std::string itemName = readString(json, "item", "Consumable");
	itemId = -1;
	if (itemName != "") {
		if (itemIdMap.count(itemName)) {
			itemId = itemIdMap.at(itemName);
		}
		else {
			std::cout << "[WARNING] item '" << itemName << "' does not exist if Consumable : " << json.dump() << std::endl;
		}
	}

	inputConsumtion = readFloat(json, "inputConsumtion", "Consumable", 0.0f);
	outputConsumtion = readFloat(json, "outputConsumtion", "Consumable", 0.0f);
	
}


Modifier::Modifier(const json11::Json& json, const std::map<std::string, int>& itemIdMap) {
	/*std::string name;
	int iconeId;
	float speedModifiers = 1.0f;
	float outputModifiers = 1.0f;
	float idlePower = 0.0f;
	float workingPower = 0.0f;
	std::vector<int> consumablesId; //consumables ar items
	std::vector<int> consumablesInputCoef; //consumable consume per inputs
	std::vector<int> consumablesOutputCoef; //consumable consume per outputs*/

	name = readString(json, "name", "Modifier", "Modifier");
	iconeId = readInt(json, "iconeId", "Modifier");
	speedModifiers = readFloat(json, "speedModifiers", "Modifier", 1.0);
	outputModifiers = readFloat(json, "outputModifiers", "Modifier", 1.0);
	idlePower = readFloat(json, "idlePower", "Modifier", 0.0f);
	workingPower = readFloat(json, "workingPower", "Modifier", 0.0f);

	consumables = readConsumable(json, itemIdMap);
}

ModiferCategory::ModiferCategory(const json11::Json& json, const std::map<std::string, int>& modifierIdMap) {
	/*std::string name;
	std::vector<int> modifiersId;*/
	name = readString(json, "name", "ModifierCategory", "ModifierCategory");

	modifiersId = readMap(json, modifierIdMap, "modifiers", "ModifierCategory", "modifiers");
}

Recipe::Recipe(const json11::Json& json, const std::map<std::string, int>& itemIdMap, const std::map<std::string, int>& modifierCategoryIdMap) {
	/*std::string name;
	int iconeId;
	std::vector<std::pair<int, int>> inputsId;
	std::vector<std::pair<int, int>> outputsId;
	float time;
	std::vector<int> modifierCategoriesId;*/

	name = readString(json, "name", "Recipe", "Recipe");
	iconeId = readInt(json, "iconeId", "Recipe");
	time = readFloat(json, "time", "Recipe", 0.0f);
	modifierCategoriesId = readMap(json, modifierCategoryIdMap, "modifierCategories", "Recipe", "modifierCategories");

	inputsId = readList(json, itemIdMap, "inputs", "Recipe", "items");
	outputsId = readList(json, itemIdMap, "outputss", "Recipe", "items");
}