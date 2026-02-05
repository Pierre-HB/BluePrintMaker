#include "node_editor.h"
#include <imnodes.h>
#include <imgui.h>
#include <iostream>
#include <vector>
#include "blue_print.h"
#include "json11.hpp"
//#include "smatrix.h"
//#include "rat.hpp"

/*
+---------------------------------------------------------------------------+
| Blue Print																|
+---------------------------------------------------------------------------+
|  void Draw() const; Make Draw call of nodes and them links and IO_object	|
|  void Update(); add or delete nodes and link								|
|  void PoolEvent(); ? call glfwPool event to get key press... ?			|
|  int GetId(); Id generator												|
|  void PushEvent() event manager											|
+---------------------------------------------------------------------------+
| -List of all nodes object													|
| -List of all links object													|
| - IO object																|
+---------------------------------------------------------------------------+


+---------------------------------------------------------------+
| Nodes															|
+---------------------------------------------------------------+
|  void Draw() const; call to ImNodes::BeginNodes(Id);			|
|  void Update(); moove attributes and create events for that	|
+---------------------------------------------------------------+
| -List of all self attribute									|
| -Id                                                           |
+---------------------------------------------------------------+

+---------------------------------------------------------------+
| Attribute														|
+---------------------------------------------------------------+
|  void Draw() const; call to ImNodes::Beginattribute(Id) const |
|                                                               |
+---------------------------------------------------------------+
| -Id															|
+---------------------------------------------------------------+

+---------------------------------------------------------------+
|  Link                                                         |
+---------------------------------------------------------------+
|  Draw() const; call to ImNodes::Link and BeginLinkLabel()     |
|  Update(); check if linkLabels are deleted or not             |
+---------------------------------------------------------------+
|  -Id                                                          |
|  -Start attrib txt                                            |
|  -End Attrib txt                                              |
+---------------------------------------------------------------+

+---------------------------------------------------------------------------+
|  IO Object																|
+---------------------------------------------------------------------------+
|  Draw() const; call to ImGui call to create the right click window		|
|  Update(); send calls to create nodes requested by the right click window	|
+---------------------------------------------------------------------------+
|  -flags (like CTRL Z, CTRL Y, CLTR A, DEL, ...							|
+---------------------------------------------------------------------------+

*/


/*
	recipies[0] = splitter
	recipies[1] = merger
	recipies[2] = input
	recipies[3] = output
	recipies[4...nb_machine] = empty machines
	recipies[nb_machine...] = recipies
	*/
static const std::vector<Node> createRecipes() {
	std::vector<Node> recipies = std::vector<Node>();
	
	{// Splitter
		Node node = Node(-1);
		node.AddInputs(NodeIO(-1, 0));
		node.AddOutputs(NodeIO(-1, 0));
		node.AddOutputs(NodeIO(-1, 0));
		recipies.push_back(node);
	}

	{// Merger
		Node node = Node(-1);
		node.AddInputs(NodeIO(-1, 0));
		node.AddInputs(NodeIO(-1, 0));
		node.AddOutputs(NodeIO(-1, 0));
		recipies.push_back(node);
	}

	{// input
		Node node = Node(-1);
		node.AddInputs(NodeIO(-1, 0));
		recipies.push_back(node);
	}

	{// output
		Node node = Node(-1);
		node.AddOutputs(NodeIO(-1, 0));
		recipies.push_back(node);
	}

	{// machine
		Node node = Node(-1);
		node.AddInputs(NodeIO(-1, 1));
		node.AddInputs(NodeIO(-1, 2));
		node.AddOutputs(NodeIO(-1, 3));
		node.AddOutputs(NodeIO(-1, 4));
		recipies.push_back(node);
	}

	return recipies;
}

BluePrint::BluePrint(DataBase* dataBase) : name("Blueprint"), filename(""), nodes(), nodeViewers(), links(), linkViewers(), recipes(createRecipes()), swapingNodeViewerId(-1), dataBase(dataBase), ioPanel(dataBase), editorContext(ImNodes::EditorContextCreate()) {
	nodeUpdator = new NodeUpdator();
	linkUpdator = new LinkUpdator();
}

BluePrint::BluePrint() : BluePrint(nullptr) {

}

BluePrint::BluePrint(DataBase* dataBase, const std::string& _filename, const json11::Json& json) : BluePrint(dataBase) {
	filename = _filename;
	LoadJson(json);
}

BluePrint::~BluePrint() {
	for (const auto& [id, node] : nodes)
		delete node;
	for (const auto& [id, nodeViewer] : nodeViewers)
		delete nodeViewer;

	for (const auto& [id, link] : links)
		delete link;
	for (const auto& [id, linkViewer] : linkViewers)
		delete linkViewer;

	delete nodeUpdator;
	delete linkUpdator;

	ImNodes::EditorContextFree(editorContext);
}

void BluePrint::Draw() const {
	ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;
	ImGui::Begin(name.c_str(), NULL, flags);

	ImNodes::BeginNodeEditor();

	for (const auto& [id, nodeViewer] : nodeViewers)
		nodeViewer->Draw();

	for (const auto& [id, linkViewer] : linkViewers)
		linkViewer->Draw();

	ImNodes::EndNodeEditor();

	ioPanel.Draw();

	ImGui::End();
}

int BluePrint::CreateNewNode(int type) {
	Node* node = new Node(dataBase, type, CreateId);
	//Node* node = new Node(BluePrint::recipes[type], CreateId);
	ImNodes::SetNodeScreenSpacePos(node->GetId(), ImGui::GetIO().MousePos);
	nodes.insert(std::make_pair(node->GetId(), node));
	NodeViewer* nodeViewer = new NodeViewer(node, dataBase, nodeUpdator);
	nodeViewers.insert(std::make_pair(nodeViewer->GetId(), nodeViewer));

	int eventId = CreateId();
	graphEvents.push(GraphEvent(eventId, CREATION, *node, *nodeViewer)); //Copy node and nodeViewer in an event
	ImNodes::PushEvent(eventId);

	return node->GetId();
}

// O(n) complexity, supposed to be used at most twice per frame
static int findNodeContainingAttr(int attrId, const std::map<int, Node*>& nodes) {
	for (const auto& [nodeId, node] : nodes) {
		const std::vector<NodeIO>& inputs = node->GetInputs();
		for (const NodeIO& nodeIO : inputs)
			if (nodeIO.GetId() == attrId)
				return nodeId;

		const std::vector<NodeIO>& outputs = node->GetOutputs();
		for (const NodeIO& nodeIO : outputs)
			if (nodeIO.GetId() == attrId)
				return nodeId;
	}
	return -1;
}

int BluePrint::CreateNewLink(int input_attr_id, int output_attr_id) {

	int start_attr_node_id = findNodeContainingAttr(input_attr_id, nodes);
	int end_attr_node_id = findNodeContainingAttr(output_attr_id, nodes);

	Link* link = new Link(CreateId(), input_attr_id, output_attr_id, start_attr_node_id, end_attr_node_id);
	ImNodes::CreateLink(link->GetId());

	links.insert(std::make_pair(link->GetId(), link));
	LinkViewer* new_linkViewer = new LinkViewer(link, dataBase, linkUpdator);
	linkViewers.insert(std::make_pair(new_linkViewer->GetId(), new_linkViewer));

	int eventId = CreateId();
	graphEvents.push(GraphEvent(eventId, CREATION, *link, *new_linkViewer));
	ImNodes::PushEvent(eventId);

	return link->GetId();
}

int BluePrint::CreateNode(Node* node, NodeViewer* nodeViewer, ImNodeData* nodeData) {
	Node* new_node = new Node(*node);
	ImNodes::SetNodeData(new_node->GetId(), nodeData);

	nodes.insert(std::make_pair(new_node->GetId(), new_node));
	NodeViewer* new_nodeViewer = new NodeViewer(*nodeViewer, new_node);
	nodeViewers.insert(std::make_pair(new_nodeViewer->GetId(), new_nodeViewer));

	return new_node->GetId();
}

int BluePrint::CreateLink(Link* link, LinkViewer* linkViewer, ImLinkData* linkData) {
	Link* new_link = new Link(*link);
	ImNodes::SetLinkData(new_link->GetId(), linkData);

	links.insert(std::make_pair(new_link->GetId(), new_link));
	LinkViewer* new_linkViewer = new LinkViewer(*linkViewer, new_link);
	linkViewers.insert(std::make_pair(new_linkViewer->GetId(), new_linkViewer));

	return new_link->GetId();
}

void BluePrint::DeleteLinks(const std::vector<int>& linkIds, GraphEvent* Event) {
	for (int linkId : linkIds) {
		if (links.find(linkId) == links.end()) // already deleted when one of the attached node was deleted
			continue;

		if (Event != nullptr) {
			Event->Push_Link(links[linkId], linkViewers[linkId]);
		}
		else {
			delete links[linkId];
			delete linkViewers[linkId];
		}
		links.erase(linkId);
		linkViewers.erase(linkId);
	}
}

void BluePrint::DeleteNodes(const std::vector<int>& nodeIds, GraphEvent* Event) {
	//TODO delete assosciated links
	
	for (int nodeId : nodeIds) {
		if (Event != nullptr) {
			Event->Push_Node(nodes[nodeId], nodeViewers[nodeId]);
		}
		else {
			delete nodes[nodeId];
			delete nodeViewers[nodeId];
		}

		nodes.erase(nodeId);
		nodeViewers.erase(nodeId);
	}

	std::vector<int> linkToDelete = std::vector<int>();
	for (const auto& [linkId, link] : links) {
		if (nodes.find(link->GetNodeInputId()) == nodes.end() || nodes.find(link->GetNodeOutputId()) == nodes.end()) {
			linkToDelete.push_back(linkId);
		}
	}

	DeleteLinks(linkToDelete, Event);
}



void BluePrint::Update() {

	int nodeCreateType = -1;
	ioPanel.Update(nodeCreateType);
	if (nodeCreateType != -1)
		CreateNewNode(nodeCreateType);
	
	if (nodeUpdator->UpdateNode()) {
		Node* node = nodes[nodeUpdator->GetNodeId()];
		NodeViewer* nodeViewer = nodeViewers[nodeUpdator->GetNodeId()];

		Node* nodePrev = new Node(*node);//copy node
		NodeViewer* nodeViewerPrev = new NodeViewer(*nodeViewer);//copy nodeViewer
		node->ChangeState(dataBase, nodeUpdator->GetNodeStateChannel(), nodeUpdator->GetNodeNewState(), CreateId);
		if (nodeUpdator->GetNodeStateChannel() == 0)
			nodeViewer->Reset();
		
		nodeUpdator->reset();
		Node* nodeNext = new Node(*node);//copy node
		NodeViewer* nodeViewerNext = new NodeViewer(*nodeViewer);//copy nodeViewer

		int eventId = CreateId();
		graphEvents.push(GraphEvent(eventId, NODE_UPDATE, nodePrev, nodeNext, nodeViewerPrev, nodeViewerNext)); //copy node by passing it's referrence
		ImNodes::PushEvent(eventId);
	}

	if (nodeUpdator->UpdateNodeIO()) {
		//NodeIO& nodeIO;
		int nodeId = findNodeContainingAttr(nodeUpdator->GetNodeIOId(), nodes);
		Node* nodePrev = new Node(*nodes[nodeId]);
		
		if (nodeUpdator->UpdateNodeIOQuantity()) {
			if(nodeUpdator->IsNodeIOSplitter())
				nodes[nodeId]->UpdateNodeIOSplitterData(nodeUpdator->GetNodeIOId(), nodeUpdator->GetIOQuantity());
			else
				nodes[nodeId]->UpdateNodeIOData(nodeUpdator->GetNodeIOId(), nodeUpdator->GetIOQuantity());
		}
		else {//update state
			if (nodeUpdator->IsNodeIOLock()) {//lock nodeIO
				nodes[nodeId]->UpdateNodeIOType(nodeUpdator->GetNodeIOId(), NODE_IO_TYPE::LOCK_IO);
			}
			else {//Unlock NodeIO
				nodes[nodeId]->UpdateNodeIOType(nodeUpdator->GetNodeIOId(), NODE_IO_TYPE::IO);
			}

		}
		nodeUpdator->reset();
		Node* nodeNext = new Node(*nodes[nodeId]);

		int eventId = CreateId();
		graphEvents.push(GraphEvent(eventId, NODE_IO_UPDATE, nodePrev, nodeNext)); //copy node by passing it's referrence
		ImNodes::PushEvent(eventId);
	}

	/*if (linkUpdator->update()) {
		linkUpdator->reset();
	}*/

	/*for (const auto& [id, linkViewer] : linkViewers)
		linkViewer->Update();*/
	//id of node, recipe target
		

	int start_attr, end_attr;
	if (ImNodes::IsLinkCreated(&start_attr, &end_attr))
		CreateNewLink(start_attr, end_attr);

	int link_id;
	if (ImNodes::IsLinkDestroyed(&link_id)) {
		int eventId = CreateId();
		graphEvents.push(GraphEvent(eventId, DESTRUCTION, links[link_id], linkViewers[link_id]));
		ImNodes::PushEvent(eventId);

		linkViewers.erase(link_id);
		links.erase(link_id);
	}

	if (ImGui::IsKeyPressed(ImGuiKey_Delete)) {
		//TODO also delete labels
		int nb_selected_node = ImNodes::NumSelectedNodes();
		int nb_selected_link = ImNodes::NumSelectedLinks();
		
		if (nb_selected_node > 0 || nb_selected_link > 0) {
			int eventId = CreateId();
			GraphEvent Event(eventId, DESTRUCTION);
			if (nb_selected_node > 0) {
				std::vector<int> selected_node_ids = std::vector<int>(nb_selected_node);
				ImNodes::GetSelectedNodes(selected_node_ids.data());

				DeleteNodes(selected_node_ids, &Event);	
			}
			if (nb_selected_link > 0) {
				std::vector<int> selected_link_ids = std::vector<int>(nb_selected_link);
				ImNodes::GetSelectedLinks(selected_link_ids.data());
				DeleteLinks(selected_link_ids, &Event);
			}

			graphEvents.push(std::move(Event));
			ImNodes::PushEvent(eventId);
			ImNodes::ClearNodeSelection();
			ImNodes::ClearLinkSelection();
		}
	}

	int src_attr, dest_attr;
	if (swapingNodeViewerId != -1 && !ImGui::IsMouseDragging(0)) {
		swapingEvent.Push_NodeViewer(*nodeViewers[swapingNodeViewerId]);
		if (swapingEvent.valid()) {
			ImNodes::PushEvent(swapingEvent.id);
			graphEvents.push(std::move(swapingEvent));
		}
		swapingNodeViewerId = -1;
		swapingEvent = GraphEvent();
	}

	if (ImNodes::IsAttributeSwapped(&src_attr, &dest_attr))
	{
		if (swapingNodeViewerId == -1) {
			for (const auto& [key, nodeViewer] : nodeViewers) {
				if (nodeViewer->Contain(src_attr)) {
					assert(nodeViewer->Contain(dest_attr));
					swapingNodeViewerId = nodeViewer->GetId();
					break;
				}
			}
			swapingEvent = GraphEvent(CreateId(), swapingNodeViewerId, *nodeViewers[swapingNodeViewerId]);
		}
		assert(nodeViewers[swapingNodeViewerId]->SwapIO(src_attr, dest_attr));
	}

	int eventId;
	if (ImNodes::GetPopedEvent(&eventId)) {
		GraphEvent* dest;
		if (graphEvents.pop(&dest)) {
			assert(*dest == eventId);
			std::cout << "Poped event " << eventId << std::endl;
			switch (dest->type)
			{
			case CREATION:
			{
				if(dest->nodeDatas.size() > 0)
					DeleteNodes(ExtractIds(dest->nodeDatas));
				if (dest->linkDatas.size() > 0)
					DeleteLinks(ExtractIds(dest->linkDatas));
				break;
			}
			case DESTRUCTION:
			{
				for (int i = 0; i < dest->nodeDatas.size(); i++) {
					CreateNode(dest->nodeDatas[i], dest->nodeViewerDatas[i], dest->nodeImNodesDatas[i]);
				}
				for (int i = 0; i < dest->linkDatas.size(); i++) {
					CreateLink(dest->linkDatas[i], dest->linkViewerDatas[i], dest->linkImNodesDatas[i]);
				}
				break;
			}
			case ATTRIUTE_SWAP:
			{
				nodeViewers[dest->targetedId]->CopyPerm(*dest->nodeViewerDatas[0]);
				break;
			}
			case NODE_UPDATE:
			{
				delete nodes[dest->nodeDatas[0]->GetId()];
				nodes[dest->nodeDatas[0]->GetId()] = new Node(*dest->nodeDatas[0]);

				std::cout << "new state 0 : " << nodes[dest->nodeDatas[0]->GetId()]->GetState(0) << std::endl;

				delete nodeViewers[dest->nodeDatas[0]->GetId()];
				nodeViewers[dest->nodeDatas[0]->GetId()] = new NodeViewer(*dest->nodeViewerDatas[0], nodes[dest->nodeDatas[0]->GetId()]);

				break;
			}
			case NODE_IO_UPDATE:
			{
				nodes[dest->nodeDatas[0]->GetId()]->Overide(*dest->nodeDatas[0]);
				break;
			}
			default:
				assert(false);
				break;
			}

		}
	}

	if (ImNodes::GetUnpopedEvent(&eventId)) {
		GraphEvent* dest;
		if (graphEvents.unpop(&dest)) {
			assert(*dest == eventId);
			std::cout << "Unoped event " << eventId << std::endl;
			switch (dest->type)
			{
			case CREATION:
			{
				for (int i = 0; i < dest->nodeDatas.size(); i++) {
					CreateNode(dest->nodeDatas[i], dest->nodeViewerDatas[i], dest->nodeImNodesDatas[i]);
				}
				for (int i = 0; i < dest->linkDatas.size(); i++) {
					CreateLink(dest->linkDatas[i], dest->linkViewerDatas[i], dest->linkImNodesDatas[i]);
				}
				break;
			}
			case DESTRUCTION:
			{
				if(dest->nodeDatas.size() > 0)
					DeleteNodes(ExtractIds(dest->nodeDatas));
				if (dest->linkDatas.size() > 0)
					DeleteLinks(ExtractIds(dest->linkDatas));
				break;
			}
			case ATTRIUTE_SWAP:
			{
				nodeViewers[dest->targetedId]->CopyPerm(*dest->nodeViewerDatas[1]);
				break;
			}
			case NODE_UPDATE:
			{
				delete nodes[dest->nodeDatas[1]->GetId()];
				nodes[dest->nodeDatas[1]->GetId()] = new Node(*dest->nodeDatas[1]);

				delete nodeViewers[dest->nodeDatas[1]->GetId()];
				nodeViewers[dest->nodeDatas[1]->GetId()] = new NodeViewer(*dest->nodeViewerDatas[1], nodes[dest->nodeDatas[1]->GetId()]);

				break;
			}
			case NODE_IO_UPDATE:
			{
				nodes[dest->nodeDatas[1]->GetId()]->Overide(*dest->nodeDatas[1]);
				break;
			}
			default:
				assert(false);
				break;
			}
		}
	}
}

json11::Json BluePrint::ToJson() const {
	json11::Json jsonBluePrint = json11::Json({
		{"nodes", MapToJson(nodes)},
		{"nodeViewers", MapToJson(nodeViewers)},
		{"links", MapToJson(links)},
		{"linkViewers", MapToJson(linkViewers)},
		{"name", name},
		{"dataBaseFile", dataBase->getFileName()},
		{"ui", ImNodes::SaveCurrentEditorStateToIniString()} });

	std::cout << "json of bluePrint : " << jsonBluePrint.dump() << std::endl;

	return jsonBluePrint;
}

void BluePrint::LoadJson(const json11::Json& json) {
	//TODO
	//initialize the Database with the adresse to the database given, in the json (if none, use a default adresse)
	const json11::Json::object obj = json.object_items();
	//name = obj.at("name").string_value().c_str();
	std::cout << "name should be : " << obj.at("name").string_value() << " | " << obj.at("name").string_value().c_str() << std::endl;
	nodes = JsonToMap<Node>(obj.at("nodes").array_items());
	links = JsonToMap<Link>(obj.at("links").array_items());


	nodeViewers = JsonToMap<Node, NodeViewer>(nodes, obj.at("nodeViewers").array_items(), dataBase, nodeUpdator);
	linkViewers = JsonToMap<Link, LinkViewer>(links, obj.at("linkViewers").array_items(), dataBase, linkUpdator);


	for (const auto& [key, node] : nodes) {
		idSeed = std::max(idSeed, node->GetId());
		const std::vector<NodeIO>& inputs = node->GetInputs();
		for (const NodeIO& nodeIO : node->GetInputs())
			idSeed = std::max(idSeed, nodeIO.GetId());
		for (const NodeIO& nodeIO : node->GetOutputs())
			idSeed = std::max(idSeed, nodeIO.GetId());
	}
	for (const auto& [key, link] : links)
		idSeed = std::max(idSeed, link->GetId());

	//TODO Store some ImNodes data to place back nodes and label at the same place
	std::string ui = obj.at("ui").string_value();
	ImNodes::LoadCurrentEditorStateFromIniString(ui.c_str(), ui.size());
}

void BluePrint::LoadDataBase(const json11::Json& json){
	//load items "ITEMS"
	//load machines "MACHINES"
	//load modifiers "MODIFIERS"
	//load modifiers categories "MODIFIER_CATEGORIES"
	//load recipes "RECIPES"


}

void BluePrint::clearStack() {
	graphEvents.clear();
	ImNodes::ClearEvent();
}