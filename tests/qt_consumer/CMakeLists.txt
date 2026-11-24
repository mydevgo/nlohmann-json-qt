cmake_minimum_required(VERSION 3.13)
project(QtInstalledConsumer LANGUAGES CXX)

find_package(nlohmann_json CONFIG REQUIRED)
add_executable(QtInstalledConsumer main.cpp Other.cpp)
target_link_libraries(QtInstalledConsumer PRIVATE nlohmann_json::nlohmann_json)
target_compile_features(QtInstalledConsumer PRIVATE cxx_std_11)

# 正向包含及链接成功后，负向结果才能解释为接口拒绝。
include(CheckCXXSourceCompiles)
set(CMAKE_REQUIRED_LIBRARIES nlohmann_json::nlohmann_json)
check_cxx_source_compiles("#include <nlohmann/json.hpp>
int main() { return nlohmann::json(1).get<int>() == 1 ? 0 : 1; }" QT_CONSUMER_POSITIVE)
if(NOT QT_CONSUMER_POSITIVE)
    message(FATAL_ERROR "Installed positive consumer failed")
endif()
check_cxx_source_compiles("#include <nlohmann/json.hpp>
#include <vector>
int main() { return nlohmann::json::array({1}).get_strict<std::vector<int>>().size(); }" QT_STRICT_VECTOR_COMPILES)
if(QT_STRICT_VECTOR_COMPILES)
    message(FATAL_ERROR "Unsupported strict target was accepted")
endif()
check_cxx_source_compiles("#include <nlohmann/json.hpp>
struct CValue {};
void swap(CValue&, CValue&) noexcept(false) {}
int main() { CValue sValue; nlohmann::json().get_to_strict(sValue); }" QT_THROWING_SWAP_COMPILES)
if(QT_THROWING_SWAP_COMPILES)
    message(FATAL_ERROR "Throwing swap target was accepted")
endif()

enable_testing()
add_test(NAME QtInstalledConsumer COMMAND QtInstalledConsumer)
