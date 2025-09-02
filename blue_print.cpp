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
static const std::vector<Node> createRecipies() {
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

BluePrint::BluePrint(const char* name) : name(name), ioPanel(), nodes(), nodeViewers(), links(), linkViewers(), recipies(createRecipies()), swapingNodeViewerId(-1) {
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
}

void BluePrint::Draw() const {
	ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;
	ImGui::Begin(name, NULL, flags);

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
	Node* node = new Node(BluePrint::recipies[type], CreateId);
	ImNodes::SetNodeScreenSpacePos(node->GetId(), ImGui::GetIO().MousePos);
	nodes.insert(std::make_pair(node->GetId(), node));
	NodeViewer* nodeViewer = new NodeViewer(node);
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
	LinkViewer* new_linkViewer = new LinkViewer(link);
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
	if (nodeCreateType != -1)
		ToJson();
	
	int node_swap_recipy;
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
				nodeViewers[dest->swapedNodeViewerId]->CopyPerm(*dest->nodeViewerDatas[0]);
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
				nodeViewers[dest->swapedNodeViewerId]->CopyPerm(*dest->nodeViewerDatas[1]);
				break;
			}
			default:
				assert(false);
				break;
			}
		}
	}
}

std::string BluePrint::ToJson() const {
	json11::Json m_json = json11::Json(42);
	std::cout << "json dump : " << m_json.dump() << std::endl;
	return m_json.dump();
}