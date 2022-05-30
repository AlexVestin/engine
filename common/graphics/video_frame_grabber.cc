#include "flutter/common/graphics/video_frame_grabber.h"

#include "flutter/lib/ui/ui_dart_state.h"
#include "third_party/tonic/converter/dart_converter.h"
#include "third_party/tonic/dart_args.h"
#include "third_party/tonic/dart_binding_macros.h"
#include "third_party/tonic/dart_library_natives.h"

namespace flutter {
static void VideoFrameGrabber_constructor(Dart_NativeArguments args) {
  UIDartState::ThrowIfUIOperationsProhibited();
  DartCallConstructor(&VideoFrameGrabber::Create, args);
}

IMPLEMENT_WRAPPERTYPEINFO(ui, VideoFrameGrabber);

#define FOR_EACH_BINDING(V)               \
  V(VideoFrameGrabber, get_duration)      \
  V(VideoFrameGrabber, get_width)         \
  V(VideoFrameGrabber, get_width)         \
  V(VideoFrameGrabber, get_frame_count)   \
  V(VideoFrameGrabber, play)              \
  V(VideoFrameGrabber, pause)             \
  V(VideoFrameGrabber, seek_to_time)      \
  V(VideoFrameGrabber, seek_to_frame)     \
  V(VideoFrameGrabber, get_latest_frame)  \
  V(VideoFrameGrabber, get_frame_at_time) \
  V(VideoFrameGrabber, get_frame)

FOR_EACH_BINDING(DART_NATIVE_CALLBACK)

void VideoFrameGrabber::RegisterNatives(tonic::DartLibraryNatives* natives) {
  natives->Register({{"VideoFrameGrabber_constructor",
                      VideoFrameGrabber_constructor, 2, true},
                     FOR_EACH_BINDING(DART_REGISTER_NATIVE)});
}

fml::RefPtr<VideoFrameGrabber> VideoFrameGrabber::Create(std::string path) {
  return fml::RefPtr<VideoFrameGrabber>(new VideoFrameGrabber(path));
}

VideoFrameGrabber::VideoFrameGrabber(std::string path) {
  // TODO: implemente concrete constructor
  /*
  #if OS_ANDROID
  #elif OS_IOS
  #else
  #endif*/
}

double VideoFrameGrabber::get_duration() {
  return _impl->get_duration();
}
double VideoFrameGrabber::get_width() {
  return _impl->get_width();
}
double VideoFrameGrabber::get_height() {
  return _impl->get_height();
}
uint64_t VideoFrameGrabber::get_frame_count() {
  return _impl->get_frame_count();
}

void VideoFrameGrabber::play() {
  _impl->play();
}
void VideoFrameGrabber::pause() {
  _impl->pause();
}

void VideoFrameGrabber::seek_to_time(double time) {
  _impl->seek_to_time(time);
}
void VideoFrameGrabber::seek_to_frame(uint64_t frame_index) {
  _impl->seek_to_frame(frame_index);
}

void VideoFrameGrabber::get_latest_frame() {}
void VideoFrameGrabber::get_frame_at_time(double time) {}
void VideoFrameGrabber::get_frame(uint64_t frame_index) {}

}  // namespace flutter
