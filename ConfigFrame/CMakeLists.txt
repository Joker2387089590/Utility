cmake_minimum_required(VERSION 3.14)

list(APPEND CMAKE_PREFIX_PATH "")

project(ConfigFrame LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOUIC_SEARCH_PATHS ${PROJECT_SOURCE_DIR}/ui)

find_package(QT
	NAMES Qt5
	COMPONENTS Widgets
	REQUIRED
)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets REQUIRED)

file(GLOB HEADERS "header/*.h")
file(GLOB SOURCES "src/*.cpp")
file(GLOB FORMS "ui/*.ui")

add_library(ConfigFrame SHARED
	${HEADERS}
	${SOURCES}
	${FORMS}
)

target_include_directories(ConfigFrame
	PUBLIC
		${PROJECT_SOURCE_DIR}/header
)

target_link_libraries(ConfigFrame PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

target_compile_definitions(ConfigFrame PRIVATE CONFIGFRAME_LIBRARY)
