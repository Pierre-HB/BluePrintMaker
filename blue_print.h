#pragma once

#include "io_panel.h"
#include "node.h"
#include "link.h"
#include <vector>

class BluePrint {
private:
	int idSeed;
	const char* name;

	IOPanel ioPanel;

	std::vector<Node*> nodes;
	std::vector<NodeViewer*> nodeViewers;

	std::vector<Link*> links;
	std::vector<LinkViewer*> linkViewers;

	//ImVector<int> links; //Get list of link caus wee need to render them after all nodes
	//int ioPanel; //object for managing user input
	/*
	asking to create node for now
	*/
public:
	
	BluePrint(const char* name);
	BluePrint() : BluePrint("hello world") {};
	~BluePrint();

	int CreateId() {
		return idSeed++;
	}

	void Draw() const;

	void Update();

	void CreateNewNode(int type);
};