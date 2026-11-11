#pragma once
#include "utils.h"

void create_brick_overlay_pipeline(Init& init, RenderData& render_data, VkPipeline& pipeline, VkPipelineLayout& pipeline_layout);
void draw_brick_overlay(const Init& init, const RenderData& data, VkCommandBuffer cmd_buf);

