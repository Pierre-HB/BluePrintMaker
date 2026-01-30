#include "data_base.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <GLFW/glfw3.h>
#include <codecvt>

static std::string ImWchar2String(const ImWchar& c) {
	static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> convert;
	return convert.to_bytes({ wchar_t(c), wchar_t(0) });
}

DataBase::~DataBase() {

}

void DataBase::CreateIcones(ImFont* font) {

	int maxItemId = 0;
	for (Item& item : items)
		if (maxItemId < item.iconeId)
			maxItemId = item.iconeId;

	maxItemId += 1;
	int w;
	int h;
	int comp;

	unsigned char* image = stbi_load(spreadsheetFilename.c_str(), &w, &h, &comp, STBI_rgb_alpha);

	ImVector<ImWchar> glyphChars = ImNodes::AddFontGlyphs(font, image, w, iconeWidth, iconeHeight, maxItemId);

	for (int i = 0; i < glyphChars.size(); i++)
		iconeStrings.push_back(ImWchar2String(glyphChars[i]));
}
