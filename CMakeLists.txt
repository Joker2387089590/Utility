cmake_minimum_required(VERSION 3.18)
project(Utility LANGUAGES CXX)

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
set(include_dirs)
set(link_libs)
set(definitions)

# includes
set(def_include_dir "${PROJECT_SOURCE_DIR}/include")
cmake_path(NORMAL_PATH def_include_dir)
list(APPEND include_dirs ${def_include_dir})
option(UTILITY_INCLUDE_SELF "make headers directly included" OFF)
if(UTILITY_INCLUDE_SELF)
	list(APPEND include_dirs "${PROJECT_SOURCE_DIR}/include/Utility")
endif()

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
if(${nlohmann_json_FOUND} AND ${QT_FOUND})
	set(UTILITY_USE_NLOHMANN_JSON_QT_DEFAULT ON)
else()
	set(UTILITY_USE_NLOHMANN_JSON_QT_DEFAULT OFF)
endif()
option(UTILITY_USE_NLOHMANN_JSON_QT "use nlohmann json wrapper" ${UTILITY_USE_NLOHMANN_JSON_QT_DEFAULT})
if(UTILITY_USE_NLOHMANN_JSON_QT)
	list(APPEND sources NlohmannQt.h)
	list(APPEND link_libs 
		nlohmann_json::nlohmann_json
		${UTILITY_Qt}::Core
	)
endif()

# convert Qt JSON and Protobuf Struct
if(${Protobuf_FOUND} AND ${QT_FOUND})
	set(UTILITY_USE_PROTOBUF_QT_DEFAULT ON)
else()
	set(UTILITY_USE_PROTOBUF_QT_DEFAULT OFF)
endif()
option(UTILITY_USE_PROTOBUF_QT "use protobuf Struct wrapper" ${UTILITY_USE_PROTOBUF_QT_DEFAULT})
if(UTILITY_USE_PROTOBUF_QT)
	list(APPEND sources ProtobufQt.h)
	list(APPEND link_libs 
		protobuf::libprotobuf
		${UTILITY_Qt}::Core
	)
endif()

# convert nlohmann json and Protobuf Struct
if(${Protobuf_FOUND} AND ${nlohmann_json_FOUND})
	set(UTILITY_USE_NLOHMANN_JSON_PROTOBUF_DEFAULT ON)
else()
	set(UTILITY_USE_NLOHMANN_JSON_PROTOBUF_DEFAULT OFF)
endif()
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
	if(${QT_FOUND})
		set(UTILITY_WINERROR_NO_QT_DEFAULT OFF)
	else()
		set(UTILITY_WINERROR_NO_QT_DEFAULT ON)
	endif()
	option(UTILITY_WINERROR_NO_QT "disable Qt support in WinError.h" ${UTILITY_WINERROR_NO_QT_DEFAULT})

	list(APPEND sources WinError.h)
	if(${UTILITY_WINERROR_NO_QT})
		list(APPEND definitions UTILITY_WINERROR_NO_QT)
	endif()
endif()

# time tool
if(${QT_FOUND})
	set(UTILITY_TIMETOOL_NO_QDATETIME_DEFAULT OFF)
else()
	set(UTILITY_TIMETOOL_NO_QDATETIME_DEFAULT ON)
endif()
option(UTILITY_TIMETOOL_NO_QDATETIME "disable Qt support in TimeTool.h" ${UTILITY_TIMETOOL_NO_QDATETIME_DEFAULT})
if(${UTILITY_TIMETOOL_NO_QDATETIME})
	list(APPEND definitions UTILITY_TIMETOOL_NO_QDATETIME)
endif()

set(source_paths)
foreach(source IN LISTS sources)
	list(APPEND source_paths "include/Utility/${source}")
endforeach()

add_library(Utility INTERFACE)
target_sources(Utility INTERFACE ${source_paths})
target_include_directories(Utility INTERFACE ${include_dirs})
target_link_libraries(Utility INTERFACE ${link_libs})
target_compile_definitions(Utility INTERFACE ${definitions})
