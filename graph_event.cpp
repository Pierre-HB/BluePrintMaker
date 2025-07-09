#include "graph_event.h"


GraphEvent::GraphEvent() : GraphEvent(-1, NONE) {

}

GraphEvent::GraphEvent(int id, GraphEventType type) : id(id), type(type), swapedNodeViewerId(-1){

}

GraphEvent::GraphEvent(int id, GraphEventType type, Node* node, NodeViewer* nodeViewer) : id(id), type(type), swapedNodeViewerId(-1) {
	nodeDatas.push_back(node);
	nodeViewerDatas.push_back(nodeViewer);
	nodeImNodesDatas.push_back(ImNodes::GetNodeData(node->GetId()));
}

GraphEvent::GraphEvent(int id, GraphEventType type, Link* link, LinkViewer* linkViewer) : id(id), type(type), swapedNodeViewerId(-1) {
	linkDatas.push_back(link);
	linkViewerDatas.push_back(linkViewer);
	linkImNodesDatas.push_back(ImNodes::GetLinkData(link->GetId()));
}

GraphEvent::GraphEvent(int id, GraphEventType type, const Node& node, const NodeViewer& nodeViewer) : GraphEvent(id, type, new Node(node), new NodeViewer(nodeViewer)) {
	std::cout << "create a node. node addr : " << &node << ", node event addr : " << GraphEvent::nodeDatas[0] << std::endl;
}

GraphEvent::GraphEvent(int id, GraphEventType type, const Link& link, const LinkViewer& linkViewer) : GraphEvent(id, type, new Link(link), new LinkViewer(linkViewer)) {

}

GraphEvent::GraphEvent(int id, int swapedNodeViewerId, const NodeViewer& nodeViewer) : id(id), swapedNodeViewerId(swapedNodeViewerId), type(ATTRIUTE_SWAP) {
	nodeViewerDatas.push_back(new NodeViewer(nodeViewer));
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

void GraphEvent::Push_NodeViewer(const NodeViewer& nodeViewer) {
	assert(type == ATTRIUTE_SWAP);
	assert(nodeViewerDatas.size() == 1);
	nodeViewerDatas.push_back(new NodeViewer(nodeViewer));
}

void GraphEvent::Push_Link(Link* link, LinkViewer* linkViewer) {
	linkDatas.push_back(link);
	linkViewerDatas.push_back(linkViewer);
	linkImNodesDatas.push_back(ImNodes::GetLinkData(link->GetId()));
}

bool GraphEvent::valid() const {
	switch (type)
	{
	case CREATION:
	case DESTRUCTION:
		return nodeDatas.size() > 0 || linkDatas.size() > 0;

	case ATTRIUTE_SWAP:
	{
		if (nodeViewerDatas.size() != 2)
			return false;

		return *nodeViewerDatas[0] != *nodeViewerDatas[1];
	}
	case NONE:
		return false;
	default:
		assert(false);
		return false;
	}
}

GraphEvent& GraphEvent::operator=(GraphEvent&& m) noexcept {
	std::cout << "CALL move assign of Graph event" << std::endl;

	GraphEvent::~GraphEvent();

	GraphEvent::id = m.id;

	//For deletion and rceation event, need only one copy of the object
	GraphEvent::nodeImNodesDatas = std::move(m.nodeImNodesDatas);
	GraphEvent::linkImNodesDatas = std::move(m.linkImNodesDatas);
	GraphEvent::labelImNodesDatas = std::move(m.labelImNodesDatas);

	GraphEvent::nodeDatas = std::move(m.nodeDatas);
	GraphEvent::nodeViewerDatas = std::move(m.nodeViewerDatas);
	GraphEvent::linkDatas = std::move(m.linkDatas);
	GraphEvent::linkViewerDatas = std::move(m.linkViewerDatas);

	GraphEvent::type = m.type;

	GraphEvent::swapedNodeViewerId = m.swapedNodeViewerId;

	m.nodeImNodesDatas.clear();
	m.linkImNodesDatas.clear();
	m.labelImNodesDatas.clear();
	m.nodeDatas.clear();
	m.nodeViewerDatas.clear();
	m.linkDatas.clear();
	m.linkViewerDatas.clear();
	m.swapedNodeViewerId = -1;

	return *this;
}