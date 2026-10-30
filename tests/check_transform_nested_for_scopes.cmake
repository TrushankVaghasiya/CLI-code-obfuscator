if(NOT DEFINED OBFUSCATOR OR NOT DEFINED INPUT OR NOT DEFINED OUTPUT)
    message(FATAL_ERROR "Missing required test arguments")
endif()

execute_process(
    COMMAND "${OBFUSCATOR}"
            --input "${INPUT}"
            --output "${OUTPUT}"
            --level 1
    RESULT_VARIABLE result
    OUTPUT_VARIABLE stdout
    ERROR_VARIABLE stderr
)

if(NOT result EQUAL 0)
    message(FATAL_ERROR
        "Obfuscator execution failed:\n${stdout}\n${stderr}")
endif()

file(READ "${OUTPUT}" transformed)

if(NOT transformed MATCHES "int obf_var_1 = 100")
    message(FATAL_ERROR "Outer variable was not renamed correctly")
endif()

if(NOT transformed MATCHES "for \\(int obf_var_2 = 0")
    message(FATAL_ERROR "Outer for-loop variable was not renamed correctly")
endif()

if(NOT transformed MATCHES "for \\(int obf_var_3 = 0")
    message(FATAL_ERROR "Inner for-loop variable was not renamed correctly")
endif()

if(transformed MATCHES "\\bi\\b")
    message(FATAL_ERROR "Original variable name still exists")
endif()

message(STATUS "Nested for-loop scopes transformation test passed")