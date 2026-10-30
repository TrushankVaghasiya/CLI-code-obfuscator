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

foreach(variable RANGE 1 4)
    if(NOT transformed MATCHES "obf_var_${variable}")
        message(FATAL_ERROR
            "Expected obf_var_${variable} was not found")
    endif()
endforeach()

if(transformed MATCHES "\\b(x|i|j|k)\\b")
    message(FATAL_ERROR "Original variable names still exist")
endif()

message(STATUS "Nested for-loop different names test passed")