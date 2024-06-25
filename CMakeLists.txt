project(soducocxx)
cmake_minimum_required(VERSION 3.11)

find_package(spdlog REQUIRED)
find_package(fmt REQUIRED)
find_package(pylene REQUIRED)
find_package(pybind11 REQUIRED)
find_package(blend2d)
find_package(GTest)
find_package(nlohmann_json)
find_package(CLI11)
include(pybind11Common)
find_package(pylene-numpy REQUIRED)
find_package(poppler REQUIRED)
# find_package(cpprestsdk REQUIRED)
# find_package(uwebsockets REQUIRED)


# set(CMAKE_POSITION_INDEPENDENT_CODE ON)
# add_compile_options("-Wno-deprecated-declarations")
add_compile_options("-W" "-Wall" "-fPIC")
add_subdirectory(thirdparty)


add_library(scribo
  sources/src/clean.cpp
  sources/src/deskew.cpp
  sources/src/background_substraction.cpp
  sources/src/skew_estimation.cpp
  sources/src/subsample.cpp
  sources/src/gaussian_directional_2d.cpp
  sources/src/CoreTypes.cpp
  sources/src/XYCutLayoutExtraction.cpp
  sources/src/config.cpp
  sources/src/signal.cpp
  sources/src/detect_separators.cpp
  sources/src/Interval.cpp
  sources/src/region_lut.cpp
  sources/src/DOMLinesExtractor.cpp
  sources/src/DOMEntriesExtractor.cpp
)

target_include_directories(scribo PUBLIC sources/include)
target_link_libraries(scribo PRIVATE spdlog::spdlog LSD pylene::scribo)
target_link_libraries(scribo PUBLIC pylene::core)

add_executable(UTInterval sources/tests/UTInterval.cpp)
target_link_libraries(UTInterval scribo GTest::gtest_main)


pybind11_add_module(scribocxx MODULE
  sources/src/scribo-python.cpp
  sources/src/preprocessing.cpp
  sources/src/DOMTypes.cpp
  # sources/src/TesseractTextExtraction.cpp
  )


target_include_directories(scribocxx PUBLIC sources/include)
# target_link_libraries(scribocxx PRIVATE pylene::core pylene::io-freeimage spdlog::spdlog pylene-numpy::pylene-numpy tesseract scribo)
target_link_libraries(scribocxx PRIVATE pylene::core pylene::io-freeimage spdlog::spdlog pylene-numpy::pylene-numpy scribo)

add_executable(cli
  sources/src/display.cpp
  sources/src/export.cpp
  sources/src/main.cpp
  sources/src/pdf_tool.hpp
  sources/src/pdf_tool.cpp)

# add_executable(server
#   sources/src/api.cpp
# )


target_include_directories(cli PUBLIC sources/include)
target_link_libraries(cli pylene::io-freeimage scribo blend2d::blend2d spdlog::spdlog nlohmann_json::nlohmann_json CLI11::CLI11 poppler::poppler)
set_target_properties(cli PROPERTIES INSTALL_RPATH "./lib" )

# target_include_directories(server PUBLIC sources/include)
# target_link_libraries(server cpprestsdk::cpprestsdk uwebsockets::uwebsockets spdlog::spdlog scribo pylene::io-freeimage CLI11::CLI11)
# set_target_properties(server PROPERTIES INSTALL_RPATH "./lib" )

include(GNUInstallDirs)
install(DIRECTORY ${CMAKE_BINARY_DIR}/lib/ DESTINATION bin/lib)
install(TARGETS scribocxx LIBRARY DESTINATION "./back")
install(TARGETS cli)
# install(TARGETS server)

set(CPACK_GENERATOR "ZIP")
set(CPACK_STRIP_FILES TRUE)
set(CPACK_PACKAGE_NAME "${PROJECT_NAME}-py${PYTHON_VERSION_MAJOR}${PYTHON_VERSION_MINOR}")
include(CPack)
