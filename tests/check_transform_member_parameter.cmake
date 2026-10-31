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

# Check that the class member declaration was renamed.
if(NOT transformed MATCHES "int obf_member_1 = 10;")
    message(FATAL_ERROR "Class member declaration was not renamed.")
endif()

# Check that both the parameter and local object access use the renamed member.
string(REGEX MATCHALL
    "obf_member_1"
    member_references
    "${transformed}"
)

list(LENGTH member_references member_count)

# One occurrence is the declaration; two are member accesses.
if(NOT member_count EQUAL 4)
    message(FATAL_ERROR
        "Expected 4 occurrences of obf_member_1 (1 declaration and 3 accesses), found ${member_count}.")
endif()

# Ensure no original member access remains.
if(transformed MATCHES "counter[.]count")
    message(FATAL_ERROR "An original counter.count access remains.")
endif()

message(STATUS "Member parameter transformation test passed.")
