#include "node_editor.h"
#include <imnodes.h>
#include <imgui.h>
#include <iostream>
#include <vector>

namespace example
{
namespace
{
class HelloWorldNodeEditor
{
    std::vector<std::pair<int, int>> links;
    bool swap = false;
public:
    void show()
    {
        ImNodesStyle& test = ImNodes::GetStyle();
        test.GridSpacing = 48/2;
        //ImNodesCol_::
        //ImNodes::GImNodes->Style.Flags
        //ImNodes::
        ImNodes::GetStyle().Flags |= ImNodesStyleFlags_::ImNodesStyleFlags_GridSnapping;
        //test.Flags = test.Flags | ImNodesStyleFlags_::ImNodesStyleFlags_GridSnapping;

        
        ImNodesStyle& test2 = ImNodes::GetStyle();

        //std::cout << "space : " << test2.GridSpacing << std::endl;

        ImGui::Begin("simple node editor");

        ImNodes::BeginNodeEditor();
        ImNodes::BeginNode(1);

        ImVec2 tmp = ImNodes::GetNodeGridSpacePos(1);
        //std::cout << tmp.x << ", " << tmp.y << " | ";

        ImNodes::BeginNodeTitleBar();
        ImGui::TextUnformatted("simple node :)");
        ImNodes::EndNodeTitleBar();

        ImNodes::BeginInputAttribute(2);
        ImGui::Text("input");
        ImNodes::EndInputAttribute();

        ImNodes::BeginOutputAttribute(3);
        ImGui::Indent(40*10);
        ImGui::Text("output");
        ImNodes::EndOutputAttribute();

        ImNodes::EndNode();

        //ImNodes::SnapNodeToGrid(1);

        //std::cout << "flag : " << ImNodes::GetStyle().Flags;
        ImNodes::GetStyle().Flags &= ~ImNodesStyleFlags_::ImNodesStyleFlags_GridSnapping;
        //std::cout << " after : " << ImNodes::GetStyle().Flags << std::endl;
        ImNodes::GetStyle().Flags |= ImNodesStyleFlags_::ImNodesStyleFlags_GridSnapping;

        ImNodes::BeginNode(4);

        tmp = ImNodes::GetNodeGridSpacePos(4);
        //std::cout << tmp.x << ", " << tmp.y << std::endl;

        ImNodes::BeginNodeTitleBar();
        ImGui::TextUnformatted("simple node 2 no snap :)");
        ImNodes::EndNodeTitleBar();

        ImNodes::BeginInputAttribute(5);
        ImGui::Text("input");
        ImNodes::EndInputAttribute();

        ImNodes::BeginOutputAttribute(6);
        ImGui::Indent(40 * 10);
        ImGui::Text("output");
        ImNodes::EndOutputAttribute();

        ImNodes::EndNode();

        //ImNodes::GetStyle().Flags |= ImNodesStyleFlags_::ImNodesStyleFlags_GridSnapping;

        ImNodes::BeginNode(7);

        tmp = ImNodes::GetNodeGridSpacePos(7);
        //std::cout << tmp.x << ", " << tmp.y << std::endl;

        ImNodes::BeginNodeTitleBar();
        ImGui::TextUnformatted("simple node 3 :)");
        ImNodes::EndNodeTitleBar();

        if (!swap) {
            ImNodes::BeginInputAttribute(8);
            ImGui::Text("input");
            ImNodes::EndInputAttribute();
        }
        

        //if (ImGui::Button("rd button"))
        //    std::cout << "PRESS" << std::endl;

        ImNodes::BeginOutputAttribute(9);
        ////ImGui::Indent(40);
        //ImGui::Text("output");
        //ImVec2 value_raw = ImGui::GetMouseDragDelta(0, 0.0f);
        //ImVec2 value_with_lock_threshold = ImGui::GetMouseDragDelta(0);
        ////ImVec2 /*mouse_delta*/ = io.MouseDelta;
        //ImGui::Text("GetMouseDragDelta(0):");
        //ImGui::Text("  w/ default threshold: (%.1f, %.1f)", value_with_lock_threshold.x, value_with_lock_threshold.y);
        //ImGui::Text("  w/ zero threshold: (%.1f, %.1f)", value_raw.x, value_raw.y);
        ImGuiIO& io = ImGui::GetIO();
        ImGui::Button("Output");
        if (ImGui::IsItemActive()) {
            ImVec2 value_raw = ImGui::GetMouseDragDelta(0, 0.0f);
            //ImGui::Text("  w/ zero threshold: (%.1f, %.1f)", value_raw.x, value_raw.y);
            ImGui::GetForegroundDrawList()->AddLine(io.MouseClickedPos[0], io.MousePos, ImGui::GetColorU32(ImGuiCol_Button), 4.0f);
            //if (abs(value_raw.y) > 20)
                //swap = !swap;
            if (value_raw.y < -20)
                swap = true;
            if (value_raw.y > 20)
                swap = false;
        }
            
        //ImGui::Text("io.MouseDelta: (%.1f, %.1f)", mouse_delta.x, mouse_delta.y);

        ImNodes::EndOutputAttribute();

        if (swap) {
            ImNodes::BeginInputAttribute(8);
            ImGui::Text("input");
            ImNodes::EndInputAttribute();
        }

        //std::cout << ImNodes::IsAttributeActive() << std::endl;
        if (ImNodes::IsAttributeActive())
            std::cout << "attribute active" << std::endl;

        

        ImNodes::EndNode();

        if (ImNodes::IsAnyAttributeActive())
            std::cout << "any atribute active" << std::endl;

        //ImNodes::Link(7, 3, 5);

        //I need to manage myself the links
        for (int i = 0; i < links.size(); ++i)
        {
            const std::pair<int, int> p = links[i];
            // in this case, we just use the array index of the link
            // as the unique identifier
            ImNodes::Link(i+10, p.first, p.second);
        }
        ImNodes::Link(15, 3, 6);
        ImNodes::Link(16, 6, 3);

        //ImNodes::MiniMap();
        ImNodes::EndNodeEditor();

        
        // elsewhere in the code...
        int start_attr, end_attr;
        if (ImNodes::IsLinkCreated(&start_attr, &end_attr))
        {
            std::cout << "NEW LINK! " << start_attr << ", " << end_attr << std::endl;

            links.push_back(std::make_pair(start_attr, end_attr));
        }



        

        //std::vector<int> selected_ids = std::vector<int>(20);

        //ImNodes::GetSelectedLinks(&(selected_ids.front()));//return the list of selected link
        //std::cout << selected_ids[0] << ", " << selected_ids[1] << std::endl;

        //ImNodes::GetSelectedNodes(&(selected_ids.front()));//return the list of selected node AND links
        //std::cout << selected_ids[0] << ", " << selected_ids[1] << ", " << selected_ids[2] << std::endl;

        //fonction to know the nb of selecte items:
        //int NumSelectedNodes();
        //int NumSelectedLinks();

        //Unselect items. Might be usefull when deleting stuff
        //void ClearNodeSelection();
        //void ClearLinkSelection();
        
        

        

        ImGui::End();
    }
};

static HelloWorldNodeEditor editor;
} // namespace

void NodeEditorInitialize() { ImNodes::SetNodeGridSpacePos(1, ImVec2(200.0f, 200.0f)); }

void NodeEditorShow() { editor.show(); }

void NodeEditorShutdown() {}

} // namespace example
