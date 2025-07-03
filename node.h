#pragma once
#include "imnodes.h"
#include <list>
#include <vector>
#include <iostream>

struct NodeIO {
	int id;
	int ressource;
	//int proliferator_lvl;
	//int proliferator_lvl2;
	//int proliferator_lvl3;
	//std::list<NodeIO*> connectedIO;

	NodeIO() : id(), ressource() {}
	NodeIO(int id) : id(id), ressource() {}
	NodeIO(int id, int ressource) : id(id), ressource(ressource) {}

	int GetId() const {
		return id;
	}

	void SetId(int id) {
		NodeIO::id = id;
		std::cout << "set nodeIO id : " << NodeIO::id << std::endl;
	}
};

struct NodeIOViewer {
	const NodeIO* nodeIO;
	bool isInput;

	NodeIOViewer(const NodeIO* nodeIO, bool isInput) : nodeIO(nodeIO), isInput(isInput) {
		std::cout << "create IOViewer : " << nodeIO->GetId() << std::endl;
	}

	int GetId() const {
		return nodeIO->id;
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
	//name ?

public:
	Node();
	Node(int id);
	Node(const Node& node);
	Node(const Node& node, int(*CreateId)());
	void Overide(const Node& node, int(*CreateId)());

	void Update();

	int GetId() const;

	const std::vector<NodeIO>& GetInputs() const;
	const std::vector<NodeIO>& GetOutputs() const;

	void AddInputs(NodeIO nodeIO);
	void AddOutputs(NodeIO nodeIO);

private:
	void SetIOIds(int(*CreateId)());
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

	void Draw(); //ImNodes API

	//void Update(); //Model API.... Maybe move this function up in the blueprint manager ?
	// or just check if it's Id have been swaped ?

	void SwapIO(int id1, int id2);

	int GetId() const {
		return node->GetId();
	}
private:
	//if the ref node changed (more or less input/outpu), reset all pins.
	//TODO a child NodeSplitterMergerViwer that only reset if the number of inputs/outputs decreased, keep same order if just one pin was added
	void Reset();
};

//IMNode is the Viewer model