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
        ImNodesStyle& style = ImNodes::GetStyle();
        
        ImNodes::PushStyleVar(ImNodesStyleVar_NodeBorderThickness, 2.0f);
        /*ImNodes::PushStyleVar(ImNodesStyleVar_LinkSlopedMinSlope, 0.0f);
        ImNodes::PushStyleVar(ImNodesStyleVar_LinkSlopedMinOffset, 50.0f);*/

        /*std::cout << ImNodes::GetStyle().LinkSlopedMinSlope << std::endl;
        ImNodes::PopStyleVar(1);
        std::cout << ImNodes::GetStyle().LinkSlopedMinSlope << std::endl;*/

        /*ImNodes::PushStyleVar(ImNodesStyleVar_GridSpacing, 50);
        ImNodes::PushStyleVar(ImNodesStyleVar_GridSpacing, 150);
        ImNodes::PopStyleVar(3);*/
        //ImNodes::GetStyle().GridSpacing = 48; // ?OT SUPPOESED TO BE CHANGE DIRECTLY
        style.Colors[ImNodesCol_GridBackground] = IM_COL32(5, 69, 141, 255);
        style.Colors[ImNodesCol_GridLine] = IM_COL32(32, 109, 177, 255);
        style.Colors[ImNodesCol_Link] = IM_COL32(200, 200, 200, 255);
        style.Colors[ImNodesCol_LinkHovered] = IM_COL32(255, 255, 255, 255);
        style.Colors[ImNodesCol_LinkSelected] = IM_COL32(255, 255, 255, 255);
        style.Colors[ImNodesCol_NodeOutline] = IM_COL32(200, 200, 200, 255);

        style.Colors[ImNodesCol_NodeBackground] = IM_COL32(5, 69, 141, 100);
        style.Colors[ImNodesCol_NodeBackgroundHovered] = IM_COL32(25, 89, 161, 150);
        style.Colors[ImNodesCol_NodeBackgroundSelected] = IM_COL32(25, 89, 161, 150);

        style.Colors[ImNodesCol_TitleBar] = IM_COL32(25, 89, 161, 0);
        style.Colors[ImNodesCol_TitleBarHovered] = IM_COL32(45, 109, 181, 0);
        style.Colors[ImNodesCol_TitleBarSelected] = IM_COL32(45, 109, 181, 0);

        style.Colors[ImNodesCol_Pin] = IM_COL32(200, 200, 200, 255);
        style.Colors[ImNodesCol_PinHovered] = IM_COL32(255, 255, 255, 255);
        style.Colors[ImNodesCol_BoxSelector] = IM_COL32(250, 250, 250, 20);
        style.Colors[ImNodesCol_BoxSelectorOutline] = IM_COL32(255, 255, 255, 255);

        /*ImGui::GetStyle().Colors[ImGuiCol_ButtonActive] = ImVec4(25 / 255.0, 89 / 255.0, 161 / 255.0, 1);
        ImGui::GetStyle().Colors[ImGuiCol_Button] = ImVec4(5 / 255.0, 69 / 255.0, 141 / 255.0, 1);
        ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered] = ImVec4(5/255.0, 69/255.0, 141/255.0, 1);*/

        //ImGui::GetStyle().Colors[ImGuiCol_ButtonActive] = ImVec4(20, 20, 20, 30);
        /*ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered] = ImVec4(20, 20, 20, 30);
        ImGui::GetStyle().Colors[ImGuiCol_Button] = ImVec4(20, 20, 20, 30);*/

        ImNodes::GetStyle().Flags |= ImNodesStyleFlags_::ImNodesStyleFlags_GridSnapping;
         
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;
        const ImGuiViewport * viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::Begin("simple node editor", NULL, flags);

        ImNodes::BeginNodeEditor();
        ImNodes::BeginNode(1);

        ImVec2 tmp = ImNodes::GetNodeGridSpacePos(1);
        //std::cout << tmp.x << ", " << tmp.y << " | ";

        ImNodes::BeginNodeTitleBar();
        ImGui::TextUnformatted("simple node :)");
        ImNodes::EndNodeTitleBar();

        
        ImNodes::PushStyleVar(ImNodesStyleVar_PinQuadSideLength, 20.0f);

        ImNodes::BeginInputAttribute(2, ImNodesPinShape_::ImNodesPinShape_QuadFilled);
        ImGui::Text("input");
        ImNodes::EndInputAttribute();

        //ImNodes::PopStyleVar(1);


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

        ImNodes::BeginInputAttribute(42);
        ImNodes::EndInputAttribute();
        ImNodes::BeginInputAttribute(43);
        ImNodes::EndInputAttribute();

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

        //const ImU32 col = GetColorU32((held && hovered) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
        //ImVec4      Colors[ImGuiCol_COUNT] in ImGuiStyle

        //ImGui::GetStyle().Colors[ImGuiCol_ButtonActive] = ImVec4(0.25, 0.5, 0.75, 1);
        //std::cout << ", " << ImGui::GetStyle().Colors[ImGuiCol_Button].x;
        //std::cout << ", " << ImGui::GetStyle().Colors[ImGuiCol_Button].y;
        //std::cout << ", " << ImGui::GetStyle().Colors[ImGuiCol_Button].z;
        //std::cout << ", " << ImGui::GetStyle().Colors[ImGuiCol_Button].w << std::endl;;
        ////ImGui::GetStyle().Colors[ImGuiCol_ButtonActive] = ImVec4(20, 20, 20, 30);
        ///*ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered] = ImVec4(20, 20, 20, 30);
        //ImGui::GetStyle().Colors[ImGuiCol_Button] = ImVec4(20, 20, 20, 30);*/

        

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
        ImNodes::Link(15, 3, 5);
        //ImNodes::Link(16, 6, 3);

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
