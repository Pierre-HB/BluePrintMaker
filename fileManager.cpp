#include "data_base.h"
#include "blue_print.h"
#include "json11.hpp"
#include <iostream>
#include <fstream>
#include "nfd/nfd.h"

bool m_nfd(std::string& filename, const std::string& ext, const std::string& defaultPath, bool open) {
	nfdchar_t* outPath = NULL;
	const nfdchar_t* ext_char = ext == "" ? NULL : ext.c_str();
	const nfdchar_t* defaultPath_char = defaultPath == "" ? NULL : defaultPath.c_str();
	nfdresult_t result;
	if (open)
		result = NFD_OpenDialog(ext_char, defaultPath_char, &outPath);
	else
		result = NFD_SaveDialog(ext_char, defaultPath_char, &outPath);

	if (result == NFD_OKAY) {
		filename = outPath;
		free(outPath);
		return true;
	}
	return false;
}

bool FileDialogSave(std::string& filename, const std::string& ext = "", const std::string& defaultPath = "") {
	return m_nfd(filename, ext, defaultPath, false);
}

bool FileDialogOpen(std::string& filename, const std::string& ext = "", const std::string& defaultPath = "") {
	return m_nfd(filename, ext, defaultPath, true);
}


static std::string readFile(const std::string& filename) {
	std::ifstream file(filename);
	std::string line;
	std::string content;
	if (file.is_open()) {
		while (std::getline(file, line))
			content += line;
	}
	else {
		std::cout << "[ERROR] Database not found" << std::endl;
		return "";
	}
	file.close();
	return content;
}

static void writeFile(const std::string& filename, const std::string& content) {
	std::ofstream file(filename);
	file << content;
	file.close();
}

std::string type2string(json11::Json::Type type) {
	switch (type)
	{
	case json11::Json::NUL:
		return "null";
	case json11::Json::NUMBER:
		return "number";
	case json11::Json::BOOL:
		return "bool";
	case json11::Json::STRING:
		return "string";
	case json11::Json::ARRAY:
		return "array";
	case json11::Json::OBJECT:
		return "object";
	}
}

static bool checkJsonType(const json11::Json& json, json11::Json::Type type, const std::string& warning = "[ERROR]") {

	if (json.type() != type) {
		std::cout << warning << " Wrong type for '" << json.dump() << "' (" << type2string(json.type())  << ") should be " << type2string(type) << "." << std::endl;
		return false;
	}
	return true;
}

static bool checkJsonTypeAtKey(const json11::Json::object& json, const std::string& key, json11::Json::Type type, const std::string& warning="[ERROR]") {
	if (json.count(key) == 0) {
		std::cout << warning << " missing key '" << key << "' in " << json11::Json(json).dump() << std::endl;
		return false;
	}
	return checkJsonType(json.at(key), type, warning + " For key '"+key+"'");
}

static json11::Json file2json(const std::string& filename) {
	std::string content = readFile(filename);

	//std::cout << "File content : \n" << content << std::endl;
	std::string error;
	json11::Json json = json11::Json::parse(content, error);

	if (!checkJsonType(json, json11::Json::Type::OBJECT)) 
		std::cout << "[ERROR] in Database" << std::endl << error << std::endl;
	
	return json;
}

DataBase::DataBase(const std::string& filename) : filename(filename), nbSpecialMachine(0) {

	json11::Json json = file2json(filename);

	spreadsheetFilename = readString(json, "spreadsheet", "DataBase", "logo2.png");
	iconeWidth = readInt(json, "iconeWidth", "DataBase", 64);
	iconeHeight = readInt(json, "iconeHeight", "DataBase", 64);

	if(checkJsonTypeAtKey(json.object_items(), "items", json11::Json::Type::ARRAY))
		for (const auto& va : json.object_items().at("items").array_items())
			if(checkJsonType(va, json11::Json::OBJECT, "[ERROR] in 'items'"))
				items.push_back(Item(va));
	std::map<std::string, int> itemIdMap = createIdMap(items);

	if(checkJsonTypeAtKey(json.object_items(), "modifiers", json11::Json::Type::ARRAY))
		for (const auto& va : json.object_items().at("modifiers").array_items())
			if (checkJsonType(va, json11::Json::OBJECT, "[ERROR] in 'modifiers'"))
				modifiers.push_back(Modifier(va, itemIdMap));
	std::map<std::string, int> modifierIdMap = createIdMap(modifiers);

	if(checkJsonTypeAtKey(json.object_items(), "modifierCategories", json11::Json::Type::ARRAY))
		for (const auto& va : json.object_items().at("modifierCategories").array_items())
			if (checkJsonType(va, json11::Json::OBJECT, "[ERROR] in 'modifierCategories'"))
				modifierCategories.push_back(ModiferCategory(va, modifierIdMap));
	std::map<std::string, int> modifierCategoryIdMap = createIdMap(modifierCategories);

	if(checkJsonTypeAtKey(json.object_items(), "recipes", json11::Json::Type::ARRAY))
		for (const auto& va : json.object_items().at("recipes").array_items())
			if (checkJsonType(va, json11::Json::OBJECT, "[ERROR] in 'recipes'"))
				recipes.push_back(Recipe(va, itemIdMap, modifierCategoryIdMap));
	std::map<std::string, int> recipeIdMap = createIdMap(recipes);

	if(checkJsonTypeAtKey(json.object_items(), "machines", json11::Json::Type::ARRAY))
		for (const auto& va : json.object_items().at("machines").array_items())
			if (checkJsonType(va, json11::Json::OBJECT, "[ERROR] in 'machines'"))
				machines.push_back(Machine(va, recipeIdMap));

	addSpecialMachines();
	loadPlaceHolders();
}

BluePrint* BluePrint::CreateBluePrint(DataBase* dataBase) {
	std::string filename;
	if (!FileDialogOpen(filename, "bp"))
		return new BluePrint();

	json11::Json json = file2json(filename);

	bool corrupted = false;
	if (!checkJsonType(json, json11::Json::Type::OBJECT))
		corrupted = true;
	if (!checkJsonTypeAtKey(json.object_items(), "name", json11::Json::Type::STRING))
		corrupted = true;
	if (!checkJsonTypeAtKey(json.object_items(), "dataBaseFile", json11::Json::Type::STRING))
		corrupted = true;

	ImNodes::ClearEvent();
	if (corrupted)
	{
		std::cout << "[ERROR] Corrupted file for blueprint" << std::endl;
		return new BluePrint();
	}
	return new BluePrint(dataBase, filename, json);
}

void BluePrint::saveBluePrint() const {
	if (filename == "") 		
		return saveUnderBluePrint();

	writeFile(filename, ToJson().dump());
}

static bool endsWith(const std::string& fullString,
	const std::string& ending)
{
	// Check if the ending string is longer than the full
	// string
	if (ending.size() > fullString.size())
		return false;

	// Compare the ending of the full string with the target
	// ending
	return fullString.compare(fullString.size()
		- ending.size(),
		ending.size(), ending)
		== 0;
}

void BluePrint::saveUnderBluePrint() const {
	std::string newFilename;
	if (!FileDialogSave(newFilename, "bp"))
		return;//no filename and aborted file search
	if(!endsWith(newFilename, ".bp"))
		newFilename += ".bp";

	writeFile(newFilename, ToJson().dump());
}