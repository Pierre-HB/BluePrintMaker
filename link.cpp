#include "link.h"
#include "imnodes.h"

Link::Link(int id, int inputId, int outputId, int inputNodeId, int outputNodeId) : id(id), inputId(inputId), outputId(outputId), inputNodeId(inputNodeId), outputNodeId(outputNodeId) {

}

Link::Link() : Link(-1, -1, -1, -1, -1) {

}

Link::Link(int id) : Link(id, -1, -1, -1, -1) {

}

void Link::Update() {
	//TODO
}

int Link::GetId() const {
	return id;
}

int Link::GetInputId() const {
	return inputId;
}

int Link::GetOutputId() const {
	return outputId;
}

int Link::GetNodeInputId() const {
	return inputNodeId;
}

int Link::GetNodeOutputId() const {
	return outputNodeId;
}

json11::Json Link::ToJson() const {
	return json11::Json({ {"id", id}, {"inputId", inputId}, {"outputId", outputId}, {"inputNodeId", inputNodeId}, {"outputNodeId", outputNodeId }});
}

Link::Link(const json11::Json& json) {
	json11::Json::object obj = json.object_items();
	id = obj.at("id").int_value();
	inputId = obj.at("inputId").int_value();
	outputId = obj.at("outputId").int_value();
	inputNodeId = obj.at("inputNodeId").int_value();
	outputNodeId = obj.at("outputNodeId").int_value();
}

//============================== Viewer ==============================//


LinkViewer::LinkViewer(Link* link, const DataBase* dataBase) : link(link), dataBase(dataBase) {

}

LinkViewer::LinkViewer(const LinkViewer& linkViewer, const Link* link) : LinkViewer(linkViewer) {
	LinkViewer::link = link;
}

void LinkViewer::Draw() const {
	ImNodes::Link(GetId(), link->GetInputId(), link->GetOutputId(), ImNodesLinkType_::ImNodesLinkType_Sloped);
}

int LinkViewer::GetId() const {
	return link->GetId();
}

json11::Json LinkViewer::ToJson() const {
	//TODO maybe remove if unecessary at end of project
	return json11::Json::object{ { "id", link->GetId() } };
}

LinkViewer::LinkViewer(std::map<int, Link*>& links, const json11::Json& json, const DataBase* dataBase) : link(links.at(json.object_items().at("id").int_value())), dataBase(dataBase){

}