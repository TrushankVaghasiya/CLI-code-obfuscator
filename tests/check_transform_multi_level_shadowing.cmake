
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

# Check all three declarations.
foreach(variable_number RANGE 1 3)
    if(NOT transformed MATCHES
        "int obf_var_${variable_number} =")
        message(FATAL_ERROR
            "Declaration obf_var_${variable_number} was not found")
    endif()
endforeach()

# Check innermost reference.
string(FIND "${transformed}"
    "std::cout << obf_var_3"
    inner_reference)

if(inner_reference EQUAL -1)
    message(FATAL_ERROR
        "Innermost variable reference was not renamed correctly")
endif()

# Check middle reference.
string(FIND "${transformed}"
    "std::cout << obf_var_2"
    middle_reference)

if(middle_reference EQUAL -1)
    message(FATAL_ERROR
        "Middle variable reference was not renamed correctly")
endif()

# Check outer reference.
string(FIND "${transformed}"
    "std::cout << obf_var_1"
    outer_reference)

if(outer_reference EQUAL -1)
    message(FATAL_ERROR
        "Outer variable reference was not renamed correctly")
endif()

message(STATUS "Multi-level shadowing test passed")
