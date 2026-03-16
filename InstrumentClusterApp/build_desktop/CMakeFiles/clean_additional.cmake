# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles/ClusterApp_autogen.dir/AutogenUsed.txt"
  "CMakeFiles/ClusterApp_autogen.dir/ParseCache.txt"
  "ClusterApp_autogen"
  )
endif()
