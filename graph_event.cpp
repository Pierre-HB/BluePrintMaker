#include "graph_event.h"


GraphEvent::GraphEvent() : GraphEvent(-1, NONE) {

}

GraphEvent::GraphEvent(int id, GraphEventType type) : id(id), type(type) {

}



GraphEvent::GraphEvent(int id, GraphEventType type, Node* node, NodeViewer* nodeViewer) : id(id), type(type) {
	nodeDatas.push_back(node);
	nodeViewerDatas.push_back(nodeViewer);
	nodeImNodesDatas.push_back(ImNodes::GetNodeData(node->GetId()));
}

GraphEvent::GraphEvent(int id, GraphEventType type, Link* link, LinkViewer* linkViewer) : id(id), type(type) {
	linkDatas.push_back(link);
	linkViewerDatas.push_back(linkViewer);
	linkImNodesDatas.push_back(ImNodes::GetLinkData(link->GetId()));
}

GraphEvent::GraphEvent(int id, GraphEventType type, const Node& node, const NodeViewer& nodeViewer) : GraphEvent(id, type, new Node(node), new NodeViewer(nodeViewer)) {
	std::cout << "create a node. node addr : " << &node << ", node event addr : " << GraphEvent::nodeDatas[0] << std::endl;
}

GraphEvent::GraphEvent(int id, GraphEventType type, const Link& link, const LinkViewer& linkViewer) : GraphEvent(id, type, new Link(link), new LinkViewer(linkViewer)) {

}

//Just to assert that I don't make copy of event, only Moves
//Because when a copy is deleted, so are the pointers
//Maybe I should just add 'delete GraphEvent(const GraphEvent& other)' in the class definition to delete the default copy ocnstructor
GraphEvent::GraphEvent(const GraphEvent& other) : id(), type() {
	assert(false);
}

GraphEvent::~GraphEvent() {
	for (ImNodeData* node : nodeImNodesDatas)
		ImNodes::DeleteNodeData(node);

	for (ImLinkData* link : linkImNodesDatas)
		ImNodes::DeleteLinkData(link);

	for (ImLabelData* label : labelImNodesDatas)
		ImNodes::DeleteLabelData(label);

	for (Node* node : nodeDatas)
		delete node;

	for (NodeViewer* nodeViewer : nodeViewerDatas)
		delete nodeViewer;

	for (Link* link : linkDatas)
		delete link;

	for (LinkViewer* linkViewer : linkViewerDatas)
		delete linkViewer;
}

void GraphEvent::Push_Node(Node* node, NodeViewer* nodeViewer) {
	nodeDatas.push_back(node);
	nodeViewerDatas.push_back(nodeViewer);
	nodeImNodesDatas.push_back(ImNodes::GetNodeData(node->GetId()));
}

void GraphEvent::Push_Link(Link* link, LinkViewer* linkViewer) {
	linkDatas.push_back(link);
	linkViewerDatas.push_back(linkViewer);
	linkImNodesDatas.push_back(ImNodes::GetLinkData(link->GetId()));
}