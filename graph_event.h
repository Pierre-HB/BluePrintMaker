#pragma once
#include <vector>
#include "imnodes.h"
#include "node.h"
#include "link.h"

enum GraphEventType {
	CREATION,
	DESTRUCTION,
	ATTRIUTE_SWAP,
	NONE
};

struct GraphEvent {
	int id;

	//For deletion and rceation event, need only one copy of the object
	std::vector<ImNodeData*> nodeImNodesDatas;
	std::vector<ImLinkData*> linkImNodesDatas;
	std::vector<ImLabelData*> labelImNodesDatas;

	std::vector<Node*> nodeDatas;
	std::vector<NodeViewer*> nodeViewerDatas;
	std::vector<Link*> linkDatas;
	std::vector<LinkViewer*> linkViewerDatas;

	//for swaped attribute
	std::pair<int, int> swaped_attribute;

	GraphEventType type;

	GraphEvent();
	GraphEvent(int id, GraphEventType type);
	~GraphEvent();

	bool operator==(int id) const {
		return id == GraphEvent::id;
	}

	bool operator==(const GraphEvent& Event) const {
		return id == Event.id;
	}
};

static GraphEvent CreationEvent(int id) {
	return GraphEvent(id, CREATION);
}

static GraphEvent CreationEvent(int id, Node* node, NodeViewer* nodeViewer, ImNodeData* nodeData) {
	GraphEvent Event = GraphEvent(id, CREATION);
	Event.nodeDatas.push_back(node);
	Event.nodeViewerDatas.push_back(nodeViewer);
	Event.nodeImNodesDatas.push_back(nodeData);
	return Event;
}

static GraphEvent DestructionEvent(int id) {
	return GraphEvent(id, DESTRUCTION);
}