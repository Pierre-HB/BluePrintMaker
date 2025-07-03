#include "io_panel.h"
#include <iostream>


static const std::vector<MenuElement*> createIOPanel() {
    //TODO actually read the json ioPanel
    std::vector<MenuElement*> elements = std::vector<MenuElement*>();
    std::vector<MenuElement*> tmp = std::vector<MenuElement*>();
    tmp.push_back(new MenuElement("smelter", 4, "CTRL 1")); //create emtpy machine n°1
    tmp.push_back(new MenuElement("fabricator", 4, "CTRL 2")); //create emtpy machine n°2
    tmp.push_back(new MenuElement("science", 4, "CTRL 3")); //create emtpy machine n°3

    elements.push_back(new MenuElement("machines", tmp));
    elements.push_back(new MenuElement("splitter", 0)); // create splitter, cpp made node
    elements.push_back(new MenuElement("merger", 1));
    elements.push_back(new MenuElement("inputs", 2));
    elements.push_back(new MenuElement("outputs", 3));

    return elements;
}

/*
Internal, private all purpose constructor
*/
MenuElement::MenuElement(const char* name, std::vector<MenuElement*>childs, int type, const char* shortcut) : name(name), childs(childs), shortcut(shortcut), type(type), toggleOn(false) {

}

/*
Constructor for leaf elements in the right click menu
*/
MenuElement::MenuElement(const char* name, int type, const char* shortcut) : MenuElement(name, std::vector<MenuElement*>(0), type, shortcut) {
}

/*
Constructor for node elements in the right click menu
*/
MenuElement::MenuElement(const char* name, std::vector<MenuElement*> childs) : MenuElement(name, childs, -1, "") {

}

MenuElement::~MenuElement() {
    for (MenuElement* element : childs)
        delete element;
}

void MenuElement::Draw() {
    if (childs.size() == 0) {
        if (ImGui::MenuItem(name, shortcut))
            toggleOn = true;
    //ImGui::SetItemTooltip("I am a tooltip over a popup");
    }
    else {
        if (ImGui::BeginMenu(name))
        {
            //ImGui::SetItemTooltip("I am a tooltip over a popup");
            for (MenuElement* element : childs)
                element->Draw();
            ImGui::EndMenu();
        }
    }
}

void MenuElement::Update(int& nodeCreateType) {
    for (MenuElement* child : childs)
        child->Update(nodeCreateType);

    if (toggleOn) {
        toggleOn = false;
        nodeCreateType = type;
    }
    //TODO manage user shortcut here
}

IOPanel::IOPanel() : show(false), elements(createIOPanel()) {

}

IOPanel::~IOPanel() {
    for (MenuElement* element : elements)
        delete element;
}

void IOPanel::Draw() const {
    if (show)
        ImGui::OpenPopup("IOPanel");
    if (ImGui::BeginPopup("IOPanel"))
    {
        for (MenuElement* element : elements)
            element->Draw();
        
        ImGui::EndPopup();
    }
}

void IOPanel::Update(int& nodeCreateType) {
	if (ImGui::IsMouseClicked(1))
		show = true;

	if (ImGui::IsMouseClicked(0))
		show = false;

    for (MenuElement* element : elements)
        element->Update(nodeCreateType);
}