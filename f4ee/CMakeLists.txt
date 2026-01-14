cmake_minimum_required(VERSION 3.18)

# ---- Project ----

project(f4ee LANGUAGES CXX)

#include(FetchContent)

#FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz DOWNLOAD_EXTRACT_TIMESTAMP YES)
#FetchContent_MakeAvailable(json)

#FetchContent_Declare(simpleini GIT_REPOSITORY https://github.com/brofield/simpleini.git)
#FetchContent_MakeAvailable(simpleini)

# ---- Include guards ----

if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
	message(
		FATAL_ERROR
			"In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there."
)
endif()

if (NOT TARGET xbyak)
	add_subdirectory(../f4se/xbyak xbyak)
endif()

file(GLOB headers CONFIGURE_DEPENDS *.h)
file(GLOB sources CONFIGURE_DEPENDS *.cpp)

include_directories(${PROJECT_NAME} PUBLIC ../f4se)
include_directories(${PROJECT_NAME} PUBLIC jsoncpp)

source_group(
	${PROJECT_NAME}
	FILES
	${headers}
	${sources}
)

include(cmake/helpers/generate_product_version.cmake)
generate_product_version(
   VersionFilesOutputVariable
   NAME "Fallout 4 Engine Extender"
   ICON ""
   VERSION_MAJOR 1
   VERSION_MINOR 7
   VERSION_PATCH 0
   VERSION_REVISION 1
)

set(F4SE_VERSION_MAJOR 0)
set(F4SE_VERSION_MINOR 7)
set(F4SE_VERSION_PATCH 1)

set(F4_VERSION_MAJOR 1)
set(F4_VERSION_MINOR 10)
set(F4_VERSION_PATCH 980)

math(
	EXPR
	F4_VERSION_PACKED
	"((${F4_VERSION_MAJOR} & 0xFF) << 24) | ((${F4_VERSION_MINOR} & 0xFF) << 16) | ((${F4_VERSION_PATCH} & 0xFFF) << 4)"
	OUTPUT_FORMAT
		HEXADECIMAL
)


add_library(
	${PROJECT_NAME}
	SHARED
	${headers}
	${sources}
	${VersionFilesOutputVariable}
)

# Build common as static library
file(GLOB common_header "../common/common/*.h")
file(GLOB common_sources "../common/common/*.cpp")
add_library(common_static STATIC ${common_header} ${common_sources})
target_include_directories(common_static PUBLIC ../common)
target_precompile_headers(common_static PUBLIC ../common/common/IPrefix.h)
set_target_properties(common_static PROPERTIES LINKER_LANGUAGE CXX MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
target_link_libraries(common_static PUBLIC)
target_compile_features(common_static PUBLIC cxx_std_11)

# Build f4se_common as static library
file(GLOB f4se_common_header "../f4se/f4se_common/*.h")
file(GLOB f4se_common_sources "../f4se/f4se_common/*.cpp")
add_library(f4se_common_static STATIC ${f4se_common_header} ${f4se_common_sources})
target_include_directories(f4se_common_static PUBLIC ../f4se)
target_include_directories(f4se_common_static PUBLIC ../common)
target_precompile_headers(f4se_common_static PUBLIC ../common/common/IPrefix.h)
set_target_properties(f4se_common_static PROPERTIES LINKER_LANGUAGE CXX MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
target_link_libraries(f4se_common_static PUBLIC)
target_compile_definitions(f4se_common_static PRIVATE RUNTIME RUNTIME_VERSION=${F4_VERSION_PACKED})
target_compile_features(f4se_common_static PUBLIC cxx_std_11)

# Build f4se as static library
file(GLOB f4se_header "../f4se/f4se/*.h")
file(GLOB f4se_sources "../f4se/f4se/*.cpp")
include(../f4se/cmake/versioning.cmake)
add_library(f4se_static STATIC ${f4se_header} ${f4se_sources} ${xbyak_SOURCE_DIR})
target_compile_definitions(f4se_static PRIVATE RUNTIME RUNTIME_VERSION=${F4_VERSION_PACKED})
target_include_directories(f4se_static PUBLIC ../f4se ../f4se/xbyak)
set_target_properties(f4se_static PROPERTIES LINKER_LANGUAGE CXX MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
target_link_libraries(f4se_static PUBLIC f4se_common_static)
target_compile_features(f4se_static PUBLIC cxx_std_11)

# Build jsoncpp as static library
file(GLOB jsoncpp_header "jsoncpp/*.h")
file(GLOB jsoncpp_sources "jsoncpp/*.cpp")
add_library(jsoncpp_static STATIC ${jsoncpp_header} ${jsoncpp_sources})
target_include_directories(jsoncpp_static PUBLIC jsoncpp)
set_target_properties(jsoncpp_static PROPERTIES LINKER_LANGUAGE CXX MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
target_compile_features(jsoncpp_static PUBLIC cxx_std_11)

# Link the new lib to the statically build libs
include(cmake/configuration.cmake)
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)
target_compile_definitions(${PROJECT_NAME} PRIVATE XBYAK_NO_OP_NAMES HALF_ROUND_STYLE=1 HALF_ENABLE_CPP11_CMATH=0)
target_include_directories(${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..> $<INSTALL_INTERFACE:include>)
target_link_libraries(
	${PROJECT_NAME} 
	PUBLIC 
	common_static 
	f4se_common_static 
	f4se_static 
	jsoncpp_static
	Shlwapi.lib
	Version.lib
)

#target_link_libraries(${PROJECT_NAME} PRIVATE nlohmann_json::nlohmann_json)
#include_directories(${PROJECT_NAME} PRIVATE ${simpleini_SOURCE_DIR})

set_target_properties(
	${PROJECT_NAME}
	common_static
	f4se_common_static
	f4se_static
	jsoncpp_static
	PROPERTIES
		MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
)

include(cmake/installation.cmake)