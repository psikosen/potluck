#pragma once

#include <string>
#include <vector>

class RelationshipEngine {
public:
    struct Relationship {
        std::string file_a;
        std::string file_b;
        std::string type;
        double strength = 0.0;
    };

    RelationshipEngine() = default;
    std::vector<Relationship> detect(const std::vector<std::string>& files) const;
};
