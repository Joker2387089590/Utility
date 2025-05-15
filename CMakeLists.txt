cmake_minimum_required(VERSION 3.28)
project(Utility VERSION 0.1.0 LANGUAGES CXX)

### add sources
set(sources
	TypeList.h
	CallableTrait.h
	VariantVisitor.h
	Tuple.h
	Literal.h
	StrEnums.h
	Access.h
	AnyHash.h
	CompareTie.h
	Complex.h
	DataView.h
	EnumRanges.h
	IntWrapper.h
	LazyGenerator.h
	LinkedMap.h
	Macros.h
	MacrosUndef.h
	MoveOnlyFunctor.h
	NumRange.h
	ObjectAddress.h
	Proxy.h
	RandomNumber.h
	ReadFile.h
	RollBack.h
	SignalSequence.h
	TimeTool.h
)
set(link_libs)
set(definitions)

# includes
option(UTILITY_INCLUDE_SELF "make headers directly included" OFF)

# find packages to be wrapped
find_package(QT NAMES Qt5 Qt6 QUIET)
if(QT_FOUND)
	set(UTILITY_Qt "Qt${QT_VERSION_MAJOR}" CACHE STRING "select Qt version")
	find_package(${UTILITY_Qt} QUIET COMPONENTS Core Widgets Sql Xml)
endif()
find_package(fmt CONFIG QUIET)
find_package(fftw3 CONFIG QUIET)
find_package(nlohmann_json CONFIG QUIET)
find_package(Protobuf CONFIG QUIET)
find_package(asio CONFIG QUIET)
find_package(range-v3 CONFIG QUIET)

# Qt Core
if(${${UTILITY_Qt}Core_FOUND})
	list(APPEND sources
		WaitForSignal.h
		UnitValue.h
		QDebugHooker.h
		VMap.h
	)
	list(APPEND link_libs ${UTILITY_Qt}::Core)
endif()

# Qt Widgets
if(${${UTILITY_Qt}Widgets_FOUND})
	list(APPEND sources Validator.h)
	list(APPEND link_libs ${UTILITY_Qt}::Widgets)
endif()

# EasyFmt
if(fmt_FOUND)
	option(UTILITY_EASYFMT_NO_CONSOLE "if EasyFmt print to console" OFF)
	if(UTILITY_EASYFMT_NO_CONSOLE)
		list(APPEND definitions EASY_FMT_NO_CONSOLE)
	endif()

	if(${${UTILITY_Qt}Core_FOUND})
		list(APPEND link_libs ${UTILITY_Qt}::Core)
		list(APPEND sources FmtQt.h)
	else()
		list(APPEND definitions EASY_FMT_NO_QT)
	endif()

	list(APPEND sources EasyFmt.h)

	set(UTILITY_EASYFMT_PRINT_COLOR "aqua" CACHE STRING "fmt::print stdout color")
	set(UTILITY_EASYFMT_ERROR_COLOR "crimson" CACHE STRING "fmt::print stderr color")
	list(APPEND definitions 
		UTILITY_EASYFMT_PRINT_COLOR=${UTILITY_EASYFMT_PRINT_COLOR}
		UTILITY_EASYFMT_ERROR_COLOR=${UTILITY_EASYFMT_ERROR_COLOR}
	)
	
	set(UTILITY_EASYFMT_LINK_LIB "fmt::fmt-header-only" CACHE STRING "library to link for fmt")
	list(APPEND link_libs ${UTILITY_EASYFMT_LINK_LIB})

	set(UTILITY_HAS_EASYFMT ON)
else()
	set(UTILITY_HAS_EASYFMT OFF)
endif()

# Qt Sql sqlite3
if(${${UTILITY_Qt}Sql_FOUND} AND ${range-v3_FOUND} AND ${UTILITY_HAS_EASYFMT})
	list(APPEND sources
		Sqlite3.h
	)
	list(APPEND link_libs ${UTILITY_Qt}::Sql range-v3::range-v3)
endif()

# HTML
if(fmt_FOUND)
	if(${QT_FOUND})
		list(APPEND link_libs ${UTILITY_Qt}::Core)
	else()
		list(APPEND definitions UTILITY_HTML_NO_QT)
	endif()
	
	if(${${UTILITY_Qt}Xml_FOUND})
		list(APPEND link_libs ${UTILITY_Qt}::Xml)
	else()
		list(APPEND definitions UTILITY_HTML_NO_QT_XML)
	endif()

	list(APPEND sources Html.h)
endif()

# FFTw3
if(${fftw3_FOUND})
	list(APPEND sources FFT.h)
endif()

# RollBack
option(UTILITY_NOT_USE_ROLLBACKS_NAMESPACE OFF)
if(UTILITY_NOT_USE_ROLLBACKS_NAMESPACE)
	list(APPEND definitions UTILITY_NOT_USE_ROLLBACKS_NAMESPACE)
endif()

# convert Qt JSON and nlohmann json
if(${nlohmann_json_FOUND} AND ${QT_FOUND})
	list(APPEND sources NlohmannQt.h)
	list(APPEND link_libs 
		nlohmann_json::nlohmann_json
		${UTILITY_Qt}::Core
	)
endif()

# convert Qt JSON and Protobuf Struct
if(${Protobuf_FOUND} AND ${QT_FOUND})
	list(APPEND sources ProtobufQt.h)
	list(APPEND link_libs 
		protobuf::libprotobuf
		${UTILITY_Qt}::Core
	)
endif()

# convert nlohmann json and Protobuf Struct
if(${Protobuf_FOUND} AND ${nlohmann_json_FOUND})
	list(APPEND sources NlohmannProtobuf.h)
	list(APPEND link_libs
		protobuf::libprotobuf
		nlohmann_json::nlohmann_json
	)
endif()

# win error
if(WIN32)
	if(NOT ${QT_FOUND})
		list(APPEND definitions UTILITY_WINERROR_NO_QT)
	endif()
	list(APPEND sources WinError.h)
endif()

# time tool
if(NOT ${QT_FOUND})
	list(APPEND definitions UTILITY_TIMETOOL_NO_QDATETIME)
endif()

# asio
if(${asio_FOUND} AND ${QT_FOUND})
	list(APPEND sources CoAsioQt.h)
	list(APPEND link_libs 
		asio::asio
		${UTILITY_Qt}::Core
	)
endif()

### add library
set(source_paths)
foreach(source IN LISTS sources)
	list(APPEND source_paths "include/Utility/${source}")
endforeach()

include(GNUInstallDirs)

add_library(Utility INTERFACE)
add_library(Utility::Utility ALIAS Utility)

set(_base_dirs include)
if(UTILITY_INCLUDE_SELF)
	list(APPEND _base_dirs include/Utility)
endif()

target_sources(Utility INTERFACE
	FILE_SET utility_file_set
	TYPE HEADERS
	BASE_DIRS ${_base_dirs}
	FILES ${source_paths}
)

target_include_directories(Utility INTERFACE
	"$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
)
if(UTILITY_INCLUDE_SELF)
	target_include_directories(Utility INTERFACE
		"$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include/Utility>"
	)
endif()

target_link_libraries(Utility INTERFACE ${link_libs})

target_compile_definitions(Utility INTERFACE ${definitions})

### install
install(
	TARGETS Utility
	EXPORT UtilityTargets
	INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
	FILE_SET utility_file_set
)

install(
	EXPORT UtilityTargets
	FILE UtilityTargets.cmake
	NAMESPACE Utility::
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Utility
)

include(CMakePackageConfigHelpers)

write_basic_package_version_file(
	"UtilityConfigVersion.cmake"
	VERSION ${PROJECT_VERSION}
	COMPATIBILITY AnyNewerVersion
)

configure_package_config_file(
    "${PROJECT_SOURCE_DIR}/cmake/UtilityConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/UtilityConfig.cmake"
    INSTALL_DESTINATION
        ${CMAKE_INSTALL_LIBDIR}/cmake/Utility
)

install(
    FILES 
        "${CMAKE_CURRENT_BINARY_DIR}/UtilityConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/UtilityConfigVersion.cmake"
    DESTINATION
        ${CMAKE_INSTALL_LIBDIR}/cmake/Utility
)
