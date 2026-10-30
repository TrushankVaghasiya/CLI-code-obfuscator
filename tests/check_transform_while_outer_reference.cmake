
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
    message(FATAL_ERROR "Count variable was not renamed correctly")
endif()

if(NOT transformed MATCHES "int obf_var_2 = 2")
    message(FATAL_ERROR "Multiplier variable was not renamed correctly")
endif()

if(NOT transformed MATCHES "while \\(int obf_var_3 = obf_var_1\\)")
    message(FATAL_ERROR "While condition declaration is incorrect")
endif()

if(NOT transformed MATCHES "obf_var_3 \\* obf_var_2")
    message(FATAL_ERROR "References inside while body are incorrect")
endif()

if(NOT transformed MATCHES "--obf_var_1")
    message(FATAL_ERROR "Outer variable reference in loop body is incorrect")
endif()

if(transformed MATCHES "\\b(count|multiplier|value)\\b")
    message(FATAL_ERROR "Original variable names still exist")
endif()

message(STATUS "While outer reference transformation test passed")
