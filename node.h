#pragma once
#include "imnodes.h"
#include <list>
#include <vector>
#include <iostream>
#include "json11.hpp"
#include "data_base.h"
#include <format>

//Node viwer will write states in this SHARED struct
struct NodeUpdator {
	int nodeId = -1;
	int stateChannel = -1;
	int newState = -1;
	int nodeIOId = -1;
	float data = -1;

	void reset() {
		nodeId = -1;
		stateChannel = -1;
		newState = -1;
		nodeIOId = -1;
		data = -1;
	}

	bool updateNode() {
		return nodeId != -1;
	}
	bool updateNodeIO() {
		return nodeIOId != -1;
	}
};

enum NODE_IO_TYPE {
	ITEM,//regular item
	IO,//io with throuput specified by the user
	LOCK_IO,//io with guessed througput
	SPLITTER,//splitter

};

struct NodeIO {
	int id;
	int itemId;
	float quantity;//serve also for throuput value and splitter
	//int proliferator_lvl;
	//int proliferator_lvl2;
	//int proliferator_lvl3;
	//std::list<NodeIO*> connectedIO;
	NODE_IO_TYPE type; //for NodeIOViewer
	//type ? regular, io, merger

	NodeIO() : id(), itemId(), quantity(), type(NODE_IO_TYPE::ITEM) {}
	NodeIO(int id) : id(id), itemId(), quantity(), type(NODE_IO_TYPE::ITEM) {}
	NodeIO(int id, int itemId) : id(id), itemId(itemId), quantity(), type(NODE_IO_TYPE::ITEM) {}
	NodeIO(int id, int itemId, float quantity) : id(id), itemId(itemId), quantity(quantity), type(NODE_IO_TYPE::ITEM) {}
	NodeIO(int id, int itemId, float quantity, NODE_IO_TYPE type) : id(id), itemId(itemId), quantity(quantity), type(type) {}
	NodeIO(const json11::Json& json) : id(json.object_items().at("id").int_value()), itemId(json.object_items().at("itemId").int_value()), quantity(json.object_items().at("quantity").number_value()) {}
	//TODO save and load type 

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

	void Draw();
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

	void ChangeState(const DataBase* dataBase, int stateChannel, int newState, int(*CreateId)());

	int GetId() const;
	int GetMachineId() const;
	float GetTime() const;
	int GetState(int i) const;
	int GetStateSize() const;
	bool GetSpecial() const;

	const std::vector<NodeIO>& GetInputs() const;
	const std::vector<NodeIO>& GetOutputs() const;

	void UpdateIO(int ioId, float newData);
	void InitNodeAsIO(int(*CreateId)(), const DataBase* dataBase, bool input);
	void InitNodeAsRegular(int(*CreateId)(), const DataBase* dataBase);
	//void InitNodeAsSplitter(const DataBase* dataBase);

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

	void DrawMachine();
	void DrawMachineTitle();
	void DrawMachineContent();
	void DrawMachineFooter();

	void DrawInput();
	void DrawInputTitle();
	void DrawInputContent();

	//void DrawSplitter();
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