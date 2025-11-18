#include "analysis/relationship_engine.hpp"
#include "assets/image_manager.hpp"

#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <system_error>
#include <vector>

namespace {
std::filesystem::path write_test_image() {
    std::error_code ec;
    std::filesystem::path target_dir = std::filesystem::temp_directory_path(ec);
    if (ec) {
        target_dir = std::filesystem::current_path();
    }
    const std::filesystem::path target = target_dir / "gridfire_analysis.bmp";
    static const uint8_t kBmpData[] = {
        0x42, 0x4D, 0x3A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x36, 0x00, 0x00, 0x00, 0x28, 0x00,
        0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x18, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x13, 0x0B, 0x00, 0x00, 0x13, 0x0B, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00,
    };

    std::ofstream file(target, std::ios::binary);
    file.write(reinterpret_cast<const char*>(kBmpData), sizeof(kBmpData));
    file.close();
    return target;
}

bool contains_relationship(const std::vector<RelationshipEngine::Relationship>& rels,
                           const std::string& type,
                           const std::string& a,
                           const std::string& b) {
    for (const auto& rel : rels) {
        if (rel.type != type) {
            continue;
        }
        const bool forward = rel.file_a == a && rel.file_b == b;
        const bool backward = rel.file_a == b && rel.file_b == a;
        if ((forward || backward) && rel.strength > 0.5) {
            return true;
        }
    }
    return false;
}
}  // namespace

int main() {
    RelationshipEngine engine;
    const std::vector<std::string> files = {
        "/repo/src/fs/filesystem_engine.cpp",
        "/repo/src/fs/filesystem_engine_test.cpp",
        "/repo/src/fs/filesystem_engine.hpp",
        "/repo/src/bar/service.py",
        "/repo/src/bar/service_impl.py",
    };

    const auto relationships = engine.detect(files);
    assert(!relationships.empty());
    assert(contains_relationship(relationships,
                                 "companion",
                                 "/repo/src/fs/filesystem_engine.cpp",
                                 "/repo/src/fs/filesystem_engine.hpp"));
    assert(contains_relationship(relationships,
                                 "validation",
                                 "/repo/src/fs/filesystem_engine.cpp",
                                 "/repo/src/fs/filesystem_engine_test.cpp"));
    assert(contains_relationship(relationships,
                                 "shared-module",
                                 "/repo/src/bar/service.py",
                                 "/repo/src/bar/service_impl.py"));

    ImageManager manager;
    const std::filesystem::path bmp = write_test_image();
    assert(manager.load_image(bmp.string()));
    const auto* data = manager.get(bmp.string());
    assert(data != nullptr);
    assert(data->width == 1);
    assert(data->height == 1);
    assert(data->channels == 4);
    assert(data->pixels.size() == 4);
    assert(manager.load_image(bmp.string()));
    std::filesystem::remove(bmp);

    std::cout << "Analysis tests passed" << std::endl;
    return 0;
}
