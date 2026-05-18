project(tests)

function(ADD_TESTS TESTS LABELS)
    foreach(TEST IN LISTS TESTS)
        add_executable(${TEST} ${TEST}.cpp)
        target_link_libraries(${TEST} ${ARGN})
        target_include_directories(${TEST} PRIVATE ${PROJECT_SOURCE_DIR}/includes)
        set_target_properties(${TEST} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests/bin")
        add_test(NAME ${TEST} COMMAND ${TEST})
        set_property(TEST ${TEST} PROPERTY LABELS ${LABELS})
    endforeach()
endfunction()

add_subdirectory(simulator_tests)
add_subdirectory(ga_tests)

add_executable(speed_checker speed_checker.cpp)
target_link_libraries(speed_checker circuitSimulator esegraph)
target_include_directories(speed_checker PRIVATE ../includes)
set_target_properties(speed_checker PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests/bin")

make_directory(${CMAKE_BINARY_DIR}/tests/bin/data)
add_custom_target(copy_data
COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/tests/data ${CMAKE_BINARY_DIR}/tests/bin/data
)

add_test(NAME speed_checker
         COMMAND timeout 10s ./speed_checker 1000
         WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/tests/bin")
set_property(TEST speed_checker PROPERTY LABELS simulator)
add_dependencies(speed_checker copy_data)


add_test(NAME executable COMMAND Circuit_Optimizer)