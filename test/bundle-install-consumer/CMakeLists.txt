cmake_minimum_required(VERSION 3.12)
project(frei0r_bundle_install_consumer C CXX)

find_package(Frei0rBundle CONFIG REQUIRED)

foreach(language IN ITEMS c cxx)
  foreach(linkage IN ITEMS static shared)
    add_executable(${language}-${linkage} ${language}-consumer.${language})
    target_link_libraries(${language}-${linkage} PRIVATE Frei0rBundle::${linkage})
  endforeach()
endforeach()
