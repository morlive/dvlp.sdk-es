cmake_minimum_required(VERSION 3.10)
project(switch_simulator C)

# Устанавливаем версию проекта
set(VERSION_MAJOR 0)
set(VERSION_MINOR 1)
set(VERSION_PATCH 0)

# Опции сборки
option(BUILD_TESTS "Build tests" ON)
option(ENABLE_LOGGING "Enable logging" ON)

# Устанавливаем флаги компиляции
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -pedantic")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -O0")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O2")

# Определяем директории
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(BSP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/bsp)
set(DRIVERS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/drivers)
set(TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tests)
set(TOOLS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tools)

# Определяем директории для include
include_directories(
    ${INCLUDE_DIR}
    ${BSP_DIR}/include
    ${DRIVERS_DIR}/include
)

# Определяем исходные файлы
file(GLOB_RECURSE SRC_FILES 
    ${SRC_DIR}/*.c
    ${BSP_DIR}/src/*.c
    ${DRIVERS_DIR}/src/*.c
)

# Определяем условные компиляции
if(ENABLE_LOGGING)
    add_definitions(-DENABLE_LOGGING=1)
else()
    add_definitions(-DENABLE_LOGGING=0)
endif()

# Создаем исполняемый файл
add_executable(switch_simulator ${SRC_FILES})

# Добавляем тесты, если необходимо
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(${TEST_DIR}/unit)
    add_subdirectory(${TEST_DIR}/integration)
endif()

# Устанавливаем правила установки
install(TARGETS switch_simulator DESTINATION bin)
install(FILES README.md LICENSE DESTINATION share/switch_simulator)

# Настраиваем пакетирование
set(CPACK_PACKAGE_NAME "switch-simulator")
set(CPACK_PACKAGE_VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")
set(CPACK_PACKAGE_CONTACT "your.email@example.com")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A network switch simulator")
include(CPack)
