// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#include "../vulkan.h"
#include "System.h"
#include "TerrainRenderer.h"

gfx::TerrainRenderer::TerrainRenderer(System *system)
    : m_system{system}
{}

gfx::TerrainRenderer::~TerrainRenderer() {
    dispose();
}

void gfx::TerrainRenderer::init() {
}

void gfx::TerrainRenderer::dispose() {
}
