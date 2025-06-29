#include "node_editor.h"
#include <imnodes.h>
#include <imgui.h>
#include <iostream>
#include <vector>
//#include "smatrix.h"
//#include "rat.hpp"


class BluePrint {
	int idSeed;
	const char* name;
	ImVector<int> nodes;
	ImVector<int> links; //Get list of link caus wee need to render them after all nodes
	int ioPanel; //object for managing user input
	/*
	asking to create node for now
	*/
	BluePrint(const char* name) : idSeed(0), name(name) {};

	int CreateId() {
		return idSeed++;
	}

	void Draw() const {
		//begin editor
		//draw node
		//draw links

		//IF click droit show IOPanel
	}

	void Update() {
		//if need to add node, link, swap attribute
		//Copie/paste, save ?
	}
};

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