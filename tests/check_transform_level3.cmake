if(NOT DEFINED OBFUSCATOR OR
   NOT DEFINED INPUT OR
   NOT DEFINED OUTPUT)

    message(FATAL_ERROR
        "OBFUSCATOR, INPUT, and OUTPUT are required")
endif()

execute_process(
    COMMAND "${OBFUSCATOR}"
        -i "${INPUT}"
        -o "${OUTPUT}"
        --level 3
    RESULT_VARIABLE result
    OUTPUT_VARIABLE stdout
    ERROR_VARIABLE stderr
)

if(NOT result EQUAL 0)
    message(FATAL_ERROR
        "Obfuscator failed.\nSTDOUT:\n${stdout}\nSTDERR:\n${stderr}")
endif()

if(NOT EXISTS "${OUTPUT}")
    message(FATAL_ERROR
        "Transformed output file was not created")
endif()

file(READ "${OUTPUT}" transformed)

if(NOT transformed MATCHES "\\(5 \\+ 0\\)")
    message(FATAL_ERROR
        "Integer literal 5 was not transformed as expected")
endif()

if(NOT transformed MATCHES "\\(42 \\+ 0\\)")
    message(FATAL_ERROR
        "Integer literal 42 was not transformed as expected")
endif()

if(transformed MATCHES "//")
    message(FATAL_ERROR
        "A line comment remains in the Level 3 output")
endif()

message(STATUS
    "Level 3 integer transformation test passed")