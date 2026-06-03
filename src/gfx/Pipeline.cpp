// -*- mode: c++; c-basic-offset: 4; encoding: utf-8; -*-

#include "../vulkan.h"

#include "Pipeline.h"
#include "Renderer.h"
#include "System.h"

gfx::Pipeline::Pipeline()
: m_renderer{nullptr},
  m_pipeline{nullptr}
{}

gfx::Pipeline::Pipeline(Renderer *renderer) : Pipeline() {
    m_renderer = renderer;
    initPipeline();
}

gfx::Pipeline::~Pipeline() {}
