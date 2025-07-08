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
	GraphEvent(const GraphEvent& other);
	GraphEvent(int id, GraphEventType type, const Node& node, const NodeViewer& nodeViewer);
	GraphEvent(int id, GraphEventType type, Node* node, NodeViewer* nodeViewer);
	~GraphEvent();

	bool operator==(int id) const {
		return id == GraphEvent::id;
	}

	bool operator==(const GraphEvent& Event) const {
		return id == Event.id;
	}

	GraphEvent& operator=(GraphEvent&& m) noexcept {
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

		//for swaped attribute
		GraphEvent::swaped_attribute = m.swaped_attribute;

		GraphEvent::type = m.type;

		m.nodeImNodesDatas.clear();
		m.linkImNodesDatas.clear();
		m.labelImNodesDatas.clear();
		m.nodeDatas.clear();
		m.nodeViewerDatas.clear();
		m.linkDatas.clear();
		m.linkViewerDatas.clear();

		return *this;
	}

	//To detect all call to the copy assign operator
	//Maybe should just add 'delete GraphEvent& operator=(const GraphEvent& m)'
	//Because every copy of a graph event will eventually be deleted and destroy the pointers
	GraphEvent& operator=(const GraphEvent& m) noexcept {
		assert(false);
		return *this;
	}

	void Push_Node(Node* node, NodeViewer* nodeViewer);
};
