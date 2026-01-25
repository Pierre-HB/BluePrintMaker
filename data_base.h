#pragma once

#include "imnodes.h"
#include "json11.hpp"

const ImVec2 iconeSize = ImVec2(32, 32); //size of one icone in the spreadsheet

class DataBase {
public:
	ImTextureID textureId;
	ImVec2 textureSize;

public:
	DataBase();
	DataBase(const json11::Json& json);
	~DataBase();

	const char* getRessourceName(int ressourceId) const;
};

void drawDataBaseIcone(int ressourceId, const DataBase* data_base, ImVec2 size=ImVec2(16, 16));