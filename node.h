#pragma once
#include "imnodes.h"
#include <list>
#include <vector>
#include <iostream>
#include "json11.hpp"
#include "data_base.h"

//Node viwer will write states in this SHARED struct
struct NodeUpdator {
	int nodeId = -1;
	int stateChannel = -1;
	int newState = -1;

	void reset() {
		nodeId = -1;
		stateChannel = -1;
		newState = -1;
	}

	bool update() {
		return nodeId != -1;
	}
};

struct NodeIO {
	int id;
	int itemId;
	float quantity;
	//int proliferator_lvl;
	//int proliferator_lvl2;
	//int proliferator_lvl3;
	//std::list<NodeIO*> connectedIO;

	NodeIO() : id(), itemId(), quantity() {}
	NodeIO(int id) : id(id), itemId(), quantity() {}
	NodeIO(int id, int itemId) : id(id), itemId(itemId), quantity() {}
	NodeIO(int id, int itemId, float quantity) : id(id), itemId(itemId), quantity(quantity) {}
	NodeIO(const json11::Json& json) : id(json.object_items().at("id").int_value()), itemId(json.object_items().at("itemId").int_value()), quantity(json.object_items().at("quantity").number_value()) {}

	int GetId() const {
		return id;
	}

	void SetId(int id) {
		NodeIO::id = id;
		std::cout << "set nodeIO id : " << NodeIO::id << std::endl;
	}

	json11::Json ToJson() const {
		return json11::Json({ {"id", id}, {"itemId", itemId}, {"quantity", quantity}});
	}
};

struct NodeIOViewer {
	const NodeIO* nodeIO;
	bool isInput;
	const DataBase* dataBase;

	NodeIOViewer(const NodeIO* nodeIO, bool isInput, const DataBase* dataBase) : nodeIO(nodeIO), isInput(isInput), dataBase(dataBase) {
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

		//ImGui::Text("NodeIO ressource : %d", nodeIO->ressource);
		const Item& item = dataBase->getItem(nodeIO->itemId);
		ImGui::Text((item.iconeString+" "+item.name).c_str());
		ImGui::SameLine();
		ImGui::Text("%.2f", nodeIO->quantity);

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
	//std::string name;
	int machineId;
	float time;
	float idlePower;
	float workingPower;
	std::vector<int> state; //for the selected recipe and the selected modifier
	bool specialNode;//true for merger, splitter, intput and output

public:
	Node();
	Node(int id);
	Node(const Node& node);
	Node(const Node& node, int(*CreateId)());
	Node(const DataBase* dataBase, int type, int(*CreateId)());
	Node(const json11::Json& json);
	void Overide(const Node& node, int(*CreateId)());
	void Overide(const Node& node);

	json11::Json ToJson() const;

	void Update();

	void changeState(const DataBase* dataBase, int stateChannel, int newState, int(*CreateId)());

	int GetId() const;
	int GetMachineId() const;
	int GetState(int i) const;
	int GetStateSize() const;

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

	ImVec2 size;

	const DataBase* dataBase;
	NodeUpdator* nodeUpdator;
public:
	NodeViewer(const Node* node, const DataBase* dataBase, NodeUpdator* nodeUpdator);
	NodeViewer(const NodeViewer& nodeViewer, const Node* node);
	NodeViewer(std::map<int, Node*>& nodes, const json11::Json& json, const DataBase* dataBase, NodeUpdator* nodeUpdator);

	void Draw(); //ImNodes API

	//void Update(); //Model API.... Maybe move this function up in the blueprint manager ?
	// or just check if it's Id have been swaped ?

	bool SwapIO(int id1, int id2);
	bool Contain(int attr_id);

	int GetId() const {
		return node->GetId();
	}

	bool operator==(const NodeViewer& other) const;
	bool operator!=(const NodeViewer& other) const;

	void CopyPerm(const NodeViewer& other);

	json11::Json ToJson() const;
	void Reset();
private:
	//if the ref node changed (more or less input/outpu), reset all pins.
	//TODO a child NodeSplitterMergerViwer that only reset if the number of inputs/outputs decreased, keep same order if just one pin was added
	
};

//IMNode is the Viewer model