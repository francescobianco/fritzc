# Fritz 2.5 Reborn
## A reverse engineering and native C porting project for the classic DOS chess software

### 1. Project vision

The goal is to build a native, cross-platform port of **Fritz 2.5 / Fritz 2.51**, the classic MS-DOS chess program developed in the nineties by Frans Morsch and distributed by ChessBase.

The project aims to recover and preserve the behavior of the original program, reconstructing in C both the chess engine and the user interface.

The intent is neither to create a new chess program merely inspired by Fritz, nor to distribute a DOS emulator bundling the original program.

The aim is to obtain a new native implementation that reproduces, as faithfully as possible, the experience of the historical software.

The result must be usable on modern operating systems:

- Linux
- Windows
- macOS

Without requiring DOSBox, virtual machines or other DOS environments.

The priority is **historical and functional fidelity**, not improving the original program.

### 2. Development philosophy

The project adopts a few fundamental principles.

**Fidelity before modernization**

The original features must be understood, documented and, where possible, preserved before introducing improvements.

**Observation-driven reverse engineering**

Every component must be studied through analysis of the original files, the machine code and the behavior of the running program.

**Understandable implementation**

The resulting code must be written in modern C, organized into simple, maintainable modules.

Reproducing the original internal structure is not a priority if it depends on limitations of the DOS architecture.

What matters instead is reproducing the observable behavior and, above all for the chess engine, the original algorithms and decisions.

**No emulation in the final product**

DOSBox and emulation tools will be used exclusively as tools for analysis, debugging and comparison.

**Separation between engine and interface**

The chess logic must be independent of the graphical interface.

The engine must also be buildable and runnable on its own.

### 3. Scope of the reverse engineering

The work covers two major subsystems.

#### 3.1 Chess engine

The following components must be analyzed and, where possible, reconstructed:

- Internal board representation.
- Encoding of pieces and moves.
- Generation of pseudo-legal and legal moves.
- Making and unmaking moves.
- Handling of castling, en passant and promotion.
- Detection of check, checkmate and stalemate.
- Draw rules.
- Static position evaluation.
- Move search.
- Move ordering.
- Transposition tables, if any.
- Time management.
- Difficulty levels.
- Opening handling.
- Any specific heuristics or optimizations.

It must not be taken for granted that Fritz uses the same data structures or the same algorithms as modern engines.

The algorithms must be identified through experiments and analysis of the binary.

The ideal goal is to obtain, given the same position and search conditions, the same decision-making behavior as the original engine.

#### 3.2 Graphical interface

The project also includes the reconstruction of the entire user interface.

The following will be analyzed:

- Main screen.
- Chessboard.
- Piece set.
- Color palettes.
- Typography and fonts.
- Menus.
- Submenus.
- Dialog boxes.
- Buttons and controls.
- Mouse handling.
- Keyboard shortcuts.
- Clocks.
- Information panels.
- Move notation.
- Analysis display.
- Settings and configuration.
- Management of games and any databases.
- Messages and confirmation dialogs.
- Behavior of windows and interactions.

This list represents the scope of investigation: the features actually present must be ascertained in the original version.

Reproducing a similar-looking screen is not enough. The behavior of the interface must be reconstructed as well.

### 4. Proposed technology

The main language will be **C11**.

**SDL3** is proposed for the graphical interface.

SDL3 will be used as a cross-platform abstraction layer for:

- Window creation and management.
- Graphics rendering.
- Mouse events.
- Keyboard events.
- Time management.
- Audio playback, if needed.

The interface itself will be implemented in C, without using modern widget frameworks.

This makes it possible to preserve the look and behavior of the original DOS interface, without having to adapt its controls to those of GTK, Qt, Cocoa or Win32.

#### 4.1 Rendering at a virtual resolution

One possible strategy is to create a virtual graphics surface matching the program's original resolution.

All elements will be drawn in virtual coordinates.

The final image will then be scaled to the size of the modern window.

The original resolution, the video mode and the pixel aspect ratio must be determined through analysis.

The following must be considered:

- Integer scaling to preserve pixels.
- Optional aspect ratio correction.
- Nearest-neighbor rendering.
- Windowed mode.
- Fullscreen mode.
- Conversion of mouse coordinates between the window and the virtual surface.

The default mode must favor visual fidelity.

In the future, alternative scaling methods may be introduced without altering the logical rendering.

### 5. Reverse engineering of the DOS program

The work will begin with an analysis of the original distribution.

#### 5.1 File inventory

All files present must be identified:

- Executables.
- Libraries.
- Configuration files.
- Databases.
- Opening books.
- Graphics resources.
- Fonts.
- Audio files.
- Any compressed or proprietary data.

For each file, the name, size, hash, structure and likely purpose will be recorded.

It is not assumed that the engine is contained in an executable separate from the interface: this separation must be verified.

#### 5.2 Executable analysis

The following must be identified:

- DOS executable format.
- Execution architecture.
- 16-bit or 32-bit code.
- Any DOS extenders.
- Segmentation and memory model.
- Imports and system calls.
- Initialization routines.
- Any compression or packing schemes.

The main tools will be:

**Ghidra**

For disassembly, function analysis, cross-references and reconstruction of data structures.

**DOSBox with debugging support**

To observe program execution, trace the code and locate the routines associated with user operations.

**Python**

To analyze binary formats, locate tables, process memory dumps and develop comparison tools.

**C toolchain**

To progressively implement and verify the recovered components.

Correctly identifying the execution environment is an essential preliminary step.

### 6. Reverse engineering of the interface

For the interface, a primarily behavioral approach is adopted.

It will not be necessary to reconstruct the original code of the graphics routines instruction by instruction.

Instead, the visual states and the transitions caused by user actions must be documented and reproduced.

#### 6.1 Cataloguing the screens

The original program will be run in DOSBox.

Each screen will be documented through screenshots and descriptions.

For each element, the following will be recorded:

- Position.
- Size.
- Colors.
- Font.
- Graphical appearance.
- Initial state.
- Available interactions.
- Generated events.
- Effects on other components.

A complete catalogue of menus and windows must be prepared.

#### 6.2 Reconstruction of the controls

A small dedicated widget library will be developed.

It may include:

- Menus.
- Cascading menus.
- Buttons.
- Dialog boxes.
- Text boxes.
- Lists.
- Scrollbars.
- Panels.
- Labels.
- Chess-specific controls.

The library must not be generalized beyond what is necessary.

The purpose is to reproduce Fritz, not to build a universal GUI framework.

#### 6.3 Graphics assets

It must be established whether the pieces, fonts and interface elements are contained in external files, embedded in the executable or generated procedurally.

In the case of proprietary formats, dedicated decoders can be written.

In the case of procedural drawing, the equivalent graphics primitives can be reconstructed.

The use and redistribution of the original resources must be assessed separately from their technical study, respecting the applicable rights.

### 7. Reverse engineering of the engine

The engine requires a different process.

In this case, it is not enough to obtain a program capable of playing chess.

It is important to understand how the original engine works.

#### 7.1 Identification of the data structures

The first goal will be to locate and interpret the position representation.

The following will be searched for:

- Board arrays.
- Piece encoding.
- Side-to-move information.
- Castling rights.
- En passant state.
- Move stack.
- Search structures.

The recovered information will be documented before proceeding with a rewrite.

#### 7.2 Identification of the search

The execution flow leading from the current position to the choice of a move must be reconstructed.

The possible routines for the following will be studied:

- Move generation.
- Evaluation.
- Recursive search.
- Pruning.
- Quiescence.
- Move ordering.
- Time control.
- Search interruption.

No specific implementation of alpha-beta, negamax or other techniques is assumed until it is confirmed.

#### 7.3 Rewrite in C

The recovered functions will be progressively reimplemented in C.

A possible modern API could be:

```c
typedef struct Position Position;
typedef struct Move Move;

void generate_moves(
    const Position *position,
    Move *moves,
    int *count
);

void make_move(
    Position *position,
    Move move
);

void unmake_move(
    Position *position,
    Move move
);

int evaluate(
    const Position *position
);

Move find_best_move(
    Position *position,
    int depth
);
```

The signatures are indicative and do not represent structures recovered from Fritz.

The final architecture must reflect what emerges from the analysis.

The rewrite must favor algorithmic equivalence and clarity.

Any optimizations will be introduced only after verifiable behavior has been achieved.

### 8. Fidelity verification

The original DOS software will serve as the behavioral reference.

The native port must undergo comparison testing.

#### 8.1 Engine tests

Both versions will be given the same positions.

Where available, the following will be compared:

- Generated moves.
- Selected moves.
- Evaluations.
- Principal variations.
- Depth reached.
- Reactions to special conditions.
- Results in tactical positions.
- Endgame behavior.

To obtain meaningful comparisons, the search conditions will need to be controlled.

Execution time on modern processors does not match that of the original software.

Therefore it will be preferable to compare depth, nodes or other deterministic criteria, when they can be reconstructed, rather than relying solely on time.

#### 8.2 Interface tests

Each reconstructed screen will be visually compared with a screenshot of the DOS version.

The following will be analyzed:

- Coordinates.
- Proportions.
- Palette.
- Layout of components.
- Responses to input.
- Navigation sequences.
- Transitions between states.

Automated tests comparing images and event sequences may be developed.

#### 8.3 Functional tests

Complete scenarios must be reproduced:

- Program startup.
- Starting a new game.
- Entering moves.
- Playing against the computer.
- Menu navigation.
- Changing options.
- Loading a game.
- Saving.
- Closing the program.

The features actually available will be determined by analyzing the original version.

### 9. Project architecture

A modular structure is proposed, with a chess core independent of the graphical environment.

```text
fritz-reborn/
|
+-- CMakeLists.txt
+-- README.md
+-- LICENSE
|
+-- docs/
|   +-- architecture.md
|   +-- reverse-engineering.md
|   +-- engine-analysis.md
|   +-- ui-analysis.md
|   +-- compatibility.md
|
+-- src/
|   |
|   +-- main.c
|   |
|   +-- engine/
|   |   +-- position.c
|   |   +-- movegen.c
|   |   +-- search.c
|   |   +-- evaluate.c
|   |   +-- timecontrol.c
|   |   +-- opening.c
|   |
|   +-- gui/
|   |   +-- screen.c
|   |   +-- renderer.c
|   |   +-- palette.c
|   |   +-- fonts.c
|   |   +-- widgets.c
|   |   +-- menus.c
|   |   +-- dialogs.c
|   |   +-- chessboard.c
|   |   +-- panels.c
|   |
|   +-- input/
|   |   +-- mouse.c
|   |   +-- keyboard.c
|   |
|   +-- game/
|   |   +-- controller.c
|   |   +-- notation.c
|   |   +-- history.c
|   |   +-- persistence.c
|   |
|   +-- platform/
|       +-- sdl.c
|
+-- include/
|
+-- assets/
|
+-- tools/
|   +-- extract/
|   +-- compare/
|   +-- inspect/
|
+-- tests/
|   +-- engine/
|   +-- gui/
|   +-- integration/
|
+-- reference/
|   +-- README.md
|
+-- cmake/
```

This structure is preliminary.

The `reference` directory will be used to document how to obtain and run the original version and to keep any manifests and analysis results. The original binaries and assets must not be automatically included in the public repository.

### 10. Interaction between GUI and engine

The interface must communicate with the engine through a limited set of operations.

The application controller maintains the game state and coordinates the two subsystems.

The interface produces events:

```text
Mouse click
    |
    v
GUI
    |
    v
Game Controller
    |
    v
Chess Engine
    |
    v
Game State
    |
    v
GUI Rendering
```

The architecture must prevent the chess search loop from blocking the handling of graphical events.

The first implementation may use a cooperative search or a separate worker, depending on the characteristics of the reconstructed logic.

In any case, synchronization between the search and the game state must be explicit.

### 11. Development strategy

No attempt will be made to automatically convert the entire DOS executable to C.

The project will be developed in verifiable increments.

#### Phase 0 — Acquisition and identification

- Obtain an original distribution of Fritz 2.5 or 2.51.
- Identify the exact version.
- Compute the file hashes.
- Identify the components.
- Verify the executable format.
- Determine how to run it correctly.

**Outcome:** complete technical inventory of the distribution.

#### Phase 1 — Reference environment

- Configure DOSBox.
- Verify that the program works.
- Document the environment settings.
- Record screens and interactions.
- Prepare reproducible test positions.

**Outcome:** reference environment for comparison with the port.

#### Phase 2 — Interface analysis

- Catalogue the screens.
- Explore all the menus.
- Identify the graphical elements.
- Study the coordinate system.
- Analyze the inputs.
- Locate any graphics resources.

**Outcome:** behavioral and visual specification of the GUI.

#### Phase 3 — SDL3 graphics prototype

- Create a native window.
- Implement the virtual surface.
- Reproduce the main screen.
- Draw the chessboard.
- Reproduce the pieces.
- Implement mouse and keyboard.
- Reconstruct the first menus.

**Outcome:** first navigable replica of the interface.

#### Phase 4 — Engine analysis

- Locate the data structures.
- Locate move generation.
- Identify the search routines.
- Locate the evaluation.
- Study the playing levels.
- Document the dependencies between functions.

**Outcome:** technical map of the original engine.

#### Phase 5 — Engine implementation in C

- Reconstruct the position.
- Implement move generation.
- Implement making and unmaking moves.
- Reconstruct the evaluation.
- Reconstruct the search.
- Integrate any recovered heuristics.

**Outcome:** C engine independent of the GUI.

#### Phase 6 — Integration

- Connect the engine to the interface.
- Implement the game controller.
- Handle user moves.
- Handle computer moves.
- Reproduce clocks and indicators.
- Integrate loading and saving.

**Outcome:** first usable native version.

#### Phase 7 — Verification and compatibility

- Compare the algorithms.
- Compare the screens.
- Run regression tests.
- Fix divergences.
- Verify resource compatibility.
- Test on Linux, Windows and macOS.

**Outcome:** port validated against the original behavior.

### 12. Optional UCI interface

Once the engine has been reconstructed, it will be possible to develop a UCI adapter.

This would allow the historical engine to be used with other chess programs, including Pragma Chess.

The UCI interface is not required for the original replica and must not influence its graphical architecture.

It will be considered an additional feature, to be implemented after the engine has been reconstructed.

### 13. Possible future developments

Once a satisfactory replica has been achieved, some extensions may be considered:

- Resizable windowed mode.
- Configurable graphics scaling.
- HiDPI display support.
- PGN export.
- UCI interface.
- Support for modern formats.
- Integration with external databases.
- Engine analysis mode.
- Tools to compare the original engine with modern engines.

These features must remain optional and must not compromise the original experience.

### 14. Legal issues and distribution

The project must clearly distinguish between reverse engineering, study of the algorithms, functional reconstruction and distribution of derived materials.

The fact that an old version of Fritz is available for free does not imply that its code, its assets or the software are in the public domain.

The possibilities for decompilation, interoperability and use of the results depend on the applicable laws and terms.

It will therefore be necessary to:

- Verify the rights status of the original version.
- Avoid unauthorized publication of the original executables.
- Verify whether graphics, fonts and other resources may be redistributed.
- Document the provenance of resources.
- Distinguish between studied original code and the new implementation.
- Consider an independent implementation of the behavior when necessary.
- Seek specific authorization before distributing protected materials or derived code.

The choice of an open source license can only be made after clarifying which components can legitimately be distributed.

### 15. Main technical risks

**Complex DOS code**

The executable might use segmentation, DOS extenders, optimized assembly or compressed formats.

**Engine hard to isolate**

The chess functions might share structures and global state with the interface.

**Assets not directly recoverable**

Fonts and graphics might be compressed, embedded or produced by proprietary routines.

**Non-deterministic behavior**

The engine might depend on time, on the machine, on memory state or on heuristics that make an exact comparison difficult.

**Visual fidelity**

Correctly reproducing the DOS video mode might require attention to palettes, pixel aspect ratios and the characteristics of the original rendering.

**Size of the effort**

The complete reconstruction might be significantly more demanding than building a new chess application.

For this reason, each phase must produce independently verifiable results.

### 16. First concrete experiment

Before proceeding with the complete reconstruction, a feasibility test will be carried out.

It will start from the original distribution.

The goals will be:

1. Identify the executable format and its execution environment.
2. Verify that it starts in DOSBox.
3. Capture the main screen.
4. Identify the video mode and resolution.
5. Analyze the files that may contain graphics resources.
6. Reproduce the main screen through SDL3.
7. Locate in the binary at least one function attributable to the chess engine.
8. Reconstruct and verify a first function in C.

This test must demonstrate the feasibility of both the graphical replica and the algorithmic recovery.

### 17. Definition of the final result

The project can be considered successful when it is possible to:

- Launch Fritz natively on a modern operating system.
- Display an interface substantially faithful to the original.
- Navigate the menus and use the controls.
- Play a complete game.
- Have the computer make moves through the reconstructed engine.
- Obtain chess behavior consistent with the DOS version, within documented limits.
- Use the main original features.
- Build the project with a modern C toolchain.
- Run the program without DOS emulators.

### 18. Closing principle

**Fritz 2.5 Reborn must not be a clone that merely looks like Fritz. It must be a documented attempt to reconstruct the original program.**

The value of the project is twofold.

On the one hand, preserving a historical software experience, making it accessible again on modern systems.

On the other, studying and recovering the programming and chess artificial intelligence techniques used in the nineties.

The ambition is to recover as much of the original behavior as possible and deliver it in a small, understandable, verifiable and portable C codebase.

The first operational activity consists of acquiring the original Fritz 2.5 / 2.51 files and analyzing their structure, without assuming anything in advance about their internal organization.
