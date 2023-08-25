cmake_minimum_required(VERSION 3.5)
project(timecost_statistic)

# Default to C99
if(NOT CMAKE_C_STANDARD)
  set(CMAKE_C_STANDARD 99)
endif()

# Default to C++14
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(yaml-cpp REQUIRED)
find_package(Boost COMPONENTS filesystem)

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

include_directories(
  include
  ${Boost_INCLUDE_DIRS}
)

add_library(yaml_exporter SHARED
  src/yaml_exporter.cpp
)
target_link_libraries(yaml_exporter
  yaml-cpp
)

install(TARGETS yaml_exporter
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

install(
  DIRECTORY include/
  DESTINATION include
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  # the following line skips the linter which checks for copyrights
  # uncomment the line when a copyright and license is not present in all source files
  #set(ament_cmake_copyright_FOUND TRUE)
  # the following line skips cpplint (only works in a git repo)
  # uncomment the line when this package is not in a git repo
  #set(ament_cmake_cpplint_FOUND TRUE)
  add_executable(${PROJECT_NAME}_test
    test/test.cpp
  )
  target_link_libraries(${PROJECT_NAME}_test
    yaml_exporter
    ${Boost_LIBRARIES}
  )

  install(TARGETS
    ${PROJECT_NAME}_test
    DESTINATION test/${PROJECT_NAME}
  )

  ament_lint_auto_find_test_dependencies()
endif()

ament_export_include_directories(include)

ament_package()
