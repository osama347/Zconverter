cmake_minimum_required(VERSION 3.28)
project(zconverter VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS
    Core Widgets Gui PrintSupport
)

# ── Main executable ───────────────────────────────────────
add_executable(zconverter WIN32
    src/zconverter.cpp
    src/ui/MainWindow.cpp
    src/engine/ConversionEngine.cpp
    src/converters/ImageToPdfConverter.cpp
    src/converters/OfficeToPdfConverter.cpp
 "src/cli/CommandLineParser.h" "src/cli/CommandLineParser.cpp")

target_include_directories(zconverter PRIVATE src)

target_link_libraries(zconverter PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Gui
    Qt6::PrintSupport
    shell32
    ole32
)

# ── Shell extension DLL ───────────────────────────────────
add_library(zconverter_shell SHARED
    src/shell/ShellExtension.cpp
    src/shell/ShellExtension.def
 "src/cli/CommandLineParser.h" "src/cli/CommandLineParser.cpp")

target_include_directories(zconverter_shell PRIVATE src)

target_link_libraries(zconverter_shell PRIVATE
    shlwapi
    shell32
    ole32
)

# Output DLL next to EXE in the same build folder
set_target_properties(zconverter_shell PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)