# BluePrintMaker
C++ project to automaticcaly generate blueprints for the game Dyson Sphere Program.

# TODO

- [x] Solver with Gauss pivot
- [x] deformation of links (maybe use link's id to track changes between frames, see selected link for inspiration)
 - [ ] Right Click "Add"
	 + [ ] Machine
	 + [ ] Source
	 + [ ] Merger/Spliter
	 + [ ] Black box
 - [ ] Supr to delet object
 - [ ] Dashed type for proliferator links (maybe not)
 - [ ] Node API to automatically manage:
	 + [ ] input and output on the same line (maybe use tables)
 - [ ] PDF Export (or SVG export) (need to hook at the function **`void  AddLine(...)`**, **`void  AddRect(...)`**, **`void  AddRectFilled(...)`**, ... in file imgui.h line 3113)
 - [ ] Black box creation from other Blue print
 - [ ] Saving of current project
 - [ ] TUTO
 - [ ] CTRL Z
 - [ ] CTRL Y
 - [x] BluePrint Style in colors
 - [x] Sloped link
 - [x] Manage several link style
 - [x] Swap of attribute