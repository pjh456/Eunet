file(GLOB_RECURSE EUNET_SOURCES
    "*.cpp" ".c"
)

list(FILTER EUNET_SOURCES EXCLUDE REGEX "main\\.cpp$")

target_sources(eunet PRIVATE ${EUNET_SOURCES})

target_include_directories(eunet
    PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)