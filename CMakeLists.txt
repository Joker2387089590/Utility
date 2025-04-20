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
	find_package(${UTILITY_Qt} QUIET COMPONENTS Core Widgets)
endif()
find_package(fmt CONFIG QUIET)
find_package(fftw3 CONFIG QUIET)
find_package(nlohmann_json CONFIG QUIET)
find_package(Protobuf CONFIG QUIET)
find_package(asio CONFIG QUIET)

# Qt
option(UTILITY_USE_QT_CORE_WRAPPERS "use Qt::Core wrappers" ${${UTILITY_Qt}Core_FOUND})
if(UTILITY_USE_QT_CORE_WRAPPERS)
	list(APPEND sources
		WaitForSignal.h
		UnitValue.h
		QDebugHooker.h
		VMap.h
	)
	list(APPEND link_libs ${UTILITY_Qt}::Core)
endif()
option(UTILITY_USE_QT_WIDGETS_WRAPPER "use Qt::Widgets wrappers" ${${UTILITY_Qt}Widgets_FOUND})
if(UTILITY_USE_QT_WIDGETS_WRAPPER)
	list(APPEND sources Validator.h)
	list(APPEND link_libs ${UTILITY_Qt}::Widgets)
endif()

# EasyFmt
if(fmt_FOUND)
	option(UTILITY_EASYFMT_NO_CONSOLE "if EasyFmt print to console" OFF)
	if(UTILITY_EASYFMT_NO_CONSOLE)
		list(APPEND definitions EASY_FMT_NO_CONSOLE)
	endif()

	option(UTILITY_EASYFMT_USE_QT "if EasyFmt support Qt" ${QT_FOUND})
	if(UTILITY_EASYFMT_USE_QT)
		list(APPEND link_libs ${UTILITY_Qt}::Core)
	else()
		list(APPEND definitions EASY_FMT_NO_QT)
	endif()

	set(UTILITY_EASYFMT_PRINT_COLOR "aqua" CACHE STRING "fmt::print stdout color")
	list(APPEND definitions UTILITY_EASYFMT_PRINT_COLOR=${UTILITY_EASYFMT_PRINT_COLOR})

	set(UTILITY_EASYFMT_ERROR_COLOR "crimson" CACHE STRING "fmt::print stderr color")
	list(APPEND definitions UTILITY_EASYFMT_ERROR_COLOR=${UTILITY_EASYFMT_ERROR_COLOR})

	set(UTILITY_EASYFMT_LINK_LIB "fmt::fmt-header-only" CACHE STRING "library to link for fmt")
	
	list(APPEND sources EasyFmt.h)
	list(APPEND link_libs ${UTILITY_EASYFMT_LINK_LIB})
endif()

# HTML
if(fmt_FOUND)
	option(UTILITY_HTML_USE_QT "if Html support Qt" ${QT_FOUND})
	if(NOT UTILITY_HTML_USE_QT)
		list(APPEND definitions UTILITY_HTML_NO_QT)
	endif()
	# TODO: add UTILITY_HTML_NO_QT_XML option
	list(APPEND sources Html.h)
endif()

# FFT
option(UTILITY_USE_FFTW3 "use fftw3 wrapper" ${fftw3_FOUND})
if(UTILITY_USE_FFTW3)
	list(APPEND sources FFT.h)
endif()

# RollBack
option(UTILITY_NOT_USE_ROLLBACKS_NAMESPACE OFF)
if(UTILITY_NOT_USE_ROLLBACKS_NAMESPACE)
	list(APPEND definitions UTILITY_NOT_USE_ROLLBACKS_NAMESPACE)
endif()

# convert Qt JSON and nlohmann json
set(UTILITY_USE_NLOHMANN_JSON_QT_DEFAULT $<AND:$<BOOL:${nlohmann_json_FOUND}>,$<BOOL:${QT_FOUND}>>)
option(UTILITY_USE_NLOHMANN_JSON_QT "use nlohmann json wrapper" ${UTILITY_USE_NLOHMANN_JSON_QT_DEFAULT})
if(UTILITY_USE_NLOHMANN_JSON_QT)
	list(APPEND sources NlohmannQt.h)
	list(APPEND link_libs 
		nlohmann_json::nlohmann_json
		${UTILITY_Qt}::Core
	)
endif()

# convert Qt JSON and Protobuf Struct
set(UTILITY_USE_PROTOBUF_QT_DEFAULT $<AND:$<BOOL:${Protobuf_FOUND}>,$<BOOL:${QT_FOUND}>>)
option(UTILITY_USE_PROTOBUF_QT "use protobuf Struct wrapper" ${UTILITY_USE_PROTOBUF_QT_DEFAULT})
if(UTILITY_USE_PROTOBUF_QT)
	list(APPEND sources ProtobufQt.h)
	list(APPEND link_libs 
		protobuf::libprotobuf
		${UTILITY_Qt}::Core
	)
endif()

# convert nlohmann json and Protobuf Struct
set(UTILITY_USE_NLOHMANN_JSON_PROTOBUF_DEFAULT $<AND:$<BOOL:${Protobuf_FOUND}>,$<BOOL:${nlohmann_json_FOUND}>>)
option(UTILITY_USE_NLOHMANN_JSON_PROTOBUF "conversion between protobuf Struct and nlohmann json" ${UTILITY_USE_NLOHMANN_JSON_PROTOBUF_DEFAULT})
if(UTILITY_USE_NLOHMANN_JSON_PROTOBUF)
	list(APPEND sources NlohmannProtobuf.h)
	list(APPEND link_libs
		protobuf::libprotobuf
		nlohmann_json::nlohmann_json
	)
endif()

# win error
if(WIN32)
	option(UTILITY_WINERROR_NO_QT "disable Qt support in WinError.h" $<NOT:$<BOOL:${QT_FOUND}>>)
	list(APPEND sources WinError.h)
	if(${UTILITY_WINERROR_NO_QT})
		list(APPEND definitions UTILITY_WINERROR_NO_QT)
	endif()
endif()

# time tool
option(UTILITY_TIMETOOL_NO_QDATETIME "disable Qt support in TimeTool.h" $<NOT:$<BOOL:${QT_FOUND}>)
if(${UTILITY_TIMETOOL_NO_QDATETIME})
	list(APPEND definitions UTILITY_TIMETOOL_NO_QDATETIME)
endif()

# asio
option(UTILITY_USE_CO_ASIO_QT "use asio wrapper" $<AND:$<BOOL:${asio_FOUND}>,$<BOOL:${QT_FOUND}>>)
if(UTILITY_USE_CO_ASIO_QT)
	list(APPEND sources CoAsioQt.h)
	list(APPEND link_libs asio::asio)
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
