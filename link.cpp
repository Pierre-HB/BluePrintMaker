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


//============================== Viewer ==============================//


LinkViewer::LinkViewer(Link* link) : link(link) {

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