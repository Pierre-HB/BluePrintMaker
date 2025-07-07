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

BluePrint::~BluePrint() {
	for (auto node : nodes)
		delete node;

	for (auto nodeViewer : nodeViewers)
		delete nodeViewer;
}

void BluePrint::Draw() const {
	ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;
	ImGui::Begin(name, NULL, flags);

	ImNodes::BeginNodeEditor();

	for (NodeViewer* nodeViewer : nodeViewers)
		nodeViewer->Draw();

	for (const LinkViewer* linkViewer : linkViewers)
		linkViewer->Draw();


	ImNodes::EndNodeEditor();

	ioPanel.Draw();

	ImGui::End();
}

int BluePrint::CreateNewNode(int type) {
	Node* node = new Node(BluePrint::recipies[type], CreateId);
	ImNodes::SetNodeScreenSpacePos(node->GetId(), ImGui::GetIO().MousePos);
	nodes.push_back(node);
	NodeViewer* nodeViewer = new NodeViewer(node);
	nodeViewers.push_back(nodeViewer);

	int eventId = CreateId();
	graphEvents.push(GraphEvent(eventId, CREATION, *node, *nodeViewer)); //Copy node and nodeViewer in an event
	ImNodes::PushEvent(eventId);

	return node->GetId();
}

int BluePrint::CreateNode(Node* node, NodeViewer* nodeViewer, ImNodeData* nodeData) {
	Node* new_node = new Node(*node);
	ImNodes::SetNodeData(new_node->GetId(), nodeData);
	nodes.push_back(new_node);
	nodeViewers.push_back(new NodeViewer(*nodeViewer, new_node));



	//WARNING TODO Be carefull when undoing node deletion (deletin them a second time) need to NOT RECREATE EVENT, bu need to DELETE for real the node

	return new_node->GetId();
}

void BluePrint::DeleteNode(int nodeId, GraphEvent* Event) {
	//TODO delete assosciated links
	int nodePtx = -1;
	int nodeViewerPtx = -1;
	for (int i = 0; i < nodes.size(); i++) {
		if (nodes[i]->GetId() == nodeId) {
			nodePtx = i;
			break;
		}
	}

	for (int i = 0; i < nodeViewers.size(); i++) {
		if (nodeViewers[i]->GetId() == nodeId) {
			nodeViewerPtx = i;
			break;			
		}
	}
	assert(nodePtx != -1);
	assert(nodeViewerPtx != -1);

	if (Event != nullptr) {
		Event->Push_Node(nodes[nodePtx], nodeViewers[nodeViewerPtx]);
	}
	else {
		delete nodes[nodePtx];
		delete nodeViewers[nodeViewerPtx];
	}
	
	nodes.erase(nodes.begin() + nodePtx);
	nodeViewers.erase(nodeViewers.begin() + nodeViewerPtx);
}

void BluePrint::Update() {

	int nodeCreateType = -1;
	ioPanel.Update(nodeCreateType);
	if (nodeCreateType != -1)
		CreateNewNode(nodeCreateType);
	
	int node_swap_recipy;
	//id of node, recipe target
		

	int start_attr, end_attr;
	if (ImNodes::IsLinkCreated(&start_attr, &end_attr))
	{
		Link* link = new Link(CreateId(), start_attr, end_attr);
		links.push_back(link);
		linkViewers.push_back(new LinkViewer(link));
	}

	int link_id;
	if (ImNodes::IsLinkDestroyed(&link_id)) {
		for (int i = 0; i < links.size(); i++) {
			if (links[i]->GetId() == link_id) {
				delete linkViewers[i];
				delete links[i];
				linkViewers.erase(linkViewers.begin() + i);
				links.erase(links.begin() + i);
				//TODO Only destroy the viewer, keep the node until the related events are themself deleted
				break;
			}
		}
	}

	if (ImGui::IsKeyPressed(ImGuiKey_Delete)) {
		//TODO find all selected link and delet them
		// find all node, and all attached link and delete them
		int nb_selected_node = ImNodes::NumSelectedNodes();
		std::cout << "deletion of " << nb_selected_node << " nodes" << std::endl;
		
		if (nb_selected_node > 0) {
			int eventId = CreateId();
			GraphEvent Event(eventId, DESTRUCTION);

			std::vector<int> selected_node_ids = std::vector<int>(nb_selected_node);
			ImNodes::GetSelectedNodes(selected_node_ids.data());

			for(int nodeId : selected_node_ids)
				DeleteNode(nodeId, &Event);

			graphEvents.push(std::move(Event));
			ImNodes::PushEvent(eventId);
		}	
	}

	int src_attr, dest_attr;
	if (ImNodes::IsAttributeSwapped(&src_attr, &dest_attr))
	{
		for (NodeViewer* nodeViewer : nodeViewers) {
			if(nodeViewer->SwapIO(src_attr, dest_attr))
				break;
		}
	}

	int eventId;
	if (ImNodes::GetPopedEvent(&eventId)) {
		GraphEvent* dest;
		if (graphEvents.pop(&dest)) {
			assert(*dest == eventId);
			//TODO for link and label
			std::cout << "Poped event " << eventId << std::endl;
			switch (dest->type)
			{
			case CREATION:
			{
				for (Node* node : dest->nodeDatas)
					DeleteNode(node->GetId(), false);
				break;
			}
			case DESTRUCTION:
			{
				for (int i = 0; i < dest->nodeDatas.size(); i++) {
					CreateNode(dest->nodeDatas[i], dest->nodeViewerDatas[i], dest->nodeImNodesDatas[i]);
				}
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
			//TODO do for link and label
			std::cout << "Unoped event " << eventId << std::endl;
			switch (dest->type)
			{
			case CREATION:
			{
				for (int i = 0; i < dest->nodeDatas.size(); i++) {
					CreateNode(dest->nodeDatas[i], dest->nodeViewerDatas[i], dest->nodeImNodesDatas[i]);
				}
				break;
			}
			case DESTRUCTION:
			{
				for (Node* node : dest->nodeDatas)
					DeleteNode(node->GetId(), false);
				break;
			}
			default:
				assert(false);
				break;
			}
		}
	}
}