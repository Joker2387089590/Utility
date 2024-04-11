cmake_minimum_required(VERSION 3.18)
project(Utility LANGUAGES CXX)

set(sources
	TypeList.h
	CallableTrait.h
	VariantVisitor.h
	Tuple.h
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
	PrivateContructor.h
	Proxy.h
	RandomNumber.h
	ReadFile.h
	RollBack.h
	SignalSequence.h
	Singleton.h
	SpinLock.h
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

# Qt
find_package(QT NAMES Qt5 Qt6 QUIET)
if(QT_FOUND)
	list(APPEND sources
		WaitForSignal.h
		UnitValue.h
		QDebugHooker.h
		Validator.h
		VMap.h
	)
endif()

# EasyFmt
find_package(fmt CONFIG QUIET)
if(fmt_FOUND)
	option(UTILITY_EASYFMT_NO_CONSOLE "if EasyFmt print to console" OFF)
	if(UTILITY_EASYFMT_NO_CONSOLE)
		list(APPEND definitions EASY_FMT_NO_CONSOLE)
	endif()

	option(UTILITY_EASYFMT_USE_QT "if EasyFmt support Qt" ${QT_FOUND})
	if(NOT UTILITY_EASYFMT_USE_QT)
		list(APPEND definitions EASY_FMT_NO_QT)
	endif()

	list(APPEND sources EasyFmt.h)
	list(APPEND link_libs fmt::fmt)
endif()

# HTML
if(fmt_FOUND)
	option(UTILITY_HTML_USE_QT "if Html support Qt" ${QT_FOUND})
	if(NOT UTILITY_HTML_USE_QT)
		list(APPEND definitions UTILITY_HTML_NO_QT)
	endif()
	list(APPEND sources Html.h)
endif()

# FFT
find_package(fftw3 CONFIG QUIET)
option(UTILITY_USE_FFTW3 "use fftw3 wrapper" ${fftw3_FOUND})
if(UTILITY_USE_FFTW3)
	list(APPEND sources FFT.h)
endif()

# RollBack
option(UTILITY_NOT_USE_ROLLBACKS_NAMESPACE OFF)
if(UTILITY_NOT_USE_ROLLBACKS_NAMESPACE)
	list(APPEND definitions UTILITY_NOT_USE_ROLLBACKS_NAMESPACE)
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
