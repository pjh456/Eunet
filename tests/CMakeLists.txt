# 1. 收集测试源文件
file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

find_package(CURL REQUIRED)

foreach(test_src ${TEST_SOURCES})
    # 2. 生成“相对路径名”作为唯一 test 名
    file(RELATIVE_PATH rel_path
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${test_src}
    )

    # 把路径变成合法 target 名
    string(REPLACE "/" "_" test_name ${rel_path})
    string(REPLACE "\\" "_" test_name ${test_name})
    string(REPLACE "." "_" test_name ${test_name})

    # 3. 可执行文件
    add_executable(${test_name} ${test_src})

    target_link_libraries(${test_name} PRIVATE eunet CURL::libcurl)

    # 4. 统一输出到 build/tests
    set_target_properties(${test_name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
    )

    # 5. 用 TARGET_FILE 注册 CTest（关键）
    add_test(
        NAME ${test_name}
        COMMAND $<TARGET_FILE:${test_name}>
    )
endforeach()