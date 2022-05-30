#ifndef FLUTTER_COMMON_GRAPHICS_VIDEO_FRAME_GRABBER_H_
#define FLUTTER_COMMON_GRAPHICS_VIDEO_FRAME_GRABBER_H_

#include <string>

#include "flutter/fml/macros.h"
#include "flutter/lib/ui/dart_wrapper.h"

#include "third_party/skia/include/core/SkImage.h"

namespace tonic {
class DartLibraryNatives;
}  // namespace tonic

namespace flutter {
class VideoFrameGrabberImpl {
 public:
  VideoFrameGrabberImpl(const std::string path);

  virtual double get_duration() = 0;
  virtual double get_width() = 0;
  virtual double get_height() = 0;
  virtual uint64_t get_frame_count() = 0;

  virtual void play() = 0;
  virtual void pause() = 0;

  virtual void seek_to_time(double time) = 0;
  virtual void seek_to_frame(uint64_t frame) = 0;

  virtual sk_sp<SkImage> get_latest_frame() = 0;
  virtual sk_sp<SkImage> get_frame_at_time(double) = 0;
  virtual sk_sp<SkImage> get_frame(uint64_t frame_index) = 0;

  virtual ~VideoFrameGrabberImpl() = 0;
};

class VideoFrameGrabber final
    : public RefCountedDartWrappable<VideoFrameGrabber> {
  DEFINE_WRAPPERTYPEINFO();
  FML_FRIEND_MAKE_REF_COUNTED(VideoFrameGrabber);

 public:
  static fml::RefPtr<VideoFrameGrabber> Create(std::string path);
  ~VideoFrameGrabber() override;

  double get_duration();
  double get_width();
  double get_height();
  uint64_t get_frame_count();

  void play();
  void pause();

  void seek_to_time(double time);
  void seek_to_frame(uint64_t frame_index);

  void get_latest_frame();
  void get_frame_at_time(double time);
  void get_frame(uint64_t frame_index);

  static void RegisterNatives(tonic::DartLibraryNatives* natives);

 private:
  VideoFrameGrabber(std::string path);

  std::shared_ptr<VideoFrameGrabberImpl> _impl;
};
}  // namespace flutter

#endif  // FLUTTER_COMMON_GRAPHICS_VIDEO_FRAME_GRABBER_H_
