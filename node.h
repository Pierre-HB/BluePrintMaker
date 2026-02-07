#pragma once
#include "imnodes.h"
#include <list>
#include <vector>
#include <iostream>
#include "json11.hpp"
#include "data_base.h"
#include <format>

enum NODE_IO_TYPE {
	ITEM=0,//regular item
	IO=1,//io with throuput specified by the user
	LOCK_IO=2,//io with guessed througput
	SPLITTER=3//splitter
};

NODE_IO_TYPE int2NodeIOType(int i);


//Node viwer will write states in this SHARED struct
struct NodeUpdator {
private:
	int nodeId = -1;
	int stateChannel = -1;
	int newState = -1;
	int nodeIOId = -1;
	float data = -1;
	int nodeUpdatorState = -1;

	const static int NODE_UPDATOR_IO_LOCK = 0;
	const static int NODE_UPDATOR_IO_UNLOCK = 1;
	const static int NODE_UPDATOR_IO_SPLITTER = 2;

public:
	void reset() {
		nodeId = -1;
		stateChannel = -1;
		newState = -1;
		nodeIOId = -1;
		data = -1;
		nodeUpdatorState = -1;
	}

	bool UpdateNode() const {
		return nodeId != -1;
	}
	bool UpdateNodeIO() const {
		return nodeIOId != -1;
	}

	int GetNodeIOId() const {
		return nodeIOId;
	}

	int GetNodeId() const {
		return nodeId;
	}

	int GetNodeStateChannel() const {
		return stateChannel;
	}

	int GetNodeNewState() const {
		return newState;
	}

	float GetIOQuantity() const {
		return data;
	}

	bool IsNodeIOLock() const {
		return nodeUpdatorState == NODE_UPDATOR_IO_LOCK;//internal encoding
	}

	bool IsNodeIOUnlock() const {
		return nodeUpdatorState == NODE_UPDATOR_IO_UNLOCK;//internal encoding
	}

	bool IsNodeIOSplitter() const {
		return nodeUpdatorState == NODE_UPDATOR_IO_SPLITTER;
	}

	bool UpdateNodeIOQuantity() const {
		return !UpdateNodeIOState();
	}

	bool UpdateNodeIOState() const {
		return nodeUpdatorState == NODE_UPDATOR_IO_UNLOCK || nodeUpdatorState == NODE_UPDATOR_IO_LOCK;
	}

	void SetNodeNewState(int _stateChannel, int _newState, int _nodeId) {
		nodeId = _nodeId;
		stateChannel = _stateChannel;
		newState = _newState;
		IM_ASSERT(nodeIOId == -1);
	}

	void SetNodeIOQuantity(float newData, int _nodeIOId) {
		data = newData;
		nodeIOId = _nodeIOId;
		IM_ASSERT(nodeId == -1);
		IM_ASSERT(newData >= 0);
	}

	void SetNodeIOSplitterPercent(float newPercent, int _nodeIOId) {
		data = newPercent;
		nodeIOId = _nodeIOId;
		nodeUpdatorState = NODE_UPDATOR_IO_SPLITTER;
		IM_ASSERT(nodeId == -1);
		IM_ASSERT(newPercent >= 0);
		IM_ASSERT(newPercent <= 100);
	}

	void SetNodeIOState(NODE_IO_TYPE newType, int _nodeIOId) {
		nodeIOId = _nodeIOId;
		IM_ASSERT(nodeId == -1);
		switch (newType)
		{
		case IO:
			nodeUpdatorState = NODE_UPDATOR_IO_UNLOCK; //internal encoding
			break;
		case LOCK_IO:
			nodeUpdatorState = NODE_UPDATOR_IO_LOCK;
			break;
		default:
			IM_ASSERT(false && "TODO");
		}
	}
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
	NodeIO(const json11::Json& json) : id(json.object_items().at("id").int_value()), itemId(json.object_items().at("itemId").int_value()), quantity(json.object_items().at("quantity").number_value()), type(int2NodeIOType(json.object_items().at("type").int_value() )){}
	//TODO save and load type 

	int GetId() const {
		return id;
	}

	void SetId(int id) {
		NodeIO::id = id;
		std::cout << "set nodeIO id : " << NodeIO::id << std::endl;
	}

	json11::Json ToJson() const {
		return json11::Json({ {"id", id}, {"itemId", itemId}, {"quantity", quantity}, {"type", type} });
	}

	void Overide(const NodeIO& nodeIO) {
		itemId = nodeIO.itemId;
		quantity = nodeIO.quantity;
		type = nodeIO.type;
	}
};

struct NodeIOViewer {
	const NodeIO* nodeIO;
	bool isInput;
	const DataBase* dataBase;
	NodeUpdator* nodeUpdator;


	NodeIOViewer(const NodeIO* nodeIO, bool isInput, const DataBase* dataBase, NodeUpdator* nodeUpdator) : nodeIO(nodeIO), isInput(isInput), dataBase(dataBase), nodeUpdator(nodeUpdator) {
		std::cout << "create IOViewer : " << nodeIO->GetId() << std::endl;
	}

	int GetId() const {
		return nodeIO->id;
	}

	void Draw();
	void DrawItem(const Item& item);
	void DrawLockIO(const Item& item);
	void DrawIO(const Item& item);
	void DrawSplitter(const Item& item);
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
	MACHINE_TYPE type;
	float throuput;
public:
	Node();
	Node(int id);
	Node(const Node& node);
	Node(const Node& node, int(*CreateId)());
	Node(const DataBase* dataBase, int machineId, int(*CreateId)());
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
	float GetThrouput() const;
	void SetThrouput(float newThrouput);
	MACHINE_TYPE GetType() const;

	const std::vector<NodeIO>& GetInputs() const;
	const std::vector<NodeIO>& GetOutputs() const;

	void UpdateNodeIOData(int ioId, float newData);
	void UpdateNodeIOSplitterData(int ioId, float newData);
	void UpdateNodeIOType(int ioId, NODE_IO_TYPE type);
	void InitNodeAsIO(int(*CreateId)(), const DataBase* dataBase, bool input);
	void InitNodeAsRegular(int(*CreateId)(), const DataBase* dataBase);
	void InitNodeAsSplitter(int(*CreateId)(), const DataBase* dataBase);

	void AddInputs(NodeIO nodeIO);
	void AddOutputs(NodeIO nodeIO);

	NodeIO const* GetIO(int nodeIOId) const;
	void SetIOItem(int nodeIOId, int itemId);
	void SetIOQuantity(float quantity);
	void ResetIOItemId(int unkownItemId);

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