// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/lib/ui/painting/image.h"

#include "flutter/lib/ui/painting/image_encoding.h"
#include "third_party/tonic/converter/dart_converter.h"
#include "third_party/tonic/dart_args.h"
#include "third_party/tonic/dart_binding_macros.h"
#include "third_party/tonic/dart_class_library.h"
#include "third_party/tonic/dart_library_natives.h"
#include "third_party/tonic/dart_list.h"

#include "flutter/fml/make_copyable.h"

#include <iostream>

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
  natives->Register({{"Image_fromTextures", Image::FromTextures, 3, true},
                     FOR_EACH_BINDING(DART_REGISTER_NATIVE)});
}

void CanvasImage::FromTextures(Dart_NativeArguments args) {
  Dart_Handle exception = nullptr;

  tonic::DartList raw_texture_addresses =
      tonic::DartConverter<tonic::DartList>::FromArguments(args, 0, exception);
  tonic::DartList raw_widths =
      tonic::DartConverter<tonic::DartList>::FromArguments(args, 1, exception);
  tonic::DartList raw_heights =
      tonic::DartConverter<tonic::DartList>::FromArguments(args, 2, exception);

  if (exception && Dart_IsApiError(exception)) {
    Dart_SetReturnValue(args, exception);
    return;
  }

  auto* dart_state = UIDartState::Current();

  if (!dart_state) {
    Dart_SetReturnValue(
        args, Dart_ThrowException(tonic::ToDart("Dart State is dead")));
    return;
  }

  auto& class_library = dart_state->class_library();
  auto type = Dart_TypeToNullableType(Dart_HandleFromPersistent(
      class_library.GetClass(kDartWrapperInfo_ui_Image)));

  auto unref_queue = dart_state->GetSkiaUnrefQueue();
  auto snapshot_delegate = dart_state->GetSnapshotDelegate().unsafeGet();

  const auto size = raw_texture_addresses.size();
  auto _raw_dart_images = Dart_NewListOfType(type, size);

  for (size_t i = 0; i < size; i++) {
    const auto raw_texture_address = raw_texture_addresses.Get<int64_t>(i);
    const auto width = raw_widths.Get<int64_t>(i);
    const auto height = raw_heights.Get<int64_t>(i);

#if __APPLE__
    const auto texture_address =
        reinterpret_cast<const void*>(raw_texture_address);
    GrMtlTextureInfo skiaTextureInfo;
    skiaTextureInfo.fTexture = sk_cfp<const void*>{texture_address};
#elif ANDROID
    std::cerr << "Texture ID" << static_cast<GrGLuint>(raw_texture_address)
              << "\n";
    // 0x0DE1 --> GL_TEXTURE_2D
    // 0x8058 --> GL_RGBA8_OES
    GrGLTextureInfo skiaTextureInfo = {
        0x0DE1, static_cast<GrGLuint>(raw_texture_address), 0x8058};
#endif

    GrBackendTexture skiaBackendTexture(width, height, GrMipMapped::kNo,
                                        skiaTextureInfo);

    auto image = snapshot_delegate->UploadTexture(skiaBackendTexture);

    auto dart_image = CanvasImage::Create();
    dart_image->set_image({std::move(image), unref_queue});

    auto raw_dart_image = tonic::ToDart(std::move(dart_image));
    Dart_ListSetAt(_raw_dart_images, i, raw_dart_image);
  }

  Dart_SetReturnValue(args, _raw_dart_images);
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
  if (auto image = image_.skia_object()) {
    const auto& info = image->imageInfo();
    const auto kMipmapOverhead = 4.0 / 3.0;
    const size_t image_byte_size = info.computeMinByteSize() * kMipmapOverhead;
    return image_byte_size + sizeof(this);
  } else {
    return sizeof(CanvasImage);
  }
}

}  // namespace flutter
