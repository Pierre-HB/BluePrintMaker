#pragma once

#include "json11.hpp"

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

public:
	LinkViewer(Link* link);
	LinkViewer(const LinkViewer& linkViewer, const Link* link);

	void Draw() const;

	int GetId() const;

	json11::Json ToJson() const;
};