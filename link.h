#pragma once

#include "json11.hpp"
#include "data_base.h"

struct LinkUpdator {

	bool update() {
		return false;
	}

	void reset() {

	}
};

class Link {
private:
	int id;
	int inputId;
	int outputId;
	int inputNodeId;
	int outputNodeId;
public:
	Link();
	Link(int id);
	Link(int id, int inputId, int outputId, int inpoutNodeId, int outputNodeId);
	Link(const json11::Json& json);

	void Update();

	int GetId() const;

	int GetInputId() const;
	int GetOutputId() const;
	int GetNodeInputId() const;
	int GetNodeOutputId() const;

	json11::Json ToJson() const;

	//void AddLabels();
};

class LinkViewer {
private:
	const Link* link;
	const DataBase* dataBase;
	LinkUpdator* linkUpdator;

public:
	LinkViewer(Link* link, const DataBase* dataBase, LinkUpdator* linkUpdator);
	LinkViewer(const LinkViewer& linkViewer, const Link* link);
	LinkViewer(std::map<int, Link*>& links, const json11::Json& json, const DataBase* dataBase, LinkUpdator* linkUpdator);
	
	void Draw() const;

	int GetId() const;

	json11::Json ToJson() const;
};