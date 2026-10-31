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

struct MemberCandidate {
    TSNode classNode;
    TSNode declaration;
    TSNode nameNode;
    std::string className;
    std::string name;
};


struct ObjectVariable {
    std::string className;
    std::string variableName;

    TSNode declaration;
    TSNode nameNode;
    TSNode scope;

    bool isPointer;
    bool isClassObject;
};

struct FunctionCandidate {
    std::string name;
    std::string signature;
    std::vector<TSNode> nameNodes;
    bool overloaded = false;
    bool hasDefinition = false;
};

std::string nodeText(TSNode node, const std::string& source) {
    const std::size_t start = ts_node_start_byte(node);
    const std::size_t end = ts_node_end_byte(node);

    if (start > end || end > source.size()) {
        return {};
    }

    return source.substr(start, end - start);
}

TSNode findFunctionNameNode(TSNode functionDeclarator) {
    const std::uint32_t count =
        ts_node_child_count(functionDeclarator);

    for (std::uint32_t i = 0; i < count; ++i) {
        TSNode child = ts_node_child(functionDeclarator, i);

        if (std::string(ts_node_type(child)) == "identifier") {
            return child;
        }
    }

    return {};
}

TSNode findDirectFunctionDeclarator(TSNode node) {
    const std::uint32_t count = ts_node_child_count(node);

    for (std::uint32_t i = 0; i < count; ++i) {
        TSNode child = ts_node_child(node, i);

        if (std::string(ts_node_type(child)) ==
            "function_declarator") {
            return child;
        }
    }

    return {};
}

std::string getFunctionSignature(
    TSNode functionDeclarator,
    const std::string& source
) {
    TSNode parameterList =
        ts_node_child_by_field_name(
            functionDeclarator,
            "parameters",
            10
        );

    if (ts_node_is_null(parameterList)) {
        return "";
    }

    std::string signature;

    const std::uint32_t count =
        ts_node_child_count(parameterList);

    for (std::uint32_t i = 0; i < count; ++i) {
        TSNode child = ts_node_child(parameterList, i);

        if (std::string(ts_node_type(child)) ==
            "parameter_declaration") {

            // Use the complete parameter declaration.
            // Remove parameter names so prototype and definition
            // signatures can be compared.
            TSNode typeNode = ts_node_child(child, 0);

            if (!ts_node_is_null(typeNode)) {
                signature += nodeText(typeNode, source);
                signature += ";";
            }
        }
    }

    return signature;
}

void findFunctionCandidates(
    TSNode root,
    const std::string& source,
    std::vector<FunctionCandidate>& candidates
) {
    const std::uint32_t count = ts_node_child_count(root);

    for (std::uint32_t i = 0; i < count; ++i) {
        TSNode child = ts_node_child(root, i);
        const std::string type = ts_node_type(child);

        TSNode functionDeclarator = {};

        if (type == "function_definition") {
            functionDeclarator =
                findDirectFunctionDeclarator(child);
        }
        else if (type == "declaration") {
            functionDeclarator =
                findDirectFunctionDeclarator(child);
        }
        else {
            continue;
        }

        if (ts_node_is_null(functionDeclarator)) {
            continue;
        }

        TSNode nameNode =
            findFunctionNameNode(functionDeclarator);

        if (ts_node_is_null(nameNode)) {
            continue;
        }

        const std::string name = nodeText(nameNode, source);

        if (name == "main") {
            continue;
        }

        const std::string signature =
            getFunctionSignature(functionDeclarator, source);

        auto existing = std::find_if(
            candidates.begin(),
            candidates.end(),
            [&](const FunctionCandidate& candidate) {
                return candidate.name == name;
            }
        );

        if (existing == candidates.end()) {
            FunctionCandidate candidate;

            candidate.name = name;
            candidate.signature = signature;
            candidate.nameNodes.push_back(nameNode);
            candidate.hasDefinition = (type == "function_definition");

            candidates.push_back(candidate);

            
        }
        else {
            if (existing->signature != signature) {
                existing->overloaded = true;
            }

            if (type == "function_definition") {
                existing->hasDefinition = true;
            }

            existing->nameNodes.push_back(nameNode);
        }
    }
}

void collectFunctionCalls(
    TSNode node,
    const std::string& source,
    const std::string& functionName,
    std::vector<TSNode>& references
) {
    if (std::string(ts_node_type(node)) == "call_expression") {

        TSNode functionNode =
            ts_node_child_by_field_name(node, "function", 8);

        if (!ts_node_is_null(functionNode) &&
            std::string(ts_node_type(functionNode)) == "identifier") {

            if (nodeText(functionNode, source) == functionName) {
                references.push_back(functionNode);
            }
        }
    }

    const std::uint32_t count = ts_node_child_count(node);

    for (std::uint32_t i = 0; i < count; ++i) {
        collectFunctionCalls(
            ts_node_child(node, i),
            source,
            functionName,
            references
        );
    }
}

TSNode findDeclaratorIdentifier(TSNode node);

bool hasPotentialFunctionShadowing(
    TSNode node,
    const std::string& source,
    const std::string& functionName
) {
    const std::string nodeType = ts_node_type(node);

    // Check variable and parameter declarations.
    if (nodeType == "init_declarator" ||
        nodeType == "parameter_declaration") {

        TSNode nameNode = findDeclaratorIdentifier(node);

        if (!ts_node_is_null(nameNode) &&
            nodeText(nameNode, source) == functionName) {

            return true;
        }
    }

    const std::uint32_t count = ts_node_child_count(node);

    for (std::uint32_t i = 0; i < count; ++i) {
        if (hasPotentialFunctionShadowing(
                ts_node_child(node, i),
                source,
                functionName)) {
            return true;
        }
    }

    return false;
}

TSNode findDeclaratorIdentifier(TSNode node) {
    if (ts_node_is_null(node)) {
        return {};
    }

    const std::string type = ts_node_type(node);

    if (type == "identifier") {
        return node;
    }

    if (type != "pointer_declarator" &&
    type != "reference_declarator" &&
    type != "array_declarator" &&
    type != "parenthesized_declarator" &&
    type != "attributed_declarator" &&
    type != "init_declarator") {
        return {};
    }

    const std::uint32_t count = ts_node_child_count(node);

    for (std::uint32_t i = 0; i < count; ++i) {
        TSNode result = findDeclaratorIdentifier(
            ts_node_child(node, i)
        );

        if (!ts_node_is_null(result)) {
            return result;
        }
    }

    return {};
}

TSNode findDeclarationScope(TSNode node) {

    TSNode current = ts_node_parent(node);

    while (!ts_node_is_null(current)) {

        const std::string type = ts_node_type(current);

        if (type == "compound_statement" ||
            type == "for_statement" ||
            type == "if_statement" ||
            type == "while_statement" ||
            type == "translation_unit") {
            return current;
        }

        current = ts_node_parent(current);
    }

    return {};
}

bool containsPointerDeclarator(TSNode node) {

    if (ts_node_is_null(node)) {
        return false;
    }

    if (std::string(ts_node_type(node)) == "pointer_declarator") {
        return true;
    }

    const std::uint32_t count = ts_node_child_count(node);

    for (std::uint32_t i = 0; i < count; ++i) {

        if (containsPointerDeclarator(ts_node_child(node, i))) {
            return true;
        }
    }

    return false;
}

const ObjectVariable* resolveObjectVariable(
    TSNode accessNode,
    const std::string& variableName,
    const std::vector<ObjectVariable>& objects
) {
    TSNode current = accessNode;
    const std::size_t accessPosition =
        ts_node_start_byte(accessNode);

    while (!ts_node_is_null(current)) {

        const std::string scopeType = ts_node_type(current);

        if (scopeType == "compound_statement" ||
            scopeType == "for_statement" ||
            scopeType == "if_statement" ||
            scopeType == "while_statement" ||
            scopeType == "translation_unit") {

            const ObjectVariable* nearest = nullptr;
            std::size_t nearestPosition = 0;

            for (const ObjectVariable& object : objects) {

                if (object.variableName != variableName ||
                    !ts_node_eq(object.scope, current)) {
                    continue;
                }

                const std::size_t declarationPosition =
                    ts_node_start_byte(object.declaration);

                // A declaration is visible from its declaration
                // point onward, not before it.
                if (declarationPosition >= accessPosition) {
                    continue;
                }

                if (nearest == nullptr ||
                    declarationPosition > nearestPosition) {

                    nearest = &object;
                    nearestPosition = declarationPosition;
                }
            }

            // A declaration in this scope shadows declarations
            // in every enclosing scope.
            if (nearest != nullptr) {
                return nearest;
            }
        }

        current = ts_node_parent(current);
    }

    return nullptr;
}

void findObjectParameters(
    TSNode node,
    const std::string& source,
    std::vector<ObjectVariable>& objects
) {
    const std::string nodeType = ts_node_type(node);

    if (nodeType == "parameter_declaration") {
        TSNode typeNode =
            ts_node_child_by_field_name(node, "type", 4);

        TSNode declaratorNode =
            ts_node_child_by_field_name(node, "declarator", 10);

        if (!ts_node_is_null(typeNode) &&
            !ts_node_is_null(declaratorNode) &&
            std::string(ts_node_type(typeNode)) == "type_identifier") {

            const std::string declaratorType =
                ts_node_type(declaratorNode);

            TSNode nameNode = findDeclaratorIdentifier(declaratorNode);

            const bool isSupportedDeclarator =
                declaratorType == "identifier" ||
                declaratorType == "pointer_declarator" ||
                declaratorType == "reference_declarator";

            if (!ts_node_is_null(nameNode) && isSupportedDeclarator) {
                const std::string nameType = ts_node_type(nameNode);

                if (nameType == "identifier") {
                    TSNode functionNode = ts_node_parent(node);

                    while (!ts_node_is_null(functionNode)) {

                        const std::string parentType =
                            ts_node_type(functionNode);

                        if (parentType == "function_definition" ||
                            parentType == "lambda_expression") {
                            break;
                        }

                        functionNode = ts_node_parent(functionNode);
                    }

                    if (!ts_node_is_null(functionNode)) {
                        TSNode bodyNode = {};

                        const std::uint32_t functionChildCount =
                            ts_node_child_count(functionNode);

                        for (std::uint32_t i = 0; i < functionChildCount; ++i) {

                            TSNode child = ts_node_child(functionNode, i);

                            if (std::string(ts_node_type(child)) ==
                                "compound_statement") {
                                bodyNode = child;
                                break;
                            }
                        }

                        if (!ts_node_is_null(bodyNode) &&
                            std::string(ts_node_type(bodyNode)) ==
                                "compound_statement") {

                            ObjectVariable object;
                            object.className = nodeText(typeNode, source);
                            object.variableName = nodeText(nameNode, source);
                            object.declaration = node;
                            object.nameNode = nameNode;
                            object.scope = bodyNode;
                            object.isPointer =
                                declaratorType == "pointer_declarator";
                            object.isClassObject = true;

                            objects.push_back(object);
                        }
                    }
                }
            }
        }
    }

    const std::uint32_t count = ts_node_child_count(node);

    for (std::uint32_t i = 0; i < count; ++i) {
        findObjectParameters(
            ts_node_child(node, i),
            source,
            objects
        );
    }
}


void findObjectDeclarations(
    TSNode node,
    const std::string& source,
    std::vector<ObjectVariable>& objects
) {
    if (std::string(ts_node_type(node)) == "declaration") {

        TSNode typeNode = {};
        TSNode nameNode = {};
        TSNode declaratorNode = {};

        const std::uint32_t count = ts_node_child_count(node);

        for (std::uint32_t i = 0; i < count; ++i) {

            TSNode child = ts_node_child(node, i);
            const std::string childType = ts_node_type(child);

            if (childType == "type_identifier" ||
                childType == "primitive_type") {
                typeNode = child;
            }

            if (childType == "identifier") {
                nameNode = child;
            }

            if (childType == "init_declarator") {

                TSNode declarator =
                    ts_node_child_by_field_name(
                        child, "declarator", 10
                    );

                if (!ts_node_is_null(declarator)) {
                    declaratorNode = declarator;
                    nameNode = findDeclaratorIdentifier(declarator);
                }
            }
        }

        if (!ts_node_is_null(typeNode) &&
            !ts_node_is_null(nameNode)) {

            const std::string typeName = nodeText(typeNode, source);

            ObjectVariable object;

            object.className =
                std::string(ts_node_type(typeNode)) == "type_identifier"
                    ? typeName
                    : "";

            object.variableName = nodeText(nameNode, source);
            object.declaration = node;
            object.nameNode = nameNode;
            object.scope = findDeclarationScope(node);
            object.isPointer =
                containsPointerDeclarator(declaratorNode);
            object.isClassObject = !object.className.empty();

            if (!ts_node_is_null(object.scope)) {

                objects.push_back(object);

                
            }
        }
    }

    const std::uint32_t count = ts_node_child_count(node);

    for (std::uint32_t i = 0; i < count; ++i) {
        findObjectDeclarations(
            ts_node_child(node, i),
            source,
            objects
        );
    }
}

void findMemberCandidates(
    TSNode node,
    const std::string& source,
    std::vector<MemberCandidate>& candidates
) {
    const std::string nodeType = ts_node_type(node);

    if (nodeType == "class_specifier") {

        TSNode classNameNode = {0};
        TSNode memberList = {0};

        const std::uint32_t count = ts_node_child_count(node);

        for (std::uint32_t i = 0; i < count; ++i) {

            TSNode child = ts_node_child(node, i);
            const std::string childType = ts_node_type(child);

            if (childType == "type_identifier") {
                classNameNode = child;
            }

            if (childType == "field_declaration_list") {
                memberList = child;
            }
        }

        if (!ts_node_is_null(classNameNode) &&
            !ts_node_is_null(memberList)) {

            const std::string className(
                source.begin() + ts_node_start_byte(classNameNode),
                source.begin() + ts_node_end_byte(classNameNode)
            );

            const std::uint32_t memberCount =
                ts_node_child_count(memberList);

            for (std::uint32_t i = 0; i < memberCount; ++i) {

                TSNode member = ts_node_child(memberList, i);

                if (std::string(ts_node_type(member)) !=
                    "field_declaration") {
                    continue;
                }

                const std::uint32_t declarationCount =
                    ts_node_child_count(member);

                for (std::uint32_t j = 0; j < declarationCount; ++j) {

                    TSNode child = ts_node_child(member, j);

                    if (std::string(ts_node_type(child)) !=
                        "field_identifier") {
                        continue;
                    }

                    std::string memberName(
                        source.begin() + ts_node_start_byte(child),
                        source.begin() + ts_node_end_byte(child)
                    );

                    candidates.push_back({
                        node,
                        member,
                        child,
                        className,
                        memberName
                    });

                   
                }
            }
        }
    }

    const std::uint32_t count = ts_node_child_count(node);

    for (std::uint32_t i = 0; i < count; ++i) {
        findMemberCandidates(
            ts_node_child(node, i),
            source,
            candidates
        );
    }
}

bool isSimpleInitializedLocal(
    TSNode declaration,
    Candidate& result,
    const std::string& source
) {
    if (std::string(ts_node_type(declaration)) != "declaration") {
        return false;
    }

    bool hasSupportedType = false;
    TSNode initDeclarator = {};
    TSNode directIdentifier = {};

    int initDeclaratorCount = 0;
    int directIdentifierCount = 0;

    bool seenEquals = false;

    const std::uint32_t count = ts_node_child_count(declaration);

    for (std::uint32_t i = 0; i < count; ++i) {
        TSNode child = ts_node_child(declaration, i);
        const std::string type = ts_node_type(child);

        if (type == "primitive_type" || type == "type_identifier") {
            hasSupportedType = true;
        }

        if (type == "init_declarator") {
            initDeclarator = child;
            ++initDeclaratorCount;
        }

        if (type == "=") {
            seenEquals = true;
        }

        // In condition declarations, the declared name appears
        // directly before '='. Ignore identifiers in the initializer.
        if (type == "identifier" && !seenEquals) {
            directIdentifier = child;
            ++directIdentifierCount;
        }
    }

    if (!hasSupportedType) {
        return false;
    }

    TSNode nameNode = {};

    if (initDeclaratorCount == 1 && directIdentifierCount == 0) {
        // Ordinary initialized declaration.
        TSNode declarator =
            ts_node_child_by_field_name(
                initDeclarator, "declarator", 10
            );

        nameNode = findDeclaratorIdentifier(declarator);
    }
    else if (initDeclaratorCount == 0 &&
             directIdentifierCount == 1) {
        // Condition declarations and declarations without
        // an init_declarator.
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

void collectMultipleDeclarators(
    TSNode declaration,
    std::vector<Candidate>& candidates,
    const std::string& source
) {
    if (std::string(ts_node_type(declaration)) != "declaration") {
        return;
    }

    bool hasSupportedType = false;
    bool addedCandidate = false;

    const std::uint32_t count = ts_node_child_count(declaration);

    for (std::uint32_t i = 0; i < count; ++i) {
        TSNode child = ts_node_child(declaration, i);
        std::string type = ts_node_type(child);

        if (type == "primitive_type" || type == "type_identifier") {
            hasSupportedType = true;
            break;
        }
    }

    if (!hasSupportedType) {
        return;
    }

    for (std::uint32_t i = 0; i < count; ++i) {
        TSNode child = ts_node_child(declaration, i);

        if (std::string(ts_node_type(child)) != "init_declarator") {
            continue;
        }

        TSNode declarator =
            ts_node_child_by_field_name(child, "declarator", 10);

        TSNode nameNode = findDeclaratorIdentifier(declarator);

        if (ts_node_is_null(nameNode)) {
            continue;
        }

        Candidate candidate{};
        candidate.declaration = declaration;
        candidate.nameNode = nameNode;
        candidate.name = nodeText(nameNode, source);

        if (!candidate.name.empty()) {
            candidates.push_back(candidate);
            addedCandidate = true;
        }
    }

    // Preserve support for declarations without init_declarator,
    // including while condition declarations.
    if (!addedCandidate) {
        Candidate candidate{};

        if (isSimpleInitializedLocal(declaration, candidate, source)) {
            candidates.push_back(candidate);
        }
    }
}


void collectThisMemberReferences(
    TSNode node,
    const MemberCandidate& candidate,
    const std::string& source,
    std::vector<TSNode>& references
) {
    const std::string type = ts_node_type(node);

    // Do not enter nested classes.
    if (type == "class_specifier" &&
        !ts_node_eq(node, candidate.classNode)) {
        return;
    }

    if (type == "field_expression") {

        bool hasThis = false;
        bool hasArrow = false;
        TSNode memberNode = {0};

        const std::uint32_t count = ts_node_child_count(node);

        for (std::uint32_t i = 0; i < count; ++i) {

            TSNode child = ts_node_child(node, i);
            const std::string childType = ts_node_type(child);
            const std::string childText = nodeText(child, source);

            if (childType == "this" && childText == "this") {
                hasThis = true;
            }

            if (childText == "->") {
                hasArrow = true;
            }

            if (childType == "field_identifier") {
                memberNode = child;
            }
        }

        if (hasThis &&
            hasArrow &&
            !ts_node_is_null(memberNode) &&
            nodeText(memberNode, source) == candidate.name) {

            references.push_back(memberNode);
            return;
        }
    }

    const std::uint32_t count = ts_node_child_count(node);

    for (std::uint32_t i = 0; i < count; ++i) {
        collectThisMemberReferences(
            ts_node_child(node, i),
            candidate,
            source,
            references
        );
    }
}

bool lambdaDeclaresNameInParameters(
    TSNode lambda,
    const std::string& name,
    const std::string& source
);

bool functionDeclaresNameInParameters(
    TSNode function,
    const std::string& name,
    const std::string& source
);

bool compoundDeclaresNameInOwnScope(
    TSNode compound,
    const std::string& name,
    const std::string& source
);

bool statementDeclaresNameInCondition(
    TSNode statement,
    const std::string& name,
    const std::string& source
);

bool forStatementDeclaresName(
    TSNode statement,
    const std::string& name,
    const std::string& source
);

bool isAnotherDeclarationName(TSNode node);

void collectImplicitMemberReferences(
    TSNode node,
    const MemberCandidate& candidate,
    const std::string& source,
    std::vector<TSNode>& references
) {
    const std::string type = ts_node_type(node);

    // Do not enter nested classes.
    if (type == "class_specifier" &&
        !ts_node_eq(node, candidate.classNode)) {
        return;
    }

    // A lambda parameter can shadow the member.
    if (type == "lambda_expression" &&
        lambdaDeclaresNameInParameters(
            node, candidate.name, source)) {
        return;
    }

    // A declaration in this block can shadow the member.
    if (type == "compound_statement" &&
        compoundDeclaresNameInOwnScope(
            node, candidate.name, source)) {
        return;
    }

    // Handle if/while condition declarations.
    if ((type == "if_statement" || type == "while_statement") &&
        statementDeclaresNameInCondition(
            node, candidate.name, source)) {
        return;
    }

    // Handle for-loop declarations.
    if (type == "for_statement" &&
        forStatementDeclaresName(
            node, candidate.name, source)) {
        return;
    }

    // Collect only implicit identifier references.
    if (type == "identifier" &&
        nodeText(node, source) == candidate.name &&
        !isAnotherDeclarationName(node)) {

        TSNode parent = ts_node_parent(node);

        // An identifier used as the object in a field expression
        // is not an implicit member reference.
        if (!ts_node_is_null(parent) &&
            std::string(ts_node_type(parent)) != "field_expression") {
            references.push_back(node);
        }
    }

    const std::uint32_t count = ts_node_child_count(node);

    for (std::uint32_t i = 0; i < count; ++i) {
        collectImplicitMemberReferences(
            ts_node_child(node, i),
            candidate,
            source,
            references
        );
    }
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

void findFunctionParameters(
    TSNode function,
    const std::string& source,
    std::vector<Candidate>& candidates
) {
    const std::uint32_t count = ts_node_child_count(function);

    for (std::uint32_t i = 0; i < count; ++i) {
        TSNode child = ts_node_child(function, i);

        if (std::string(ts_node_type(child)) !=
            "function_declarator") {
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

bool functionDeclaresNameInParameters(
    TSNode function,
    const std::string& name,
    const std::string& source
) {
    const std::uint32_t count = ts_node_child_count(function);

    for (std::uint32_t i = 0; i < count; ++i) {
        TSNode child = ts_node_child(function, i);

        if (std::string(ts_node_type(child)) !=
            "function_declarator") {
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

void collectMemberReferences(
    const MemberCandidate& candidate,
    const std::string& source,
    std::vector<TSNode>& references
) {
    TSNode classNode = candidate.classNode;

    const std::uint32_t classCount =
        ts_node_child_count(classNode);

    TSNode memberList = {0};

    for (std::uint32_t i = 0; i < classCount; ++i) {

        TSNode child = ts_node_child(classNode, i);

        if (std::string(ts_node_type(child)) ==
            "field_declaration_list") {
            memberList = child;
            break;
        }
    }

    if (ts_node_is_null(memberList)) {
        return;
    }

    const std::uint32_t memberCount =
        ts_node_child_count(memberList);

    for (std::uint32_t i = 0; i < memberCount; ++i) {

        TSNode member = ts_node_child(memberList, i);

        if (std::string(ts_node_type(member)) !=
            "function_definition") {
            continue;
        }

        TSNode body = {0};

        const std::uint32_t functionCount =
            ts_node_child_count(member);

        for (std::uint32_t j = 0; j < functionCount; ++j) {

            TSNode child = ts_node_child(member, j);

            if (std::string(ts_node_type(child)) ==
                "compound_statement") {
                body = child;
                break;
            }
        }

        if (ts_node_is_null(body)) {
            continue;
        }

        // Explicit this->member access is independent of
        // local-variable and parameter shadowing.
        collectThisMemberReferences(
            body,
            candidate,
            source,
            references
        );

        // If a function parameter has the same name,
        // implicit references in that function are shadowed.
        if (functionDeclaresNameInParameters(
                member, candidate.name, source)) {
            continue;
        }

        collectImplicitMemberReferences(
            body,
            candidate,
            source,
            references
        );
    }
}

void collectObjectMemberReferences(
    TSNode node,
    const MemberCandidate& candidate,
    const std::vector<ObjectVariable>& objects,
    const std::string& source,
    std::vector<TSNode>& references
) {
    const std::string nodeType = ts_node_type(node);

    if (nodeType == "field_expression") {

        TSNode objectNode = {};
        TSNode operatorNode = {};
        TSNode memberNode = {};

        const std::uint32_t count = ts_node_child_count(node);

        for (std::uint32_t i = 0; i < count; ++i) {

            TSNode child = ts_node_child(node, i);
            const std::string childType = ts_node_type(child);

            if (childType == "identifier" ||
                childType == "field_expression") {
                if (ts_node_is_null(objectNode)) {
                    objectNode = child;
                }
            }

            if (childType == "." || childType == "->") {
                operatorNode = child;
            }

            if (childType == "field_identifier") {
                memberNode = child;
            }
        }

        if (!ts_node_is_null(objectNode) &&
            !ts_node_is_null(operatorNode) &&
            !ts_node_is_null(memberNode) &&
            nodeText(memberNode, source) == candidate.name &&
            std::string(ts_node_type(objectNode)) == "identifier") {

            const std::string objectName =
                nodeText(objectNode, source);

            const ObjectVariable* object =
                resolveObjectVariable(
                    node,
                    objectName,
                    objects
                );

            if (object != nullptr &&
                object->isClassObject &&
                object->className == candidate.className) {

                const std::string op =
                    nodeText(operatorNode, source);

                if ((op == "." && !object->isPointer) ||
                    (op == "->" && object->isPointer)) {

                    references.push_back(memberNode);
                }
            }
        }
    }

    const std::uint32_t count = ts_node_child_count(node);

    for (std::uint32_t i = 0; i < count; ++i) {
        collectObjectMemberReferences(
            ts_node_child(node, i),
            candidate,
            objects,
            source,
            references
        );
    }
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

        // A nested function parameter with the same name
        // shadows the variable from the outer scope.
        if (type == "function_definition" &&
            functionDeclaresNameInParameters(
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

    if (nodeType == "function_definition") {
        findFunctionParameters(node, source, candidates);
    }

    if (nodeType == "lambda_expression") {
        findLambdaParameters(node, source, candidates);
    }

    // Handle declarations inside for statements.
    if (nodeType == "for_statement") {
        const std::uint32_t count = ts_node_child_count(node);

        for (std::uint32_t i = 0; i < count; ++i) {
            TSNode child = ts_node_child(node, i);

            if (std::string(ts_node_type(child)) == "init_statement") {
                const std::uint32_t initCount =
                    ts_node_child_count(child);

                for (std::uint32_t j = 0; j < initCount; ++j) {
                    collectMultipleDeclarators(
                        ts_node_child(child, j), candidates, source
                    );
                }
            } else {
                collectMultipleDeclarators(child, candidates, source);
            }
        }
    }

    // Handle declarations inside if statements.
    if (nodeType == "if_statement") {
        const std::uint32_t count = ts_node_child_count(node);

        for (std::uint32_t i = 0; i < count; ++i) {
            TSNode child = ts_node_child(node, i);
            const std::string childType = ts_node_type(child);

            // Handle condition_clause.
            if (childType == "condition_clause") {
                const std::uint32_t clauseCount =
                    ts_node_child_count(child);

                for (std::uint32_t j = 0; j < clauseCount; ++j) {
                    TSNode clauseChild = ts_node_child(child, j);
                    const std::string clauseType =
                        ts_node_type(clauseChild);

                    if (clauseType == "declaration") {
                        collectMultipleDeclarators(
                            clauseChild, candidates, source
                        );
                    }

                    if (clauseType == "init_statement") {
                        const std::uint32_t initCount =
                            ts_node_child_count(clauseChild);

                        for (std::uint32_t k = 0; k < initCount; ++k) {
                            collectMultipleDeclarators(
                                ts_node_child(clauseChild, k),
                                candidates,
                                source
                            );
                        }
                    }
                }
            }

            // Also handle init_statement if it is a direct child.
            if (childType == "init_statement") {
                const std::uint32_t initCount =
                    ts_node_child_count(child);

                for (std::uint32_t j = 0; j < initCount; ++j) {
                    collectMultipleDeclarators(
                        ts_node_child(child, j), candidates, source
                    );
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

                collectMultipleDeclarators(
                    declaration, candidates, source
                );
            }

            break;
        }
    }

    // Handle declarations directly inside compound statements.
    if (nodeType == "compound_statement") {
        const std::uint32_t count = ts_node_child_count(node);

        for (std::uint32_t i = 0; i < count; ++i) {
            TSNode child = ts_node_child(node, i);

            collectMultipleDeclarators(child, candidates, source);
        }
    }

    // Recursively inspect nested scopes and syntax nodes.
    const std::uint32_t count = ts_node_child_count(node);

    for (std::uint32_t i = 0; i < count; ++i) {
        findCandidates(
            ts_node_child(node, i), source, candidates
        );
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

    std::vector<MemberCandidate> memberCandidates;

    findMemberCandidates(
        root,
        source,
        memberCandidates
    );

    std::vector<FunctionCandidate> functionCandidates;

    findFunctionCandidates(
        root,
        source,
        functionCandidates
    );

        for (const FunctionCandidate& candidate : functionCandidates) {

        std::vector<TSNode> references;

        collectFunctionCalls(
            root,
            source,
            candidate.name,
            references
        );

        
    }

    std::vector<ObjectVariable> objectVariables;

    findObjectDeclarations(
        root,
        source,
        objectVariables
    );

    findObjectParameters(root, source, objectVariables);



    std::cout << "\n[DEBUG] Discovered object variables:\n";

    for (const ObjectVariable& object : objectVariables) {
       
    }

    // Collect references to discovered class members.
    // This stage only reports references; it does not rename them.
    // Store member candidates and their references for the editing stage.
    struct MemberReferenceGroup {
        const MemberCandidate* candidate;
        std::vector<TSNode> references;
    };

    std::vector<MemberReferenceGroup> memberReferenceGroups;

    for (const MemberCandidate& candidate : memberCandidates) {

        std::vector<TSNode> references;

        collectMemberReferences(
            candidate,
            source,
            references
        );

        collectObjectMemberReferences(
            root,
            candidate,
            objectVariables,
            source,
            references
        );

        

        for (TSNode reference : references) {

            TSNode parent = ts_node_parent(reference);

            const std::string referenceType =
                !ts_node_is_null(parent) &&
                std::string(ts_node_type(parent)) == "field_expression"
                    ? "this"
                    : "implicit";

           
        }

        memberReferenceGroups.push_back({
            &candidate,
            references
        });
    }

    std::vector<Edit> edits;
    std::size_t renamedCount = 0;
    std::size_t skippedCount = 0;
    std::size_t sequence = 0;
    std::size_t functionSequence = 0;
    std::size_t functionsRenamed = 0;
    std::size_t functionsSkipped = 0;

   
for (const Candidate& candidate : candidates) {

    // Find the scope containing this declaration.
    TSNode scope = ts_node_parent(candidate.declaration);

   

    // Lambda parameters belong to the lambda's body scope.
    // Resolve the body scope for lambda and function parameters.
    if (std::string(ts_node_type(candidate.declaration)) ==
        "parameter_declaration") {

        TSNode current = candidate.declaration;
        TSNode parameterScope = {0};

        while (!ts_node_is_null(current)) {

            const std::string currentType = ts_node_type(current);

            if (currentType == "lambda_expression") {

                const std::uint32_t count =
                    ts_node_child_count(current);

                for (std::uint32_t i = 0; i < count; ++i) {
                    TSNode child = ts_node_child(current, i);

                    if (std::string(ts_node_type(child)) ==
                        "compound_statement") {
                        parameterScope = child;
                        break;
                    }
                }

                break;
            }

            if (currentType == "function_definition") {

                const std::uint32_t count =
                    ts_node_child_count(current);

                for (std::uint32_t i = 0; i < count; ++i) {
                    TSNode child = ts_node_child(current, i);

                    if (std::string(ts_node_type(child)) ==
                        "compound_statement") {
                        parameterScope = child;
                        break;
                    }
                }

                break;
            }

            current = ts_node_parent(current);
        }

        scope = parameterScope;
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

// --------------------------------------------------
// Class/struct data-member renaming
// --------------------------------------------------

std::size_t memberSequence = 0;
std::size_t renamedMemberCount = 0;

for (const MemberReferenceGroup& group : memberReferenceGroups) {

    const MemberCandidate& candidate = *group.candidate;

    const std::string replacement =
        "obf_member_" + std::to_string(++memberSequence);

    // Rename the member declaration.
    edits.push_back({
        static_cast<std::size_t>(
            ts_node_start_byte(candidate.nameNode)
        ),
        static_cast<std::size_t>(
            ts_node_end_byte(candidate.nameNode)
        ),
        replacement
    });

    // Rename all collected references.
    for (TSNode reference : group.references) {

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

    ++renamedMemberCount;

    
}



for (const FunctionCandidate& candidate : functionCandidates) {

    if (candidate.overloaded) {
        ++functionsSkipped;

        std::cout << "[INFO] Function skipped (overloaded): "
                << candidate.name << '\n';

        continue;
    }

    if (!candidate.hasDefinition) {
        ++functionsSkipped;

        std::cout << "[INFO] Function skipped (no definition): "
                << candidate.name << '\n';

        continue;
    }


    if (hasPotentialFunctionShadowing(
        root,
        source,
        candidate.name)) {

        ++functionsSkipped;

        std::cout << "[INFO] Function skipped (potential shadowing): "
                << candidate.name << '\n';

        continue;
    }

    std::vector<TSNode> references;

    collectFunctionCalls(
        root,
        source,
        candidate.name,
        references
    );

    // Generate one replacement name for this function.
    ++functionSequence;

    const std::string newName =
        "obf_func_" + std::to_string(functionSequence);

    // Rename every prototype and definition.
    for (TSNode nameNode : candidate.nameNodes) {
        edits.push_back({
            ts_node_start_byte(nameNode),
            ts_node_end_byte(nameNode),
            newName
        });
    }

    // Rename all direct calls, including recursive calls.
    for (TSNode reference : references) {
        edits.push_back({
            ts_node_start_byte(reference),
            ts_node_end_byte(reference),
            newName
        });
    }

    ++functionsRenamed;

    std::cout << "[INFO] Function renamed: "
              << candidate.name << " -> "
              << newName << '\n';
}

    std::cout << "[INFO] Class members renamed: "
          << renamedMemberCount << '\n';

          std::cout << "[INFO] Functions renamed: "
          << functionsRenamed << '\n';

    std::cout << "[INFO] Functions skipped: "
          << functionsSkipped << '\n';
    

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