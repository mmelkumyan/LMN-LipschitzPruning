// Generates a synthetic .bricks file (see bricks-file-format.md) by baking an
// analytic sphere. Lets the brick visualization (Track C) and the RT engine
// (Track B) work before the real bake (Track A) exists.
//
// Build:  g++ -std=c++17 -O2 tools/gen_synthetic_bricks.cpp -o gen_synthetic_bricks
//     or: cl /std:c++17 /O2 /EHsc tools\gen_synthetic_bricks.cpp
// Usage:  gen_synthetic_bricks [out.bricks] [radius] [domainHalfExtent]
//
// Per the M1 decision, aabb[i] is the brick's FULL box (no tight boxes yet).

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>

static const int GRID_BRICKS = 64;           // bricks per axis
static const int BRICK_VOXELS = 8;           // voxels per brick axis
static const int SAMPLES_PER_AXIS = 9;       // BRICK_VOXELS + 1
static const int SAMPLES_PER_BRICK = 729;    // 9^3

// Morton encode: identical to the Lipschitz repo (x in bit 0, y in bit 1, z in bit 2).
static uint32_t part1By2(uint32_t x) {
    x &= 0x000003ff;
    x = (x ^ (x << 16)) & 0xff0000ff;
    x = (x ^ (x << 8)) & 0x0300f00f;
    x = (x ^ (x << 4)) & 0x030c30c3;
    x = (x ^ (x << 2)) & 0x09249249;
    return x;
}
static uint32_t mortonEncode(uint32_t x, uint32_t y, uint32_t z) {
    return (part1By2(z) << 2) | (part1By2(y) << 1) | part1By2(x);
}

int main(int argc, char** argv) {
    const char* outPath = argc > 1 ? argv[1] : "sphere.bricks";
    const float radius = argc > 2 ? (float)atof(argv[2]) : 0.5f;
    const float half = argc > 3 ? (float)atof(argv[3]) : 1.0f;   // domain = [-half, half]^3

    const float domainMin[3] = { -half, -half, -half };
    const int totalVoxels = GRID_BRICKS * BRICK_VOXELS;          // 512
    const float voxelSize = (2.0f * half) / (float)totalVoxels;
    const float voxelDiag = voxelSize * std::sqrt(3.0f);
    const float band = 2.0f * voxelDiag;                         // dequantization scale
    const float keepThreshold = 0.5f * voxelDiag;

    std::vector<uint32_t> mortons;
    std::vector<float> aabbs;
    std::vector<int8_t> samples;

    for (int bz = 0; bz < GRID_BRICKS; bz++)
    for (int by = 0; by < GRID_BRICKS; by++)
    for (int bx = 0; bx < GRID_BRICKS; bx++) {
        int8_t q[SAMPLES_PER_BRICK];
        bool keep = false;

        for (int sz = 0; sz < SAMPLES_PER_AXIS; sz++)
        for (int sy = 0; sy < SAMPLES_PER_AXIS; sy++)
        for (int sx = 0; sx < SAMPLES_PER_AXIS; sx++) {
            // Position from the GLOBAL sample index so samples shared between
            // neighboring bricks are bit-identical (no cracks).
            int gx = bx * BRICK_VOXELS + sx;
            int gy = by * BRICK_VOXELS + sy;
            int gz = bz * BRICK_VOXELS + sz;
            float px = domainMin[0] + gx * voxelSize;
            float py = domainMin[1] + gy * voxelSize;
            float pz = domainMin[2] + gz * voxelSize;

            float d = std::sqrt(px * px + py * py + pz * pz) - radius;
            if (std::fabs(d) < keepThreshold) keep = true;

            float n = std::min(1.0f, std::max(-1.0f, d / band));
            q[(sz * SAMPLES_PER_AXIS + sy) * SAMPLES_PER_AXIS + sx] = (int8_t)std::lround(n * 127.0f);
        }

        if (!keep) continue;

        mortons.push_back(mortonEncode(bx, by, bz));
        float ox = domainMin[0] + bx * BRICK_VOXELS * voxelSize;
        float oy = domainMin[1] + by * BRICK_VOXELS * voxelSize;
        float oz = domainMin[2] + bz * BRICK_VOXELS * voxelSize;
        float ext = BRICK_VOXELS * voxelSize;
        const float box[6] = { ox, oy, oz, ox + ext, oy + ext, oz + ext };
        aabbs.insert(aabbs.end(), box, box + 6);
        samples.insert(samples.end(), q, q + SAMPLES_PER_BRICK);
    }

    uint32_t brickCount = (uint32_t)mortons.size();

    FILE* f = fopen(outPath, "wb");
    if (!f) {
        fprintf(stderr, "failed to open %s for writing\n", outPath);
        return 1;
    }
    fwrite(&brickCount, 4, 1, f);
    fwrite(domainMin, 4, 3, f);
    fwrite(&voxelSize, 4, 1, f);
    fwrite(&band, 4, 1, f);
    fwrite(mortons.data(), 4, brickCount, f);
    fwrite(aabbs.data(), 4, (size_t)brickCount * 6, f);
    fwrite(samples.data(), 1, (size_t)brickCount * SAMPLES_PER_BRICK, f);
    fclose(f);

    printf("wrote %s: %u bricks, voxelSize=%g, band=%g, expected size=%zu bytes\n",
           outPath, brickCount, voxelSize, band, (size_t)24 + (size_t)brickCount * (4 + 24 + SAMPLES_PER_BRICK));
    return 0;
}
