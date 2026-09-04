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
