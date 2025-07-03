#include "node.h"
#include "imnodes.h"
#include <utility>
#include <iostream>

Node::Node() : id() {

}

Node::Node(int id) : id(id) {

}

//copy a node
Node::Node(const Node& node) : id(node.id), inputs(node.inputs), outputs(node.outputs) {

}

//copy a node with brand new ids
Node::Node(const Node& node, int(*CreateId)() ) : Node(node) {
	id = CreateId();
	SetIOIds(CreateId);
}

//change all node parameter to mimic a targeted node. Don't change Node::id
void Node::Overide(const Node& node, int(*CreateId)()) {
	inputs = std::vector<NodeIO>(node.GetInputs());
	outputs = std::vector<NodeIO>(node.GetOutputs());
	SetIOIds(CreateId);
}

void Node::SetIOIds(int(*CreateId)()) {
	for (NodeIO& input : inputs)
		input.SetId(CreateId());

	for (NodeIO& output : outputs)
		output.SetId(CreateId());
}

void Node::Update() {
	//TODO overide nodes if needed
}

int Node::GetId() const {
	return id;
}

const std::vector<NodeIO>& Node::GetInputs() const {
	return inputs;
}

const std::vector<NodeIO>& Node::GetOutputs() const {
	return outputs;
}

void Node::AddInputs(NodeIO nodeIO) {
	inputs.push_back(nodeIO);
}

void Node::AddOutputs(NodeIO nodeIO) {
	outputs.push_back(nodeIO);
}


//============================== Viewer ==============================//


NodeViewer::NodeViewer(const Node* node) : node(node), input_ref(), output_ref(), input_perm(), output_perm() {
	Reset();
}

// View, only draw data
void NodeViewer::Draw() {
	if (input_ref.size() != node->GetInputs().size() || output_ref.size() != node->GetOutputs().size())
		Reset();

	ImNodes::BeginNode(GetId());
	//TODO Draw Title
	for (int i = 0; i < std::max(input_perm.size(), output_perm.size()); i++) {
		if (input_perm.size() > i)
			input_ref[input_perm[i]].Draw();			

		if (input_perm.size() > i && output_ref.size() > i)
			ImGui::SameLine();

		if (output_ref.size() > i)
			output_ref[output_perm[i]].Draw();
	}
	ImNodes::EndNode();	
}

static void swap(std::vector<int>& v, int index1, int index2) {
	int tmp = v[index1];
	v[index1] = v[index2];
	v[index2] = tmp;
}

static int FindIndex(const std::vector<NodeIOViewer>& nodeios, const std::vector<int>& permutation, int id) {
	for (int i = 0; i < permutation.size(); i++) {
		if ((nodeios[permutation[i]]).GetId() == id)
			return i;
	}
	return -1;
}

void NodeViewer::SwapIO(int id1, int id2) {
	int index1 = FindIndex(input_ref, input_perm, id1);
	int index2 = -1;

	if (index1 != -1) {
		index2 = FindIndex(input_ref, input_perm, id2);
		assert(index2 != -1);
		swap(input_perm, index1, index2);
		return;
	}
	index1 = FindIndex(output_ref, output_perm, id1);
	if (index1 != -1) {
		index2 = FindIndex(output_ref, output_perm, id2);
		assert(index2 != -1);
		swap(output_perm, index1, index2);
	}
}

void NodeViewer::Reset() {
	input_ref.clear();
	input_perm.clear();
	output_ref.clear();
	output_perm.clear();
	const std::vector<NodeIO>& nodeInput = node->GetInputs();
	const std::vector<NodeIO>& nodeOutput = node->GetOutputs();

	for (int i = 0; i < nodeInput.size(); i++) {
		input_perm.push_back(i);
		input_ref.push_back(NodeIOViewer(&nodeInput[i], true));
	}

	for (int i = 0; i < nodeOutput.size(); i++) {
		output_perm.push_back(i);
		output_ref.push_back(NodeIOViewer(&nodeOutput[i], false));
	}
}