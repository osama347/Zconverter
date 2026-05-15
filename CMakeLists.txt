cmake_minimum_required(VERSION 3.28)
project(zconverter VERSION 1.0.0 LANGUAGES CXX)

include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(ZCONVERTER_LIBREOFFICE_ROOT "" CACHE PATH
    "Optional portable LibreOffice app root to bundle with the installer")

find_package(Qt6 REQUIRED COMPONENTS
    Core Widgets Gui PrintSupport
)

if(WIN32)
    find_program(WINDEPLOYQT_EXECUTABLE
        NAMES windeployqt windeployqt.exe
        HINTS
            "${Qt6_DIR}/../../../bin"
            "${Qt6_DIR}/../../bin"
            "${CMAKE_PREFIX_PATH}/bin"
    )
elseif(APPLE)
    find_program(MACDEPLOYQT_EXECUTABLE
        NAMES macdeployqt
        HINTS
            "${Qt6_DIR}/../../../bin"
            "${Qt6_DIR}/../../bin"
            "${CMAKE_PREFIX_PATH}/bin"
    )
endif()

set(ZCONVERTER_APP_SOURCES
    src/zconverter.cpp
    src/ui/MainWindow.cpp
    src/engine/ConversionEngine.cpp
    src/converters/ImageToPdfConverter.cpp
    src/converters/OfficeToPdfConverter.cpp
    src/cli/CommandLineParser.cpp
)

set(ZCONVERTER_APP_HEADERS
    src/cli/CommandLineParser.h
)

# ── Main executable ───────────────────────────────────────
add_executable(zconverter
    ${ZCONVERTER_APP_SOURCES}
    ${ZCONVERTER_APP_HEADERS}
)

target_include_directories(zconverter PRIVATE src)

target_link_libraries(zconverter PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Gui
    Qt6::PrintSupport
)

if(WIN32)
    target_link_libraries(zconverter PRIVATE
        shell32
        ole32
    )
    set_target_properties(zconverter PROPERTIES WIN32_EXECUTABLE TRUE)
elseif(APPLE)
    set_target_properties(zconverter PROPERTIES MACOSX_BUNDLE TRUE)
endif()

if(WIN32)
    # ── Shell extension DLL ───────────────────────────────────
    add_library(zconverter_shell SHARED
        src/shell/ShellExtension.cpp
        src/shell/ShellExtension.def
    )

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
endif()

install(TARGETS zconverter
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    BUNDLE DESTINATION .
)

if(WIN32)
    install(TARGETS zconverter_shell
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

    if(ZCONVERTER_LIBREOFFICE_ROOT AND EXISTS "${ZCONVERTER_LIBREOFFICE_ROOT}/program/soffice.exe")
        install(DIRECTORY "${ZCONVERTER_LIBREOFFICE_ROOT}/"
            DESTINATION ${CMAKE_INSTALL_BINDIR}/libreoffice
        )
    endif()

    if(WINDEPLOYQT_EXECUTABLE)
        configure_file(
            ${CMAKE_SOURCE_DIR}/cmake/DeployQtInstall.cmake.in
            ${CMAKE_BINARY_DIR}/DeployQtInstall.cmake
            @ONLY
        )
        install(SCRIPT ${CMAKE_BINARY_DIR}/DeployQtInstall.cmake)
    endif()
elseif(APPLE)
    if(ZCONVERTER_LIBREOFFICE_ROOT AND EXISTS "${ZCONVERTER_LIBREOFFICE_ROOT}/Contents/MacOS/soffice")
        install(DIRECTORY "${ZCONVERTER_LIBREOFFICE_ROOT}"
            DESTINATION "zconverter.app/Contents/Frameworks"
            USE_SOURCE_PERMISSIONS
        )
    endif()

    if(MACDEPLOYQT_EXECUTABLE)
        configure_file(
            ${CMAKE_SOURCE_DIR}/cmake/DeployMacInstall.cmake.in
            ${CMAKE_BINARY_DIR}/DeployMacInstall.cmake
            @ONLY
        )
        install(SCRIPT ${CMAKE_BINARY_DIR}/DeployMacInstall.cmake)
    endif()
endif()

if(WIN32)
    set(CPACK_GENERATOR "NSIS")
    set(CPACK_PACKAGE_NAME "zconverter")
    set(CPACK_PACKAGE_VENDOR "zconverter")
    set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "zconverter")
    set(CPACK_PACKAGE_FILE_NAME "zconverter-${PROJECT_VERSION}-win64")
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
    set(CPACK_NSIS_MODIFY_PATH OFF)
    set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS
        "ExecWait '\"$SYSDIR\\regsvr32.exe\" /s \"$INSTDIR\\bin\\zconverter_shell.dll\"'"
    )
    set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS
        "ExecWait '\"$SYSDIR\\regsvr32.exe\" /s /u \"$INSTDIR\\bin\\zconverter_shell.dll\"'"
    )
elseif(APPLE)
    set(CPACK_GENERATOR "DragNDrop")
    set(CPACK_PACKAGE_NAME "zconverter")
    set(CPACK_PACKAGE_VENDOR "zconverter")
    set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
    set(CPACK_PACKAGE_FILE_NAME "zconverter-${PROJECT_VERSION}-macos")
    set(CPACK_DMG_VOLUME_NAME "zconverter")
endif()

include(CPack)