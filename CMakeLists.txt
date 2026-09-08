cmake_minimum_required(VERSION 3.16)
project(smart_package_detector LANGUAGES CXX)

enable_testing()

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

# Arduino builds each sketch in isolation. Keep the demo's bundled headers
# identical to the sensor sketch's source of truth.
foreach(header package_detector.h detector_config.h)
  file(SHA256 "${CMAKE_CURRENT_SOURCE_DIR}/firmware/smart_package_detector/${header}" sensor_hash)
  file(SHA256 "${CMAKE_CURRENT_SOURCE_DIR}/firmware/wifi_status/${header}" demo_hash)
  if(NOT sensor_hash STREQUAL demo_hash)
    message(FATAL_ERROR "Synchronize wifi_status/${header} from smart_package_detector/${header}")
  endif()
endforeach()
add_executable(simulation_tests tests/test_simulation.cpp)
target_include_directories(simulation_tests PRIVATE firmware/wifi_status)
target_compile_features(simulation_tests PRIVATE cxx_std_17)
add_test(NAME simulation_tests COMMAND simulation_tests)
