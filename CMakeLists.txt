cmake_minimum_required(VERSION 3.15)
project(yuno_gasai VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Find required packages
find_package(CURL REQUIRED)
find_package(SQLite3 REQUIRED)

# Concord library (assumes installed via system or manually)
# You may need to adjust these paths based on your Concord installation
find_library(CONCORD_LIBRARY NAMES discord concord PATHS /usr/local/lib /usr/lib)
find_path(CONCORD_INCLUDE_DIR NAMES concord/discord.h PATHS /usr/local/include /usr/include)

if(NOT CONCORD_LIBRARY)
    message(WARNING "Concord library not found. You may need to install it from https://github.com/Cogmasters/concord")
endif()

# JSON-C for config parsing
find_package(PkgConfig REQUIRED)
pkg_check_modules(JSON_C REQUIRED json-c)

# Source files
set(SOURCES
    src/main.c
    src/bot.c
    src/database.c
    src/config.c
    src/commands/moderation.c
    src/commands/utility.c
    src/commands/fun.c
    src/modules/auto_cleaner.c
    src/modules/spam_filter.c
)

# Header files
set(HEADERS
    include/bot.h
    include/database.h
    include/config.h
    include/commands/moderation.h
    include/commands/utility.h
    include/commands/fun.h
    include/modules/auto_cleaner.h
    include/modules/spam_filter.h
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CONCORD_INCLUDE_DIR}
    ${JSON_C_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${CONCORD_LIBRARY}
    CURL::libcurl
    SQLite::SQLite3
    ${JSON_C_LIBRARIES}
    pthread
    m
)

# Set output directory
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Install target
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
