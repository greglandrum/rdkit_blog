cmake_minimum_required(VERSION 3.18)

project(simple_cxx_example)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)


find_package(RDKit REQUIRED)
find_package(Boost COMPONENTS timer system REQUIRED)
add_executable(tautomer_hash tautomer_hash.cpp)
target_link_libraries(tautomer_hash RDKit::SmilesParse RDKit::MolHash
   Boost::timer)
