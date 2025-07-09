# BluePrintMaker
C++ project to automaticcaly generate blueprints for the game Dyson Sphere Program.

# TODO

- [x] Solver with Gauss pivot
- [x] deformation of links (maybe use link's id to track changes between frames, see selected link for inspiration)
 - [x] Right Click "Add"
	 + [ ] Machine
	 + [ ] Source
	 + [ ] Merger/Spliter
	 + [ ] Black box
 - [ ] JSON reader for factory data
 - [x] Supr to delet object
 - [ ] Dashed type for proliferator links (maybe not)
 - [x] Node API to automatically manage:
	 + [x] input and output on the same line (maybe use tables)
 - [ ] PDF Export (or SVG export) (need to hook at the function **`void  AddLine(...)`**, **`void  AddRect(...)`**, **`void  AddRectFilled(...)`**, ... in file imgui.h line 3113)
 - [ ] Black box creation from other Blue print
 - [ ] Saving of current project
 - [ ] TUTO
 - [x] text on top of links
 - [x] CTRL Z
 - [x] CTRL Y
 - [x] BluePrint Style in colors
 - [x] Sloped link
 - [x] Manage several link style
 - [x] Swap of attribute
 - [x] add a control to lock moves on X or Y axis (for link control movement and link label movement and node movement)

# Note
- if I want several windows with independant blueprint, I might take a look at the ImGui::BeginChild call in the ImNodes::BeginNodeEditor(). I may also use several context and switch between them