#pragma once

#include "io_panel.h"
#include "node.h"
#include "link.h"
#include <vector>
#include <map>
#include "graph_event.h"
#include "bififo.h"


static int idSeed = 0;

static int CreateId() {
	return idSeed++;
}

class BluePrint {
private:
	
	const char* name;

	IOPanel ioPanel;

	std::map<int, Node*> nodes;
	std::map<int, NodeViewer*> nodeViewers;

	std::map<int, Link*> links;
	std::map<int, LinkViewer*> linkViewers;

	BiFIFO<GraphEvent, 1024> graphEvents;

	//ImVector<int> links; //Get list of link caus wee need to render them after all nodes
	//int ioPanel; //object for managing user input
	/*
	asking to create node for now
	*/

	const std::vector<Node> recipies;
	/*
	recipies[0] = splitter
	recipies[1] = merger
	recipies[2] = input
	recipies[3] = output
	recipies[4...nb_machine] = empty machines
	recipies[nb_machine...] = recipies
	*/

	const std::vector<int> ressources;
	/*
	ressources[0] = all_ressources
	ressources[1] = iron ?
	ressources[2] = coal ?
	...
	*/

public:
	
	BluePrint(const char* name);
	BluePrint() : BluePrint("hello world") {};
	~BluePrint();

	void Draw() const;

	void Update();

	int CreateNewNode(int type);
	int CreateNewLink(int input_attr_id, int output_attr_id);
	int CreateNode(Node* node, NodeViewer* nodeViewer, ImNodeData* nodeData);
	int CreateLink(Link* link, LinkViewer* linkViewer, ImLinkData* linkData);
	void DeleteNodes(const std::vector<int>& nodeIds, GraphEvent* Event = nullptr);
	void DeleteLinks(const std::vector<int>& linkIds, GraphEvent* Event = nullptr);
};

template<typename T>
std::vector<int> ExtractIds(const std::vector<T>& datas) {
	std::vector<int> ids = std::vector<int>(datas.size());
	for (int i = 0; i < datas.size(); i++)
		ids[i] = datas[i]->GetId();
	return ids;
}