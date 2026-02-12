cmake_minimum_required(VERSION 3.15)
project(yuno_gasai VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Optional: SQLCipher for encrypted database
option(USE_SQLCIPHER "Use SQLCipher for database encryption" OFF)

# Sanitizer options (combinable, use with -DCMAKE_BUILD_TYPE=Debug)
option(SANITIZE_ADDRESS "Enable AddressSanitizer (memory errors, leaks)" OFF)
option(SANITIZE_UNDEFINED "Enable UndefinedBehaviorSanitizer" OFF)
option(SANITIZE_THREAD "Enable ThreadSanitizer (data races)" OFF)

if(SANITIZE_ADDRESS AND SANITIZE_THREAD)
    message(FATAL_ERROR "ASan and TSan cannot be used together")
endif()

set(SANITIZER_FLAGS "")
if(SANITIZE_ADDRESS)
    set(SANITIZER_FLAGS "${SANITIZER_FLAGS} -fsanitize=address")
endif()
if(SANITIZE_UNDEFINED)
    set(SANITIZER_FLAGS "${SANITIZER_FLAGS} -fsanitize=undefined")
endif()
if(SANITIZE_THREAD)
    set(SANITIZER_FLAGS "${SANITIZER_FLAGS} -fsanitize=thread")
endif()

if(NOT "${SANITIZER_FLAGS}" STREQUAL "")
    string(STRIP "${SANITIZER_FLAGS}" SANITIZER_FLAGS)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${SANITIZER_FLAGS} -fno-omit-frame-pointer -g")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${SANITIZER_FLAGS}")
    message(STATUS "Sanitizers enabled: ${SANITIZER_FLAGS}")
endif()

# Find required packages
find_package(CURL REQUIRED)
if(USE_SQLCIPHER)
    find_library(SQLCIPHER_LIBRARY NAMES sqlcipher PATHS /usr/local/lib /usr/lib)
    find_path(SQLCIPHER_INCLUDE_DIR NAMES sqlcipher/sqlite3.h PATHS /usr/local/include /usr/include)
    if(SQLCIPHER_LIBRARY)
        message(STATUS "SQLCipher found: ${SQLCIPHER_LIBRARY}")
    else()
        message(WARNING "SQLCipher not found, falling back to SQLite3")
        set(USE_SQLCIPHER OFF)
    endif()
endif()
if(NOT USE_SQLCIPHER)
    find_package(SQLite3 REQUIRED)
endif()

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
    src/modules/activity_logger.c
    src/modules/terminal.c
    src/modules/http_client.c
    src/modules/lru_cache.c
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
    include/modules/activity_logger.h
    include/modules/terminal.h
    include/modules/http_client.h
    include/modules/lru_cache.h
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
if(USE_SQLCIPHER)
    target_compile_definitions(${PROJECT_NAME} PRIVATE YUNO_SQLCIPHER)
    target_include_directories(${PROJECT_NAME} PRIVATE ${SQLCIPHER_INCLUDE_DIR})
    target_link_libraries(${PROJECT_NAME} PRIVATE
        ${CONCORD_LIBRARY}
        CURL::libcurl
        ${SQLCIPHER_LIBRARY}
        ${JSON_C_LIBRARIES}
        pthread
    )
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE
        ${CONCORD_LIBRARY}
        CURL::libcurl
        SQLite::SQLite3
        ${JSON_C_LIBRARIES}
        pthread
    )
endif()

# Set output directory
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Install target
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
