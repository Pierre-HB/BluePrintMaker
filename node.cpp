#include "node.h"
#include "imnodes.h"
#include <utility>
#include <iostream>

NODE_IO_TYPE int2NodeIOType(int i){
	static const NODE_IO_TYPE types[4] = { ITEM , IO , LOCK_IO , SPLITTER };
	return types[i];
}

void NodeIOViewer::DrawItem(const Item& item) {
	ImGui::Text((item.iconeString + item.name + std::format(" {}", (nodeIO->quantity.to_double()))).c_str());
	//std::cout << "quantity : " << nodeIO->quantity.to_double() << ", " << std::format(" {}", nodeIO->quantity.to_double()) << ", " << Rat2string(nodeIO->quantity) << std::endl;
}

void NodeIOViewer::DrawLockIO(const Item& item) {
	static bool guess;
	guess = true;
	ImGui::Checkbox("Guess", &guess);
	ImGui::SameLine();

	static int tmp;
	tmp = int(nodeIO->quantity.to_double());
	if(tmp >= 0)
	{
		ImGui::BeginDisabled();
		ImNodes::SetNextItemWidth(15 * int(1 + log10f(tmp)));
		ImGui::InputInt("##throuput", &tmp, 0, 0);
		ImGui::EndDisabled();
	}
	if(nodeIO->itemId != dataBase->GetUnkownItemId())
	{
		ImGui::SameLine();
		ImGui::Text(item.iconeString.c_str());
	}

	if (!guess)
		nodeUpdator->SetNodeIOState(NODE_IO_TYPE::IO, nodeIO->GetId());
}

void NodeIOViewer::DrawIO(const Item& item) {
	static bool guess;
	static int tmp;
	tmp = int(nodeIO->quantity.to_double());
	/*if (tmp < 0)
		tmp = 360;*/
	guess = false;
	ImGui::Checkbox("Guess", &guess);
	ImGui::SameLine();
	ImNodes::SetNextItemWidth(15 * int(1 + log10f(abs(tmp))));
	ImGui::InputInt("##throuput", &tmp, 0, 0);
	if (ImGui::IsItemDeactivatedAfterEdit() && tmp != int(nodeIO->quantity.to_double()) && !ImGui::IsKeyPressed(ImGuiKey_Escape))
		nodeUpdator->SetNodeIOQuantity(tmp, nodeIO->GetId());

	if (nodeIO->itemId != dataBase->GetUnkownItemId())
	{
		ImGui::SameLine();
		ImGui::Text(item.iconeString.c_str());
	}
	if (guess)
		nodeUpdator->SetNodeIOState(NODE_IO_TYPE::LOCK_IO, nodeIO->GetId());
}

void NodeIOViewer::DrawSplitter(const Item& item) {
	static int tmp;
	tmp = int(nodeIO->quantity.to_double());
	static std::string quantity;
	quantity = "";
	if (!isInput) {
		quantity = std::format(" {}", nodeIO->quantity.to_double());
	}
	if (!isInput) {
		ImNodes::SetNextItemWidth(45);
		ImGui::InputInt("##percentage", &tmp, 0, 0);
		if (tmp < 0)
			tmp = 0;
		if (tmp > 100)
			tmp = 100;

		if (ImGui::IsItemDeactivatedAfterEdit() && tmp != int(nodeIO->quantity.to_double()) && !ImGui::IsKeyPressed(ImGuiKey_Escape))
		{
			nodeUpdator->SetNodeIOSplitterPercent(tmp, nodeIO->GetId());
		}
		ImGui::SameLine();
	}
	if (nodeIO->itemId != dataBase->GetUnkownItemId())
	{
		ImGui::Text(item.iconeString.c_str());
		ImGui::SameLine();
	}
	if(isInput)
		ImGui::Text(std::format(" {}", nodeIO->quantity.to_double()).c_str());
}

void NodeIOViewer::Draw() {
	if (isInput)
		ImNodes::BeginInputAttribute(GetId());
	else
		ImNodes::BeginOutputAttribute(GetId());

	const Item& item = dataBase->getItem(nodeIO->itemId);

	switch (nodeIO->type)
	{
	case NODE_IO_TYPE::ITEM:
		DrawItem(item);
		break;
	case NODE_IO_TYPE::LOCK_IO:
		DrawLockIO(item);
		break;
	case NODE_IO_TYPE::IO:
		DrawIO(item);
		break;
	case NODE_IO_TYPE::SPLITTER:
		DrawSplitter(item);
		break;
	}

	if (isInput)
		ImNodes::EndInputAttribute();
	else
		ImNodes::EndOutputAttribute();
}


Node::Node() : id(), throuput(-1) {

}

Node::Node(int id) : id(id), throuput(-1) {

}

//copy a node
Node::Node(const Node& node) : id(node.id), inputs(node.inputs), outputs(node.outputs), machineId(node.machineId), time(node.time), idlePower(node.idlePower), workingPower(node.workingPower), state(node.state), type(node.type), throuput(throuput) {

}

//copy a node with brand new ids
Node::Node(const Node& node, int(*CreateId)() ) : Node(node) {
	id = CreateId();
	SetIOIds(CreateId);
}

//create node from dataBase
Node::Node(const DataBase* dataBase, int machineId, int(*CreateId)()) : machineId(machineId), throuput(-1) {
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
		InitNodeAsSplitter(CreateId, dataBase);
		return;

	case MACHINE_MERGER:
	case MACHINE_BLACKBOX:
		IM_ASSERT(false && "TODO");
	}

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
	type = node.type;
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
		outputs[i].quantity = recipe.outputsId[i].second;
		outputs[i].itemId = recipe.outputsId[i].first;
	}
	for (int i = 0; i < recipe.inputsId.size(); i++) {
		inputs[i].quantity = recipe.inputsId[i].second;
		inputs[i].itemId = recipe.inputsId[i].first;
	}
	time = recipe.time;
	idlePower = 0;
	workingPower = 0;
	//name = recipe.name;

	
	for (int j = 0; j < recipe.modifierCategoriesId.size(); j++) {
		int modifierCategory = recipe.modifierCategoriesId[j];
		int modifierId = dataBase->getModifierCategory(modifierCategory).modifiersId[state[j+1]];
		const Modifier& modifier = dataBase->getModifier(modifierId);

		idlePower += modifier.idlePower;
		workingPower += modifier.workingPower;
		time = time / modifier.speedModifier;
		for(int i = 0; i < recipe.outputsId.size(); i++)
		{
			std::cout << "old output : " << Rat2string(outputs[i].quantity) << std::endl;
			outputs[i].quantity *= modifier.outputModifier;
			std::cout << "new output : " << Rat2string(outputs[i].quantity);
			std::cout << ", modifier : " << Rat2string(modifier.outputModifier) << std::endl;
		}
		std::cout << "Output quantity modifier : " << modifier.outputModifier.to_double() << std::endl;
	}
	for (int j = 0; j < recipe.modifierCategoriesId.size(); j++) {
		int modifierCategory = recipe.modifierCategoriesId[j];
		int modifierId = dataBase->getModifierCategory(modifierCategory).modifiersId[state[j + 1]];
		const Modifier& modifier = dataBase->getModifier(modifierId);

		workingPower *= modifier.powerModifier;
	}
}

int Node::GetId() const {
	return id;
}

int Node::GetMachineId() const {
	return machineId;
}

Rat Node::GetTime() const {
	return time;
}

int Node::GetState(int i) const {
	return state[i];
}

int Node::GetStateSize() const {
	return state.size();
}

Rat Node::GetThrouput() const {
	return throuput;
}

void Node::SetThrouput(Rat newThrouput){
	throuput = newThrouput;
}

MACHINE_TYPE Node::GetType() const {
	return type;
}

const std::vector<NodeIO>& Node::GetInputs() const {
	return inputs;
}

const std::vector<NodeIO>& Node::GetOutputs() const {
	return outputs;
}

NodeIO const* Node::GetIO(int nodeIOId) const {
	for (const NodeIO& nodeIO : inputs)
		if (nodeIO.id == nodeIOId)
			return &nodeIO;
	for (const NodeIO& nodeIO : outputs)
		if (nodeIO.id == nodeIOId)
			return &nodeIO;
	return nullptr;
}

void Node::SetIOItem(int nodeIOId, int itemId) {
	for (NodeIO& nodeIO : inputs)
	{
		if (nodeIO.id == nodeIOId && nodeIO.type != ITEM) {
			nodeIO.itemId = itemId;
			return;
		}
	}
	for (NodeIO& nodeIO : outputs)
	{
		if (nodeIO.id == nodeIOId && nodeIO.type != ITEM) {
			nodeIO.itemId = itemId;
			return;
		}
	}
}

void Node::SetIOQuantity(Rat quantity) {
	for (NodeIO& nodeIO : inputs)
		if (nodeIO.type == LOCK_IO) 
			nodeIO.quantity = quantity;
	for (NodeIO& nodeIO : outputs)
		if (nodeIO.type == LOCK_IO)
			nodeIO.quantity = quantity;
}

void Node::ResetIOItemId(int unkownItemId) {
	for (NodeIO& nodeIO : inputs)
		if (nodeIO.type != ITEM) 
			nodeIO.itemId = unkownItemId;
	
	for (NodeIO& nodeIO : outputs)
		if (nodeIO.type != ITEM)
			nodeIO.itemId = unkownItemId;
}

void Node::UpdateNodeIOData(int nodeIOId, Rat newData) {
	for (NodeIO& nodeIO : inputs)
		if (nodeIO.GetId() == nodeIOId)
			nodeIO.quantity = newData;
	for (NodeIO& nodeIO : outputs)
		if (nodeIO.GetId() == nodeIOId)
			nodeIO.quantity = newData;
}

void Node::UpdateNodeIOSplitterData(int nodeIOId, Rat newData) {
	for (NodeIO& nodeIO : outputs)
	{
		if (nodeIO.GetId() == nodeIOId)
			nodeIO.quantity = newData;
		else
			nodeIO.quantity = Rat(100) - newData;
	}
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
		outputs.push_back(NodeIO(CreateId(), dataBase->GetUnkownItemId(), 360, NODE_IO_TYPE::LOCK_IO));
		type = MACHINE_INPUT;
	}
	else
	{
		inputs.push_back(NodeIO(CreateId(), dataBase->GetUnkownItemId(), 360, NODE_IO_TYPE::IO));
		type = MACHINE_OUTPUT;
	}
}

void Node::InitNodeAsSplitter(int(*CreateId)(), const DataBase* dataBase) {
	id = CreateId();
	inputs.push_back(NodeIO(CreateId(), dataBase->GetUnkownItemId(), 100, NODE_IO_TYPE::SPLITTER));
	outputs.push_back(NodeIO(CreateId(), dataBase->GetUnkownItemId(), 50, NODE_IO_TYPE::SPLITTER));
	outputs.push_back(NodeIO(CreateId(), dataBase->GetUnkownItemId(), 50, NODE_IO_TYPE::SPLITTER));
	type = MACHINE_SPLITTER;
}
void Node::InitNodeAsRegular(int(*CreateId)(), const DataBase* dataBase) {
	type = MACHINE_REGULAR;
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
		{"time", Rat2string(time).c_str()},
		{"idlePower", Rat2string(idlePower).c_str()},
		{"workingPower", Rat2string(workingPower).c_str()},
		{"state", state},
		{"type", type} });
}

Node::Node(const json11::Json& json) {
	const json11::Json::object obj = json.object_items();
	inputs = JsonToVector(obj.at("inputs").array_items());
	outputs = JsonToVector(obj.at("outputs").array_items());
	state = JsonToVectorInt(obj.at("state").array_items());
	id = obj.at("id").int_value();
	machineId = obj.at("machineId").int_value();
	time = Rat(obj.at("time").string_value().c_str());
	idlePower = Rat(obj.at("idlePower").string_value().c_str());
	workingPower = Rat(obj.at("workingPower").string_value().c_str());
	type = int2MachineType(obj.at("type").int_value());
	throuput = -1;
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
	if (node->GetType() == MACHINE_REGULAR)
		DrawMachine();
	else
		DrawInput();
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
		//ImGui::SameLine();

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
	float minPosX = INFINITY;
	float maxPosX = -INFINITY;

	int nb_col = 2;
	if (input_perm.size() == 0 || output_perm.size() == 0)
		nb_col = 1;
		
	if (ImGui::BeginTable("table1", nb_col, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoPadInnerX | ImGuiTableFlags_NoPadOuterX, size))
	{
		for (int i = 0; i < std::max(input_perm.size(), output_perm.size()); i++) {
			ImGui::TableNextRow();
			if (input_perm.size() > i) {
				ImGui::TableSetColumnIndex(0);
				if (input_perm.size() == 1)
					ImNodes::PushAttributeFlag(ImNodesStyleFlags_AttributeSwappable, false);
				minPosX = std::min(minPosX, ImGui::GetCursorPosX());
				input_ref[input_perm[i]].Draw();
				maxPosX = std::max(maxPosX, ImGui::GetCursorPosX()+ ImGui::GetItemRectSize().x);
				if (input_perm.size() == 1)
					ImNodes::PopAttributeFlag();
			}

			if (output_perm.size() > i) {
				if (nb_col == 2)
					ImGui::TableSetColumnIndex(1);
				else
					ImGui::TableSetColumnIndex(0);
				if (output_perm.size() == 1)
					ImNodes::PushAttributeFlag(ImNodesStyleFlags_AttributeSwappable, false);
				minPosX = std::min(minPosX, ImGui::GetCursorPosX());
				output_ref[output_perm[i]].Draw();
				maxPosX = std::max(maxPosX, ImGui::GetCursorPosX() + ImGui::GetItemRectSize().x);
				if (output_perm.size() == 1)
					ImNodes::PopAttributeFlag();
			}
		}
		size.x = maxPosX - minPosX;
		size.y = 0;
		ImGui::EndTable();
	}
}

void NodeViewer::DrawMachineFooter() {
	ImNodes::BeginNodeFooter();
	if(node->GetThrouput() >= 0)
	{
		Rat nbMachine = node->GetThrouput() * node->GetTime() / 60;
		int nbMachineCeil = ceil(nbMachine.to_double());
		int usage = nbMachine.to_double() * 100 / nbMachineCeil;
		ImGui::Text(std::format("nb machines : {} ({}%%) -  time : {:.2f}s", nbMachineCeil, usage, node->GetTime().to_double()).c_str());
	}
	else
		ImGui::Text(std::format("time : {:.2f}s", node->GetTime().to_double()).c_str());
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