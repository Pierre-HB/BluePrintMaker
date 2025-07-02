#pragma once
#include "imgui.h"
#include <vector>

//struct UserEventManager {
//	int nodeCreationQuerry; // type of node to create. CAN ONLY CREATE ONE NODE PER UPDATE
//
//	UserEventManager() : nodeCreationQuerry(-1) {
//
//	}
//
//	void CreateNodeCreationQuerry(int nodeType) {
//		nodeCreationQuerry = nodeType;
//	}
//};

class MenuElement {
private:
	const char* name;
	const char* shortcut;
	std::vector<MenuElement*> childs; //if any
	int type;
	bool toggleOn;

private:
	MenuElement(const char* name, std::vector<MenuElement*> childs, int type, const char* shortcut);

public:
	MenuElement(const char* name, int type, const char* shortcut = "");
	MenuElement(const char* name, std::vector<MenuElement*> childs);
	~MenuElement();

	void Draw();

	void Update(int& nodeCreateType); //TODO manage user shortcut here


};

class IOPanel {
private:
	bool show;
	ImVec2 origin;
	std::vector<MenuElement*> elements;

public:
	IOPanel();
	~IOPanel();
	
	void Draw() const;
	
	void Update(int& nodeCreateType); //TODO manage user shortcut here
};