
if(NOT DEFINED OBFUSCATOR OR
   NOT DEFINED INPUT OR
   NOT DEFINED OUTPUT OR
   NOT DEFINED CXX_COMPILER)
    message(FATAL_ERROR "Missing required test arguments")
endif()

# Step 1: Run the obfuscator.
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
        "Obfuscator execution failed:\n"
        "${obfuscator_stdout}\n${obfuscator_stderr}")
endif()

if(NOT EXISTS "${OUTPUT}")
    message(FATAL_ERROR "Obfuscated output was not generated")
endif()

# Step 2: Compile the transformed C++ source.
get_filename_component(output_directory "${OUTPUT}" DIRECTORY)
get_filename_component(output_name "${OUTPUT}" NAME_WE)

set(executable "${output_directory}/${output_name}_compiled.exe")

execute_process(
    COMMAND "${CXX_COMPILER}"
            -std=c++17
            "${OUTPUT}"
            -o "${executable}"
    RESULT_VARIABLE compile_result
    OUTPUT_VARIABLE compile_stdout
    ERROR_VARIABLE compile_stderr
)

if(NOT compile_result EQUAL 0)
    message(FATAL_ERROR
        "Compilation of transformed output failed:\n"
        "${compile_stdout}\n${compile_stderr}")
endif()

if(NOT EXISTS "${executable}")
    message(FATAL_ERROR
        "Compilation reported success but no executable was generated")
endif()

message(STATUS "Transformed C++ compiled successfully")
