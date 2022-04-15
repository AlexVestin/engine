// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/lib/ui/painting/image.h"

#include "flutter/lib/ui/painting/image_encoding.h"
#include "third_party/tonic/converter/dart_converter.h"
#include "third_party/tonic/dart_args.h"
#include "third_party/tonic/dart_binding_macros.h"
#include "third_party/tonic/dart_library_natives.h"
#include "third_party/tonic/dart_list.h"
#include "third_party/tonic/dart_class_library.h"


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
    natives->Register({{"Image_fromTexture", Image::FromTexture, 4, true}, /*{"Image_fromTextures", Image::FromTextures, 4, true},*/ FOR_EACH_BINDING(DART_REGISTER_NATIVE)});
}

void CanvasImage::FromTexture(Dart_NativeArguments args) {
    
    Dart_Handle raw_image_callback = Dart_GetNativeArgument(args, 3);
    
    if (!Dart_IsClosure(raw_image_callback)) {
      Dart_SetReturnValue(args, tonic::ToDart("Callback must be a function"));
      return;
    }
    
    int64_t texture_address;
    int64_t width;
    int64_t height;

    Dart_GetNativeIntegerArgument(args, 0, &texture_address);
    Dart_GetNativeIntegerArgument(args, 1, &width);
    Dart_GetNativeIntegerArgument(args, 2, &height);

    const void* texture_pointer = reinterpret_cast<const void*>(texture_address);

    auto* dart_state = UIDartState::Current();
    
  auto image_callback = std::make_unique<tonic::DartPersistentValue>(dart_state, raw_image_callback);
  auto unref_queue = dart_state->GetSkiaUnrefQueue();
  auto ui_task_runner = dart_state->GetTaskRunners().GetUITaskRunner();
  auto raster_task_runner = dart_state->GetTaskRunners().GetRasterTaskRunner();
  auto snapshot_delegate = dart_state->GetSnapshotDelegate();

  auto ui_task = fml::MakeCopyable([image_callback = std::move(image_callback), unref_queue](sk_sp<SkImage> raster_image) mutable {
    auto dart_state = image_callback->dart_state().lock();
    if (!dart_state) {
      // The root isolate could have died in the meantime.
      return;
    }
    tonic::DartState::Scope scope(dart_state);

    if (!raster_image) {
      tonic::DartInvoke(image_callback->Get(), {Dart_Null()});
      return;
    }

    auto dart_image = CanvasImage::Create();
    dart_image->set_image({std::move(raster_image), std::move(unref_queue)});
    auto* raw_dart_image = tonic::ToDart(std::move(dart_image));

    // All done!
    tonic::DartInvoke(image_callback->Get(), {raw_dart_image});

    // image_callback is associated with the Dart isolate and must be deleted
    // on the UI thread.
    image_callback.reset();
  });

  // Kick things off on the raster rask runner.
  fml::TaskRunner::RunNowOrPostTask(
      raster_task_runner, [ui_task_runner, snapshot_delegate, ui_task, texture_pointer, width, height] {

        #if __APPLE__
        GrMtlTextureInfo skiaTextureInfo;
        skiaTextureInfo.fTexture = sk_cfp<const void*>{texture_pointer};
        #endif

        GrBackendTexture skiaBackendTexture(width,
                                            height,
                                            GrMipMapped::kNo,
                                            skiaTextureInfo);

        sk_sp<SkImage> raster_image = snapshot_delegate->UploadTexture(skiaBackendTexture);

        fml::TaskRunner::RunNowOrPostTask(
            ui_task_runner,
            [ui_task, raster_image]() { ui_task(raster_image); });
      });


    /*
    auto dart_state = UIDartState::Current();
    auto tonic_dart_state = std::make_shared<tonic::DartPersistentValue>(dart_state, callback_handle);

    auto result =
      [ui_runner = dart_state->GetTaskRunners().GetUITaskRunner(), tonic_dart_state](SkiaGPUObject<SkImage> image) {
        ui_runner->PostTask(fml::MakeCopyable(
            [image = std::move(image), tonic_dart_state]() mutable {
              
              auto _dart_state = tonic_dart_state.get()->dart_state().lock();

              if (!_dart_state) {
                return;
              }

              tonic::DartState::Scope scope{_dart_state.get()};

              auto canvas_image = fml::MakeRefCounted<CanvasImage>();
              canvas_image->set_image(std::move(image));

              tonic::DartInvoke(tonic_dart_state->Get(), {tonic::ToDart(canvas_image)});
            }));
      };
        
    GrMtlTextureInfo skiaTextureInfo;
    skiaTextureInfo.fTexture = sk_cfp<const void*>{texture_pointer};

    GrBackendTexture skiaBackendTexture(width=width,
                                        height=height,
                                        mipMapped=GrMipMapped::kNo,
                                        textureInfo=skiaTextureInfo);
  
    auto raster_runner = dart_state->GetTaskRunners().GetRasterTaskRunner();

    

    raster_runner->PostTask(fml::MakeCopyable([skiaBackendTexture, result, snapshot_delegate =
           UIDartState::Current()->GetSnapshotDelegate(), unref_queue = dart_state->GetSkiaUnrefQueue()]() mutable {
      
      auto _snapshot_delegate = snapshot_delegate.get();

      if (!_snapshot_delegate) {
        FML_DLOG(ERROR) << "Could not acquire the snapshot delegate.";
        result({});
        return;
      }

      
      auto image = snapshot_delegate->UploadTexture(skiaBackendTexture);

      auto gpu_image = SkiaGPUObject<SkImage>{std::move(image), unref_queue};

      result(std::move(gpu_image));

    }));
*/
}

struct TextureDescriptor {
  const void* texture_pointer;
  int width;
  int height;

  TextureDescriptor(const void* texture_pointer, int width, int height) : texture_pointer{texture_pointer}, width{width}, height{height} {};
};
/*
void CanvasImage::FromTextures(Dart_NativeArguments args) {
    
    Dart_Handle raw_image_callback = Dart_GetNativeArgument(args, 3);
    
    if (!Dart_IsClosure(raw_image_callback)) {
      Dart_SetReturnValue(args, tonic::ToDart("Callback must be a function"));
      return;
    }


    Dart_Handle exception;
    tonic::DartList raw_texture_addresses = tonic::DartConverter<tonic::DartList>::FromArguments(args, 0, exception);
    tonic::DartList raw_widths = tonic::DartConverter<tonic::DartList>::FromArguments(args, 1, exception);
    tonic::DartList raw_heights = tonic::DartConverter<tonic::DartList>::FromArguments(args, 2, exception);

    std::vector<TextureDescriptor> texture_descriptors;

    for(size_t i = 0; i < raw_texture_addresses.size(); i++) {
        texture_descriptors.emplace_back(reinterpret_cast<const void*>(tonic::DartConverter<int64_t>::FromDart(raw_texture_addresses.Get(i))), tonic::DartConverter<int64_t>::FromDart(raw_widths.Get(i)), tonic::DartConverter<int64_t>::FromDart(raw_heights.Get(i)));
    }

    auto* dart_state = UIDartState::Current();
    auto& class_library = dart_state->class_library();
    auto type = Dart_HandleFromPersistent(class_library.GetClass("ui", "_Image"));
    
    auto image_callback = std::make_unique<tonic::DartPersistentValue>(dart_state, raw_image_callback);
    auto unref_queue = dart_state->GetSkiaUnrefQueue();
    auto ui_task_runner = dart_state->GetTaskRunners().GetUITaskRunner();
    auto raster_task_runner = dart_state->GetTaskRunners().GetRasterTaskRunner();
    auto snapshot_delegate = dart_state->GetSnapshotDelegate();

  auto ui_task = fml::MakeCopyable([image_callback = std::move(image_callback), unref_queue, _type = std::move(type)](std::vector<sk_sp<SkImage>> raster_images) mutable {
    auto dart_state = image_callback->dart_state().lock();
    if (!dart_state) {
      // The root isolate could have died in the meantime.
      return;
    }
    tonic::DartState::Scope scope(dart_state);

    auto _raw_dart_images = Dart_NewListOfType(_type, raster_images.size());
    size_t i = 0;
    for (const auto& raster_image : raster_images) {
      auto dart_image = CanvasImage::Create();
      dart_image->set_image({std::move(raster_image), std::move(unref_queue)});
      auto raw_dart_image = tonic::ToDart(std::move(dart_image));
      Dart_ListSetAt(_raw_dart_images, i, raw_dart_image);
      i++;
    }

   

    // All done!
    tonic::DartInvoke(image_callback->Get(), {_raw_dart_images});

    // image_callback is associated with the Dart isolate and must be deleted
    // on the UI thread.
    image_callback.reset();
  });

  // Kick things off on the raster rask runner.
  fml::TaskRunner::RunNowOrPostTask(
      raster_task_runner, [ui_task_runner, snapshot_delegate, ui_task, _texture_descriptors = std::move(texture_descriptors)] {

        std::vector<sk_sp<SkImage>> raster_images;

        for (const auto& texture_descriptor : _texture_descriptors) {

          #if __APPLE__
          GrMtlTextureInfo skiaTextureInfo;
          skiaTextureInfo.fTexture = sk_cfp<const void*>{texture_descriptor.texture_pointer};
          #endif

          GrBackendTexture skiaBackendTexture(texture_descriptor.width,
                                              texture_descriptor.height,
                                              GrMipMapped::kNo,
                                              skiaTextureInfo);

          raster_images.push_back(snapshot_delegate->UploadTexture(skiaBackendTexture));

        }
        

        fml::TaskRunner::RunNowOrPostTask(
            ui_task_runner,
            [ui_task, _raster_images = std::move(raster_images)]() { ui_task(_raster_images); });
      });
}
*/
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
