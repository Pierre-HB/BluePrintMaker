#include "node_editor.h"
#include <imnodes.h>
#include <imgui.h>
#include <iostream>
#include <vector>
#include "smatrix.h"
#include "rat.hpp"
#include "blue_print.h"


static SMatrix<Rat> create_PB() {

    SMatrix<Rat> m = SMatrix<Rat>(8);
    m.insert(-1.5f, 0, 2);
    m.insert(1, 0, 4);
    m.insert(1, 0, 6);

    m.insert(-1, 1, 3);
    m.insert(1, 1, 5);
    m.insert(1, 1, 7);

    m.insert(-1, 2, 0);
    m.insert(1, 2, 4);

    m.insert(-1, 3, 0);
    m.insert(1, 3, 5);

    m.insert(-1, 4, 1);
    m.insert(1, 4, 6);

    m.insert(-1, 5, 1);
    m.insert(1, 5, 7);

    m.insert(1, 6, 0);

    m.insert(1, 7, 1);

    return m;
}

namespace example
{
namespace
{
class HelloWorldNodeEditor
{
    std::vector<std::pair<int, int>> links;
    bool swap = false;
    SMatrix<Rat>* m;

    BluePrint* bp;
public:
    void show()
    {
        /*ImNodes::StyleColorsBluePrint();
        ImNodes::PushAttributeFlag(ImNodesStyleFlags_AttributeSwappable);*/
        //ImNodesStyle& style = ImNodes::GetStyle();
        
        //ImNodes::PushStyleVar(ImNodesStyleVar_NodeBorderThickness, 2.0f);
        //
        ////To change the link type of currently created links
        //ImNodes::PushStyleVar(ImNodesStyleVar_LinkCreationType, ImNodesLinkType_Sloped);
        ////ImNodes::PopStyleVar(1);
        //
        ////ImNodesStyleFlags_AttributeSwappable
        ////ImNodesStyleFlags
        ////ImNodes::PushStyleVar(ImNodesStyleVar_ImNodesStyleFlags, ImNodesStyleFlags_AttributeSwappable);

        //ImNodes::GetStyle().Flags |= ImNodesStyleFlags_AttributeSwappable;
        //ImNodes::PushAttributeFlag(ImNodesStyleFlags_AttributeSwappable);
        //

        ////ImNodes::PushStyleVar(ImNodesStyleVar_LinkThickness, 10.0f);
        ///*ImNodes::PushStyleVar(ImNodesStyleVar_LinkSlopedMinSlope, 0.0f);
        //ImNodes::PushStyleVar(ImNodesStyleVar_LinkSlopedMinOffset, 50.0f);*/

        ///*std::cout << ImNodes::GetStyle().LinkSlopedMinSlope << std::endl;
        //ImNodes::PopStyleVar(1);
        //std::cout << ImNodes::GetStyle().LinkSlopedMinSlope << std::endl;*/

        ///*ImNodes::PushStyleVar(ImNodesStyleVar_GridSpacing, 50);
        //ImNodes::PushStyleVar(ImNodesStyleVar_GridSpacing, 150);
        //ImNodes::PopStyleVar(3);*/
        ////ImNodes::GetStyle().GridSpacing = 48; // ?OT SUPPOESED TO BE CHANGE DIRECTLY
        //style.Colors[ImNodesCol_GridBackground] = IM_COL32(5, 69, 141, 255);
        //style.Colors[ImNodesCol_GridLine] = IM_COL32(32, 109, 177, 255);
        //style.Colors[ImNodesCol_Link] = IM_COL32(200, 200, 200, 255);
        //style.Colors[ImNodesCol_LinkHovered] = IM_COL32(255, 255, 255, 255);
        //style.Colors[ImNodesCol_LinkSelected] = IM_COL32(255, 255, 255, 255);
        //style.Colors[ImNodesCol_NodeOutline] = IM_COL32(200, 200, 200, 255);

        //style.Colors[ImNodesCol_NodeBackground] = IM_COL32(5, 69, 141, 100);
        //style.Colors[ImNodesCol_NodeBackgroundHovered] = IM_COL32(25, 89, 161, 150);
        //style.Colors[ImNodesCol_NodeBackgroundSelected] = IM_COL32(25, 89, 161, 150);

        //style.Colors[ImNodesCol_TitleBar] = IM_COL32(25, 89, 161, 0);
        //style.Colors[ImNodesCol_TitleBarHovered] = IM_COL32(45, 109, 181, 0);
        //style.Colors[ImNodesCol_TitleBarSelected] = IM_COL32(45, 109, 181, 0);

        //style.Colors[ImNodesCol_Pin] = IM_COL32(200, 200, 200, 255);
        //style.Colors[ImNodesCol_PinHovered] = IM_COL32(255, 255, 255, 255);
        //style.Colors[ImNodesCol_BoxSelector] = IM_COL32(250, 250, 250, 20);
        //style.Colors[ImNodesCol_BoxSelectorOutline] = IM_COL32(255, 255, 255, 255);

        ///*ImGui::GetStyle().Colors[ImGuiCol_ButtonActive] = ImVec4(25 / 255.0, 89 / 255.0, 161 / 255.0, 1);
        //ImGui::GetStyle().Colors[ImGuiCol_Button] = ImVec4(5 / 255.0, 69 / 255.0, 141 / 255.0, 1);
        //ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered] = ImVec4(5/255.0, 69/255.0, 141/255.0, 1);*/

        ////ImGui::GetStyle().Colors[ImGuiCol_ButtonActive] = ImVec4(20, 20, 20, 30);
        ///*ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered] = ImVec4(20, 20, 20, 30);
        //ImGui::GetStyle().Colors[ImGuiCol_Button] = ImVec4(20, 20, 20, 30);*/

        //ImNodes::GetStyle().Flags |= ImNodesStyleFlags_::ImNodesStyleFlags_GridSnapping;
         
            ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;
            const ImGuiViewport * viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(viewport->Pos);
            ImGui::SetNextWindowSize(viewport->Size);
            ImGui::Begin("simple node editor", NULL, flags);

        ImNodes::BeginNodeEditor();

        
        ImNodes::BeginNode(1);

        ImNodes::BeginNodeTitleBar();
        ImGui::TextUnformatted("simple node :)");
        ImNodes::EndNodeTitleBar();

        
        ImNodes::PushStyleVar(ImNodesStyleVar_PinQuadSideLength, 20.0f);
        //ImNodes::PushAttributeFlag(ImNodesStyleFlags_AttributeSwappable);


        ImNodes::BeginInputAttribute(302, ImNodesPinShape_::ImNodesPinShape_QuadFilled);
        ImGui::Text("302");
        ImNodes::EndInputAttribute();


        //ImNodes::PopAttributeFlag();
        ImNodes::PopStyleVar(1);



        static bool swap = true;
        if (ImGui::Button("Swap"))
            swap = !swap;

        if (swap) {
            ImNodes::BeginOutputAttribute(300);
            ImGui::TextUnformatted("300");
            ImNodes::EndOutputAttribute();
        }
        else {
            ImNodes::BeginInputAttribute(300);
            ImGui::TextUnformatted("300");
            ImNodes::EndInputAttribute();
        }
        

        ImNodes::EndNode();



        ImNodes::BeginNode(7);

        ImNodes::BeginInputAttribute(259);
        ImGui::Text("2");
        ImNodes::EndInputAttribute();

        ImNodes::BeginNodeTitleBar();
        ImGui::TextUnformatted("simple:)");
        ImNodes::EndNodeTitleBar();


        ImNodes::PushStyleVar(ImNodesStyleVar_PinQuadSideLength, 20.0f);
        
        ImNodes::BeginInputAttribute(2);
        ImGui::Text("2");
        ImNodes::EndInputAttribute();

        ImNodes::PopStyleVar(1);
        ImGui::SameLine();

        //ImGui::Indent(40*2);
        ImGui::TextUnformatted("          ");
        ImGui::SameLine();
        ImNodes::BeginOutputAttribute(3);
        ImGui::Text("3");
        ImNodes::EndOutputAttribute();

        ImNodes::EndNode();

//======================= SELECTABLE attribute =============================
        ImNodes::BeginNode(4);

        ImNodes::BeginNodeTitleBar();
        ImGui::TextUnformatted("Node with selectable Attribute");
        ImNodes::EndNodeTitleBar();

        /*ImVec2 vmin;
        ImVec2 vmax;*/

        ImNodes::BeginInputAttribute(5);
        ImGui::Text("long input to test");
        ImGui::Text("on two lines");
        ImGui::Text("5");
        ImNodes::EndInputAttribute();
        //Hack to precompute the size of the Attribute and add a selectable of the correct size on top of it
        /*vmin = ImGui::GetItemRectMin();
        vmax = ImGui::GetItemRectMax();
        ImGui::SameLine();
        ImGui::Indent(-0.1);
        ImGui::Selectable("##input", false, 0, ImVec2(vmax.x - vmin.x, vmax.y - vmin.y));*/

        ImNodes::BeginInputAttribute(58);
        ImGui::Text("long input to test");
        ImGui::Text("on two lines");
        ImGui::Text("58");
        ImNodes::EndInputAttribute();
        //Hack to precompute the size of the Attribute and add a selectable of the correct size on top of it
        /*vmin = ImGui::GetItemRectMin();
        vmax = ImGui::GetItemRectMax();
        ImGui::SameLine();
        ImGui::Indent(-0.1);
        ImGui::Selectable("##input2", false, 0, ImVec2(vmax.x - vmin.x, vmax.y - vmin.y));*/


        //Allow for same line input/Output
        ImGui::SameLine();


        //Create a topGroup for an attribute
        /*ImGui::BeginGroup();
        ImGui::PushID("69");*/
        //start a bottom group for the attribute
        ImNodes::BeginOutputAttribute(69);
        ImGui::Text("output");
        ImGui::Text("??");
        ImGui::Text("!!!");
        ImGui::Text("69");
        ImGui::Button("test");
        ImNodes::EndOutputAttribute();

        ImNodes::BeginStaticAttribute(690);
        ImGui::Text("None");
        ImNodes::EndStaticAttribute();
        ImNodes::BeginStaticAttribute(691);
        ImGui::Text("None 2");
        ImNodes::EndStaticAttribute();
        //end the bottom group of teh attribute, compute it's size and add a selectable of the desired size on top of it

        //vmin = ImGui::GetItemRectMin();
        //vmax = ImGui::GetItemRectMax();
        //ImGui::SameLine();
        //ImGui::Indent(-0.1);
        //ImGui::Selectable("##output2", false, 0, ImVec2(vmax.x - vmin.x, vmax.y - vmin.y));
        ////end top group
        //ImGui::PopID();
        //ImGui::EndGroup();
        



//#define TMP
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
#ifdef SMATRIX
        {
            
            ImNodes::BeginNode(8459);

            ImNodes::BeginNodeTitleBar();
            ImGui::TextUnformatted("Rat");
            ImNodes::EndNodeTitleBar();
            ImNodes::BeginStaticAttribute(843597);

            if (ImGui::Button("start")) {
                
                Rat a = Rat(0.5f);
                Rat b = 1 / a;
                std::cout << a.to_double() << std::endl;
                std::cout << b.to_double() << std::endl;
                std::cout << (a * b).to_double() << std::endl;
                std::cout << (a / b).to_double() << std::endl;
                std::cout << (a + b).to_double() << std::endl;
                std::cout << (a - b).to_double() << std::endl;
                std::cout << Num().to_double() << std::endl;

            }


            ImNodes::EndStaticAttribute();
            ImNodes::EndNode();
        }


        {

            ImNodes::BeginNode(845);

            ImNodes::BeginNodeTitleBar();
            ImGui::TextUnformatted("Sparse matrix");
            ImNodes::EndNodeTitleBar();
            ImNodes::BeginStaticAttribute(84359);
            if (ImGui::Button("create matrix"))
                init_m(4);

            if (m != nullptr) {
                ImGui::Text("sparsity : %.0f%c", (m->sparsity() * 100), '%');
                if (ImGui::Button("Identity")) {
                    for (int i = 0; i < 4; i++)
                        m->insert(1, i, i);
                }

                if (ImGui::Button("Attila")) {
                    for (int i = 0; i < 4; i++)
                        for (int j = 0; j < 4; j++)
                            m->insert(1, i, j);
                }

                if (ImGui::Button("insert")) {
                    m->insert(42, 2, 3);
                }

                if (ImGui::Button("pAdd")) {
                    /*SMatrix<float> m1 = SMatrix<float>(*m);
                    m = new SMatrix<float>(m1 += m1);*/
                    m->operator+=(*m);
                }
                if (ImGui::Button("Add")) {
                    SMatrix<Rat> m1 = SMatrix<Rat>(*m);
                    SMatrix<Rat> m2 = SMatrix<Rat>(*m);

                    //m = new SMatrix<Rat>(m1 + m2); //need new to use the heap
                    m = new SMatrix<Rat>(*m + *m); //need new to use the heap
                    std::cout << "result of add is zero : " << m->is_zero() << std::endl;
                }

                if (ImGui::Button("pMinus")) {
                    m->operator-=(*m);
                }
                if (ImGui::Button("Minus")) {
                    SMatrix<Rat> m1 = SMatrix<Rat>(*m);
                    SMatrix<Rat> m2 = SMatrix<Rat>(*m);

                    m = new SMatrix<Rat>(m1 - m2); //need new to use the heap
                }

                if (ImGui::Button("pT")) {
                    m->transpose();
                }
                if (ImGui::Button("T")) {
                    m = new SMatrix<Rat>(m->transposed()); //need new to use the heap
                }

                if (ImGui::Button("pNeg")) {
                    m = new SMatrix<Rat>(-(*m));
                }
                if (ImGui::Button("Neg")) {
                    SMatrix<Rat> tmp = SMatrix<Rat>(*m);
                    m = new SMatrix<Rat>(-tmp);
                }
                if (ImGui::Button("test Neg")) {
                    SMatrix<Rat> a = SMatrix<Rat>(4);
                    //SMatrix<float> b = SMatrix<float>(4);
                    a.insert(2, 2, 3);
                    //b.insert(3, 0, 3);

                    //b = -(const SMatrix<float>)(a);
                    //b = -a;
                    //b = -std::move(a);


                    //SMatrix<float> tmp = SMatrix<float>(*m);
                    m = new SMatrix<Rat>(-std::move(a));
                }

                if (ImGui::Button("Mult")) {
                    SMatrix<Rat> tmp = SMatrix<Rat>(*m);
                    std::cout << "\n\n start mult\n\n" << std::endl;
                    m = new SMatrix<Rat>(m->operator*(tmp)); //need new to use the heap
                }

                if (ImGui::Button("Inverse")) {
                    std::cout << "\n\n start invert\n\n" << std::endl;
                    SMatrix<Rat> tmp = m->inversed();

                    m = new SMatrix<Rat>(tmp); //need new to use the heap
                }

                if (ImGui::Button("Set Pb")) {
                    delete m;

                    m = new SMatrix<Rat>(create_PB());
                }
                if (ImGui::Button("mutl by pB")) {
                    m = new SMatrix<Rat>(*m * create_PB());
                }
                if (ImGui::Button("mutl by pBT")) {
                    m = new SMatrix<Rat>(*m * create_PB().transposed());
                }
                if (ImGui::Button("post mutl by pBT")) {
                    m = new SMatrix<Rat>(create_PB().transposed() * (*m));
                }

                if (ImGui::Button("post mutl by pB")) {
                    m = new SMatrix<Rat>(create_PB() * (*m));
                }

                if (ImGui::Button("Set small Pb")) {
                    delete m;

                    m = new SMatrix<Rat>(2);
                    //m->insert(1, 0, 0);
                    m->insert(1, 0, 1);
                    m->insert(1, 1, 0);
                    //m->insert(4, 1, 1);
                }

                if (ImGui::Button("Set positive definite")) {

                    const SMatrix<Rat> mT = m->transposed();
                    m = new SMatrix<Rat>(mT * (*m));

                }




                int n = m->get_n();
                //ImGui::Table
                //ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_SizingFixedSame | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_SizingStretchSame,

                if (ImGui::BeginTable("table1", n, ImGuiTableFlags_SizingFixedFit))
                {
                    for (int row = 0; row < n; row++)
                    {
                        ImGui::TableNextRow();
                        for (int column = 0; column < n; column++)
                        {
                            ImGui::TableSetColumnIndex(column);
                            ImGui::Text("%.2f", (float)m->at(row, column).to_double());
                            //ImGui::Text("%.2f", (float)m->at(row, column).toFloat());
                        }
                    }
                    ImGui::EndTable();
                }
            }


            ImNodes::EndStaticAttribute();
            ImNodes::EndNode();
        }
#endif
//======================= END SELECTABLE attribute =============================


        //ImNodes::GetStyle().Flags |= ImNodesStyleFlags_::ImNodesStyleFlags_GridSnapping;

        //detect double ckick
        //std::cout << "double click" << ImGui::IsMouseDoubleClicked(0) << std::endl;

        /*if (ImNodes::IsAnyAttributeActive())
            std::cout << "any atribute active" << std::endl;*/

        //ImNodes::Link(7, 3, 5);

        //I need to manage myself the links
        for (int i = 0; i < links.size(); ++i)
        {
            const std::pair<int, int> p = links[i];
            // in this case, we just use the array index of the link
            // as the unique identifier
            //can push/pop style for changing the color of link (if two ressources does not correspond for instance)
            ImNodes::Link(i+10, p.first, p.second, ImNodesLinkType_::ImNodesLinkType_Sloped);
        }
        ImNodes::Link(15, 3, 5, ImNodesLinkType_::ImNodesLinkType_Sloped);
        
        ImNodes::BeginPinLabel(3, 456523);
        ImGui::Text("link label");
        ImNodes::EndPinLabel();
        ImNodes::BeginPinLabel(5, 456523+1);
        ImGui::Text("end label");
        ImNodes::EndPinLabel();
        ImNodes::BeginLinkLabel(15, 456523 + 2);
        ImGui::Text("end link label");
        ImNodes::EndLinkLabel();
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

        int src_attr, dest_attr;
        if (ImNodes::IsAttributeSwapped(&src_attr, &dest_attr))
        {
            std::cout << "SWAP ATTRIBUTE " << src_attr << ", " << dest_attr << std::endl;
        }

        /*int node_ids[50];
        ImNodes::GetSelectedNodes(node_ids);
        for (int i = 0; i < ImNodes::NumSelectedNodes(); i++)
            std::cout << "selecte node " << node_ids[i] << std::endl;

        ImNodes::GetSelectedLinks(node_ids);
        for (int i = 0; i < ImNodes::NumSelectedLinks(); i++)
            std::cout << "selecte link " << node_ids[i] << std::endl;*/
        //if suppr is press, delet the nodes and link in my app and stop calling BeginNode on them to delet them from ImNode

        //ImNode already allow to detect deleted link with ImNodes::IsLinkDestroyed() when pressing CTRL (can be changed)


        

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

    void clear() {
        if(m != nullptr)
            delete m;
    }

    void init_m(int n) {
        m = new SMatrix<Rat>(n);
        std::cout << "init m" << std::endl;
    }
};

static HelloWorldNodeEditor editor;
static BluePrint bp;
} // namespace

void NodeEditorInitialize() {
    //ImNodes::SetNodeGridSpacePos(1, ImVec2(200.0f, 200.0f)); 
    ImNodes::SetNodeGridSpacePos(1, ImVec2(200.0f, 200.0f));
    ImNodes::SetNodeGridSpacePos(7, ImVec2(400.0f, 400.0f));
    ImNodes::SetNodeGridSpacePos(4, ImVec2(600.0f, 600.0f));
    ImNodes::SetNodeGridSpacePos(845, ImVec2(1000.0f, 600.0f));
    ImNodes::SetNodeGridSpacePos(8459, ImVec2(100.0f, 200.0f));

    //Set up the modifier key.
    //When press the user can click a link, it will:
    // -> destroy the link (detectable with ImNodes::IsLinkDestroyed)
    // -> create a pending link from the farthest pin of the destroyed link
    ImNodes::GetIO().LinkDetachWithModifierClick.Modifier = &ImGui::GetIO().KeyCtrl;
    ImNodes::GetIO().TranslationModifier.X_Modifier = &ImGui::GetIO().KeysData[ImGuiKey_X - ImGuiKey_NamedKey_BEGIN].Down;
    ImNodes::GetIO().TranslationModifier.Y_Modifier = &(ImGui::GetIO().KeysData[ImGuiKey_Y - ImGuiKey_NamedKey_BEGIN].Down);
    //ImGui::IsKeyDown(ImGuiKey_Y)
    //ImGui::IsKey
    //ImGuiKeyData  KeysData[ImGuiKey_NamedKey_COUNT];// Key state for all known keys. Use IsKeyXXX() functions to access this.
    ImNodes::StyleColorsBluePrint();
    ImNodes::PushAttributeFlag(ImNodesStyleFlags_AttributeSwappable);

    
}
//bp = new BluePrint();


//void NodeEditorShow() { editor.show(); }
void NodeEditorShow() {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    bp.Draw();
    bp.Update();
}

void NodeEditorShutdown() {
    editor.clear();
}

} // namespace example
