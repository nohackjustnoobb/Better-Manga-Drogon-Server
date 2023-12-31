cmake_minimum_required(VERSION 3.5)
project(Better-Manga-Drogon-Server CXX)

include(CheckIncludeFileCXX)

check_include_file_cxx(any HAS_ANY)
check_include_file_cxx(string_view HAS_STRING_VIEW)
check_include_file_cxx(coroutine HAS_COROUTINE)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(${PROJECT_NAME} main.cc)

# ##############################################################################
# If you include the drogon source code locally in your project, use this method
# to add drogon
# add_subdirectory(drogon)
# target_link_libraries(${PROJECT_NAME} PRIVATE drogon)
#
# and comment out the following lines
find_package(Drogon CONFIG REQUIRED)
find_package(fmt REQUIRED CONFIG)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(cpr CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(lexbor REQUIRED)
find_package(re2 REQUIRED)
find_package(argon2 REQUIRED CONFIG)
find_package(jwt-cpp REQUIRED)
find_package(SQLiteCpp REQUIRED)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
    cpr::cpr
    OpenSSL::Crypto OpenSSL::SSL
    re2::re2
    nlohmann_json::nlohmann_json
    lexbor::lexbor_static
    Drogon::Drogon
    argon2::argon2
    jwt-cpp::jwt-cpp
    fmt::fmt
    SQLiteCpp)

# ##############################################################################'
if(CMAKE_CXX_STANDARD LESS 17)
    message(FATAL_ERROR "c++17 or higher is required")
elseif(CMAKE_CXX_STANDARD LESS 20)
    message(STATUS "use c++17")
else()
    message(STATUS "use c++20")
endif()

aux_source_directory(controllers CTL_SRC)
aux_source_directory(models MODEL_SRC)
aux_source_directory(plugins PLUGIN_SRC)
aux_source_directory(utils UTILS_SRC)

# use the following line to create views with namespaces.
# drogon_create_views(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/views
# ${CMAKE_CURRENT_BINARY_DIR} TRUE)
# use the following line to create views with namespace CHANGE_ME prefixed
# and path namespaces.
# drogon_create_views(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/views
# ${CMAKE_CURRENT_BINARY_DIR} TRUE CHANGE_ME)
target_include_directories(${PROJECT_NAME}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/models)
target_sources(${PROJECT_NAME}
    PRIVATE
    ${SRC_DIR}
    ${UTILS_SRC}
    ${PLUGIN_SRC}
    ${CTL_SRC}
    ${MODEL_SRC})

# ##############################################################################
# uncomment the following line for dynamically loading views
# set_property(TARGET ${PROJECT_NAME} PROPERTY ENABLE_EXPORTS ON)

# ##############################################################################
