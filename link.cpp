#include "link.h"
#include "imnodes.h"

Link::Link(int id, int inputId, int outputId) : id(id), inputId(inputId), outputId(outputId){

}

Link::Link() : Link(-1, -1, -1) {

}

Link::Link(int id) : Link(id, -1, -1) {

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


//============================== Viewer ==============================//


LinkViewer::LinkViewer(Link* link) : link(link) {

}

void LinkViewer::Draw() const {
	ImNodes::Link(GetId(), link->GetInputId(), link->GetOutputId(), ImNodesLinkType_::ImNodesLinkType_Sloped);
}

int LinkViewer::GetId() const {
	return link->GetId();
}