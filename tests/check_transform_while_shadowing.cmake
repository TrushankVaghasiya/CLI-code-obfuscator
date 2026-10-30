
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

if(NOT transformed MATCHES "int obf_var_1 = 3")
    message(FATAL_ERROR "Outer variable was not renamed correctly")
endif()

if(NOT transformed MATCHES "while \\(int obf_var_2 = obf_var_1\\)")
    message(FATAL_ERROR "While condition variable was not renamed correctly")
endif()

if(NOT transformed MATCHES "std::cout << obf_var_2")
    message(FATAL_ERROR "While condition variable reference is incorrect")
endif()

if(NOT transformed MATCHES "int obf_var_3 = 20")
    message(FATAL_ERROR "Shadowed variable was not renamed correctly")
endif()

if(NOT transformed MATCHES "std::cout << obf_var_3")
    message(FATAL_ERROR "Shadowed variable reference is incorrect")
endif()

if(transformed MATCHES "value")
    message(FATAL_ERROR "Original variable name 'value' still exists")
endif()

message(STATUS "While shadowing transformation test passed")
