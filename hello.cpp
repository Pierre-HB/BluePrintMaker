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
        
        //To change the link type of currently created links
        ImNodes::PushStyleVar(ImNodesStyleVar_LinkCreationType, ImNodesLinkType_::ImNodesLinkType_Sloped);
        ImNodes::PopStyleVar(1);

        //ImNodes::PushStyleVar(ImNodesStyleVar_LinkThickness, 10.0f);
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
        ImGui::Indent(40);
        //ImGui::Indent(40*10);
        ImGui::Text("output");
        ImNodes::EndOutputAttribute();

        ImNodes::EndNode();



        ImNodes::BeginNode(7);


        ImNodes::BeginNodeTitleBar();
        ImGui::TextUnformatted("simple node..... :)");
        ImNodes::EndNodeTitleBar();


        //ImNodes::PushStyleVar(ImNodesStyleVar_PinQuadSideLength, 20.0f);

        ImNodes::BeginInputAttribute(2);
        ImGui::Text("input");
        ImNodes::EndInputAttribute();

        //ImNodes::PopStyleVar(1);
        ImGui::SameLine();


        ImNodes::BeginOutputAttribute(3);
        ImGui::Indent(40);
        ImGui::Text("output");
        ImNodes::EndOutputAttribute();

        //if (!swap) {
        //    ImNodes::BeginInputAttribute(8);
        //    ImGui::Text("input");
        //    ImNodes::EndInputAttribute();
        //}


        ////if (ImGui::Button("rd button"))
        ////    std::cout << "PRESS" << std::endl;

        //ImNodes::BeginOutputAttribute(9);
        //////ImGui::Indent(40);
        ////ImGui::Text("output");
        ////ImVec2 value_raw = ImGui::GetMouseDragDelta(0, 0.0f);
        ////ImVec2 value_with_lock_threshold = ImGui::GetMouseDragDelta(0);
        //////ImVec2 /*mouse_delta*/ = io.MouseDelta;
        ////ImGui::Text("GetMouseDragDelta(0):");
        ////ImGui::Text("  w/ default threshold: (%.1f, %.1f)", value_with_lock_threshold.x, value_with_lock_threshold.y);
        ////ImGui::Text("  w/ zero threshold: (%.1f, %.1f)", value_raw.x, value_raw.y);
        //ImGuiIO& io = ImGui::GetIO();

        ////const ImU32 col = GetColorU32((held && hovered) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
        ////ImVec4      Colors[ImGuiCol_COUNT] in ImGuiStyle

        ////ImGui::GetStyle().Colors[ImGuiCol_ButtonActive] = ImVec4(0.25, 0.5, 0.75, 1);
        ////std::cout << ", " << ImGui::GetStyle().Colors[ImGuiCol_Button].x;
        ////std::cout << ", " << ImGui::GetStyle().Colors[ImGuiCol_Button].y;
        ////std::cout << ", " << ImGui::GetStyle().Colors[ImGuiCol_Button].z;
        ////std::cout << ", " << ImGui::GetStyle().Colors[ImGuiCol_Button].w << std::endl;;
        //////ImGui::GetStyle().Colors[ImGuiCol_ButtonActive] = ImVec4(20, 20, 20, 30);
        /////*ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered] = ImVec4(20, 20, 20, 30);
        ////ImGui::GetStyle().Colors[ImGuiCol_Button] = ImVec4(20, 20, 20, 30);*/



        //ImGui::Button("Output");
        //if (ImGui::IsItemActive()) {
        //    ImVec2 value_raw = ImGui::GetMouseDragDelta(0, 0.0f);
        //    //ImGui::Text("  w/ zero threshold: (%.1f, %.1f)", value_raw.x, value_raw.y);
        //    ImGui::GetForegroundDrawList()->AddLine(io.MouseClickedPos[0], io.MousePos, ImGui::GetColorU32(ImGuiCol_Button), 4.0f);
        //    //if (abs(value_raw.y) > 20)
        //        //swap = !swap;
        //    if (value_raw.y < -20)
        //        swap = true;
        //    if (value_raw.y > 20)
        //        swap = false;
        //}

        ////ImGui::Text("io.MouseDelta: (%.1f, %.1f)", mouse_delta.x, mouse_delta.y);

        //ImNodes::EndOutputAttribute();

        //if (swap) {
        //    ImNodes::BeginInputAttribute(8);
        //    ImGui::Text("input");
        //    ImNodes::EndInputAttribute();
        //}

        ////std::cout << ImNodes::IsAttributeActive() << std::endl;
        //if (ImNodes::IsAttributeActive())
        //    std::cout << "attribute active" << std::endl;



        ImNodes::EndNode();

//======================= SELECTABLE attribute =============================
        ImNodes::BeginNode(4);

        ImNodes::BeginNodeTitleBar();
        ImGui::TextUnformatted("Node with selectable Attribute");
        ImNodes::EndNodeTitleBar();

        ImVec2 vmin;
        ImVec2 vmax;

        ImNodes::BeginInputAttribute(5);
        ImGui::Text("long input to test");
        ImGui::Text("on two lines");
        ImNodes::EndInputAttribute();
        //Hack to precompute the size of the Attribute and add a selectable of the correct size on top of it
        vmin = ImGui::GetItemRectMin();
        vmax = ImGui::GetItemRectMax();
        ImGui::SameLine();
        ImGui::Indent(-0.1);
        ImGui::Selectable("##input", false, 0, ImVec2(vmax.x - vmin.x, vmax.y - vmin.y));

        ImNodes::BeginInputAttribute(58);
        ImGui::Text("long input to test");
        ImGui::Text("on two lines");
        ImNodes::EndInputAttribute();
        //Hack to precompute the size of the Attribute and add a selectable of the correct size on top of it
        vmin = ImGui::GetItemRectMin();
        vmax = ImGui::GetItemRectMax();
        ImGui::SameLine();
        ImGui::Indent(-0.1);
        ImGui::Selectable("##input2", false, 0, ImVec2(vmax.x - vmin.x, vmax.y - vmin.y));


        //Allow for same line input/Output
        ImGui::SameLine();


        //Create a topGroup for an attribute
        ImGui::BeginGroup();
        ImGui::PushID(888);
        //start a bottom group for the attribute
        ImNodes::BeginOutputAttribute(69);
        ImGui::Text("output");
        ImGui::Text("??");
        ImGui::Text("!!!");
        ImNodes::EndOutputAttribute();
        //end the bottom group of teh attribute, compute it's size and add a selectable of the desired size on top of it

        vmin = ImGui::GetItemRectMin();
        vmax = ImGui::GetItemRectMax();
        ImGui::SameLine();
        ImGui::Indent(-0.1);
        ImGui::Selectable("##output2", false, 0, ImVec2(vmax.x - vmin.x, vmax.y - vmin.y));
        //end top group
        ImGui::PopID();
        ImGui::EndGroup();
        



#define TMP
#ifdef TMP
        ImGui::PushItemFlag(ImGuiItemFlags_AllowDuplicateId, true);

        // Simple reordering
        
        static const char* item_names[] = { "Item One", "Item Two", "Item Three", "Item Four", "Item Five" };
        for (int n = 0; n < IM_ARRAYSIZE(item_names); n++)
        {
            const char* item = item_names[n];
            ImGui::Selectable(item);

            if (ImGui::IsItemActive() && !ImGui::IsItemHovered())
            {
                int n_next = n + (ImGui::GetMouseDragDelta(0).y < 0.f ? -1 : 1);
                if (n_next >= 0 && n_next < IM_ARRAYSIZE(item_names))
                {
                    item_names[n] = item_names[n_next];
                    item_names[n_next] = item;
                    ImGui::ResetMouseDragDelta();
                }
            }
        }

        ImGui::PopItemFlag();
#endif

        ImNodes::EndNode();

//======================= END SELECTABLE attribute =============================


        //ImNodes::GetStyle().Flags |= ImNodesStyleFlags_::ImNodesStyleFlags_GridSnapping;

        

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
        ImNodes::Link(15, 3, 5, ImNodesLinkType_::ImNodesLinkType_Sloped);
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

        int link_id;
        if (ImNodes::IsLinkDestroyed(&link_id)) {
            std::cout << "destroyed link " << link_id << std::endl;
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

void NodeEditorInitialize() {
    //ImNodes::SetNodeGridSpacePos(1, ImVec2(200.0f, 200.0f)); 

    //Set up the modifier key.
    //When press the user can click a link, it will:
    // -> destroy the link (detectable with ImNodes::IsLinkDestroyed)
    // -> create a pending link from the farthest pin of the destroyed link
    ImNodes::GetIO().LinkDetachWithModifierClick.Modifier = &ImGui::GetIO().KeyCtrl;
}

void NodeEditorShow() { editor.show(); }

void NodeEditorShutdown() {}

} // namespace example
