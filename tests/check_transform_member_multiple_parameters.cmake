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

# Check class member declaration.
if(NOT transformed MATCHES "int obf_member_1 = 10;")
    message(FATAL_ERROR "Class member declaration was not renamed.")
endif()

# Check both parameter member accesses.
if(NOT transformed MATCHES "first[.]obf_member_1")
    message(FATAL_ERROR "First parameter member access was not renamed.")
endif()

if(NOT transformed MATCHES "second[.]obf_member_1")
    message(FATAL_ERROR "Second parameter member access was not renamed.")
endif()

# Ensure original member accesses are absent.
if(transformed MATCHES "first[.]count" OR
   transformed MATCHES "second[.]count")
    message(FATAL_ERROR "An original member access remains.")
endif()

message(STATUS "Member multiple parameters test passed.")