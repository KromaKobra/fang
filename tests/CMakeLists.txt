# Core test suites (docs/09). Each links fangcore and runs headlessly.

find_package(Qt6 6.6 REQUIRED COMPONENTS Test)

set(FANG_TESTS
    # populated as suites are implemented (stage 10)
)

foreach(t IN LISTS FANG_TESTS)
    qt_add_executable(${t} ${t}.cpp)
    target_link_libraries(${t} PRIVATE fangcore fangcoreplugin Qt6::Test)
    add_test(NAME ${t} COMMAND ${t})
    set_tests_properties(${t} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endforeach()
