#ifndef SDFCULLING_BRICKS_H
#define SDFCULLING_BRICKS_H

#include <cstdint>
#include <vector>
#include "utils.h"

// CPU-side copy of a .bricks file (see bricks-file-format.md).
// The sample data is not loaded: the brick visualization only needs the
// Morton indices and AABBs.
struct BricksFile {
    uint32_t brick_count = 0;
    float domain_min[3] = {};
    float voxel_size = 0;
    float band = 0;
    std::vector<uint32_t> mortons;  // N
    std::vector<float> aabbs;       // N * 6: minX, minY, minZ, maxX, maxY, maxZ
};

// Returns false (and prints why) if the file is missing or malformed.
bool read_bricks_file(const char* path, BricksFile& bricks);

// Uploads the AABBs to a GPU buffer (render_data.brick_aabb_buffer) and sets
// render_data.brick_count. Replaces any previously uploaded bricks.
void upload_bricks(Init& init, RenderData& render_data, const BricksFile& bricks);

#endif //SDFCULLING_BRICKS_H
