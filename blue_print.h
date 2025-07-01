#pragma once

#include "node.h"
#include <vector>

class BluePrint {
private:
	int idSeed;
	const char* name;

	std::vector<Node*> nodes;
	std::vector<NodeViewer*> nodeViewers;

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
		//begin editor
		//draw node
		//draw links

		//IF click droit show IOPanel
	

	//void Update() {
	//	//if need to add node, link, swap attribute
	//	//Copie/paste, save ?
	//}
};