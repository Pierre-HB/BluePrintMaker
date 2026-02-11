#pragma once

#include "io_panel.h"
#include "node.h"
#include "link.h"
#include <vector>
#include <map>
#include "graph_event.h"
#include "bififo.h"
#include "json11.hpp"
#include "smatrix.h"


static int idSeed = 0;

static int CreateId() {
	return idSeed++;
}

static void SetIdSeed(int seed) {
	idSeed = seed;
}

class BluePrint {
private:
	
	//const char* name;
	std::string name;
	std::string filename;

	std::map<int, Node*> nodes;
	std::map<int, NodeViewer*> nodeViewers;
	NodeUpdator* nodeUpdator;

	std::map<int, Link*> links;
	std::map<int, LinkViewer*> linkViewers;
	LinkUpdator* linkUpdator;

	BiFIFO<GraphEvent, 1024> graphEvents;

	GraphEvent swapingEvent;
	int swapingNodeViewerId;

	ImNodesEditorContext* editorContext;

	//ImVector<int> links; //Get list of link caus wee need to render them after all nodes
	//int ioPanel; //object for managing user input
	/*
	asking to create node for now
	*/

	const std::vector<Node> recipes;//TODO remove
	/*
	recipies[0] = splitter
	recipies[1] = merger
	recipies[2] = input
	recipies[3] = output
	recipies[4...nb_machine] = empty machines
	recipies[nb_machine...] = recipies
	*/

	const std::vector<int> ressources;
	//ressources viwer : name ressource, icone ressource
	/*
	ressources[0] = all_ressources
	ressources[1] = iron ?
	ressources[2] = coal ?
	...
	*/
	DataBase* dataBase;
	IOPanel ioPanel;

	bool solved;
private:

	void clearStack();
	void LoadJson(const json11::Json& json);
public:

	// empty blueprint for placeholder
	BluePrint();
	// new blueprint from a database
	BluePrint(DataBase* dataBase);
	// blueprint from an old save
	BluePrint(DataBase* dataBase, const std::string& filename, const json11::Json& json);
	~BluePrint();

	void LoadDataBase(const json11::Json& json);

	void Draw() const;

	void Update();
	bool CheckGraph();
	bool SolveGraph();
	void ResetGraph();

	int CreateNewNode(int type);
	int CreateNewLink(int input_attr_id, int output_attr_id);
	int CreateNode(Node* node, NodeViewer* nodeViewer, ImNodeData* nodeData);
	int CreateLink(Link* link, LinkViewer* linkViewer, ImLinkData* linkData);
	void DeleteLinkedLink(int nodeId, GraphEvent* Event);
	void DeleteNodes(const std::vector<int>& nodeIds, GraphEvent* Event = nullptr);
	void DeleteLinks(const std::vector<int>& linkIds, GraphEvent* Event = nullptr);

	json11::Json ToJson() const;

	static BluePrint* CreateBluePrint(DataBase* dataBase);
	void saveBluePrint();
	std::string saveUnderBluePrint() const;//don't use internal filename
};

template<typename T>
std::vector<int> ExtractIds(const std::vector<T>& datas) {
	std::vector<int> ids = std::vector<int>(datas.size());
	for (int i = 0; i < datas.size(); i++)
		ids[i] = datas[i]->GetId();
	return ids;
}

template<typename T>
json11::Json MapToJson(const std::map<int, T*> m) {
	std::vector<json11::Json> jsonVector;
	for (const auto& [key, value] : m)
		jsonVector.push_back(value->ToJson());
	return json11::Json(jsonVector);
}

template<typename T>
std::map<int, T*> JsonToMap(const json11::Json::array json) {
	std::map<int, T*> m;
	for (const auto& a : json)
		m[a.object_items().at("id").int_value()] = new T(a);
	return m;
}

template<typename T, typename TV, typename TU>
std::map<int, TV*> JsonToMap(std::map<int, T*>& m, const json11::Json::array json, const DataBase* dataBase, TU* updater) {
	std::map<int, TV*> mv;
	for (const auto& a : json)
		mv[a.object_items().at("id").int_value()] = new TV(m, a, dataBase, updater);
	return mv;
}