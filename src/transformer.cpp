#include "transformer.hpp"
#include "literal_transformer.hpp"
#include <tree_sitter/api.h>
#include <iostream>
#include <algorithm>
#include <cstdint>
#include <utility>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <string>
#include <vector>

extern "C" const TSLanguage* tree_sitter_cpp(void);

namespace fs = std::filesystem;

namespace {

struct Edit {
    std::size_t start;
    std::size_t end;
    std::string replacement;
};

struct Candidate {
    TSNode declaration;
    TSNode nameNode;
    std::string name;
};

std::string nodeText(TSNode node, const std::string& source) {
    const std::size_t start = ts_node_start_byte(node);
    const std::size_t end = ts_node_end_byte(node);

    if (start > end || end > source.size()) {
        return {};
    }

    return source.substr(start, end - start);
}


bool isSimpleInitializedLocal(TSNode declaration, Candidate& result,
                              const std::string& source) {
    if (std::string(ts_node_type(declaration)) != "declaration") {
        return false;
    }

    
    bool hasPrimitiveType = false;
    TSNode initDeclarator = {};
    TSNode directIdentifier = {};
    int initDeclaratorCount = 0;
    int directIdentifierCount = 0;
    bool seenEquals = false;

    const std::uint32_t count = ts_node_child_count(declaration);

    for (std::uint32_t i = 0; i < count; ++i) {
        TSNode child = ts_node_child(declaration, i);
        const std::string type = ts_node_type(child);

        if (type == "primitive_type") {
            hasPrimitiveType = true;
        }

        if (type == "init_declarator") {
            initDeclarator = child;
            ++initDeclaratorCount;
        }

        if (type == "=") {
            seenEquals = true;
        }

        // Only consider identifiers before the initializer.
        if (type == "identifier" && !seenEquals) {
            directIdentifier = child;
            ++directIdentifierCount;
        }
    }


    if (!hasPrimitiveType) {
        return false;
    }

    TSNode nameNode = {};

    if (initDeclaratorCount == 1 && directIdentifierCount == 0) {

        // Ordinary initialized declaration.
        nameNode = ts_node_child_by_field_name(
            initDeclarator, "declarator", 10
        );

    }
    else if (initDeclaratorCount == 0 &&
             directIdentifierCount == 1) {

        // Condition declaration, such as while (int value = count).
        nameNode = directIdentifier;

    }
    else {
        return false;
    }

    if (ts_node_is_null(nameNode) ||
        std::string(ts_node_type(nameNode)) != "identifier") {
        return false;
    }

    result.declaration = declaration;
    result.nameNode = nameNode;
    result.name = nodeText(nameNode, source);

    return !result.name.empty();
}




bool isAnotherDeclarationName(TSNode node) {
    TSNode parent = ts_node_parent(node);

    if (ts_node_is_null(parent)) {
        return false;
    }

    const std::string parentType = ts_node_type(parent);

    if (parentType == "init_declarator" ||
        parentType == "parameter_declaration" ||
        parentType == "optional_parameter_declaration" ||
        parentType == "condition_clause") {
        TSNode declarator = ts_node_child_by_field_name(
            parent, "declarator", 10
        );

        if (!ts_node_is_null(declarator) &&
            ts_node_eq(declarator, node)) {
            return true;
        }
    }

    if (parentType == "declaration") {
        TSNode declarator = ts_node_child_by_field_name(
            parent, "declarator", 10
        );

        if (!ts_node_is_null(declarator) &&
            ts_node_eq(declarator, node)) {
            return true;
        }
    }

    return false;
}


bool subtreeDeclaresName(
    TSNode node,
    const std::string& name,
    const std::string& source
) {
    if (std::string(ts_node_type(node)) == "identifier" &&
        nodeText(node, source) == name &&
        isAnotherDeclarationName(node)) {
        return true;
    }

    const std::uint32_t count = ts_node_child_count(node);

    for (std::uint32_t i = 0; i < count; ++i) {
        if (subtreeDeclaresName(
                ts_node_child(node, i), name, source)) {
            return true;
        }
    }

    return false;
}

bool compoundDeclaresNameInOwnScope(
    TSNode compound,
    const std::string& name,
    const std::string& source
) {
    const std::uint32_t count = ts_node_child_count(compound);

    for (std::uint32_t i = 0; i < count; ++i) {
        TSNode child = ts_node_child(compound, i);

        // Only inspect direct declarations.
        // Do not recursively inspect nested compound statements.
        if (std::string(ts_node_type(child)) == "declaration" &&
            subtreeDeclaresName(child, name, source)) {
            return true;
        }
    }

    return false;
}


bool statementDeclaresNameInCondition(
    TSNode statement,
    const std::string& name,
    const std::string& source
) {
    const std::string statementType = ts_node_type(statement);

    if (statementType != "if_statement" &&
        statementType != "while_statement") {
        return false;
    }

    const std::uint32_t count = ts_node_child_count(statement);

    for (std::uint32_t i = 0; i < count; ++i) {
        TSNode child = ts_node_child(statement, i);

        if (std::string(ts_node_type(child)) != "condition_clause") {
            continue;
        }

        const std::uint32_t clauseCount = ts_node_child_count(child);

        for (std::uint32_t j = 0; j < clauseCount; ++j) {
            TSNode declaration = ts_node_child(child, j);

            Candidate candidate{};

            if (isSimpleInitializedLocal(
                    declaration, candidate, source) &&
                candidate.name == name) {
                return true;
            }
        }
    }

    return false;
}


bool forStatementDeclaresName(
    TSNode statement,
    const std::string& name,
    const std::string& source
) {
    if (std::string(ts_node_type(statement)) != "for_statement") {
        return false;
    }

    const std::uint32_t count = ts_node_child_count(statement);

    for (std::uint32_t i = 0; i < count; ++i) {
        TSNode child = ts_node_child(statement, i);

        if (std::string(ts_node_type(child)) != "declaration") {
            continue;
        }

        Candidate candidate{};

        if (isSimpleInitializedLocal(child, candidate, source) &&
            candidate.name == name) {
            return true;
        }
    }

    return false;
}

bool isSimpleLambdaParameter(
    TSNode parameter,
    Candidate& candidate,
    const std::string& source
) {
    if (std::string(ts_node_type(parameter)) !=
        "parameter_declaration") {
        return false;
    }

    TSNode typeNode = ts_node_child_by_field_name(
        parameter, "type", 4
    );

    TSNode declarator = ts_node_child_by_field_name(
        parameter, "declarator", 10
    );

    if (ts_node_is_null(typeNode) ||
        ts_node_is_null(declarator)) {
        return false;
    }

    if (std::string(ts_node_type(typeNode)) != "primitive_type" ||
        std::string(ts_node_type(declarator)) != "identifier") {
        return false;
    }

    candidate.declaration = parameter;
    candidate.nameNode = declarator;
    candidate.name = nodeText(declarator, source);

    return true;
}

void findLambdaParameters(
    TSNode lambda,
    const std::string& source,
    std::vector<Candidate>& candidates
) {
    const std::uint32_t count = ts_node_child_count(lambda);

    for (std::uint32_t i = 0; i < count; ++i) {
        TSNode child = ts_node_child(lambda, i);

        if (std::string(ts_node_type(child)) !=
            "abstract_function_declarator") {
            continue;
        }

        const std::uint32_t declaratorCount =
            ts_node_child_count(child);

        for (std::uint32_t j = 0; j < declaratorCount; ++j) {
            TSNode declaratorChild = ts_node_child(child, j);

            if (std::string(ts_node_type(declaratorChild)) !=
                "parameter_list") {
                continue;
            }

            const std::uint32_t parameterCount =
                ts_node_child_count(declaratorChild);

            for (std::uint32_t k = 0; k < parameterCount; ++k) {
                TSNode parameter = ts_node_child(
                    declaratorChild, k
                );

                if (std::string(ts_node_type(parameter)) !=
                    "parameter_declaration") {
                    continue;
                }

                Candidate candidate{};

                if (isSimpleLambdaParameter(
                        parameter, candidate, source)) {
                    candidates.push_back(candidate);
                }
            }
        }
    }
}

bool lambdaDeclaresNameInParameters(
    TSNode lambda,
    const std::string& name,
    const std::string& source
) {
    const std::uint32_t count = ts_node_child_count(lambda);

    for (std::uint32_t i = 0; i < count; ++i) {
        TSNode child = ts_node_child(lambda, i);

        if (std::string(ts_node_type(child)) !=
            "abstract_function_declarator") {
            continue;
        }

        const std::uint32_t declaratorCount =
            ts_node_child_count(child);

        for (std::uint32_t j = 0; j < declaratorCount; ++j) {
            TSNode parameterList = ts_node_child(child, j);

            if (std::string(ts_node_type(parameterList)) !=
                "parameter_list") {
                continue;
            }

            const std::uint32_t parameterCount =
                ts_node_child_count(parameterList);

            for (std::uint32_t k = 0; k < parameterCount; ++k) {
                TSNode parameter = ts_node_child(
                    parameterList, k
                );

                Candidate candidate{};

                if (isSimpleLambdaParameter(
                        parameter, candidate, source) &&
                    candidate.name == name) {
                    return true;
                }
            }
        }
    }

    return false;
}

void collectReferences(
    TSNode node,
    TSNode rootBody,
    const Candidate& candidate,
    const std::string& source,
    std::vector<TSNode>& references,
    bool& unsafe
) {
    const std::string type = ts_node_type(node);

    // Check nested blocks for declarations that shadow this variable.
    // Check nested scopes for declarations that shadow this variable.
    if (!ts_node_eq(node, rootBody)) {

        // A lambda parameter with the same name shadows
        // the variable from the outer scope.
        if (type == "lambda_expression" &&
            lambdaDeclaresNameInParameters(
                node, candidate.name, source)) {
            return;
        }

        // Check nested compound blocks.
        if (type == "compound_statement" &&
            compoundDeclaresNameInOwnScope(
                node, candidate.name, source)) {
            return;
        }

        // Check nested if/while condition declarations.
        if ((type == "if_statement" || type == "while_statement") &&
            statementDeclaresNameInCondition(
                node, candidate.name, source)) {
            return;
        }

        // Check nested for-loop declarations.
        // Check nested for-loop declarations.
        if (type == "for_statement" &&
            forStatementDeclaresName(
                node, candidate.name, source)) {

            // Only treat this loop as shadowing if it is
            // actually inside the candidate's scope.
            TSNode current = ts_node_parent(node);
            bool insideRootScope = false;

            while (!ts_node_is_null(current)) {
                if (ts_node_eq(current, rootBody)) {
                    insideRootScope = true;
                    break;
                }

                current = ts_node_parent(current);
            }

            if (insideRootScope) {
                return;
            }
        }
    }

    if (type == "identifier" &&
        nodeText(node, source) == candidate.name) {

        if (ts_node_eq(node, candidate.nameNode)) {
            // Declaration itself is handled separately.
        }
        else if (isAnotherDeclarationName(node)) {
            unsafe = true;
        }
        else {
            references.push_back(node);
        }
    }

    const std::uint32_t count = ts_node_child_count(node);

    for (std::uint32_t i = 0; i < count && !unsafe; ++i) {
        collectReferences(
            ts_node_child(node, i),
            rootBody,
            candidate,
            source,
            references,
            unsafe
        );
    }
}






void findCandidates(
    TSNode node,
    const std::string& source,
    std::vector<Candidate>& candidates
) {
    const std::string nodeType = ts_node_type(node);

    if (nodeType == "lambda_expression") {
        findLambdaParameters(node, source, candidates);
    }

    if (nodeType == "for_statement") {
    const std::uint32_t count = ts_node_child_count(node);

    for (std::uint32_t i = 0; i < count; ++i) {
        TSNode child = ts_node_child(node, i);

        Candidate candidate{};

        if (isSimpleInitializedLocal(child, candidate, source)) {
            candidates.push_back(candidate);
            break;
        }
    }
}

// Look for declarations inside if initializer statements.
    
// Look for declarations inside if condition clauses.
    if (nodeType == "if_statement") {
        const std::uint32_t count = ts_node_child_count(node);

        for (std::uint32_t i = 0; i < count; ++i) {
            TSNode child = ts_node_child(node, i);

            if (std::string(ts_node_type(child)) != "condition_clause") {
                continue;
            }

            const std::uint32_t clauseCount = ts_node_child_count(child);

            for (std::uint32_t j = 0; j < clauseCount; ++j) {
                TSNode clauseChild = ts_node_child(child, j);
                const std::string clauseType = ts_node_type(clauseChild);

                // Handle a declaration directly inside condition_clause.
                if (clauseType == "declaration") {
                    Candidate candidate{};

                    if (isSimpleInitializedLocal(
                            clauseChild, candidate, source)) {
                        candidates.push_back(candidate);
                    }
                }

                // Also support declarations inside init_statement.
                if (clauseType == "init_statement") {
                    const std::uint32_t initCount =
                        ts_node_child_count(clauseChild);

                    for (std::uint32_t k = 0; k < initCount; ++k) {
                        TSNode declaration = ts_node_child(clauseChild, k);
                        Candidate candidate{};

                        if (isSimpleInitializedLocal(
                                declaration, candidate, source)) {
                            candidates.push_back(candidate);
                        }
                    }
                }
            }

            break;
        }
    }


    // Handle declarations inside while condition clauses.
    if (nodeType == "while_statement") {

        const std::uint32_t count = ts_node_child_count(node);

        for (std::uint32_t i = 0; i < count; ++i) {

            TSNode child = ts_node_child(node, i);

            if (std::string(ts_node_type(child)) != "condition_clause") {
                continue;
            }

            const std::uint32_t conditionCount =
                ts_node_child_count(child);

            for (std::uint32_t j = 0; j < conditionCount; ++j) {

                TSNode declaration = ts_node_child(child, j);

                Candidate candidate{};

                if (isSimpleInitializedLocal(
                        declaration, candidate, source)) {

                    candidates.push_back(candidate);
                    break;
                }
            }

            break;
        }
    }
    // Look for declarations directly inside every compound scope.
    if (nodeType == "compound_statement") {
        const std::uint32_t count = ts_node_child_count(node);

        for (std::uint32_t i = 0; i < count; ++i) {
            TSNode child = ts_node_child(node, i);

            Candidate candidate{};

            if (isSimpleInitializedLocal(child, candidate, source)) {
                candidates.push_back(candidate);
            }
        }
    }

    // Recursively inspect nested scopes and other syntax nodes.
    const std::uint32_t count = ts_node_child_count(node);

    for (std::uint32_t i = 0; i < count; ++i) {
        findCandidates(ts_node_child(node, i), source, candidates);
    }
}


} // namespace



static void collectCommentRanges(
    TSNode node,
    std::vector<std::pair<uint32_t, uint32_t>>& ranges
) {
    if (std::string(ts_node_type(node)) == "comment") {
        ranges.emplace_back(
            ts_node_start_byte(node),
            ts_node_end_byte(node)
        );
        return;
    }

    uint32_t childCount = ts_node_child_count(node);

    for (uint32_t i = 0; i < childCount; ++i) {
        collectCommentRanges(ts_node_child(node, i), ranges);
    }
}

static bool removeCommentsFromSource(
    std::string& source,
    std::string& error
) {
    TSParser* parser = ts_parser_new();

    if (parser == nullptr) {
        error = "Failed to create Tree-sitter parser for comment removal.";
        return false;
    }

    if (!ts_parser_set_language(parser, tree_sitter_cpp())) {
        ts_parser_delete(parser);
        error = "Failed to set the C++ grammar for comment removal.";
        return false;
    }

    TSTree* tree = ts_parser_parse_string(
        parser,
        nullptr,
        source.data(),
        static_cast<uint32_t>(source.size())
    );

    if (tree == nullptr) {
        ts_parser_delete(parser);
        error = "Failed to parse source during comment removal.";
        return false;
    }

    TSNode root = ts_tree_root_node(tree);

    if (ts_node_has_error(root)) {
        ts_tree_delete(tree);
        ts_parser_delete(parser);
        error = "Source contains syntax errors during comment removal.";
        return false;
    }

    std::vector<std::pair<uint32_t, uint32_t>> ranges;
    collectCommentRanges(root, ranges);

    ts_tree_delete(tree);
    ts_parser_delete(parser);

    // Replace comment characters with spaces, preserving line breaks.
    // This avoids accidentally joining adjacent C++ tokens.
    for (const auto& range : ranges) {
        uint32_t start = range.first;
        uint32_t end = range.second;

        for (uint32_t i = start; i < end && i < source.size(); ++i) {
            if (source[i] != '\n' && source[i] != '\r') {
                source[i] = ' ';
            }
        }
    }

    return true;
}

bool transformCppFile(
    const std::string& inputPath,
    const std::string& outputPath,
    int level,
    std::string& error
) {


    if (level < 1 || level > 3) {
        error = "Invalid obfuscation level.";
        return false;
    }
    std::error_code ec;

    const fs::path inputAbsolute = fs::absolute(inputPath, ec).lexically_normal();

    if (ec) {
        error = "Could not resolve input path.";
        return false;
    }

    const fs::path outputAbsolute = fs::absolute(outputPath, ec).lexically_normal();

    if (ec) {
        error = "Could not resolve output path.";
        return false;
    }

    if (inputAbsolute == outputAbsolute) {
        error = "Input and output paths must be different.";
        return false;
    }

    std::ifstream input(inputAbsolute, std::ios::binary);

    if (!input) {
        error = "Could not open input file for transformation.";
        return false;
    }

    

    std::string source{
        std::istreambuf_iterator<char>(input),
        std::istreambuf_iterator<char>()
    };

    

    if (input.bad()) {
        error = "Failed while reading input file.";
        return false;
    }

    if (source.size() > std::numeric_limits<std::uint32_t>::max()) {
        error = "Input file is too large for this implementation.";
        return false;
    }

    TSParser* parser = ts_parser_new();

    if (parser == nullptr) {
        error = "Could not create parser for transformation.";
        return false;
    }

    if (!ts_parser_set_language(parser, tree_sitter_cpp())) {
        ts_parser_delete(parser);
        error = "Could not load the C++ grammar.";
        return false;
    }

    TSTree* tree = ts_parser_parse_string(
        parser,
        nullptr,
        source.data(),
        static_cast<std::uint32_t>(source.size())
    );

    if (tree == nullptr) {
        ts_parser_delete(parser);
        error = "Could not parse source for transformation.";
        return false;
    }

    TSNode root = ts_tree_root_node(tree);

    if (ts_node_has_error(root)) {
        ts_tree_delete(tree);
        ts_parser_delete(parser);
        error = "Refusing to transform source containing syntax errors.";
        return false;
    }

    std::vector<Candidate> candidates;
    findCandidates(root, source, candidates);

    std::vector<Edit> edits;
    std::size_t renamedCount = 0;
    std::size_t skippedCount = 0;
    std::size_t sequence = 0;

   
for (const Candidate& candidate : candidates) {

    // Find the scope containing this declaration.
    TSNode scope = ts_node_parent(candidate.declaration);

    std::cout << "[DEBUG] Candidate: " << candidate.name << '\n';

    // Lambda parameters belong to the lambda's body scope.
    if (std::string(ts_node_type(candidate.declaration)) ==
        "parameter_declaration") {

        TSNode current = candidate.declaration;
        TSNode lambdaBody = {0};

        while (!ts_node_is_null(current)) {

            if (std::string(ts_node_type(current)) ==
                "lambda_expression") {

                const std::uint32_t count =
                    ts_node_child_count(current);

                for (std::uint32_t i = 0; i < count; ++i) {
                    TSNode child = ts_node_child(current, i);

                    if (std::string(ts_node_type(child)) ==
                        "compound_statement") {
                        lambdaBody = child;
                        break;
                    }
                }

                break;
            }

            current = ts_node_parent(current);
        }

        scope = lambdaBody;
    }

    if (!ts_node_is_null(scope)) {
        std::cout << "[DEBUG] Initial parent scope: "
                << ts_node_type(scope) << '\n';
    }

    if (!ts_node_is_null(scope) &&
        std::string(ts_node_type(scope)) == "init_statement") {

        TSNode conditionClause = ts_node_parent(scope);

        if (!ts_node_is_null(conditionClause) &&
            std::string(ts_node_type(conditionClause)) == "condition_clause") {

            scope = ts_node_parent(conditionClause);
        }
    }

   // Handle declarations inside condition clauses.
    if (!ts_node_is_null(scope) &&
        std::string(ts_node_type(scope)) == "condition_clause") {

        TSNode parentStatement = ts_node_parent(scope);

        if (!ts_node_is_null(parentStatement)) {
            const std::string parentType = ts_node_type(parentStatement);

            if (parentType == "while_statement" ||
                parentType == "if_statement") {

                scope = parentStatement;
            }
        }
    }

    if (!ts_node_is_null(scope)) {
    std::cout << "[DEBUG] Final scope: "
              << ts_node_type(scope) << '\n';
    }

    if (ts_node_is_null(scope)) {
        ++skippedCount;
        continue;
    }

    
    const std::string scopeType = ts_node_type(scope);

    // Accept ordinary block scopes and for-loop scopes.
    if (scopeType != "compound_statement" &&
    scopeType != "for_statement" &&
    scopeType != "if_statement" &&
    scopeType != "while_statement") {
        ++skippedCount;
        continue;
    }

    std::cout << "[DEBUG] Checking accepted scope: "
          << scopeType << '\n';

    std::vector<TSNode> references;
    bool unsafe = false;

    collectReferences(
        scope, scope, candidate, source, references, unsafe
    );

    std::cout << "[DEBUG] Candidate name: "
          << candidate.name << '\n';

    std::cout << "[DEBUG] Reference count: "
          << references.size() << '\n';

    std::cout << "[DEBUG] Unsafe status: "
          << (unsafe ? "true" : "false") << '\n';

    if (unsafe) {
        std::cout << "[DEBUG] Candidate rejected due to unsafe references\n";
        ++skippedCount;
        continue;
    }

    

    std::cout << "[DEBUG] References found: "
          << references.size() << '\n';

    std::cout << "[DEBUG] Unsafe: "
          << (unsafe ? "true" : "false") << '\n';
    const std::string replacement =
        "obf_var_" + std::to_string(++sequence);

    edits.push_back({
        static_cast<std::size_t>(
            ts_node_start_byte(candidate.nameNode)
        ),
        static_cast<std::size_t>(
            ts_node_end_byte(candidate.nameNode)
        ),
        replacement
    });

    for (TSNode reference : references) {
        edits.push_back({
            static_cast<std::size_t>(
                ts_node_start_byte(reference)
            ),
            static_cast<std::size_t>(
                ts_node_end_byte(reference)
            ),
            replacement
        });
    }

    ++renamedCount;
}


    

    // Apply edits right-to-left so earlier byte offsets remain valid.
    std::sort(edits.begin(), edits.end(),
        [](const Edit& a, const Edit& b) {
            return a.start > b.start;
        });

    for (const Edit& edit : edits) {
        source.replace(
            edit.start,
            edit.end - edit.start,
            edit.replacement
        );
    }

    if (level >= 2) {
        if (!removeCommentsFromSource(source, error)) {
            return false;
        }

        std::cout << "[INFO] Comment removal completed.\n";
    }

    // Level 3: transform eligible integer literals.
    if (level >= 3) {
        if (!transformIntegerLiterals(source, error)) {
            std::cerr << "[ERROR] " << error << '\n';
            return false;
        }

        std::cout << "[INFO] Integer literal transformation completed.\n";
    }

    ts_tree_delete(tree);
    ts_parser_delete(parser);

    std::ofstream output(outputAbsolute, std::ios::binary | std::ios::trunc);

    if (!output) {
        error = "Could not open output file for writing.";
        return false;
    }

    output.write(source.data(), static_cast<std::streamsize>(source.size()));

    if (!output) {
        error = "Failed while writing transformed source.";
        return false;
    }

    output.close();

    if (!output) {
        error = "Failed while closing transformed output file.";
        return false;
    }

    std::cout << "[INFO] Local variables renamed: " << renamedCount << '\n';
    std::cout << "[INFO] Variables skipped for safety: " << skippedCount << '\n';
    std::cout << "[INFO] Output written to: " << outputAbsolute.string() << '\n';

    return true;
}