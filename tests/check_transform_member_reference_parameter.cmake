execute_process(
    COMMAND "${OBFUSCATOR}"
        -i "${INPUT}"
        -o "${OUTPUT}"
        --level 1
    RESULT_VARIABLE result
)

if(NOT result EQUAL 0)
    message(FATAL_ERROR "Obfuscator execution failed.")
endif()

if(NOT EXISTS "${OUTPUT}")
    message(FATAL_ERROR "Output file was not generated.")
endif()

file(READ "${OUTPUT}" transformed)

# Check member declaration.
if(NOT transformed MATCHES "int obf_member_1 = 10;")
    message(FATAL_ERROR "Class member declaration was not renamed.")
endif()

# Check reference parameter member access.
if(NOT transformed MATCHES "counter[.]obf_member_1")
    message(FATAL_ERROR "Reference parameter member access was not renamed.")
endif()

# Ensure original member access is absent.
if(transformed MATCHES "counter[.]count")
    message(FATAL_ERROR "Original reference member access remains.")
endif()

message(STATUS "Member reference parameter test passed.")