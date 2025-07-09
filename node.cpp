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


NodeViewer::NodeViewer(const Node* node) : node(node), input_ref(), output_ref(), input_perm(), output_perm(), size(1.0f, 1.0f) {
	Reset();
}

NodeViewer::NodeViewer(const NodeViewer& nodeViewer, const Node* node) : NodeViewer(nodeViewer) {
	NodeViewer::node = node;
	input_ref.clear();
	output_ref.clear();

	for (const NodeIO& nodeInput : node->GetInputs())
		input_ref.push_back(NodeIOViewer(&nodeInput, true));

	for (const NodeIO& nodeOutput : node->GetOutputs())
		output_ref.push_back(NodeIOViewer(&nodeOutput, false));

	//Reset();
}

// View, only draw data
void NodeViewer::Draw() {
	if (input_ref.size() != node->GetInputs().size() || output_ref.size() != node->GetOutputs().size())
		Reset();
	ImNodes::BeginNode(GetId());
	//TODO Draw Title
	//ImNodes::BeginNodeTitleBar();
	//ImGui::Text("title");
	//ImNodes::EndNodeTitleBar();

	float width_input = 0;
	float width_output = 0;
	float height_column = 0;
	float height_total = 0;

	if (ImGui::BeginTable("table1", 2, ImGuiTableFlags_SizingFixedFit, size))
	{
		for (int i = 0; i < std::max(input_perm.size(), output_perm.size()); i++) {
			ImGui::TableNextRow();
			if (input_perm.size() > i) {
				ImGui::TableSetColumnIndex(0);
				input_ref[input_perm[i]].Draw();

				ImVec2 r = ImGui::GetItemRectSize();
				width_input = std::max(width_input, r.x);
				height_column = std::max(height_column, r.y);
			}

			if (output_ref.size() > i) {
				ImGui::TableSetColumnIndex(1);
				output_ref[output_perm[i]].Draw();

				ImVec2 r = ImGui::GetItemRectSize();
				width_output = std::max(width_output, r.x);
				height_column = std::max(height_column, r.y);
			}
			height_total += height_column;
			height_column = 0;
		}

		ImGui::EndTable();
		size = ImVec2(width_input + width_output+ ImGui::GetStyle().CellPadding.x, height_total);
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

bool NodeViewer::Contain(int attr_id) {
	for (const NodeIOViewer& nodeIOViewer : input_ref)
		if (nodeIOViewer.GetId() == attr_id)
			return true;

	for (const NodeIOViewer& nodeIOViewer : output_ref)
		if (nodeIOViewer.GetId() == attr_id)
			return true;

	return false;
}

bool NodeViewer::SwapIO(int id1, int id2) {
	int index1 = FindIndex(input_ref, input_perm, id1);
	int index2 = -1;

	if (index1 != -1) {
		index2 = FindIndex(input_ref, input_perm, id2);
		assert(index2 != -1);
		swap(input_perm, index1, index2);
		return true;
	}
	index1 = FindIndex(output_ref, output_perm, id1);
	if (index1 != -1) {
		index2 = FindIndex(output_ref, output_perm, id2);
		assert(index2 != -1);
		swap(output_perm, index1, index2);
		return true;
	}
	return false;
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

void NodeViewer::CopyPerm(const NodeViewer& other) {
	assert(input_perm.size() == other.input_perm.size());
	assert(output_perm.size() == other.output_perm.size());

	for (int i = 0; i < input_perm.size(); i++)
		input_perm[i] = other.input_perm[i];

	for (int i = 0; i < output_perm.size(); i++)
		output_perm[i] = other.output_perm[i];
}

bool NodeViewer::operator==(const NodeViewer& other) const {
	if (input_perm.size() != other.input_perm.size() || output_perm.size() != other.output_perm.size())
		return false;

	for (int i = 0; i < input_perm.size(); i++)
		if (input_perm[i] != other.input_perm[i])
			return false;

	for (int i = 0; i < output_perm.size(); i++)
		if (output_perm[i] != other.output_perm[i])
			return false;

	return true;
}

bool NodeViewer::operator!=(const NodeViewer& other) const {
	return !operator==(other);
}