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

//create node from dataBase
Node::Node(const DataBase* dataBase, int machineId, int(*CreateId)()) : machineId(machineId) {
	if (dataBase->getMachine(machineId).recipiesId.size() == 0) {
		//special machine : merger or sorter or input or output
		id = CreateId();
		specialNode = true;
		return;
	}
	specialNode = false;
	const Recipe& recipe = dataBase->getRecipe(dataBase->getMachine(machineId).recipiesId[0]);

	state = std::vector<int>(recipe.modifierCategoriesId.size(), -1);
	std::vector<int> newState = std::vector<int>(recipe.modifierCategoriesId.size(), 0);

	id = CreateId();
	changeState(dataBase, newState, CreateId);
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

void Node::changeState(const DataBase* dataBase, const std::vector<int> newState, int(*CreateId)()) {
	const Recipe& recipe = dataBase->getRecipe(dataBase->getMachine(machineId).recipiesId[newState[0]]);

	if (newState[0] != state[0]) {
		inputs.clear();
		outputs.clear();
		for (int i = 0; i < recipe.inputsId.size(); i++)
			inputs.push_back(NodeIO(CreateId()));
		for (int i = 0; i < recipe.outputsId.size(); i++)
			outputs.push_back(NodeIO(CreateId()));
	}
	
	//Asume outputs and outputs already in place
	for (int i = 0; i < recipe.outputsId.size(); i++) {
		int itemId = recipe.outputsId[i].first;
		int itemQuantity = recipe.outputsId[i].second;
		outputs[i].quantity = itemQuantity;
		outputs[i].itemId = itemId;
	}
	for (int i = 0; i < recipe.inputsId.size(); i++) {
		int itemId = recipe.inputsId[i].first;
		int itemQuantity = recipe.inputsId[i].second;
		inputs[i].quantity = itemQuantity;
		inputs[i].itemId = itemId;
	}
	time = recipe.time;
	idlePower = 0.0;
	workingPower = 0.0;
	//name = recipe.name;

	
	for (int j = 0; j < recipe.modifierCategoriesId.size(); j++) {
		int modifierCategory = recipe.modifierCategoriesId[j];
		int modifierId = dataBase->getModifierCategory(modifierCategory).modifiersId[newState[j]];
		const Modifier& modifier = dataBase->getModifier(modifierId);

		idlePower += modifier.idlePower;
		workingPower += modifier.workingPower;
		time *= modifier.speedModifier;
		for(int i = 0; i < recipe.outputsId.size(); i++)
			outputs[i].quantity *= modifier.outputModifier;
	}
	state = std::vector<int>(newState);
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

static json11::Json VectorToJson(const std::vector<NodeIO>& v) {
	std::vector<json11::Json> json;
	for (const NodeIO& obj : v)
		json.push_back(obj.ToJson());
	return json11::Json(json);
}

static std::vector<NodeIO> JsonToVector(const json11::Json::array& json) {
	std::vector<NodeIO> arr;
	for (const auto& obj : json)
		arr.push_back(NodeIO(obj));
	return arr;
}

json11::Json Node::ToJson() const {
	return json11::Json({ {"inputs", VectorToJson(inputs)}, {"outputs", VectorToJson(outputs)}, {"id", id}, {"machineId", machineId} });
}

Node::Node(const json11::Json& json) {
	const json11::Json::object obj = json.object_items();
	inputs = JsonToVector(obj.at("inputs").array_items());
	outputs = JsonToVector(obj.at("outputs").array_items());
	id = obj.at("id").int_value();
	machineId = obj.at("machineId").int_value();
}

//============================== Viewer ==============================//


NodeViewer::NodeViewer(const Node* node, const DataBase* dataBase) : node(node), input_ref(), output_ref(), input_perm(), output_perm(), size(1.0f, 1.0f), dataBase(dataBase){
	Reset();
}

NodeViewer::NodeViewer(const NodeViewer& nodeViewer, const Node* node) : NodeViewer(nodeViewer) {
	NodeViewer::node = node;
	input_ref.clear();
	output_ref.clear();

	for (const NodeIO& nodeInput : node->GetInputs())
		input_ref.push_back(NodeIOViewer(&nodeInput, true, nodeViewer.dataBase));

	for (const NodeIO& nodeOutput : node->GetOutputs())
		output_ref.push_back(NodeIOViewer(&nodeOutput, false, nodeViewer.dataBase));

	//Reset();
}

void print_bits(char c) {
	for (int i = 0; i < 8; i++)
	{
		std::cout << ((c & 1) == 1) ? "1" : "0";
		c = c >> 1;
	}
}
#include <codecvt>

static std::string ImWchar2String(const ImWchar& c) {
	static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> convert;
	return convert.to_bytes({wchar_t(c), wchar_t(0)});
}

// View, only draw data
void NodeViewer::Draw() {
	if (input_ref.size() != node->GetInputs().size() || output_ref.size() != node->GetOutputs().size())
		Reset();
	ImNodes::BeginNode(GetId());
	//TODO Draw Title
	ImNodes::BeginNodeTitleBar();
	ImGui::Text("title\U000000ff\U00000100\U00000101\U00000102\U00000103");//should be recipe name
	ImGui::Text("%stitlééée\U00000061 \U00000101");//should be recipe name

	// 
	//wchar_t const* utf16_string = L"Hello, World!";
	wchar_t utf16_string[2];
	utf16_string[0] = 257;
	utf16_string[1] = 0;
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> convert;

	std::string utf8_string = convert.to_bytes(utf16_string);
	//const char* utf8_string_ = convert.to_bytes(utf16_string);

	wchar_t t = 101;
	ImGui::Text(("Proliferator : "+utf8_string).c_str());
	std::string test = "\U000001ff";
	const char* tmp = test.c_str();
	//std::cout << "TEST : " << unsigned short(tmp[0]) << ", " << unsigned short(tmp[1]) << std::endl;
	/*print_bits(tmp[0]);
	std::cout << ".";
	print_bits(tmp[1]);
	std::cout << std::endl;*/
	//\U00000100 -> 00100011.00000001 => 00100_011.000000_01 -> 001.00000000
	//\U00000101 -> 00100011.10000001 => 00100_011.100000_01 -> 001.00100000
	//\U00000102 -> 00100011.01000001 => 00100_011.010000_01
	//\U000001ff -> 11100011.11111101 => 11100_011.111111_01
	//should be clickable
	//257 = 256+1 = FF+01 = 101
	//97 = 0x61
	const char* items[] = { "\U00000100\U00000101\U00000102 AAAA\x61 aaa \xee\x01\x01", "\U00000101 BBBB", "CCCC", "DDDD", "EEEE", "FFFF", "GGGG", "HHHH", "IIII", "JJJJ", "KKKK", "LLLLLLL", "MMMM", "OOOOOOO"};
	static int item_selected_idx = 0; // Here we store our selection data as an index.

	// Pass in the preview value visible before opening the combo (it could technically be different contents or not pulled from items[])
	const char* combo_preview_value = items[item_selected_idx];

	if (ImGui::BeginCombo("##combo 1", combo_preview_value))
	{
		for (int n = 0; n < IM_ARRAYSIZE(items); n++)
		{
			const bool is_selected = (item_selected_idx == n);
			if (ImGui::Selectable(items[n], is_selected))
				item_selected_idx = n;
			ImGui::SameLine();
			ImGui::Text("test");

			// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
			if (is_selected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}
	ImGui::SameLine();
	ImGui::Text("test preview");

	ImNodes::EndNodeTitleBar();

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
		input_ref.push_back(NodeIOViewer(&nodeInput[i], true, dataBase));
	}

	for (int i = 0; i < nodeOutput.size(); i++) {
		output_perm.push_back(i);
		output_ref.push_back(NodeIOViewer(&nodeOutput[i], false, dataBase));
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

json11::Json NodeViewer::ToJson() const {
	return json11::Json({ 
		{"input_perm", input_perm}, 
		{"output_perm", output_perm}, 
		{"id", node->GetId()}, 
		{"size", json11::Json::array{ size.x, size.y }}});
}

static std::vector<int> JsonToVectorInt(const json11::Json::array& arr) {
	std::vector<int> v;
	for (const auto& a : arr)
		v.push_back(a.int_value());
	return v;
}

NodeViewer::NodeViewer(std::map<int, Node*>& nodes, const json11::Json& json, const DataBase* dataBase) : node(nodes.at(json.object_items().at("id").int_value())), dataBase(dataBase) {
	const json11::Json::object obj = json.object_items();

	input_perm = JsonToVectorInt(obj.at("input_perm").array_items());
	output_perm = JsonToVectorInt(obj.at("output_perm").array_items());

	size = ImVec2(obj.at("size").array_items()[0].number_value(), obj.at("size").array_items()[1].number_value());

	const std::vector<NodeIO>& nodeInput = node->GetInputs();
	const std::vector<NodeIO>& nodeOutput = node->GetOutputs();
	for (int i = 0; i < nodeInput.size(); i++)
		input_ref.push_back(NodeIOViewer(&nodeInput[i], true, dataBase));
	for (int i = 0; i < nodeOutput.size(); i++)
		output_ref.push_back(NodeIOViewer(&nodeOutput[i], false, dataBase));
}