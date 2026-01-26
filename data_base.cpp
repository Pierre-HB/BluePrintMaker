#include "data_base.h"
#include <iostream>
#include <fstream>

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


DataBase::DataBase(const std::string& filename) : textureId(0), textureSize(0, 0) {
	std::ifstream file(filename);
	std::string line;
	std::string content;
	if (file.is_open()) {
		while (std::getline(file, line))
			content += line;
	}
	else {
		std::cout << "[ERROR] Database not found" << std::endl;
		return;
	}
	file.close();
	std::cout << "File content : \n" << content << std::endl;
	//json11::Json json = json11::Json::parse("", "err");
	json11::Json json = json11::Json::parse(content, std::string("ERROR"));

	if (!json.is_object()) {
		std::cout << "[ERROR] database is not an object" << std::endl;
		return;
	}

	std::string fileSpreadSheet = readString(json, "spreadsheet", "DataBase", "logo2.png");

	loadIcones(fileSpreadSheet);

	if(json.object_items().count("items"))
	{
		if(json.object_items().at("items").is_array())
		{
			for (const auto& va : json.object_items().at("items").array_items()) {
				if (va.is_object())
					items.push_back(Item(va));
				else
					std::cout << "[ERROR] Wrong type for 'item' (should be object) in items : " << va.dump() << std::endl;

				
			}
		}
		else
			std::cout << "[ERROR] Wrong type for 'items' (should be array) in database : " << json.object_items().at("items").dump() << std::endl;
	}
	else {
		std::cout << "[ERROR] did not found 'items' in database" << std::endl;
	}
	std::map<std::string, int> itemIdMap = createIdMap(items);

	if (json.object_items().count("modifiers"))
	{
		if (json.object_items().at("modifiers").is_array())
		{
			for (const auto& va : json.object_items().at("modifiers").array_items()) {
				if(va.is_object())
					modifiers.push_back(Modifier(va, itemIdMap));
				else
					std::cout << "[ERROR] Wrong type for 'modifier' (should be object) in modifiers : " << va.dump() << std::endl;
			}
		}
		else
			std::cout << "[ERROR] Wrong type for 'modifiers' (should be array) in database : " << json.object_items().at("modifiers").dump() << std::endl;
	}
	else {
		std::cout << "[ERROR] did not found 'modifiers' in database" << std::endl;
	}
	std::map<std::string, int> modifierIdMap = createIdMap(modifiers);

	if (json.object_items().count("modifierCategories"))
	{
		if (json.object_items().at("modifierCategories").is_array())
		{
			for (const auto& va : json.object_items().at("modifierCategories").array_items()) {
				if (va.is_object())
					modifierCategories.push_back(ModiferCategory(va, modifierIdMap));
				else
					std::cout << "[ERROR] Wrong type for 'modifierCategory' (should be object) in modifierCategorys : " << va.dump() << std::endl;
				
			}
		}
		else
			std::cout << "[ERROR] Wrong type for 'modifierCategories' (should be array) in database : " << json.object_items().at("modifierCategories").dump() << std::endl;
	}
	else {
		std::cout << "[ERROR] did not found 'modifierCategories' in database" << std::endl;
	}
	std::map<std::string, int> modifierCategoryIdMap = createIdMap(modifierCategories);

	if (json.object_items().count("recipes"))
	{
		if (json.object_items().at("recipes").is_array())
		{
			for (const auto& va : json.object_items().at("recipes").array_items()) {
				if (va.is_object())
					recipes.push_back(Recipe(va, itemIdMap, modifierCategoryIdMap));
				else
					std::cout << "[ERROR] Wrong type for 'recipe' (should be object) in recipes : " << va.dump() << std::endl;
			}
		}
		else
			std::cout << "[ERROR] Wrong type for 'recipes' (should be array) in database : " << json.object_items().at("recipes").dump() << std::endl;
	}
	else {
		std::cout << "[ERROR] did not found 'recipes' in database" << std::endl;
	}
	std::map<std::string, int> recipeIdMap = createIdMap(recipes);

	if (json.object_items().count("machines"))
	{
		if (json.object_items().at("machines").is_array())
		{
			for (const auto& va : json.object_items().at("machines").array_items()) {
				if (va.is_object())
					machines.push_back(Machine(va, recipeIdMap));
				else
					std::cout << "[ERROR] Wrong type for 'machine' (should be object) in machines : " << va.dump() << std::endl;
			}
		}
		else
			std::cout << "[ERROR] Wrong type for 'machines' (should be array) in database : " << json.object_items().at("machines").dump() << std::endl;
	}
	else {
		std::cout << "[ERROR] did not found 'machines' in database" << std::endl;
	}

	loadPlaceHolders();
}

void DataBase::loadPlaceHolders() {
	items.push_back(Item());
	modifiers.push_back(Modifier());
	modifierCategories.push_back(ModiferCategory());
	recipes.push_back(Recipe());
	machines.push_back(Machine());
}

DataBase::DataBase() : textureId(0), textureSize(0, 0) {
	loadIcones("logo2.png");
	loadPlaceHolders();
}

void drawDataBaseIcone(int ressourceId, const DataBase* dataBase, ImVec2 size) {

	if (dataBase->textureSize.x == 0)
		return;

	int nb_icones_line = int(dataBase->textureSize.x) / int(iconeSize.x); //supposed to be power of 2
	int nb_icones_column =  int(dataBase->textureSize.y)/ int(iconeSize.y); //supposed to be power of 2

	int line = ressourceId / nb_icones_line;
	int column = ressourceId % nb_icones_line;

	ImVec2 iconeUV = ImVec2(1.0f / float(nb_icones_line), 1.0f / float(nb_icones_column));
	ImVec2 uv0 = ImVec2(iconeUV.x*column, iconeUV.y* line);
	ImVec2 uv1 = ImVec2(uv0.x + iconeUV.x, uv0.y + iconeUV.y);

	ImGui::Image(dataBase->textureId, size, uv0, uv1);
}



Item::Item(const json11::Json& json) {
	name = readString(json, "name", "Item", "Item");
	iconeId = readInt(json, "iconeId", "Item");
}

Item::Item() {
	name = "???";
	iconeId = 0;
}

Machine::Machine(const json11::Json& json, const std::map<std::string, int>& recipyIdMap) {
	name = readString(json, "name", "Machine", "Machine");
	iconeId = readInt(json, "iconeId", "Machine");

	recipiesId = readMap(json, recipyIdMap, "recipes", "Machine", "recipes");
}

Machine::Machine() {
	name = "???";
	iconeId = 0;
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
	speedModifier = readFloat(json, "speedModifier", "Modifier", 1.0);
	outputModifier = readFloat(json, "outputModifier", "Modifier", 1.0);
	idlePower = readFloat(json, "idlePower", "Modifier", 0.0f);
	workingPower = readFloat(json, "workingPower", "Modifier", 0.0f);

	consumables = readConsumable(json, itemIdMap);
}

Modifier::Modifier() {
	name = "???";
	iconeId = 0;
	speedModifier = 1;
	outputModifier = 1;
	idlePower = 0;
	workingPower = 0;
}

ModiferCategory::ModiferCategory(const json11::Json& json, const std::map<std::string, int>& modifierIdMap) {
	/*std::string name;
	std::vector<int> modifiersId;*/
	name = readString(json, "name", "ModifierCategory", "ModifierCategory");

	modifiersId = readMap(json, modifierIdMap, "modifiers", "ModifierCategory", "modifiers");
}

ModiferCategory::ModiferCategory() {
	name = "???";
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
	outputsId = readList(json, itemIdMap, "outputs", "Recipe", "items");
}

Recipe::Recipe() {
	name = "???";
	iconeId = 0;
	time = 0.0f;
}