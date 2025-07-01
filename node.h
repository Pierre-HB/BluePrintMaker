#pragma once
#include "imnodes.h"
#include <list>
#include <vector>

struct NodeIO {
	int Id;
	int ressource;
	//int proliferator_lvl;
	//int proliferator_lvl2;
	//int proliferator_lvl3;
	//std::list<NodeIO*> connectedIO;

	NodeIO() : Id(), ressource() {}
	NodeIO(int id) : Id(id), ressource() {}
	NodeIO(int id, int ressource) : Id(id), ressource(ressource) {}

	int GetId() const {
		return Id;
	}
};

struct NodeIOViewer {
	const NodeIO* nodeIO;
	bool isInput;

	NodeIOViewer(const NodeIO* nodeIO, bool isInput) : nodeIO(nodeIO), isInput(isInput) {}

	int GetId() const {
		return nodeIO->Id;
	}

	void Draw() const {
		if(isInput)
			ImNodes::BeginInputAttribute(GetId());
		else
			ImNodes::BeginOutputAttribute(GetId());

		ImGui::Text("NodeIO ressource : %d", nodeIO->ressource);

		if (isInput)
			ImNodes::EndInputAttribute();
		else
			ImNodes::EndOutputAttribute();
	}
};

// MODEL class
class Node {
protected:
	int id;
	std::vector<NodeIO> inputs;
	std::vector<NodeIO> outputs;

public:
	Node();
	Node(int id);

	void Update();

	int GetId() const;

	const std::vector<NodeIO>& GetInputs() const;
	const std::vector<NodeIO>& GetOutputs() const;

	void AddInputs(NodeIO nodeIO);
	void AddOutputs(NodeIO nodeIO);
};

// CONTROLLER CLASS
class NodeViewer {
protected:
	const Node* node;
	std::vector<NodeIOViewer> input_ref; //id of input in node.inputs
	std::vector<NodeIOViewer> output_ref; //id of output in node.inputs

	std::vector<int> input_perm;
	std::vector<int> output_perm;
public:
	NodeViewer(const Node* node);

	void Draw() const; //ImNodes API

	//void Update(); //Model API.... Maybe move this function up in the blueprint manager ?
	// or just check if it's Id have been swaped ?

	void SwapIO(int id1, int id2);

	int GetId() const {
		return node->GetId();
	}

};

//IMNode is the Viewer model