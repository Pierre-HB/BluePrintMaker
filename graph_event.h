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

	GraphEventType type;

	int swapedNodeViewerId;
	
	//default constructor of an empty NONE event
	GraphEvent(); 

	//Default contructor for an empty valid event
	GraphEvent(int id, GraphEventType type); 

	// Copy constructor raising an error. Coping GraphEvent is forbiden, only moves are allowed
	GraphEvent(const GraphEvent& other) = delete;

	// create a GraphEvent by coping the state of a Node and a NodeViewer
	GraphEvent(int id, GraphEventType type, const Node& node, const NodeViewer& nodeViewer);

	// create a GraphEvent by coping the state of a Link and a LinkViewer
	GraphEvent(int id, GraphEventType type, const Link& link, const LinkViewer& linkViewer);

	// create a GraphEvent by stealing the state of a Node and a NodeViewer
	GraphEvent(int id, GraphEventType type, Node* node, NodeViewer* nodeViewer);

	// create a GraphEvent by stealing the state of a Link and a LinkViewer
	GraphEvent(int id, GraphEventType type, Link* link, LinkViewer* linkViewer);

	// default constructor for a swap event
	GraphEvent(int id, int swapedNodeViewerId, const NodeViewer& nodeViewer);

	//Destructor of a graph event. WILL FREE the assosciated pointer. The user have to make sure that there is no copy of pointers still alive somewhere.
	~GraphEvent();

	//Compare the event id with a given id
	bool operator==(int id) const {
		return id == GraphEvent::id;
	}

	//compare both event id
	bool operator==(const GraphEvent& Event) const {
		return id == Event.id;
	}

	//only move assign is allowed for GraphEvent. Copy are forbiden
	GraphEvent& operator=(GraphEvent&& m) noexcept;

	//Copy assign is deleted to avoid all copy of a GraphEvent. Copy are forbiden
	GraphEvent& operator=(const GraphEvent& m) noexcept = delete;

	//Steal a Node and a NodeViewer and put them in the event
	void Push_Node(Node* node, NodeViewer* nodeViewer);

	//Steal a Link and a LinkViewer and put them in the event
	void Push_Link(Link* link, LinkViewer* linkViewer);

	//Copy a NodeViewer and put it in the event. Only for SWAP event
	void Push_NodeViewer(const NodeViewer& nodeViewer);

	bool valid() const;
};
