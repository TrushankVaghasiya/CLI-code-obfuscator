
if(NOT DEFINED INPUT OR
   NOT DEFINED OBFUSCATOR OR
   NOT DEFINED CXX_COMPILER OR
   NOT DEFINED OUTPUT OR
   NOT DEFINED LEVEL)
    message(FATAL_ERROR "Missing required test arguments")
endif()

get_filename_component(output_directory "${OUTPUT}" DIRECTORY)
get_filename_component(output_name "${OUTPUT}" NAME_WE)

set(original_exe "${output_directory}/${output_name}_original.exe")
set(transformed_exe "${output_directory}/${output_name}_transformed.exe")

# Step 1: Compile original source.
execute_process(
    COMMAND "${CXX_COMPILER}" -std=c++17
            "${INPUT}" -o "${original_exe}"
    RESULT_VARIABLE original_compile_result
    OUTPUT_VARIABLE original_compile_stdout
    ERROR_VARIABLE original_compile_stderr
)

if(NOT original_compile_result EQUAL 0)
    message(FATAL_ERROR
        "Original source compilation failed:\n${original_compile_stderr}")
endif()

# Step 2: Run obfuscator.
execute_process(
    COMMAND "${OBFUSCATOR}"
            --input "${INPUT}"
            --output "${OUTPUT}"
            --level "${LEVEL}"
    RESULT_VARIABLE obfuscator_result
    OUTPUT_VARIABLE obfuscator_stdout
    ERROR_VARIABLE obfuscator_stderr
)

if(NOT obfuscator_result EQUAL 0)
    message(FATAL_ERROR
        "Obfuscator execution failed:\n${obfuscator_stderr}")
endif()

# Step 3: Compile transformed source.
execute_process(
    COMMAND "${CXX_COMPILER}" -std=c++17
            "${OUTPUT}" -o "${transformed_exe}"
    RESULT_VARIABLE transformed_compile_result
    OUTPUT_VARIABLE transformed_compile_stdout
    ERROR_VARIABLE transformed_compile_stderr
)

if(NOT transformed_compile_result EQUAL 0)
    message(FATAL_ERROR
        "Transformed source compilation failed:\n${transformed_compile_stderr}")
endif()

# Step 4: Execute original program.
execute_process(
    COMMAND "${original_exe}"
    RESULT_VARIABLE original_run_result
    OUTPUT_VARIABLE original_output
    ERROR_VARIABLE original_run_stderr
)

# Step 5: Execute transformed program.
execute_process(
    COMMAND "${transformed_exe}"
    RESULT_VARIABLE transformed_run_result
    OUTPUT_VARIABLE transformed_output
    ERROR_VARIABLE transformed_run_stderr
)

# Step 6: Compare exit codes.
if(NOT original_run_result EQUAL transformed_run_result)
    message(FATAL_ERROR
        "Exit codes differ: original=${original_run_result}, transformed=${transformed_run_result}")
endif()

# Step 7: Compare standard output.
if(NOT original_output STREQUAL transformed_output)
    message(FATAL_ERROR
        "Program outputs differ!\n"
        "Original:\n${original_output}\n"
        "Transformed:\n${transformed_output}")
endif()

message(STATUS "Runtime equivalence test passed")
message(STATUS "Output: ${original_output}")
message(STATUS "Exit code: ${original_run_result}")
