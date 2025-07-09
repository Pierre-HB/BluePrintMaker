# BluePrintMaker
C++ project to automaticcaly generate blueprints for the game Dyson Sphere Program.

# TODO

- [x] Solver with Gauss pivot
- [x] deformation of links (maybe use link's id to track changes between frames, see selected link for inspiration)
 - [x] Right Click "Add"
 - [x] Supr to delet object
 - [ ] Dashed type for proliferator links (maybe not)
 - [x] Node API to automatically manage:
	 + [x] input and output on the same line (maybe use tables)
 - [x] text on top of links
 - [x] CTRL Z
 - [x] CTRL Y
 - [x] BluePrint Style in colors
 - [x] Sloped link
 - [x] Manage several link style
 - [x] Swap of attribute
 - [x] add a control to lock moves on X or Y axis (for link control movement and link label movement and node movement)
- [ ] zoom
- [ ] Saving
- [ ] Nice node visual
- [ ] Json reading
- [ ] PDF export
- [ ] Label management (hide/show/lock)
- [ ] Computation of the flows
- [ ] Black box from other BluePrint
- [ ] Constraint help ?
- [ ] Splitter/Merger/Input/Output
- [ ] TUTO

# Note
- if I want several windows with independant blueprint, I might take a look at the ImGui::BeginChild call in the ImNodes::BeginNodeEditor(). I may also use several context and switch between them
- For SVG Export, need to hook at the function **`void  AddLine(...)`**, **`void  AddRect(...)`**, **`void  AddRectFilled(...)`**, ... in file imgui.h line 3113
- For the computation of the flow solver, use a separate thread and add a rotating ImGui widget to show that something is being computed.

# Projet

The projet is build upon ImGui, ImNode and Num library (https://github.com/983/Num). Id did everything else including:

- BluePrint theme
- Sloped curve in node editor
- swap of attribute
- deformation (and GUI of it) of links
- labels (label did not exists in ImNode)
- Event stack for CTRL Z and CTRL Y
- ZOOM
- Saving
- PDF export
- node graph management
- Sparse matrix operations
- Customizable nodes
- Set of customized nodes for the game DysonSphere program
- Flow solver
- Tutorial

Everything was coded with performance in mind without overoptimization. I restricted myself to only operation in O(n.log(n) (n beeing the number of nodes present in the blueprint) during a frame update. Heavier operation (namely the flow solver) are done using a separate thread.