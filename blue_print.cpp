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

BluePrint::BluePrint(const char* name) : idSeed(0), name(name), ioPanel(), nodes(), nodeViewers(), links(), linkViewers() {
	//TODO Change this setup for a json reading
	//PLACEHOLDER
	Node* node = new Node(CreateId());
	node->AddInputs(NodeIO(CreateId(), 42));
	node->AddInputs(NodeIO(CreateId(), 2));
	node->AddInputs(NodeIO(CreateId(), 3));

	node->AddOutputs(NodeIO(CreateId(), 50));
	node->AddOutputs(NodeIO(CreateId(), 1));

	nodes.push_back(node);
	nodeViewers.push_back(new NodeViewer(node));
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

void BluePrint::Update() {

	int nodeCreateType = -1;
	ioPanel.Update(nodeCreateType);
	if (nodeCreateType != -1) {
		std::cout << "ask to create " << nodeCreateType << std::endl;

		//TODO 
		// create a static array of all possible empty nodes
		// (smelter, science, splitter, merger, input, output, etc
		// add the corrsponding empty node to the node list

		Node* node = new Node(CreateId());
		node->AddInputs(NodeIO(CreateId(), 2));
		node->AddOutputs(NodeIO(CreateId(), 1));

		nodes.push_back(node);
		nodeViewers.push_back(new NodeViewer(node));
	}
		

	int start_attr, end_attr;
	if (ImNodes::IsLinkCreated(&start_attr, &end_attr))
	{
		Link* link = new Link(CreateId(), start_attr, end_attr);
		links.push_back(link);
		linkViewers.push_back(new LinkViewer(link));
	}
}