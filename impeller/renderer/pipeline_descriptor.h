// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <Metal/Metal.h>

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>

#include "flutter/fml/hash_combine.h"
#include "flutter/fml/macros.h"
#include "impeller/renderer/comparable.h"
#include "impeller/renderer/formats.h"
#include "impeller/renderer/shader_types.h"

namespace impeller {

class ShaderFunction;
class PipelineVertexDescriptor;

class PipelineDescriptor final : public Comparable<PipelineDescriptor> {
 public:
  PipelineDescriptor();

  ~PipelineDescriptor();

  PipelineDescriptor& SetLabel(const std::string_view& label);

  PipelineDescriptor& SetSampleCount(size_t samples);

  PipelineDescriptor& AddStageEntrypoint(
      std::shared_ptr<const ShaderFunction> function);

  PipelineDescriptor& SetVertexDescriptor(
      std::shared_ptr<PipelineVertexDescriptor> vertex_descriptor);

  PipelineDescriptor& SetColorAttachmentDescriptor(
      size_t index,
      PipelineColorAttachment desc);

  const PipelineColorAttachment* GetColorAttachmentDescriptor(
      size_t index) const;

  PipelineDescriptor& SetDepthStencilAttachmentDescriptor(
      PipelineDepthAttachment desc);

  PipelineDescriptor& SetStencilAttachmentDescriptors(
      PipelineStencilAttachment front_and_back);

  PipelineDescriptor& SetStencilAttachmentDescriptors(
      PipelineStencilAttachment front,
      PipelineStencilAttachment back);

  PipelineDescriptor& SetDepthPixelFormat(PixelFormat format);

  PipelineDescriptor& SetStencilPixelFormat(PixelFormat format);

  MTLRenderPipelineDescriptor* GetMTLRenderPipelineDescriptor() const;

  id<MTLDepthStencilState> CreateDepthStencilDescriptor(
      id<MTLDevice> device) const;

  // Comparable<PipelineDescriptor>
  std::size_t GetHash() const override;

  // Comparable<PipelineDescriptor>
  bool IsEqual(const PipelineDescriptor& other) const override;

 private:
  std::string label_;
  size_t sample_count_ = 1;
  std::map<ShaderStage, std::shared_ptr<const ShaderFunction>> entrypoints_;
  std::map<size_t, PipelineColorAttachment> color_attachment_descriptors_;
  std::shared_ptr<PipelineVertexDescriptor> vertex_descriptor_;
  PixelFormat depth_pixel_format_ = PixelFormat::kUnknown;
  PixelFormat stencil_pixel_format_ = PixelFormat::kUnknown;
  std::optional<PipelineDepthAttachment> depth_attachment_descriptor_;
  std::optional<PipelineStencilAttachment> front_stencil_attachment_descriptor_;
  std::optional<PipelineStencilAttachment> back_stencil_attachment_descriptor_;
};

}  // namespace impeller