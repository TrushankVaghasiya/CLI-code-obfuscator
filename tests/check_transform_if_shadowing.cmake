
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
        --level 1
    RESULT_VARIABLE result
    OUTPUT_VARIABLE stdout
    ERROR_VARIABLE stderr
)

if(NOT result EQUAL 0)
    message(FATAL_ERROR
        "Obfuscator failed.\nSTDOUT:\n${stdout}\nSTDERR:\n${stderr}")
endif()

if(NOT EXISTS "${OUTPUT}")
    message(FATAL_ERROR "Output file was not created")
endif()

file(READ "${OUTPUT}" transformed)

# Outer if initializer.
if(NOT transformed MATCHES "int obf_var_1 = 10")
    message(FATAL_ERROR "If initializer was not renamed")
endif()

# Inner shadowing declaration.
if(NOT transformed MATCHES "int obf_var_2 = 20")
    message(FATAL_ERROR "Inner shadowing variable was not renamed")
endif()

# Outer references: condition and two body references.
string(REGEX MATCHALL
    "obf_var_1"
    outer_names
    "${transformed}")

list(LENGTH outer_names outer_count)

if(NOT outer_count EQUAL 4)
    message(FATAL_ERROR
        "Expected 4 outer variable occurrences, found ${outer_count}")
endif()

# Inner reference.
string(FIND "${transformed}"
    "std::cout << obf_var_2"
    inner_reference)

if(inner_reference EQUAL -1)
    message(FATAL_ERROR "Inner variable reference was not renamed")
endif()

message(STATUS "If shadowing test passed")
