#include "bricks.h"
#include <fstream>
#include <cstdio>

static constexpr uint64_t BRICKS_HEADER_BYTES = 24;
static constexpr uint64_t SAMPLES_PER_BRICK = 729;
// mortonIndex (4) + aabb (24) + samples (729)
static constexpr uint64_t BYTES_PER_BRICK = 4 + 24 + SAMPLES_PER_BRICK;

bool read_bricks_file(const char* path, BricksFile& bricks) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) {
        fprintf(stderr, "bricks: cannot open '%s'\n", path);
        return false;
    }
    const uint64_t file_size = (uint64_t)f.tellg();
    f.seekg(0);

    if (file_size < BRICKS_HEADER_BYTES) {
        fprintf(stderr, "bricks: '%s' is too small to hold a header (%llu bytes)\n", path, (unsigned long long)file_size);
        return false;
    }

    BricksFile b;
    f.read(reinterpret_cast<char*>(&b.brick_count), 4);
    f.read(reinterpret_cast<char*>(b.domain_min), 12);
    f.read(reinterpret_cast<char*>(&b.voxel_size), 4);
    f.read(reinterpret_cast<char*>(&b.band), 4);

    // Validate before allocating anything based on the count.
    const uint64_t expected = BRICKS_HEADER_BYTES + (uint64_t)b.brick_count * BYTES_PER_BRICK;
    if (file_size != expected) {
        fprintf(stderr, "bricks: '%s' has %llu bytes but a header claiming %u bricks implies %llu\n",
                path, (unsigned long long)file_size, b.brick_count, (unsigned long long)expected);
        return false;
    }

    b.mortons.resize(b.brick_count);
    b.aabbs.resize((size_t)b.brick_count * 6);
    f.read(reinterpret_cast<char*>(b.mortons.data()), (std::streamsize)b.mortons.size() * 4);
    f.read(reinterpret_cast<char*>(b.aabbs.data()), (std::streamsize)b.aabbs.size() * 4);
    // The 729 int8 samples per brick follow; the visualization doesn't need them.

    if (!f) {
        fprintf(stderr, "bricks: read error in '%s'\n", path);
        return false;
    }

    bricks = std::move(b);
    return true;
}

void upload_bricks(Init& init, RenderData& data, const BricksFile& bricks) {
    // Nothing may still be reading the old buffer when it is replaced.
    VK_CHECK(vkDeviceWaitIdle(init.device));
    if (data.brick_aabb_buffer.address) {
        vmaDestroyBuffer(data.alloc, data.brick_aabb_buffer.buf, data.brick_aabb_buffer.alloc);
        data.brick_aabb_buffer = {};
    }
    data.brick_count = 0;

    if (bricks.brick_count == 0) return;

    const unsigned int size = bricks.brick_count * 6 * sizeof(float);
    VkBufferUsageFlags usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    data.brick_aabb_buffer = create_buffer(init, data, size, usage, "brick_aabb_buffer");

    TransferToBuffer(data.alloc, data.staging_buffer, bricks.aabbs.data(), size);
    CopyBuffer(data, init, data.staging_buffer, data.brick_aabb_buffer, size);

    data.brick_count = bricks.brick_count;
    printf("bricks: uploaded %u bricks (%u bytes of AABBs)\n", data.brick_count, size);
}
