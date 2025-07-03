#pragma once

#include "io_panel.h"
#include "node.h"
#include "link.h"
#include <vector>

static int idSeed = 0;

static int CreateId() {
	return idSeed++;
}

class BluePrint {
private:
	
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

	void CreateNewNode(int type);
};