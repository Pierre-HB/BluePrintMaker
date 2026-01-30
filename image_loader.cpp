#include "data_base.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <GLFW/glfw3.h>
#include <codecvt>

static std::string ImWchar2String(const ImWchar& c) {
	static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> convert;
	return convert.to_bytes(wchar_t(c));
	//return convert.to_bytes({ wchar_t(c), wchar_t(0) });
}

DataBase::~DataBase() {

}

void DataBase::CreateIcones(ImFont* font) {

	int maxIconeId = 0;
	for (Item& item : items)
		if (maxIconeId < item.iconeId)
			maxIconeId = item.iconeId;

	for (Machine& machine : machines)
		if (maxIconeId < machine.iconeId)
			maxIconeId = machine.iconeId;

	for (Recipe& recipe : recipes)
		if (maxIconeId < recipe.iconeId)
			maxIconeId = recipe.iconeId;

	for (Modifier& modifier : modifiers)
		if (maxIconeId < modifier.iconeId)
			maxIconeId = modifier.iconeId;

	maxIconeId += 1;
	int w;
	int h;
	int comp;

	unsigned char* image = stbi_load(spreadsheetFilename.c_str(), &w, &h, &comp, STBI_rgb_alpha);

	ImVector<ImWchar> glyphChars = ImNodes::AddFontGlyphs(font, image, w, iconeWidth, iconeHeight, maxIconeId);

	for (int i = 0; i < glyphChars.size(); i++)
		iconeStrings.push_back(ImWchar2String(glyphChars[i]));

	for (int i = 0; i < items.size(); i++) {
		items[i].iconeString = iconeStrings[items[i].iconeId];
	}

	for (int i = 0; i < machines.size(); i++) {
		machines[i].iconeString = iconeStrings[machines[i].iconeId];
	}

	for (int i = 0; i < recipes.size(); i++) {
		recipes[i].iconeString = iconeStrings[recipes[i].iconeId];
	}

	for (int i = 0; i < modifiers.size(); i++) {
		modifiers[i].iconeString = iconeStrings[modifiers[i].iconeId];
	}

	precomputeComboText();
}
