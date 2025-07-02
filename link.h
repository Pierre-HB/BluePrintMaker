#pragma once

class Link {
private:
	int id;
	int inputId;
	int outputId;
public:
	Link();
	Link(int id);
	Link(int id, int inputId, int outputId);

	void Update();

	int GetId() const;

	int GetInputId() const;
	int GetOutputId() const;

	//void AddLabels();
};

class LinkViewer {
private:
	Link* link;

public:
	LinkViewer(Link* link);

	void Draw() const;

	int GetId() const;
};