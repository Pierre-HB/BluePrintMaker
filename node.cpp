#include "node.h"
#include "imnodes.h"
#include <utility>
#include <iostream>

NODE_IO_TYPE int2NodeIOType(int i){
	static const NODE_IO_TYPE types[4] = { ITEM , IO , LOCK_IO , SPLITTER };
	return types[i];
}
void NodeIOViewer::Draw() {
	if (isInput)
		ImNodes::BeginInputAttribute(GetId());
	else
		ImNodes::BeginOutputAttribute(GetId());
	const Item& item = dataBase->getItem(nodeIO->itemId);

	//static bool guess = true;
	static bool guess;//static memory slot for imgui input
	static int tmp;
	tmp = int(nodeIO->quantity);
	switch (nodeIO->type)
	{
	case NODE_IO_TYPE::ITEM:
		ImGui::Text((item.iconeString + " " + item.name + std::format(" {}", nodeIO->quantity)).c_str());
		break;
	case NODE_IO_TYPE::LOCK_IO:
		guess = true;
		
		ImGui::Checkbox("Guess", &guess);

		ImGui::BeginDisabled();
		ImGui::InputInt("##throuput", &tmp, 0, 0);
		ImGui::EndDisabled();
		ImGui::SameLine();
		ImGui::Text((item.iconeString + " " + item.name).c_str());

		if (!guess)
			nodeUpdator->SetNodeIOState(NODE_IO_TYPE::IO, nodeIO->GetId());
		break;
	case NODE_IO_TYPE::IO:
		guess = false;
		ImGui::Checkbox("Guess", &guess);
		ImGui::InputInt("##throuput", &tmp, 0, 0);

		if (ImGui::IsItemDeactivatedAfterEdit() && tmp != int(nodeIO->quantity) && !ImGui::IsKeyPressed(ImGuiKey_Escape))
			nodeUpdator->SetNodeIOQuantity(tmp, nodeIO->GetId());
		
		ImGui::SameLine();
		ImGui::Text((item.iconeString + " " + item.name).c_str());
		if (guess)
			nodeUpdator->SetNodeIOState(NODE_IO_TYPE::LOCK_IO, nodeIO->GetId());
		break;
	case NODE_IO_TYPE::SPLITTER:
		IM_ASSERT(false && "TODO");
	}

	if (isInput)
		ImNodes::EndInputAttribute();
	else
		ImNodes::EndOutputAttribute();
}


Node::Node() : id() {

}

Node::Node(int id) : id(id) {

}

//copy a node
Node::Node(const Node& node) : id(node.id), inputs(node.inputs), outputs(node.outputs), machineId(node.machineId), time(node.time), idlePower(node.idlePower), workingPower(node.workingPower), state(node.state), specialNode(node.specialNode) {

}

//copy a node with brand new ids
Node::Node(const Node& node, int(*CreateId)() ) : Node(node) {
	id = CreateId();
	SetIOIds(CreateId);
}

//create node from dataBase
Node::Node(const DataBase* dataBase, int machineId, int(*CreateId)()) : machineId(machineId) {
	const Machine& machine = dataBase->getMachine(machineId);
	switch (machine.type)
	{
	
	case MACHINE_REGULAR:
		InitNodeAsRegular(CreateId, dataBase);
		return;
	case MACHINE_INPUT:
		InitNodeAsIO(CreateId, dataBase, true);
		return;
	case MACHINE_OUTPUT:
		InitNodeAsIO(CreateId, dataBase, false);
		return;

	case MACHINE_SPLITTER:
	case MACHINE_MERGER:
	case MACHINE_BLACKBOX:
		IM_ASSERT(false && "TODO");
	}
	if (machine.recipiesId.size() == 0) {
		//special machine : merger or sorter or input or output
		id = CreateId();
		specialNode = true;
		InitNodeAsIO(CreateId, dataBase, true);
		//Database need to create INPOUT archetype ?
		//I whant 
		//if dataBase->GetMachineNode() == REGULAR/INPUT/OUTPUT/SPLITTER

		//IF input, create special IONode containing 'allItem' item
		//can specified throuput of item (item/min) or lock this number
		//NO recipe list, automatic item deduction
		//No title/footer
		//WIll lock 
		return;
	}
	specialNode = false;
	state = std::vector<int>(1, 0);
	id = CreateId();
	ChangeState(dataBase, 0, 0, CreateId);
}

//change all node parameter to mimic a targeted node. Don't change Node::id
void Node::Overide(const Node& node, int(*CreateId)()) {
	inputs = std::vector<NodeIO>(node.GetInputs());
	outputs = std::vector<NodeIO>(node.GetOutputs());
	SetIOIds(CreateId);
}
//change all node parameter to mimic a targeted node. Don't change Node::id or NodeIO::id
void Node::Overide(const Node& node) {
	for (int i = 0; i < inputs.size(); i++)
		inputs[i].Overide(node.inputs[i]);
	for (int i = 0; i < outputs.size(); i++)
		outputs[i].Overide(node.outputs[i]);

	//inputs = std::vector<NodeIO>(node.GetInputs());
	//outputs = std::vector<NodeIO>(node.GetOutputs());
	machineId = node.machineId;
	time = node.time;
	idlePower = node.idlePower;
	workingPower = node.workingPower;
	state = std::vector<int>(node.state);
	specialNode = node.specialNode;
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

//changeNodeIOState ?
void Node::ChangeState(const DataBase* dataBase, int stateChannel, int newState, int(*CreateId)()) {
	state[stateChannel] = newState;
	const Recipe& recipe = dataBase->getRecipe(dataBase->getMachine(machineId).recipiesId[state[0]]);
	if (stateChannel == 0) {
		//changed recipe, need to reset the entire node
		inputs.clear();
		outputs.clear();
		for (int i = 0; i < recipe.inputsId.size(); i++)
			inputs.push_back(NodeIO(CreateId()));
		for (int i = 0; i < recipe.outputsId.size(); i++)
			outputs.push_back(NodeIO(CreateId()));
		state.clear();
		state.push_back(newState);
		for (int i = 0; i < recipe.modifierCategoriesId.size(); i++)
			state.push_back(0);
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
		int modifierId = dataBase->getModifierCategory(modifierCategory).modifiersId[state[j+1]];
		const Modifier& modifier = dataBase->getModifier(modifierId);

		idlePower += modifier.idlePower;
		workingPower += modifier.workingPower;
		time *= modifier.speedModifier;
		for(int i = 0; i < recipe.outputsId.size(); i++)
			outputs[i].quantity *= modifier.outputModifier;
		std::cout << "Output quantity modifier : " << modifier.outputModifier << std::endl;
	}
}

int Node::GetId() const {
	return id;
}

int Node::GetMachineId() const {
	return machineId;
}

float Node::GetTime() const {
	return time;
}

int Node::GetState(int i) const {
	return state[i];
}

int Node::GetStateSize() const {
	return state.size();
}

bool Node::GetSpecial() const {
	return specialNode;
}

const std::vector<NodeIO>& Node::GetInputs() const {
	return inputs;
}

const std::vector<NodeIO>& Node::GetOutputs() const {
	return outputs;
}

void Node::UpdateNodeIOData(int nodeIOId, float newData) {
	for (NodeIO& nodeIO : inputs)
		if (nodeIO.GetId() == nodeIOId)
			nodeIO.quantity = newData;
	for (NodeIO& nodeIO : outputs)
		if (nodeIO.GetId() == nodeIOId)
			nodeIO.quantity = newData;
}

void Node::UpdateNodeIOType(int nodeIOId, NODE_IO_TYPE type) {
	for (NodeIO& nodeIO : inputs)
		if (nodeIO.GetId() == nodeIOId)
			nodeIO.type = type;
	for (NodeIO& nodeIO : outputs)
		if (nodeIO.GetId() == nodeIOId)
			nodeIO.type = type;
}

void Node::InitNodeAsIO(int(*CreateId)(), const DataBase* dataBase, bool input) {
	id = CreateId();
	if(input)
	{
		//outputs.push_back(NodeIO(CreateId(), 0, 360, NODE_IO_TYPE::LOCK_IO));
		outputs.push_back(NodeIO(CreateId(), 0, 360, NODE_IO_TYPE::IO));
	}
	else
	{
		inputs.push_back(NodeIO(CreateId(), 0, 360, NODE_IO_TYPE::LOCK_IO));
	}
}

void Node::InitNodeAsRegular(int(*CreateId)(), const DataBase* dataBase) {
	specialNode = false;
	state = std::vector<int>(1, 0);
	id = CreateId();
	ChangeState(dataBase, 0, 0, CreateId);
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

static std::vector<int> JsonToVectorInt(const json11::Json::array& arr) {
	std::vector<int> v;
	for (const auto& a : arr)
		v.push_back(a.int_value());
	return v;
}

json11::Json Node::ToJson() const {
	return json11::Json({ 
		{"inputs", VectorToJson(inputs)},
		{"outputs", VectorToJson(outputs)},
		{"id", id},
		{"machineId", machineId},
		{"time", time},
		{"idlePower", idlePower},
		{"workingPower", workingPower},
		{"state", state},
		{"specialNode", specialNode} });
}

Node::Node(const json11::Json& json) {
	const json11::Json::object obj = json.object_items();
	inputs = JsonToVector(obj.at("inputs").array_items());
	outputs = JsonToVector(obj.at("outputs").array_items());
	state = JsonToVectorInt(obj.at("state").array_items());
	id = obj.at("id").int_value();
	machineId = obj.at("machineId").int_value();
	time = obj.at("time").number_value();
	idlePower = obj.at("idlePower").int_value();
	workingPower = obj.at("workingPower").int_value();
	specialNode = obj.at("specialNode").bool_value();
}

//============================== Viewer ==============================//


NodeViewer::NodeViewer(const Node* node, const DataBase* dataBase, NodeUpdator* nodeUpdator) : node(node), input_ref(), output_ref(), input_perm(), output_perm(), size(1.0f, 1.0f), dataBase(dataBase), nodeUpdator(nodeUpdator){
	Reset();
}

NodeViewer::NodeViewer(const NodeViewer& nodeViewer, const Node* node) : NodeViewer(nodeViewer) {
	NodeViewer::node = node;
	input_ref.clear();
	output_ref.clear();

	for (const NodeIO& nodeInput : node->GetInputs())
		input_ref.push_back(NodeIOViewer(&nodeInput, true, nodeViewer.dataBase, nodeViewer.nodeUpdator));

	for (const NodeIO& nodeOutput : node->GetOutputs())
		output_ref.push_back(NodeIOViewer(&nodeOutput, false, nodeViewer.dataBase, nodeViewer.nodeUpdator));

	//Reset();
}

// View, only draw data
void NodeViewer::Draw() {
	if (node->GetSpecial())
		DrawInput();
	else
		DrawMachine();
}

void NodeViewer::DrawMachineTitle() {
	ImNodes::BeginNodeTitleBar();

	const Machine& machine = dataBase->getMachine(node->GetMachineId());
	const Recipe& recipe = dataBase->getRecipe(machine.recipiesId[node->GetState(0)]);

	int newRecipe = node->GetState(0);
	if (ImGui::BeginCombo("##combo 1", machine.recipeNames[newRecipe].c_str(), ImGuiComboFlags_WidthFitPreview))
	{
		for (int n = 0; n < machine.recipeNames.size(); n++)
		{
			const bool is_selected = (newRecipe == n);
			if (ImGui::Selectable(machine.recipeNames[n].c_str(), is_selected))
				newRecipe = n;

			if (is_selected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
		if (newRecipe != node->GetState(0)) 
			nodeUpdator->SetNodeNewState(0, newRecipe, node->GetId());
	}

	const int nbState = node->GetStateSize() - 1;
	for (int i = 0; i < nbState; i++) {
		ImGui::SameLine();

		int newState = node->GetState(i + 1);
		char comboName[16];

		sprintf(comboName, "##combo -%i", i);
		if (ImGui::BeginCombo(comboName, recipe.modifierNames[i][newState].c_str(), ImGuiComboFlags_WidthFitPreview))
		{
			for (int n = 0; n < recipe.modifierNames[i].size(); n++)
			{
				const bool is_selected = (newState == n);
				if (ImGui::Selectable(recipe.modifierNames[i][n].c_str(), is_selected))
					newState = n;

				if (is_selected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
			if (newState != node->GetState(i + 1))
				nodeUpdator->SetNodeNewState(i+1, newState, node->GetId());
		}
	}

	ImNodes::EndNodeTitleBar();
}

void NodeViewer::DrawMachineContent() {
	float width_input = 0;
	float width_output = 0;
	float height_column = 0;
	float height_total = 0;

	//g.Style.CellPadding.x


	//ImGuiTableFlags_NoPadInnerX
		//ImGuiTableFlags_SizingFixedFit
	int nb_col = 2;
	if (input_perm.size() + output_perm.size() == 1)
		nb_col = 1;
	if (ImGui::BeginTable("table1", nb_col, ImGuiTableFlags_NoPadInnerX, size))
	{
		for (int i = 0; i < std::max(input_perm.size(), output_perm.size()); i++) {
			ImGui::TableNextRow();
			if (input_perm.size() > i) {
				ImGui::TableSetColumnIndex(0);
				if(input_perm.size() == 1)
					ImNodes::PushAttributeFlag(ImNodesStyleFlags_AttributeSwappable, false);
				input_ref[input_perm[i]].Draw();
				if (input_perm.size() == 1)
					ImNodes::PopAttributeFlag();

				ImVec2 r = ImGui::GetItemRectSize();
				width_input = std::max(width_input, r.x);
				height_column = std::max(height_column, r.y);
			}

			if (output_perm.size() > i) {
				if(nb_col == 2)
					ImGui::TableSetColumnIndex(1);
				else
					ImGui::TableSetColumnIndex(0);
				if (output_perm.size() == 1)
					ImNodes::PushAttributeFlag(ImNodesStyleFlags_AttributeSwappable, false);
				output_ref[output_perm[i]].Draw();
				if (output_perm.size() == 1)
					ImNodes::PopAttributeFlag();

				ImVec2 r = ImGui::GetItemRectSize();
				width_output = std::max(width_output, r.x);
				height_column = std::max(height_column, r.y);
			}
			height_total += height_column;
			height_column = 0;
		}

		ImGui::EndTable();
		size = ImVec2(width_input + width_output + ImGui::GetStyle().CellPadding.x, height_total);
	}
}

void NodeViewer::DrawMachineFooter() {
	ImNodes::BeginNodeFooter();
	ImGui::Text(std::format("nb machines : {}  -  time : {}s", "?", node->GetTime()).c_str());
	ImNodes::EndNodeFooter();
}

void NodeViewer::DrawMachine() {
	ImNodes::BeginNode(GetId());

	DrawMachineTitle();
	DrawMachineContent();
	DrawMachineFooter();

	ImNodes::EndNode();
}

void NodeViewer::DrawInput() {
	ImNodes::BeginNode(GetId());

	//DrawMachineTitle();
	DrawMachineContent();

	ImNodes::EndNode();
}

void NodeViewer::DrawInputTitle() {

}

void NodeViewer::DrawInputContent() {

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
		input_ref.push_back(NodeIOViewer(&nodeInput[i], true, dataBase, nodeUpdator));
	}

	for (int i = 0; i < nodeOutput.size(); i++) {
		output_perm.push_back(i);
		output_ref.push_back(NodeIOViewer(&nodeOutput[i], false, dataBase, nodeUpdator));
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


NodeViewer::NodeViewer(std::map<int, Node*>& nodes, const json11::Json& json, const DataBase* dataBase, NodeUpdator* nodeUpdator) : node(nodes.at(json.object_items().at("id").int_value())), dataBase(dataBase), nodeUpdator(nodeUpdator){
	const json11::Json::object obj = json.object_items();

	input_perm = JsonToVectorInt(obj.at("input_perm").array_items());
	output_perm = JsonToVectorInt(obj.at("output_perm").array_items());

	size = ImVec2(obj.at("size").array_items()[0].number_value(), obj.at("size").array_items()[1].number_value());

	const std::vector<NodeIO>& nodeInput = node->GetInputs();
	const std::vector<NodeIO>& nodeOutput = node->GetOutputs();
	for (int i = 0; i < nodeInput.size(); i++)
		input_ref.push_back(NodeIOViewer(&nodeInput[i], true, dataBase, nodeUpdator));
	for (int i = 0; i < nodeOutput.size(); i++)
		output_ref.push_back(NodeIOViewer(&nodeOutput[i], false, dataBase, nodeUpdator));
}