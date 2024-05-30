cmake_minimum_required(VERSION 3.22)
set(CMAKE_C_FLAGS_RELEASE "-Os -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-Os -DNDEBUG")
set(CMAKE_C_FLAGS_DEBUG "-g -O0 -DDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG")
set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 20)
set(ignoreMe "${NO_PKGCFG} ${ASAN} ${NO_XCODE}")
project(patchfinder)
if("${CMAKE_HOST_SYSTEM_NAME}" MATCHES "Darwin")
	if(NOT DEFINED NO_XCODE AND NOT DEFINED ENV{NO_XCODE})
	execute_process(COMMAND xcrun --find clang WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}" OUTPUT_VARIABLE CC OUTPUT_STRIP_TRAILING_WHITESPACE)
	execute_process(COMMAND xcrun --find clang++ WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}" OUTPUT_VARIABLE CXX OUTPUT_STRIP_TRAILING_WHITESPACE)
	execute_process(COMMAND xcrun --find ar WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}" OUTPUT_VARIABLE AR OUTPUT_STRIP_TRAILING_WHITESPACE)
	execute_process(COMMAND xcrun --find ld WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}" OUTPUT_VARIABLE LD OUTPUT_STRIP_TRAILING_WHITESPACE)
	execute_process(COMMAND xcrun --show-sdk-path WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}" OUTPUT_VARIABLE SYSROOT OUTPUT_STRIP_TRAILING_WHITESPACE)
	set(CMAKE_C_COMPILER "${CC}")
	set(CMAKE_CXX_COMPILER "${CXX}")
	endif()
	if(NOT DEFINED ARCH)
	set(ARCH "$ENV{ARCH}")
	if(ARCH STREQUAL "")
		set(ARCH "${CMAKE_SYSTEM_PROCESSOR}")
	endif()
	endif()
	if(NOT DEFINED MINVER OR NOT DEFINED "$ENV{MINVER}" AND DEFINED NO_PKGCFG OR "$ENV{NO_PKGCFG}" MATCHES "1")
		if("${ARCH}" STREQUAL "x86_64" OR "$ENV{ARCH}" STREQUAL "x86_64")
			set(MINVER -mmacosx-version-min=10.12)
			set(CMAKE_OSX_DEPLOYMENT_TARGET 10.12)
		else()
			set(MINVER -mmacosx-version-min=11.0)
			set(CMAKE_OSX_DEPLOYMENT_TARGET 11.0)
		endif()
	endif()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -arch ${ARCH} -isysroot ${SYSROOT} ${MINVER} -fembed-bitcode=off -flto=thin -pthread -mrelax-all -std=gnu++20")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -arch ${ARCH} -isysroot ${SYSROOT} ${MINVER} -fembed-bitcode=off -flto=thin -pthread -mrelax-all -std=gnu17")
elseif("${CMAKE_HOST_SYSTEM_NAME}" MATCHES "Linux")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,--allow-multiple-definition -pthread")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,--allow-multiple-definition -pthread")
endif()
FILE(GLOB ibootpatchfinder32_sources src/ibootpatchfinder/ibootpatchfinder32_iOS*.cpp)
FILE(GLOB ibootpatchfinder64_sources src/ibootpatchfinder/ibootpatchfinder64_iOS*.cpp)
FILE(GLOB kernelpatchfinder32_sources src/kernelpatchfinder/kernelpatchfinder32_iOS*.cpp)
FILE(GLOB kernelpatchfinder64_sources src/kernelpatchfinder/kernelpatchfinder64_iOS*.cpp)
set(patchfinder_src
		src/ibootpatchfinder/ibootpatchfinder.cpp
		src/ibootpatchfinder/ibootpatchfinder32.cpp
		src/ibootpatchfinder/ibootpatchfinder32_base.cpp
		${ibootpatchfinder32_sources}
		src/ibootpatchfinder/ibootpatchfinder64.cpp
		src/ibootpatchfinder/ibootpatchfinder64_base.cpp
		${ibootpatchfinder64_sources}
		src/kernelpatchfinder/kernelpatchfinder.cpp
		src/kernelpatchfinder/kernelpatchfinder32.cpp
		src/kernelpatchfinder/kernelpatchfinder32_base.cpp
		${kernelpatchfinder32_sources}
		src/kernelpatchfinder/kernelpatchfinder64.cpp
		src/kernelpatchfinder/kernelpatchfinder64_base.cpp
		${kernelpatchfinder64_sources}
		src/machopatchfinder32.cpp
		src/machopatchfinder64.cpp
		src/patch.cpp
		src/patchfinder.cpp
		src/patchfinder32.cpp
		src/patchfinder64.cpp
		src/StableHash.cpp)
add_library(patchfinder STATIC ${patchfinder_src})
add_library(patchfinder_shared SHARED ${patchfinder_src})
set_target_properties(patchfinder_shared PROPERTIES OUTPUT_NAME "patchfinder.0")
set(patchfinder_include
        "${CMAKE_SOURCE_DIR}/dep_root/include"
		src
		include/libpatchfinder
		include)
target_include_directories(patchfinder PRIVATE ${patchfinder_include})
target_include_directories(patchfinder_shared PRIVATE ${patchfinder_include})
set(patchfinder_link_dirs "${CMAKE_SOURCE_DIR}/dep_root/lib")
target_link_directories(patchfinder PRIVATE ${patchfinder_link_dirs})
target_link_directories(patchfinder_shared PRIVATE ${patchfinder_link_dirs})
if(NOT DEFINED NO_PKGCFG AND NOT "$ENV{NO_PKGCFG}" MATCHES "1")
    find_package(PkgConfig REQUIRED)
	if("${CMAKE_HOST_SYSTEM_NAME}" MATCHES "Darwin")
	else()
		set(compression_lib lzfse)
	endif()
    pkg_check_modules(DEPS REQUIRED
			${compression_lib}
			openssl
            libgeneral
            libinsn
			libplist-2.0
			img3tool
			img4tool)
    target_include_directories(patchfinder PRIVATE "${DEPS_INCLUDE_DIRS}")
    target_include_directories(patchfinder_shared PRIVATE "${DEPS_INCLUDE_DIRS}")
	target_link_directories(patchfinder PRIVATE "${DEPS_LIBRARY_DIRS}")
	target_link_directories(patchfinder_shared PRIVATE "${DEPS_LIBRARY_DIRS}")
endif()
if("${CMAKE_HOST_SYSTEM_NAME}" MATCHES "Darwin")
	set(patchfinder_libs compression)
else()
	set(patchfinder_libs lzfse)
endif()
set(patchfinder_libs
		${patchfinder_libs}
		crypto
		"-lgeneral"
		insn
		plist-2.0
		img3tool
		img4tool)
target_link_libraries(patchfinder PRIVATE ${patchfinder_libs})
target_link_libraries(patchfinder_shared PRIVATE ${patchfinder_libs})

if(NOT DEFINED VERSION_COMMIT_COUNT)
	execute_process(COMMAND git rev-list --count HEAD WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}" OUTPUT_VARIABLE VERSION_COMMIT_COUNT ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()
if(NOT DEFINED VERSION_COMMIT_SHA)
	execute_process(COMMAND git rev-parse HEAD WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}" OUTPUT_VARIABLE VERSION_COMMIT_SHA ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()
execute_process(COMMAND cp -aL "${CMAKE_SOURCE_DIR}/pkgconfig/libpatchfinder.pc.in" "${CMAKE_SOURCE_DIR}/pkgconfig/libpatchfinder.pc" WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}" ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
if(DEFINED CMAKE_MODULE_PATH)
	set(pkg_prefix_path1 "${CMAKE_MODULE_PATH}/")
else()
	set(pkg_prefix_path1 "${CMAKE_SOURCE_DIR}/dep_root/")
endif()
string(REPLACE "/" "\\/" pkg_prefix_path ${pkg_prefix_path1})
execute_process(COMMAND sed -i "s/@prefix@/${pkg_prefix_path}/g" ${CMAKE_SOURCE_DIR}/pkgconfig/libpatchfinder.pc WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}" ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND sed -i "s/@exec_prefix@/$\{prefix\}\\/bin/g" ${CMAKE_SOURCE_DIR}/pkgconfig/libpatchfinder.pc WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}" ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND sed -i "s/@libdir@/$\{prefix\}\\/lib/g" ${CMAKE_SOURCE_DIR}/pkgconfig/libpatchfinder.pc WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}" ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND sed -i "s/@includedir@/$\{prefix\}\\/include/g" ${CMAKE_SOURCE_DIR}/pkgconfig/libpatchfinder.pc WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}" ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND sed -i "s/@VERSION_COUNT@/${VERSION_COMMIT_COUNT}/g" ${CMAKE_SOURCE_DIR}/pkgconfig/libpatchfinder.pc WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}" ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND sed -i "s/@VERSION_COMMIT_COUNT@/${VERSION_COMMIT_COUNT}/g" ${CMAKE_SOURCE_DIR}/pkgconfig/libpatchfinder.pc WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}" ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND sed -i "s/@img3tool_requires@/libimg3tool/g" ${CMAKE_SOURCE_DIR}/pkgconfig/libpatchfinder.pc WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}" ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND sed -i "s/@img4tool_requires@/libimg4tool/g" ${CMAKE_SOURCE_DIR}/pkgconfig/libpatchfinder.pc WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}" ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND sed -i "s/@libgeneral_requires@/libgeneral/g" ${CMAKE_SOURCE_DIR}/pkgconfig/libpatchfinder.pc WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}" ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND sed -i "s/@libinsn_requires@/libinsn/g" ${CMAKE_SOURCE_DIR}/pkgconfig/libpatchfinder.pc WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}" ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
string(REPLACE "/" "\\/" CMAKE_INSTALL_PREFIX1 ${CMAKE_INSTALL_PREFIX})
execute_process(COMMAND sed -i "s/PREFIX/${CMAKE_INSTALL_PREFIX1}\\//g" "${CMAKE_SOURCE_DIR}/pkgconfig/libpatchfinder.pc" WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}" OUTPUT_STRIP_TRAILING_WHITESPACE)
if("${CMAKE_HOST_SYSTEM_NAME}" MATCHES "MSYS" OR "${CMAKE_HOST_SYSTEM_NAME}" MATCHES "Windows")
	set(net_def -DHAVE_WINSOCK_H=1)
else()
	set(net_def -DHAVE_ARPA_INET_H=1)
endif()
add_definitions(
		-DVERSION_COMMIT_COUNT="${VERSION_COMMIT_COUNT}"
		-DVERSION_COMMIT_SHA="${VERSION_COMMIT_SHA}"
		-DPACKAGE_NAME="libpatchfinder"
		-DPACKAGE_VERSION="Build: ${VERSION_COMMIT_COUNT}\(${VERSION_COMMIT_SHA}\)"
		-DHAVE_MEMMEM=1
		${net_def}
		-DHAVE_IMG3TOOL=1
		-DHAVE_IMG4TOOL=1)
if(DEFINED DESTDIR)
	set(CMAKE_INSTALL_PREFIX ${DESTDIR}${CMAKE_INSTALL_PREFIX})
endif()
install(FILES pkgconfig/libpatchfinder.pc
		DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/pkgconfig")
ADD_CUSTOM_TARGET(link_target ALL
		COMMAND ln -sfr "${CMAKE_BINARY_DIR}/libpatchfinder.0.dylib" "${CMAKE_BINARY_DIR}/libpatchfinder.dylib")
install(FILES
		include/libpatchfinder/ibootpatchfinder/ibootpatchfinder.hpp
		include/libpatchfinder/ibootpatchfinder/ibootpatchfinder32.hpp
		include/libpatchfinder/ibootpatchfinder/ibootpatchfinder64.hpp
		include/libpatchfinder/kernelpatchfinder/kernelpatchfinder.hpp
		include/libpatchfinder/kernelpatchfinder/kernelpatchfinder32.hpp
		include/libpatchfinder/kernelpatchfinder/kernelpatchfinder64.hpp
		include/libpatchfinder/machopatchfinder32.hpp
		include/libpatchfinder/machopatchfinder64.hpp
		include/libpatchfinder/OFexception.hpp
		include/libpatchfinder/patch.hpp
		include/libpatchfinder/patchfinder.hpp
		include/libpatchfinder/patchfinder32.hpp
		include/libpatchfinder/patchfinder64.hpp
		DESTINATION "${CMAKE_INSTALL_PREFIX}/include/libpatchfinder")
install(FILES
		include/libpatchfinder/ibootpatchfinder/ibootpatchfinder.hpp
		include/libpatchfinder/ibootpatchfinder/ibootpatchfinder32.hpp
		include/libpatchfinder/ibootpatchfinder/ibootpatchfinder64.hpp
		DESTINATION "${CMAKE_INSTALL_PREFIX}/include/libpatchfinder/ibootpatchfinder")
install(FILES
		include/libpatchfinder/kernelpatchfinder/kernelpatchfinder.hpp
		include/libpatchfinder/kernelpatchfinder/kernelpatchfinder32.hpp
		include/libpatchfinder/kernelpatchfinder/kernelpatchfinder64.hpp
		DESTINATION "${CMAKE_INSTALL_PREFIX}/include/libpatchfinder/kernelpatchfinder")
install(FILES
		${CMAKE_BINARY_DIR}/libpatchfinder.dylib
		DESTINATION "${CMAKE_INSTALL_PREFIX}/lib")
install(TARGETS patchfinder
        DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
install(TARGETS patchfinder_shared
        DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)

#SET(CMAKE_EXPORT_COMPILE_COMMANDS 1)
#get_cmake_property(_variableNames VARIABLES)
#list (SORT _variableNames)
#foreach (_variableName ${_variableNames})
#    message(STATUS "${_variableName}=${${_variableName}}")
#endforeach()
