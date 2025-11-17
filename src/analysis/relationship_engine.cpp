#include "analysis/relationship_engine.hpp"

#include <algorithm>

std::vector<RelationshipEngine::Relationship> RelationshipEngine::detect(const std::vector<std::string>& files) const {
    std::vector<Relationship> relationships;
    for (size_t i = 1; i < files.size(); ++i) {
        Relationship rel;
        rel.file_a = files[i - 1];
        rel.file_b = files[i];
        rel.type = "sequence";
        rel.strength = 0.1;
        relationships.push_back(rel);
    }
    return relationships;
}
