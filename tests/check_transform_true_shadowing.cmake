
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

# Outer variable must be renamed.
string(FIND "${transformed}"
    "int obf_var_1 = 10"
    outer_pos)

if(outer_pos EQUAL -1)
    message(FATAL_ERROR "Outer variable was not renamed")
endif()

# Inner shadowing variable must receive a different name.
string(FIND "${transformed}"
    "int obf_var_2 = 20"
    inner_pos)

if(inner_pos EQUAL -1)
    message(FATAL_ERROR "Inner shadowing variable was not renamed correctly")
endif()

# Inner reference must use the inner variable's name.
string(FIND "${transformed}"
    "std::cout << obf_var_2"
    inner_reference_pos)

if(inner_reference_pos EQUAL -1)
    message(FATAL_ERROR "Inner variable reference was not renamed correctly")
endif()

# Outer reference after the nested block must use the outer variable's name.
string(FIND "${transformed}"
    "std::cout << obf_var_1"
    outer_reference_pos)

if(outer_reference_pos EQUAL -1)
    message(FATAL_ERROR "Outer variable reference was not renamed correctly")
endif()

message(STATUS "True shadowing test passed")
