#include "node_editor.h"
#include <imnodes.h>
#include <imgui.h>
#include <iostream>
#include <vector>
#include "blue_print.h"
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

BluePrint::BluePrint(const char* name) : name(name), ioPanel(), nodes(), nodeViewers(), links(), linkViewers(), recipies(createRecipies()) {
}

#include <map>

BluePrint::~BluePrint() {
	for (const auto& [id, node] : nodes)
		delete node;
	for (const auto& [id, nodeViewer] : nodeViewers)
		delete nodeViewer;

	/*for (auto node : nodes)
		delete node;

	for (auto nodeViewer : nodeViewers)
		delete nodeViewer;*/
}

void BluePrint::Draw() const {
	ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;
	ImGui::Begin(name, NULL, flags);

	ImNodes::BeginNodeEditor();

	/*for (NodeViewer* nodeViewer : nodeViewers)
		nodeViewer->Draw();

	for (const LinkViewer* linkViewer : linkViewers)
		linkViewer->Draw();*/

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
	//nodes.push_back(node);
	nodes.insert(std::make_pair(node->GetId(), node));
	NodeViewer* nodeViewer = new NodeViewer(node);
	//nodeViewers.push_back(nodeViewer);
	nodeViewers.insert(std::make_pair(nodeViewer->GetId(), nodeViewer));

	graphEvents.push(CreationEvent(CreateId(), node, nodeViewer, ImNodes::GetNodeData(node->GetId())));

	return node->GetId();
}

int BluePrint::CreateNewNode(Node* node, NodeViewer* nodeViewer, ImNodeData* nodeData) {
	Node* new_node = new Node(*node);
	ImNodes::SetNodeData(new_node->GetId(), nodeData);
	//nodes.push_back(new_node);
	nodes.insert(std::make_pair(node->GetId(), node));
	//nodeViewers.push_back(new NodeViewer(*nodeViewer, new_node));
	NodeViewer* new_nodeViewer = new NodeViewer(*nodeViewer, new_node);
	nodeViewers.insert(std::make_pair(new_nodeViewer->GetId(), new_nodeViewer));



	//WARNING TODO Be carefull when undoing node deletion (deletin them a second time) need to NOT RECREATE EVENT, bu need to DELETE for real the node

	return node->GetId();
}

void BluePrint::DeleteNewNode(int nodeId) {
	//TODO delete assosciated links

}

void BluePrint::Update() {

	int nodeCreateType = -1;
	ioPanel.Update(nodeCreateType);
	if (nodeCreateType != -1) {
		CreateNewNode(nodeCreateType);
		/*Node* node = new Node(BluePrint::recipies[nodeCreateType], CreateId);
		ImNodes::SetNodeScreenSpacePos(node->GetId(), ImGui::GetIO().MousePos);
		nodes.push_back(node);
		nodeViewers.push_back(new NodeViewer(node));*/

		graphEvents.push(CreationEvent(42));
	}

	if (ImGui::IsKeyPressed(ImGuiKey_O)) {
		GraphEvent dest;
		if (graphEvents.pop(&dest)) {
			std::cout << "poped" << std::endl;
		}
	}

	int node_swap_recipy;
	//id of node, recipe target
		

	int start_attr, end_attr;
	if (ImNodes::IsLinkCreated(&start_attr, &end_attr))
	{
		Link* link = new Link(CreateId(), start_attr, end_attr);
		//links.push_back(link);
		//linkViewers.push_back(new LinkViewer(link));

		links.insert(std::make_pair(link->GetId(), link));
		LinkViewer* new_linkViewer = new LinkViewer(link);
		linkViewers.insert(std::make_pair(new_linkViewer->GetId(), new_linkViewer));
	}

	int link_id;
	if (ImNodes::IsLinkDestroyed(&link_id)) {

		delete linkViewers[link_id];
		delete links[link_id];

		linkViewers.erase(link_id);
		links.erase(link_id);
		//for (int i = 0; i < links.size(); i++) {
		//	if (links[i]->GetId() == link_id) {
		//		delete linkViewers[i];
		//		delete links[i];
		//		linkViewers.erase(linkViewers.begin() + i);
		//		links.erase(links.begin() + i);
		//		//TODO Only destroy the viewer, keep the node until the related events are themself deleted
		//		break;
		//	}
		//}
	}

	if (ImGui::IsKeyPressed(ImGuiKey_Delete)) {
		//TODO find all selected link and delet them
		// find all node, and all attached link and delete them
	}

	int src_attr, dest_attr;
	if (ImNodes::IsAttributeSwapped(&src_attr, &dest_attr))
	{
		/*for (NodeViewer* nodeViewer : nodeViewers) {
			if(nodeViewer->SwapIO(src_attr, dest_attr))
				break;
		}*/
		for (const auto& [id, nodeViewer] : nodeViewers) {
			if (nodeViewer->SwapIO(src_attr, dest_attr))
				break;
		}
	}

	int eventId;
	if (ImNodes::GetPopedEvent(&eventId)) {
		GraphEvent dest;
		if (graphEvents.pop(&dest)) {
			assert(dest == eventId);
			//TODO


		}
	}

	if (ImNodes::GetUnpopedEvent(&eventId)) {
		GraphEvent dest;
		if (graphEvents.unpop(&dest)) {
			assert(dest == eventId);
			//TODO
		}
	}
}