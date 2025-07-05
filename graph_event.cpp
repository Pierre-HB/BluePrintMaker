#include "graph_event.h"


GraphEvent::GraphEvent() : GraphEvent(-1, NONE) {

}

GraphEvent::GraphEvent(int id, GraphEventType type) : id(), type(type) {

}

GraphEvent::~GraphEvent() {
	for (ImNodeData* node : nodeImNodesDatas)
		ImNodes::DeleteNodeData(node);

	for (ImLinkData* link : linkImNodesDatas)
		ImNodes::DeleteLinkData(link);

	for (ImLabelData* label : labelImNodesDatas)
		ImNodes::DeleteLabelData(label);

	for (Node* node : nodeDatas)
		delete node;

	for (NodeViewer* nodeViewer : nodeViewerDatas)
		delete nodeViewer;

	for (Link* link : linkDatas)
		delete link;

	for (LinkViewer* linkViewer : linkViewerDatas)
		delete linkViewer;
}