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
    message(FATAL_ERROR "Variable a was not renamed correctly")
endif()

if(NOT transformed MATCHES "int obf_var_2 = 20;")
    message(FATAL_ERROR "Variable b was not renamed correctly")
endif()

if(NOT transformed MATCHES "int obf_var_3 = 30;")
    message(FATAL_ERROR "Variable c was not renamed correctly")
endif()

if(NOT transformed MATCHES "int obf_var_4 = obf_var_1 \\+ obf_var_2 \\+ obf_var_3;")
    message(FATAL_ERROR "Multiple variable references are incorrect")
endif()

if(NOT transformed MATCHES "cout << obf_var_4")
    message(FATAL_ERROR "Result variable reference is incorrect")
endif()

message(STATUS "Multiple variables transformation test passed")