
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

# Declaration must be renamed.
string(FIND "${transformed}"
    "int obf_var_1 = 10"
    declaration_pos)

if(declaration_pos EQUAL -1)
    message(FATAL_ERROR "Outer variable declaration was not renamed")
endif()

# Reference inside nested block must also be renamed.
string(FIND "${transformed}"
    "std::cout << obf_var_1"
    reference_pos)

if(reference_pos EQUAL -1)
    message(FATAL_ERROR "Nested reference was not renamed")
endif()

# Original identifier must no longer exist.
string(FIND "${transformed}" "value" original_pos)

if(NOT original_pos EQUAL -1)
    message(FATAL_ERROR "Original variable name still exists")
endif()

message(STATUS "Nested reference test passed")
