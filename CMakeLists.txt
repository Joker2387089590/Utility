cmake_minimum_required(VERSION 3.18)
project(Utility LANGUAGES CXX)

add_library(Utility INTERFACE)

target_sources(Utility INTERFACE
	Access.h
	AnyHash.h
	CallableTrait.h
	CompareTie.h
	Complex.h
	DataView.h
	EasyFmt.h
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
	QDebugHooker.h
	RandomNumber.h
	ReadFile.h
	RollBack.h
	SignalSequence.h
	Singleton.h
	SpinLock.h
	Tuple.h
	TypeList.h
	UnitValue.h
	Validator.h
	VariantVisitor.h
	VMap.h
	WaitForSignal.h
)

# includes
list(APPEND include_dirs ${PROJECT_SOURCE_DIR}/..)
option(UTILITY_INCLUDE_SELF "make headers directly included" OFF)
if(UTILITY_INCLUDE_SELF)
	list(APPEND include_dirs ${PROJECT_SOURCE_DIR})
endif()
target_include_directories(Utility INTERFACE ${include_dirs})

# EasyFmt
option(UTILITY_EASYFMT_NO_CONSOLE "if EasyFmt print to console" OFF)
if(UTILITY_EASYFMT_NO_CONSOLE)
	target_compile_definitions(Utility INTERFACE EASY_FMT_NO_CONSOLE)
endif()

# FFT
find_package(fftw3 QUIET)
option(UTILITY_USE_FFTW3 "use fftw3 wrapper" ${fftw3_FOUND})
if(UTILITY_USE_FFTW3)
	target_sources(Utility FFT.h)
endif()
