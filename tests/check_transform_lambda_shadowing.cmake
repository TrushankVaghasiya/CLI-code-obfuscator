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

if(NOT transformed MATCHES "int obf_var_1 = 10;")
    message(FATAL_ERROR "Outer variable was not renamed correctly")
endif()

if(NOT transformed MATCHES "int obf_var_2 = 20;")
    message(FATAL_ERROR "Lambda-local variable was not renamed correctly")
endif()

if(NOT transformed MATCHES "cout << obf_var_2")
    message(FATAL_ERROR "Lambda reference is incorrect")
endif()

if(NOT transformed MATCHES "cout << obf_var_1")
    message(FATAL_ERROR "Outer reference is incorrect")
endif()

if(transformed MATCHES "\\bvalue\\b")
    message(FATAL_ERROR "Original variable name still exists")
endif()

message(STATUS "Lambda shadowing transformation test passed")