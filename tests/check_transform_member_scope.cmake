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
    message(FATAL_ERROR "Class member was not renamed correctly.")
endif()

# Check object.member accesses.
string(REGEX MATCHALL
    "obf_var_1[.]obf_member_1"
    member_accesses
    "${transformed}"
)



list(LENGTH member_accesses member_count)

if(NOT member_count EQUAL 2)
    message(FATAL_ERROR
        "Expected 2 renamed object-member accesses, found ${member_count}.")
endif()

# Check that the shadowing local variable is renamed independently.
if(NOT transformed MATCHES
    "int obf_var_2 = 5;[ \t\r\n]*obf_var_2\\+\\+;")
    message(FATAL_ERROR
        "Nested local variable was not renamed correctly.")
endif()

# Ensure the original object-member access was not left behind.
if(transformed MATCHES "counter[.]count")
    message(FATAL_ERROR
        "An original counter.count access was not renamed.")
endif()

message(STATUS "Member scope transformation test passed.")