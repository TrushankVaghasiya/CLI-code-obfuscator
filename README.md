# CLI-code-obfuscator

A C++ source-code obfuscator built using **C++20**, **Tree-sitter**, and **CLI11**. It analyzes C++ source code using its Abstract Syntax Tree (AST) and applies source transformations at configurable obfuscation levels.

**Repository:** [CLI-code-obfuscator](https://github.com/TrushankVaghasiya/CLI-code-obfuscator)

## Features

- **AST-based analysis:** Uses Tree-sitter C++ to parse and analyze source code.
- **Identifier obfuscation:** Renames eligible local variables and other supported identifiers.
- **Scope-aware renaming:** Handles nested scopes, shadowing, and variable references.
- **Function obfuscation:** Renames eligible free functions, including supported declarations, definitions, recursive calls, and direct function calls.
- **Function parameter obfuscation:** Supports eligible function and lambda parameters.
- **Member obfuscation:** Renames eligible class and struct data members and their references.
- **Comment removal:** Removes comments at obfuscation level 2 and above.
- **Integer literal transformation:** Transforms eligible integer literals at obfuscation level 3.
- **Conservative transformations:** Skips certain identifiers when safe renaming cannot be established.
- **Command-line interface:** Supports input/output paths and configurable obfuscation levels.
- **Testing and validation:** Includes transformation, compilation, and runtime-equivalence tests.

## Obfuscation Levels

| Level | Transformations |
|---|---|
| 1 | Base identifier and AST-based obfuscation transformations |
| 2 | Level 1 transformations + comment removal |
| 3 | Level 2 transformations + eligible integer literal transformations |

Higher levels include the transformations of all lower levels.

## Technologies Used

| Technology | Version / Standard | Purpose |
|---|---|---|
| C++ | C++20 | Core implementation |
| CMake | 3.20 or higher | Build system |
| CLI11 | v2.5.0 | Command-line argument parsing |
| Tree-sitter | v0.25.10 | Source-code parsing |
| Tree-sitter C++ | v0.23.4 | C++ grammar |

Dependencies are configured through CMake FetchContent and downloaded during the initial configuration.

## Project Structure

```text
CLI-code-obfuscator/
│
├── include/
│   ├── cli.hpp
│   ├── identifier_analyzer.hpp
│   ├── literal_transformer.hpp
│   ├── parser.hpp
│   ├── transformer.hpp
│   └── validator.hpp
│
├── src/
│   ├── main.cpp
│   ├── cli.cpp
│   ├── validator.cpp
│   ├── parser.cpp
│   ├── identifier_analyzer.cpp
│   ├── transformer.cpp
│   └── literal_transformer.cpp
│
├── tests/
│   ├── data/
│   ├── ast_inspector.cpp
│   ├── check_compilation.cmake
│   ├── check_runtime_equivalence.cmake
│   ├── check_transform_basic.cmake
│   └── ... additional test scripts
│
├── manual_tests/
│
├── CMakeLists.txt
├── .gitignore
└── README.md
```

### Main Components

| File | Responsibility |
|---|---|
| `main.cpp` | Application entry point and orchestration |
| `cli.cpp` / `cli.hpp` | Defines and configures command-line options |
| `parser.cpp` / `parser.hpp` | Parses C++ source code using Tree-sitter |
| `validator.cpp` / `validator.hpp` | Handles source validation |
| `identifier_analyzer.cpp` / `.hpp` | Analyzes identifiers in the parsed source |
| `transformer.cpp` / `.hpp` | Performs AST-based source transformations |
| `literal_transformer.cpp` / `.hpp` | Handles integer literal transformations |
| `ast_inspector.cpp` | Utility for inspecting parsed AST structures |

## Getting Started

### Prerequisites

- Windows
- Visual Studio with C++ development tools (MSVC)
- CMake 3.20 or higher
- Git
- An internet connection for the initial dependency download

The runtime-equivalence test scripts accept a compiler path through the `CXX_COMPILER` argument. The configured compiler must support the `-std=c++17` option used by the test harness.

### 1. Clone the Repository

```powershell
git clone https://github.com/TrushankVaghasiya/CLI-code-obfuscator.git
cd CLI-code-obfuscator
```

### 2. Configure the Project

```powershell
cmake -S . -B build
```

This configures the project and downloads its declared dependencies through CMake FetchContent.

### 3. Build the Project

```powershell
cmake --build build --config Debug
```

The executable will be generated at:

```text
build/Debug/obfuscator.exe
```

## Usage

### Display Help

```powershell
.\build\Debug\obfuscator.exe --help
```

### Basic Obfuscation

```powershell
.\build\Debug\obfuscator.exe -i sample.cpp -o output.cpp
```

The default obfuscation level is `1`.

### Specify an Obfuscation Level

```powershell
.\build\Debug\obfuscator.exe -i sample.cpp -o output.cpp --level 2
```

```powershell
.\build\Debug\obfuscator.exe -i sample.cpp -o output.cpp --level 3
```

### Command-Line Options

| Option | Description |
|---|---|
| `-h`, `--help` | Display help information |
| `--version` | Display program version |
| `-i`, `--input` | Input C++ source file (required) |
| `-o`, `--output` | Output file for transformed source (required) |
| `--level` | Obfuscation level from 1 to 3 (default: 1) |

## Testing

The project uses CTest for automated testing.

### Run All Tests

```powershell
ctest --test-dir build -C Debug --output-on-failure
```

### List Registered Tests

```powershell
ctest --test-dir build -C Debug -N
```

### Test Coverage

The project currently contains **53 registered tests**, covering:

- CLI help and argument validation
- Invalid obfuscation levels
- Input/output validation
- Basic transformations
- Comment removal
- Integer literal transformations
- Nested scopes and variable shadowing
- For-loop, if/else, and while-loop scopes
- Lambda captures and parameters
- Function parameters and function shadowing
- Class and struct member transformations
- Pointer and reference parameters
- Compilation of transformed programs
- Runtime equivalence between original and transformed programs

Runtime-equivalence tests compare execution results of original and transformed programs using the test harness.

## Design Considerations

- Tree-sitter provides syntactic structure for source-code analysis.
- Transformations use AST information rather than relying only on text replacement.
- Scope and reference relationships are considered when renaming identifiers.
- Potentially unsafe transformations are skipped conservatively.
- Compilation and runtime-equivalence tests help validate transformation correctness.

## Current Limitations

- Not every valid C++ construct is guaranteed to be transformable.
- Overloaded functions are skipped by the current function-renaming logic.
- Functions without definitions in the current translation unit are skipped.
- Potential function-name shadowing can cause a function to be skipped to avoid unsafe renaming.
- Transformations are limited to constructs that the current analysis can resolve safely.

Obfuscation is not encryption or a security boundary. Transformed source code can still be analyzed and reverse-engineered.

## Future Improvements

- Improved handling of overloaded functions.
- More complete C++ semantic analysis.
- Broader support for complex templates and function-pointer expressions.
- Additional transformation strategies.
- Expanded regression and runtime-equivalence testing.
