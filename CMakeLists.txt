cmake_minimum_required(VERSION 3.16)

project(RateLimiterResearch
    VERSION 1.0
    DESCRIPTION "Исследование алгоритмов ограничения частоты запросов"
    LANGUAGES CXX
)

# -------------------- C++ --------------------
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# -------------------- Options --------------------
option(BUILD_TESTS        "Собрать юнит-тесты"          ON)
option(BUILD_BENCHMARKS   "Собрать микробенчмарки"      OFF)
option(BUILD_LOAD_TESTER  "Собрать нагрузочный тестер"  ON)
option(ENABLE_TSAN        "Включить ThreadSanitizer"    OFF)
option(ENABLE_COVERAGE    "Включить покрытие тестами"   OFF)

# -------------------- Common dependencies --------------------
find_package(Threads REQUIRED)
include(FetchContent)

# -------------------- Coverage --------------------
function(enable_coverage_for target_name)
    if(ENABLE_COVERAGE)
        if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
            target_compile_options(${target_name} PRIVATE
                -O0
                -g
                --coverage
            )

            target_link_options(${target_name} PRIVATE
                --coverage
            )
        else()
            message(WARNING "Coverage поддерживается только для GCC/Clang")
        endif()
    endif()
endfunction()

# -------------------- Google Test --------------------
if(BUILD_TESTS)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.14.0
    )

    # Для Windows, чтобы не было проблем с runtime library
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

    FetchContent_MakeAvailable(googletest)
    enable_testing()
endif()

# -------------------- Google Benchmark --------------------
if(BUILD_BENCHMARKS)
    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(
        benchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG        v1.8.3
    )

    FetchContent_MakeAvailable(benchmark)
endif()

# -------------------- Include-only interface --------------------
add_library(rate_limiter_headers INTERFACE)

target_include_directories(rate_limiter_headers INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/rate_limiter
)

# -------------------- Rate limiter sources --------------------
file(GLOB RATE_LIMITER_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

# main.cpp не должен попадать в библиотеку
list(FILTER RATE_LIMITER_SOURCES EXCLUDE REGEX ".*/main\\.cpp$")

if(RATE_LIMITER_SOURCES)
    add_library(rate_limiter_lib STATIC
        ${RATE_LIMITER_SOURCES}
    )

    target_include_directories(rate_limiter_lib PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include/rate_limiter
    )

    target_link_libraries(rate_limiter_lib PUBLIC
        rate_limiter_headers
        Threads::Threads
    )

    enable_coverage_for(rate_limiter_lib)
else()
    add_library(rate_limiter_lib INTERFACE)

    target_link_libraries(rate_limiter_lib INTERFACE
        rate_limiter_headers
        Threads::Threads
    )
endif()

# -------------------- Load tester patterns --------------------
file(GLOB LOAD_PATTERN_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/load_tester/*.cpp"
)

# load_tester/main.cpp не должен попадать в библиотеку паттернов
list(FILTER LOAD_PATTERN_SOURCES EXCLUDE REGEX ".*/main\\.cpp$")

if(LOAD_PATTERN_SOURCES)
    add_library(load_patterns STATIC
        ${LOAD_PATTERN_SOURCES}
    )

    target_include_directories(load_patterns PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include/rate_limiter
        ${CMAKE_CURRENT_SOURCE_DIR}/load_tester
    )

    target_link_libraries(load_patterns PUBLIC
        rate_limiter_lib
        Threads::Threads
    )

    enable_coverage_for(load_patterns)
endif()

# -------------------- Unit tests --------------------
if(BUILD_TESTS)
    file(GLOB TEST_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp"
    )

    if(TEST_SOURCES)
        add_executable(unit_tests
            ${TEST_SOURCES}
        )

        target_include_directories(unit_tests PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include
            ${CMAKE_CURRENT_SOURCE_DIR}/include/rate_limiter
            ${CMAKE_CURRENT_SOURCE_DIR}/load_tester
        )

        target_link_libraries(unit_tests PRIVATE
            rate_limiter_lib
            GTest::gtest_main
            Threads::Threads
        )

        enable_coverage_for(unit_tests)

        if(TARGET load_patterns)
            target_link_libraries(unit_tests PRIVATE load_patterns)
        endif()

        if(ENABLE_TSAN)
            target_compile_options(unit_tests PRIVATE
                -fsanitize=thread
                -fno-omit-frame-pointer
            )

            target_link_options(unit_tests PRIVATE
                -fsanitize=thread
            )
        endif()

        include(GoogleTest)

        gtest_discover_tests(unit_tests
            DISCOVERY_TIMEOUT 60
        )
    else()
        message(WARNING "BUILD_TESTS=ON, но tests/*.cpp не найдены")
    endif()
endif()

# -------------------- Load tester executable --------------------
if(BUILD_LOAD_TESTER)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/load_tester/main.cpp")
        add_executable(load_tester_app
            "${CMAKE_CURRENT_SOURCE_DIR}/load_tester/main.cpp"
        )

        target_include_directories(load_tester_app PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include
            ${CMAKE_CURRENT_SOURCE_DIR}/include/rate_limiter
            ${CMAKE_CURRENT_SOURCE_DIR}/load_tester
        )

        target_link_libraries(load_tester_app PRIVATE
            rate_limiter_lib
            Threads::Threads
        )

        if(TARGET load_patterns)
            target_link_libraries(load_tester_app PRIVATE load_patterns)
        endif()
    else()
        message(WARNING "BUILD_LOAD_TESTER=ON, но load_tester/main.cpp не найден")
    endif()
endif()

# -------------------- Benchmarks --------------------
if(BUILD_BENCHMARKS)
    file(GLOB BENCHMARK_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/benchmarks/*.cpp"
    )

    if(BENCHMARK_SOURCES)
        foreach(BENCH_FILE ${BENCHMARK_SOURCES})
            get_filename_component(BENCH_NAME ${BENCH_FILE} NAME_WE)

            add_executable(${BENCH_NAME}
                ${BENCH_FILE}
            )

            target_include_directories(${BENCH_NAME} PRIVATE
                ${CMAKE_CURRENT_SOURCE_DIR}/include
                ${CMAKE_CURRENT_SOURCE_DIR}/include/rate_limiter
                ${CMAKE_CURRENT_SOURCE_DIR}/load_tester
            )

            target_link_libraries(${BENCH_NAME} PRIVATE
                rate_limiter_lib
                benchmark::benchmark
                Threads::Threads
            )

            if(TARGET load_patterns)
                target_link_libraries(${BENCH_NAME} PRIVATE load_patterns)
            endif()
        endforeach()
    else()
        message(WARNING "BUILD_BENCHMARKS=ON, но benchmarks/*.cpp не найдены")
    endif()
endif()

# -------------------- Optional src/main.cpp app --------------------
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")
    add_executable(rate_limiter_app
        "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp"
    )

    target_include_directories(rate_limiter_app PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include/rate_limiter
        ${CMAKE_CURRENT_SOURCE_DIR}/load_tester
    )

    target_link_libraries(rate_limiter_app PRIVATE
        rate_limiter_lib
        Threads::Threads
    )

    if(TARGET load_patterns)
        target_link_libraries(rate_limiter_app PRIVATE load_patterns)
    endif()
endif()