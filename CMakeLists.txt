# TegraRcmGUI-Linux — build (Fase 1)
# Port of eliboa/TegraRcmGUI (GPL-2.0) from Win32/MFC to Linux/Qt6.
cmake_minimum_required(VERSION 3.25)
project(tegrarcm-gui LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBUSB REQUIRED IMPORTED_TARGET libusb-1.0)

add_executable(tegrarcm-gui
    src/main.cpp
    src/MainWindow.cpp
    src/MainWindow.h
    src/Injector.cpp
    src/Injector.h
    src/Favorites.cpp
    src/Favorites.h
    src/FavoritesTab.cpp
    src/FavoritesTab.h
    src/SettingsTab.cpp
    src/SettingsTab.h
    src/ToolsTab.cpp
    src/ToolsTab.h
    src/UdevRules.cpp
    src/UdevRules.h
    assets/resources.qrc
)

target_link_libraries(tegrarcm-gui PRIVATE Qt6::Widgets PkgConfig::LIBUSB)

# Layout mirrors Injector::fuseeLauncherPath() / UdevRules::rulesSourcePath(),
# que buscan tools/ y udev/ como hermanos de bin/ (prefix/../tools, prefix/../udev).
install(TARGETS tegrarcm-gui RUNTIME DESTINATION bin)
install(FILES tools/fusee-launcher.py DESTINATION tools)
install(PROGRAMS tools/launcher_rcm DESTINATION tools)
install(FILES tools/launcher_rcm.cpp DESTINATION tools)
install(FILES tools/intermezzo.bin DESTINATION tools)
install(FILES udev/50-tegrarcm.rules DESTINATION udev)
install(FILES README.md DESTINATION share/doc/tegrarcm)
