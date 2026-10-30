
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

# Loop variable declaration must be renamed.
if(NOT transformed MATCHES "for \\(int obf_var_1 = 0;")
    message(FATAL_ERROR "Loop variable declaration was not renamed")
endif()

# Loop condition and increment must use the renamed variable.
if(NOT transformed MATCHES "obf_var_1 < 5")
    message(FATAL_ERROR "Loop condition reference was not renamed")
endif()

if(NOT transformed MATCHES "obf_var_1\\+\\+")
    message(FATAL_ERROR "Loop increment reference was not renamed")
endif()

# First outer reference must use obf_var_1.
string(FIND "${transformed}"
    "std::cout << obf_var_1"
    first_outer_reference)

if(first_outer_reference EQUAL -1)
    message(FATAL_ERROR "First outer reference was not renamed")
endif()

# Inner shadowing variable must receive a different name.
if(NOT transformed MATCHES "int obf_var_2 = 100")
    message(FATAL_ERROR "Inner shadowing variable was not renamed correctly")
endif()

# Inner reference must use obf_var_2.
string(FIND "${transformed}"
    "std::cout << obf_var_2"
    inner_reference)

if(inner_reference EQUAL -1)
    message(FATAL_ERROR "Inner reference was not renamed correctly")
endif()

# There must be two outer references in the transformed output.
string(REGEX MATCHALL
    "std::cout << obf_var_1"
    outer_references
    "${transformed}")

list(LENGTH outer_references outer_reference_count)

if(NOT outer_reference_count EQUAL 2)
    message(FATAL_ERROR
        "Expected 2 outer references, found ${outer_reference_count}")
endif()

message(STATUS "For-loop shadowing test passed")
