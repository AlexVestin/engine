// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/lib/ui/painting/image.h"

#include "flutter/lib/ui/painting/image_encoding.h"
#include "third_party/dart/runtime/include/dart_api.h"
#include "third_party/tonic/converter/dart_converter.h"
#include "third_party/tonic/dart_args.h"
#include "third_party/tonic/dart_binding_macros.h"
#include "third_party/tonic/dart_library_natives.h"

namespace tonic {

inline Dart_Handle LookupNullableType(const std::string& library_name,
                                      const std::string& type_name) {
  auto library =
      Dart_LookupLibrary(DartConverter<std::string>::ToDart(library_name));
  if (LogIfError(library)) {
    return library;
  }
  auto type_string = DartConverter<std::string>::ToDart(type_name);
  if (LogIfError(type_string)) {
    return type_string;
  }
  auto type = Dart_GetNullableType(library, type_string, 0, nullptr);
  if (LogIfError(type)) {
    return type;
  }
  return type;
}
}  // namespace tonic
namespace flutter {

typedef CanvasImage Image;

// Since _Image is a private class, we can't use IMPLEMENT_WRAPPERTYPEINFO
static const tonic::DartWrapperInfo kDartWrapperInfo_ui_Image = {
    "ui",
    "_Image",
    sizeof(Image),
};
const tonic::DartWrapperInfo& Image::dart_wrapper_info_ =
    kDartWrapperInfo_ui_Image;

#define FOR_EACH_BINDING(V) \
  V(Image, width)           \
  V(Image, height)          \
  V(Image, toByteData)      \
  V(Image, dispose)

FOR_EACH_BINDING(DART_NATIVE_CALLBACK)

void CanvasImage::RegisterNatives(tonic::DartLibraryNatives* natives) {
  natives->Register({{"Image_FromTextures", FromTextures, 1, true},
                     FOR_EACH_BINDING(DART_REGISTER_NATIVE)});
}

CanvasImage::CanvasImage() = default;

CanvasImage::~CanvasImage() = default;

Dart_Handle CanvasImage::toByteData(int format, Dart_Handle callback) {
  return EncodeImage(this, format, callback);
}

void CanvasImage::dispose() {
  image_.reset();
  ClearDartWrapper();
}

size_t CanvasImage::GetAllocationSize() const {
  auto size = sizeof(this);
  if (image_) {
    size += image_->GetApproximateByteSize();
  }
  return size;
}

void CanvasImage::FromTextures(Dart_NativeArguments args) {
  Dart_Handle texture_descriptor_list = Dart_GetNativeArgument(args, 0);

  if (!Dart_IsList(texture_descriptor_list)) {
    Dart_ThrowException(tonic::ToDart("Texture descriptors is not a list!"));
  }

  auto* dart_state = UIDartState::Current();
  auto unref_queue = dart_state->GetSkiaUnrefQueue();
  auto snapshot_delegate = dart_state->GetSnapshotDelegate().unsafeGet();

  intptr_t count;
  Dart_ListLength(texture_descriptor_list, &count);

  Dart_Handle type = tonic::LookupNullableType("dart:ui", "_Image");
  Dart_Handle image_list = Dart_NewListOfType(type, count);

  for (intptr_t i = 0; i < count; i++) {
    auto dart_texture_descriptor = Dart_ListGetAt(texture_descriptor_list, i);
    auto raw_texture_descritptor = tonic::Int64List(dart_texture_descriptor);
    auto texture_descriptor = TextureDescriptor::Init(raw_texture_descritptor);
    auto _image = snapshot_delegate->UploadTexture(texture_descriptor);
    auto gpu_image = SkiaGPUObject{std::move(_image), unref_queue};
    auto dart_image = CanvasImage::Create();
    dart_image->set_image(DlImageGPU::Make(std::move(gpu_image)));
    Dart_ListSetAt(
        image_list, i,
        tonic::DartConverter<decltype(dart_image)>::ToDart(dart_image));
  }

  Dart_SetReturnValue(args, image_list);
}
}  // namespace flutter
