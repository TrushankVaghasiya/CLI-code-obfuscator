
cmake_minimum_required(VERSION 3.20)

project(cpp_obfuscator
    VERSION 0.1.0
    DESCRIPTION "C++ Source Code Obfuscator CLI"
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

# Download CLI11 during CMake configuration
FetchContent_Declare(
    CLI11
    GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
    GIT_TAG v2.5.0
)

FetchContent_MakeAvailable(CLI11)

add_executable(obfuscator
    src/main.cpp
    src/cli.cpp
    src/validator.cpp
    src/parser.cpp
    src/identifier_analyzer.cpp
    src/transformer.cpp
    src/literal_transformer.cpp
)

target_include_directories(obfuscator
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(obfuscator
    PRIVATE
        CLI11::CLI11
        tree_sitter_runtime_lib
        tree_sitter_cpp_grammar
)


include(CTest)

if(BUILD_TESTING)
    set(TEST_INPUT
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/data/sample.cpp"
    )

    set(TEST_OUTPUT
        "${CMAKE_CURRENT_BINARY_DIR}/test-output.cpp"
    )

    # Help command should succeed.
    add_test(
        NAME cli_help
        COMMAND obfuscator --help
    )

    # Valid arguments should pass validation.
    add_test(
        NAME cli_valid_arguments
        COMMAND obfuscator
            -i "${TEST_INPUT}"
            -o "${TEST_OUTPUT}"
            --level 2
    )

    # Invalid level should fail.
    add_test(
        NAME cli_invalid_level
        COMMAND obfuscator
            -i "${TEST_INPUT}"
            -o "${TEST_OUTPUT}"
            --level 5
    )
    set_tests_properties(
        cli_invalid_level
        PROPERTIES WILL_FAIL TRUE
    )

    # Missing input should fail validation.
    add_test(
        NAME cli_missing_input
        COMMAND obfuscator
            -i "${CMAKE_CURRENT_SOURCE_DIR}/missing.cpp"
            -o "${TEST_OUTPUT}"
            --level 2
    )
    set_tests_properties(
        cli_missing_input
        PROPERTIES WILL_FAIL TRUE
    )

    # Input and output must differ.
    add_test(
        NAME cli_same_input_output
        COMMAND obfuscator
            -i "${TEST_INPUT}"
            -o "${TEST_INPUT}"
            --level 2
    )
    set_tests_properties(
        cli_same_input_output
        PROPERTIES WILL_FAIL TRUE
    )

    add_test(
    NAME transform_basic
    COMMAND ${CMAKE_COMMAND}
        "-DOBFUSCATOR=$<TARGET_FILE:obfuscator>"
        "-DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/tests/data/transform_basic.cpp"
        "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/test-transform-basic.cpp"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/check_transform_basic.cmake"
    )

    # Level 3 integer literal transformation test.
    add_test(
        NAME transform_level3
        COMMAND ${CMAKE_COMMAND}
            "-DOBFUSCATOR=$<TARGET_FILE:obfuscator>"
            "-DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/tests/data/transform_level3.cpp"
            "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/test-transform-level3.cpp"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/check_transform_level3.cmake"
    )

    # Level 2 comment-removal test.
    add_test(
    NAME validate_comment_removal
    COMMAND ${CMAKE_COMMAND}
        -DOBFUSCATOR=$<TARGET_FILE:obfuscator>
        -DINPUT=${CMAKE_SOURCE_DIR}/tests/data/transform_comments.cpp
        -DOUTPUT=${CMAKE_BINARY_DIR}/validated_comments.cpp
        -P ${CMAKE_SOURCE_DIR}/tests/check_transform_comments.cmake
    )

    # String and character literal preservation test.
   add_test(
    NAME validate_literal_transformation
    COMMAND ${CMAKE_COMMAND}
        -DOBFUSCATOR=$<TARGET_FILE:obfuscator>
        -DINPUT=${CMAKE_SOURCE_DIR}/tests/data/transform_literals.cpp
        -DOUTPUT=${CMAKE_BINARY_DIR}/validated_literals.cpp
        -P ${CMAKE_SOURCE_DIR}/tests/check_transform_literals.cmake
    )

    # Nested reference test.
    add_test(
        NAME transform_nested_reference
        COMMAND ${CMAKE_COMMAND}
            "-DOBFUSCATOR=$<TARGET_FILE:obfuscator>"
            "-DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/tests/data/transform_nested_reference.cpp"
            "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/test-transform-nested-reference.cpp"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/check_transform_nested_reference.cmake"
    )

    # True variable shadowing test.
    add_test(
        NAME transform_true_shadowing
        COMMAND ${CMAKE_COMMAND}
            "-DOBFUSCATOR=$<TARGET_FILE:obfuscator>"
            "-DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/tests/data/transform_true_shadowing.cpp"
            "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/test-transform-true-shadowing.cpp"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/check_transform_true_shadowing.cmake"
    )

        # For-loop variable renaming test.
    add_test(
        NAME transform_for_loop
        COMMAND ${CMAKE_COMMAND}
            "-DOBFUSCATOR=$<TARGET_FILE:obfuscator>"
            "-DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/tests/data/for_loop.cpp"
            "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/test-transform-for-loop.cpp"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/check_transform_for_loop.cmake"
    )

        # Verify for-loop variable scope.
    add_test(
        NAME transform_for_loop_scope
        COMMAND ${CMAKE_COMMAND}
            "-DOBFUSCATOR=$<TARGET_FILE:obfuscator>"
            "-DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/tests/data/transform_for_loop_scope.cpp"
            "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/test-transform-for-loop-scope.cpp"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/check_transform_for_loop_scope.cmake"
    )

    add_test(
    NAME transform_for_loop_shadowing
    COMMAND ${CMAKE_COMMAND}
        -DOBFUSCATOR=$<TARGET_FILE:obfuscator>
        -DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/tests/data/transform_for_loop_shadowing.cpp
        -DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/for_loop_shadowing_output.cpp
        -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/check_transform_for_loop_shadowing.cmake
    )

    
    add_test(
        NAME transform_multi_level_shadowing
        COMMAND ${CMAKE_COMMAND}
            -DOBFUSCATOR=$<TARGET_FILE:obfuscator>
            -DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/tests/data/transform_multi_level_shadowing.cpp
            -DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/multi_level_shadowing_output.cpp
            -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/check_transform_multi_level_shadowing.cmake
    )

        
    add_test(
        NAME transform_if_condition
        COMMAND ${CMAKE_COMMAND}
            -DOBFUSCATOR=$<TARGET_FILE:obfuscator>
            -DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/tests/data/transform_if_condition.cpp
            -DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/if_condition_output.cpp
            -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/check_transform_if_condition.cmake
    )

    add_test(
        NAME transform_if_else
        COMMAND ${CMAKE_COMMAND}
            -DOBFUSCATOR=$<TARGET_FILE:obfuscator>
            -DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/tests/data/transform_if_else.cpp
            -DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/if_else_output.cpp
            -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/check_transform_if_else.cmake
    )

    add_test(
        NAME transform_if_shadowing
        COMMAND ${CMAKE_COMMAND}
            -DOBFUSCATOR=$<TARGET_FILE:obfuscator>
            -DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/tests/data/transform_if_shadowing.cpp
            -DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/if_shadowing_output.cpp
            -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/check_transform_if_shadowing.cmake
    )
    add_test(
    NAME transform_while_condition
    COMMAND ${CMAKE_COMMAND}
        -DOBFUSCATOR=$<TARGET_FILE:obfuscator>
        -DINPUT=${CMAKE_SOURCE_DIR}/tests/data/transform_while_condition.cpp
        -DOUTPUT=${CMAKE_BINARY_DIR}/while_condition_output.cpp
        -P ${CMAKE_SOURCE_DIR}/tests/check_transform_while_condition.cmake
    )

    add_test(
    NAME transform_while_shadowing
    COMMAND ${CMAKE_COMMAND}
        -DOBFUSCATOR=$<TARGET_FILE:obfuscator>
        -DINPUT=${CMAKE_SOURCE_DIR}/tests/data/transform_while_shadowing.cpp
        -DOUTPUT=${CMAKE_BINARY_DIR}/while_shadowing_output.cpp
        -P ${CMAKE_SOURCE_DIR}/tests/check_transform_while_shadowing.cmake
    )

    add_test(
    NAME transform_while_outer_reference
    COMMAND ${CMAKE_COMMAND}
        -DOBFUSCATOR=$<TARGET_FILE:obfuscator>
        -DINPUT=${CMAKE_SOURCE_DIR}/tests/data/transform_while_outer_reference.cpp
        -DOUTPUT=${CMAKE_BINARY_DIR}/while_outer_reference_output.cpp
        -P ${CMAKE_SOURCE_DIR}/tests/check_transform_while_outer_reference.cmake
    )

    
    add_test(
        NAME transform_nested_scopes
        COMMAND ${CMAKE_COMMAND}
            -DOBFUSCATOR=$<TARGET_FILE:obfuscator>
            -DINPUT=${CMAKE_SOURCE_DIR}/tests/data/transform_nested_scopes.cpp
            -DOUTPUT=${CMAKE_BINARY_DIR}/nested_scopes_output.cpp
            -P ${CMAKE_SOURCE_DIR}/tests/check_transform_nested_scopes.cmake
    )

    add_test(
    NAME transform_nested_for_scopes
    COMMAND ${CMAKE_COMMAND}
        -DOBFUSCATOR=$<TARGET_FILE:obfuscator>
        -DINPUT=${CMAKE_SOURCE_DIR}/tests/data/transform_nested_for_scopes.cpp
        -DOUTPUT=${CMAKE_BINARY_DIR}/nested_for_scopes_output.cpp
        -P ${CMAKE_SOURCE_DIR}/tests/check_transform_nested_for_scopes.cmake
    )

    add_test(
    NAME transform_nested_for_different_names
    COMMAND ${CMAKE_COMMAND}
        -DOBFUSCATOR=$<TARGET_FILE:obfuscator>
        -DINPUT=${CMAKE_SOURCE_DIR}/tests/data/transform_nested_for_different_names.cpp
        -DOUTPUT=${CMAKE_BINARY_DIR}/nested_for_different_names_output.cpp
        -P ${CMAKE_SOURCE_DIR}/tests/check_transform_nested_for_different_names.cmake
    )

    add_test(
    NAME transform_multiple_variables
    COMMAND ${CMAKE_COMMAND}
        -DOBFUSCATOR=$<TARGET_FILE:obfuscator>
        -DINPUT=${CMAKE_SOURCE_DIR}/tests/data/transform_multiple_variables.cpp
        -DOUTPUT=${CMAKE_BINARY_DIR}/multiple_variables_output.cpp
        -P ${CMAKE_SOURCE_DIR}/tests/check_transform_multiple_variables.cmake
    )

    add_test(
    NAME transform_lambda_capture
    COMMAND ${CMAKE_COMMAND}
        -DOBFUSCATOR=$<TARGET_FILE:obfuscator>
        -DINPUT=${CMAKE_SOURCE_DIR}/tests/data/transform_lambda_capture.cpp
        -DOUTPUT=${CMAKE_BINARY_DIR}/lambda_capture_output.cpp
        -P ${CMAKE_SOURCE_DIR}/tests/check_transform_lambda_capture.cmake
    )

    add_test(
    NAME validate_lambda_shadowing
    COMMAND ${CMAKE_COMMAND}
        -DOBFUSCATOR=$<TARGET_FILE:obfuscator>
        -DINPUT=${CMAKE_SOURCE_DIR}/tests/data/transform_lambda_shadowing.cpp
        -DOUTPUT=${CMAKE_BINARY_DIR}/validated_lambda_shadowing.cpp
        -P ${CMAKE_SOURCE_DIR}/tests/check_transform_lambda_shadowing.cmake
    )

    add_test(
    NAME transform_lambda_parameters
    COMMAND obfuscator
        --input ${CMAKE_SOURCE_DIR}/tests/data/transform_lambda_parameters.cpp
        --output ${CMAKE_BINARY_DIR}/lambda_parameters_output.cpp
        --level 1
    )

    add_test(
    NAME transform_function_parameters
    COMMAND obfuscator
        --input ${CMAKE_SOURCE_DIR}/tests/data/transform_function_parameters.cpp
        --output ${CMAKE_BINARY_DIR}/function_parameters_output.cpp
        --level 1
    )

    add_test(
        NAME transform_function_shadowing
        COMMAND obfuscator
            --input ${CMAKE_SOURCE_DIR}/tests/data/transform_function_shadowing.cpp
            --output ${CMAKE_BINARY_DIR}/function_shadowing_output.cpp
            --level 1
    )

    add_test(
        NAME transform_function_lambda
        COMMAND obfuscator
            --input ${CMAKE_SOURCE_DIR}/tests/data/transform_function_lambda.cpp
            --output ${CMAKE_BINARY_DIR}/function_lambda_output.cpp
            --level 1
    )

        
    add_test(
        NAME validate_function_parameters
        COMMAND ${CMAKE_COMMAND}
            -DOBFUSCATOR=$<TARGET_FILE:obfuscator>
            -DINPUT=${CMAKE_SOURCE_DIR}/tests/data/transform_function_parameters.cpp
            -DOUTPUT=${CMAKE_BINARY_DIR}/validated_function_parameters.cpp
            -P ${CMAKE_SOURCE_DIR}/tests/check_transform_function_parameters.cmake
    )

    add_test(
    NAME validate_lambda_parameters
    COMMAND ${CMAKE_COMMAND}
        -DOBFUSCATOR=$<TARGET_FILE:obfuscator>
        -DINPUT=${CMAKE_SOURCE_DIR}/tests/data/transform_lambda_parameters.cpp
        -DOUTPUT=${CMAKE_BINARY_DIR}/validated_lambda_parameters.cpp
        -P ${CMAKE_SOURCE_DIR}/tests/check_transform_lambda_parameters.cmake
    )

    add_test(
    NAME compile_transformed_basic
    COMMAND ${CMAKE_COMMAND}
        -DOBFUSCATOR=$<TARGET_FILE:obfuscator>
        -DCXX_COMPILER=C:/msys64/ucrt64/bin/g++.exe
        -DINPUT=${CMAKE_SOURCE_DIR}/tests/data/transform_basic.cpp
        -DOUTPUT=${CMAKE_BINARY_DIR}/compiled_basic_output.cpp
        -DLEVEL=1
        -P ${CMAKE_SOURCE_DIR}/tests/check_compilation.cmake
    )

    add_test(
    NAME compile_transformed_comments
    COMMAND ${CMAKE_COMMAND}
        -DOBFUSCATOR=$<TARGET_FILE:obfuscator>
        -DCXX_COMPILER=C:/msys64/ucrt64/bin/g++.exe
        -DINPUT=${CMAKE_SOURCE_DIR}/tests/data/transform_comments.cpp
        -DOUTPUT=${CMAKE_BINARY_DIR}/compiled_comments_output.cpp
        -DLEVEL=2
        -P ${CMAKE_SOURCE_DIR}/tests/check_compilation.cmake
    )

    add_test(
        NAME compile_transformed_literals
        COMMAND ${CMAKE_COMMAND}
            -DOBFUSCATOR=$<TARGET_FILE:obfuscator>
            -DCXX_COMPILER=C:/msys64/ucrt64/bin/g++.exe
            -DINPUT=${CMAKE_SOURCE_DIR}/tests/data/transform_literals.cpp
            -DOUTPUT=${CMAKE_BINARY_DIR}/compiled_literals_output.cpp
            -DLEVEL=3
            -P ${CMAKE_SOURCE_DIR}/tests/check_compilation.cmake
    )

    add_test(
    NAME runtime_equivalence_basic
    COMMAND ${CMAKE_COMMAND}
        -DOBFUSCATOR=$<TARGET_FILE:obfuscator>
        -DCXX_COMPILER=C:/msys64/ucrt64/bin/g++.exe
        -DINPUT=${CMAKE_SOURCE_DIR}/tests/data/transform_basic.cpp
        -DOUTPUT=${CMAKE_BINARY_DIR}/runtime_basic_output.cpp
        -DLEVEL=1
        -P ${CMAKE_SOURCE_DIR}/tests/check_runtime_equivalence.cmake
    )

    add_test(
    NAME runtime_equivalence_comments
    COMMAND ${CMAKE_COMMAND}
        -DOBFUSCATOR=$<TARGET_FILE:obfuscator>
        -DCXX_COMPILER=C:/msys64/ucrt64/bin/g++.exe
        -DINPUT=${CMAKE_SOURCE_DIR}/tests/data/transform_comments.cpp
        -DOUTPUT=${CMAKE_BINARY_DIR}/runtime_comments_output.cpp
        -DLEVEL=2
        -P ${CMAKE_SOURCE_DIR}/tests/check_runtime_equivalence.cmake
    )

    add_test(
        NAME runtime_equivalence_literals
        COMMAND ${CMAKE_COMMAND}
            -DOBFUSCATOR=$<TARGET_FILE:obfuscator>
            -DCXX_COMPILER=C:/msys64/ucrt64/bin/g++.exe
            -DINPUT=${CMAKE_SOURCE_DIR}/tests/data/transform_literals.cpp
            -DOUTPUT=${CMAKE_BINARY_DIR}/runtime_literals_output.cpp
            -DLEVEL=3
            -P ${CMAKE_SOURCE_DIR}/tests/check_runtime_equivalence.cmake
    )

    add_test(
    NAME runtime_equivalence_pointers
    COMMAND ${CMAKE_COMMAND}
        -DOBFUSCATOR=$<TARGET_FILE:obfuscator>
        -DCXX_COMPILER=C:/msys64/ucrt64/bin/g++.exe
        -DINPUT=${CMAKE_SOURCE_DIR}/tests/data/transform_pointers_references.cpp
        -DOUTPUT=${CMAKE_BINARY_DIR}/runtime_pointers_output.cpp
        -DLEVEL=1
        -P ${CMAKE_SOURCE_DIR}/tests/check_runtime_equivalence.cmake
    )

    add_test(
    NAME runtime_equivalence_const
    COMMAND ${CMAKE_COMMAND}
        -DOBFUSCATOR=$<TARGET_FILE:obfuscator>
        -DCXX_COMPILER=C:/msys64/ucrt64/bin/g++.exe
        -DINPUT=${CMAKE_SOURCE_DIR}/tests/data/transform_const.cpp
        -DOUTPUT=${CMAKE_BINARY_DIR}/runtime_const_output.cpp
        -DLEVEL=1
        -P ${CMAKE_SOURCE_DIR}/tests/check_runtime_equivalence.cmake
    )

    add_test(
    NAME runtime_equivalence_global_variables
    COMMAND ${CMAKE_COMMAND}
        -DOBFUSCATOR=$<TARGET_FILE:obfuscator>
        -DCXX_COMPILER=C:/msys64/ucrt64/bin/g++.exe
        -DINPUT=${CMAKE_SOURCE_DIR}/tests/data/transform_global_variables.cpp
        -DOUTPUT=${CMAKE_BINARY_DIR}/runtime_global_output.cpp
        -DLEVEL=1
        -P ${CMAKE_SOURCE_DIR}/tests/check_runtime_equivalence.cmake
    )

    add_test(
    NAME runtime_equivalence_class_members
    COMMAND ${CMAKE_COMMAND}
        -DOBFUSCATOR=$<TARGET_FILE:obfuscator>
        -DCXX_COMPILER=C:/msys64/ucrt64/bin/g++.exe
        -DINPUT=${CMAKE_SOURCE_DIR}/tests/data/transform_class_members.cpp
        -DOUTPUT=${CMAKE_BINARY_DIR}/runtime_class_members_output.cpp
        -DLEVEL=1
        -P ${CMAKE_SOURCE_DIR}/tests/check_runtime_equivalence.cmake
    )

    add_test(
    NAME runtime_equivalence_struct_members
    COMMAND ${CMAKE_COMMAND}
        -DOBFUSCATOR=$<TARGET_FILE:obfuscator>
        -DCXX_COMPILER=C:/msys64/ucrt64/bin/g++.exe
        -DINPUT=${CMAKE_SOURCE_DIR}/tests/data/transform_struct_members.cpp
        -DOUTPUT=${CMAKE_BINARY_DIR}/runtime_struct_members_output.cpp
        -DLEVEL=1
        -P ${CMAKE_SOURCE_DIR}/tests/check_runtime_equivalence.cmake
    )


    add_test(
    NAME runtime_equivalence_arrays
    COMMAND ${CMAKE_COMMAND}
        -DOBFUSCATOR=$<TARGET_FILE:obfuscator>
        -DCXX_COMPILER=C:/msys64/ucrt64/bin/g++.exe
        -DINPUT=${CMAKE_SOURCE_DIR}/tests/data/transform_arrays.cpp
        -DOUTPUT=${CMAKE_BINARY_DIR}/runtime_arrays_output.cpp
        -DLEVEL=1
        -P ${CMAKE_SOURCE_DIR}/tests/check_runtime_equivalence.cmake
    )

    add_test(
    NAME runtime_equivalence_multidimensional_arrays
    COMMAND ${CMAKE_COMMAND}
        -DOBFUSCATOR=$<TARGET_FILE:obfuscator>
        -DCXX_COMPILER=C:/msys64/ucrt64/bin/g++.exe
        -DINPUT=${CMAKE_SOURCE_DIR}/tests/data/transform_multidimensional_arrays.cpp
        -DOUTPUT=${CMAKE_BINARY_DIR}/runtime_multidimensional_arrays_output.cpp
        -DLEVEL=1
        -P ${CMAKE_SOURCE_DIR}/tests/check_runtime_equivalence.cmake
    )

          # Class member access with nested-scope shadowing.
    add_test(
        NAME transform_member_scope
        COMMAND ${CMAKE_COMMAND}
            "-DOBFUSCATOR=$<TARGET_FILE:obfuscator>"
            "-DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/tests/data/member_scope.cpp"
            "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/member_scope_output.cpp"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/check_transform_member_scope.cmake"
    )

        # Class member access through function parameters.
    add_test(
        NAME transform_member_parameter
        COMMAND ${CMAKE_COMMAND}
            "-DOBFUSCATOR=$<TARGET_FILE:obfuscator>"
            "-DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/tests/data/member_parameter.cpp"
            "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/member_parameter_output.cpp"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/check_transform_member_parameter.cmake"
    )

    # Class member access through pointer parameters.
    add_test(
        NAME transform_member_pointer_parameter
        COMMAND ${CMAKE_COMMAND}
            "-DOBFUSCATOR=$<TARGET_FILE:obfuscator>"
            "-DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/tests/data/transform_member_pointer_parameter.cpp"
            "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/member_pointer_parameter_output.cpp"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/check_transform_member_pointer_parameter.cmake"
    )

    # Class member access through reference parameters.
    add_test(
        NAME transform_member_reference_parameter
        COMMAND ${CMAKE_COMMAND}
            "-DOBFUSCATOR=$<TARGET_FILE:obfuscator>"
            "-DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/tests/data/transform_member_reference_parameter.cpp"
            "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/member_reference_parameter_output.cpp"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/check_transform_member_reference_parameter.cmake"
    )

    # Class member access through multiple function parameters.
    add_test(
        NAME transform_member_multiple_parameters
        COMMAND ${CMAKE_COMMAND}
            "-DOBFUSCATOR=$<TARGET_FILE:obfuscator>"
            "-DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/tests/data/transform_member_multiple_parameters.cpp"
            "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/member_multiple_parameters_output.cpp"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/check_transform_member_multiple_parameters.cmake"
    )

    # Class member access through mixed function parameter types.
    add_test(
        NAME transform_member_mixed_parameters
        COMMAND ${CMAKE_COMMAND}
            "-DOBFUSCATOR=$<TARGET_FILE:obfuscator>"
            "-DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/tests/data/transform_member_mixed_parameters.cpp"
            "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/member_mixed_parameters_output.cpp"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/check_transform_member_mixed_parameters.cmake"
    )

    # Class member access through lambda parameters.
    add_test(
        NAME transform_member_lambda_parameter
        COMMAND ${CMAKE_COMMAND}
            "-DOBFUSCATOR=$<TARGET_FILE:obfuscator>"
            "-DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/tests/data/transform_member_lambda_parameter.cpp"
            "-DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/member_lambda_parameter_output.cpp"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/check_transform_member_lambda_parameter.cmake"
    )
endif()

# Tree-sitter runtime and C++ grammar
FetchContent_Declare(
    tree_sitter_runtime
    GIT_REPOSITORY https://github.com/tree-sitter/tree-sitter.git
    GIT_TAG v0.25.10
    GIT_SHALLOW TRUE
)

FetchContent_Declare(
    tree_sitter_cpp
    GIT_REPOSITORY https://github.com/tree-sitter/tree-sitter-cpp.git
    GIT_TAG v0.23.4
    GIT_SHALLOW TRUE
)

# Download the source repositories without adding their own
# projects to our build.
FetchContent_GetProperties(tree_sitter_runtime)
if(NOT tree_sitter_runtime_POPULATED)
    FetchContent_Populate(tree_sitter_runtime)
endif()

FetchContent_GetProperties(tree_sitter_cpp)
if(NOT tree_sitter_cpp_POPULATED)
    FetchContent_Populate(tree_sitter_cpp)
endif()

# Build the Tree-sitter runtime.
add_library(tree_sitter_runtime_lib STATIC
    "${tree_sitter_runtime_SOURCE_DIR}/lib/src/lib.c"
)

target_include_directories(tree_sitter_runtime_lib
    PUBLIC
        "${tree_sitter_runtime_SOURCE_DIR}/lib/include"
    PRIVATE
        "${tree_sitter_runtime_SOURCE_DIR}/lib/src"
)

set_target_properties(tree_sitter_runtime_lib PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
)

# Build the generated C++ grammar and its external scanner.
add_library(tree_sitter_cpp_grammar STATIC
    "${tree_sitter_cpp_SOURCE_DIR}/src/parser.c"
    "${tree_sitter_cpp_SOURCE_DIR}/src/scanner.c"
)

target_include_directories(tree_sitter_cpp_grammar
    PRIVATE
        "${tree_sitter_runtime_SOURCE_DIR}/lib/include"
        "${tree_sitter_cpp_SOURCE_DIR}/src"
)

set_target_properties(tree_sitter_cpp_grammar PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
)

# Temporary AST inspector for debugging Tree-sitter syntax trees.
add_executable(ast_inspector
    tests/ast_inspector.cpp
)

target_link_libraries(ast_inspector
    PRIVATE
        tree_sitter_runtime_lib
        tree_sitter_cpp_grammar
)
