cmake_minimum_required(VERSION 3.16)
project(smart_package_detector LANGUAGES CXX)

enable_testing()

add_executable(desktop_profile_tests tests/test_desktop_profile.cpp)
target_include_directories(desktop_profile_tests PRIVATE firmware/smart_package_detector)
target_compile_features(desktop_profile_tests PRIVATE cxx_std_17)
add_test(NAME desktop_profile_tests COMMAND desktop_profile_tests)

add_executable(package_detector_tests tests/test_package_detector.cpp)
target_include_directories(
  package_detector_tests
  PRIVATE firmware/smart_package_detector
)
target_compile_features(package_detector_tests PRIVATE cxx_std_17)

if(MSVC)
  target_compile_options(package_detector_tests PRIVATE /W4 /WX)
else()
  target_compile_options(package_detector_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_test(NAME package_detector_tests COMMAND package_detector_tests)

# Arduino builds each sketch in isolation. Keep bundled headers identical to
# the sensor sketch's source of truth.
foreach(header package_detector.h detector_config.h)
  file(READ "${CMAKE_CURRENT_SOURCE_DIR}/firmware/smart_package_detector/${header}" sensor_source)
  file(READ "${CMAKE_CURRENT_SOURCE_DIR}/firmware/wifi_status/${header}" demo_source)
  string(REPLACE "\r\n" "\n" sensor_source "${sensor_source}")
  string(REPLACE "\r\n" "\n" demo_source "${demo_source}")
  string(REGEX REPLACE "\n+$" "" sensor_source "${sensor_source}")
  string(REGEX REPLACE "\n+$" "" demo_source "${demo_source}")
  if(NOT sensor_source STREQUAL demo_source)
    message(FATAL_ERROR "Synchronize wifi_status/${header} from smart_package_detector/${header}")
  endif()
  file(READ "${CMAKE_CURRENT_SOURCE_DIR}/firmware/real_sensor_dashboard/${header}" dashboard_source)
  string(REPLACE "\r\n" "\n" dashboard_source "${dashboard_source}")
  string(REGEX REPLACE "\n+$" "" dashboard_source "${dashboard_source}")
  if(NOT sensor_source STREQUAL dashboard_source)
    message(FATAL_ERROR "Synchronize real_sensor_dashboard/${header} from smart_package_detector/${header}")
  endif()
endforeach()
add_executable(simulation_tests tests/test_simulation.cpp)
target_include_directories(simulation_tests PRIVATE firmware/wifi_status)
target_compile_features(simulation_tests PRIVATE cxx_std_17)
add_test(NAME simulation_tests COMMAND simulation_tests)
