#include "data_base.h"

DataBase::DataBase(const json11::Json& json) : textureId(0), textureSize(0, 0) {

}

const char* DataBase::getRessourceName(int ressourceId) const {
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