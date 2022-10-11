cmake_minimum_required(VERSION 3.1.0)

file(STRINGS ../../config.txt ProjectConfig)
string(SUBSTRING ${ProjectConfig} 0 1 FutureOptionOnly)
string(SUBSTRING ${ProjectConfig} 1 1 ReleaseVersion)
  
if(FutureOptionOnly MATCHES "1")
project(ctp-future-option VERSION 1.0.0 LANGUAGES CXX)
else()
project(ctp-stock-option VERSION 1.0.0 LANGUAGES CXX)
endif()

#define macros SEE_THROUGH_SUPERVISION, FUTURE_OPTION_ONLY
if(FutureOptionOnly MATCHES "1")
add_definitions(-DFUTURE_OPTION_ONLY)
endif()
add_definitions(-DSEE_THROUGH_SUPERVISION)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(ReleaseVersion MATCHES "1")
SET(CMAKE_BUILD_TYPE "Release")
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3  -lpthread -fopenmp")
else()
SET(CMAKE_BUILD_TYPE "Debug")
SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -g -ggdb -lpthread -fopenmp")
endif()

set(srcpath ../../code/core)
set(guipath ../../code/gui)
set(apipath ../../ctp-api)
set(sqlpath ../../sqlite-api)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(CMAKE_VERSION VERSION_LESS "3.7.0")
    set(CMAKE_INCLUDE_CURRENT_DIR ON)
endif()

find_package(Qt5 COMPONENTS Core Gui Widgets REQUIRED)

#add include directories
include_directories(
	${guipath}
	${apipath}
	${sqlpath}
	${srcpath}/AppTop
	${srcpath}/RealProj/CtpCnt
	${srcpath}/RealProj/RealTop
	${srcpath}/RealProj/Strategy/VoidTest
	${srcpath}/RealProj/Strategy/Top
	${srcpath}/RealProj/Tshape
	${srcpath}/TestProj
	${srcpath}/Utility
)

#add source file directories
aux_source_directory(${srcpath}/AppTop AppTop)
aux_source_directory(${srcpath}/RealProj/CtpCnt CtpCnt)
aux_source_directory(${srcpath}/RealProj/RealTop RealTop)
aux_source_directory(${srcpath}/RealProj/Strategy/VoidTest VoidTest)
aux_source_directory(${srcpath}/RealProj/Tshape Tshape)
aux_source_directory(${srcpath}/TestProj TestProj)
aux_source_directory(${srcpath}/Utility Utility)
aux_source_directory(${guipath} GuiSrc)

#aux_source_directory(${srcpath}/RealProj/Strategy/Top StrategyTop)
#add_library(soptstrategy SHARED ${StrategyTop})

#add dynamic link libraries
if(FutureOptionOnly MATCHES "1")
add_library(thosttraderapi_se SHARED IMPORTED)
SET_PROPERTY(TARGET thosttraderapi_se PROPERTY IMPORTED_LOCATION ./thosttraderapi_se.so)
add_library(thostmduserapi_se SHARED IMPORTED)
SET_PROPERTY(TARGET thostmduserapi_se PROPERTY IMPORTED_LOCATION ./thostmduserapi_se.so)
else()
add_library(soptthosttraderapi_se SHARED IMPORTED)
SET_PROPERTY(TARGET soptthosttraderapi_se PROPERTY IMPORTED_LOCATION ./soptthosttraderapi_se.so)
add_library(soptthostmduserapi_se SHARED IMPORTED)
SET_PROPERTY(TARGET soptthostmduserapi_se PROPERTY IMPORTED_LOCATION ./soptthostmduserapi_se.so)
endif()
add_library(sqlite3 SHARED IMPORTED)
SET_PROPERTY(TARGET sqlite3 PROPERTY IMPORTED_LOCATION ./libsqlite3.so)

add_library(soptstrategy SHARED IMPORTED)
SET_PROPERTY(TARGET soptstrategy PROPERTY IMPORTED_LOCATION ./libsoptstrategy.so)

if(FutureOptionOnly MATCHES "1")
add_executable(${CMAKE_PROJECT_NAME} 
	${GuiSrc}
	${AppTop}
	${CtpCnt}
	${RealTop}
	${VoidTest}
	${Tshape}
	${TestProj}
	${Utility}
)
target_link_libraries(${CMAKE_PROJECT_NAME} thostmduserapi_se thosttraderapi_se sqlite3 soptstrategy Qt5::Core Qt5::Gui Qt5::Widgets) 
else()
add_executable(${CMAKE_PROJECT_NAME} 
	${GuiSrc}
	${AppTop}
	${CtpCnt}
	${RealTop}
	${VoidTest}
	${Tshape}
	${TestProj}
	${Utility}
)
target_link_libraries(${CMAKE_PROJECT_NAME} soptthostmduserapi_se soptthosttraderapi_se sqlite3 soptstrategy Qt5::Core Qt5::Gui Qt5::Widgets) 
endif()

