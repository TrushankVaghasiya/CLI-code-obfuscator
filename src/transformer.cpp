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
    if (!ts_node_eq(node, rootBody) && type == "compound_statement") {

    if (compoundDeclaresNameInOwnScope(
        node, candidate.name, source)) {
    // This block declares a variable that shadows
    // the current variable.
    return; 
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

                if (std::string(ts_node_type(clauseChild)) != "init_statement") {
                    continue;
                }

                const std::uint32_t initCount =
                    ts_node_child_count(clauseChild);

                for (std::uint32_t k = 0; k < initCount; ++k) {
                    TSNode declaration = ts_node_child(clauseChild, k);

                    Candidate candidate{};

                    if (isSimpleInitializedLocal(
                            declaration, candidate, source)) {
                        candidates.push_back(candidate);
                        std::cout << "[DEBUG] While candidate detected: "
                        << candidate.name << '\n';
                    }
                }
            }
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

    // Handle declarations inside while condition clauses.
    if (!ts_node_is_null(scope) &&
        std::string(ts_node_type(scope)) == "condition_clause") {

        TSNode whileStatement = ts_node_parent(scope);

        if (!ts_node_is_null(whileStatement) &&
            std::string(ts_node_type(whileStatement)) == "while_statement") {

            scope = whileStatement;
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

    std::vector<TSNode> references;
    bool unsafe = false;

    collectReferences(
        scope, scope, candidate, source, references, unsafe
    );

    if (unsafe) {
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