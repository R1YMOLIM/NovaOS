#include <video.h>

static const boot_video_info_t *g_video;

void init_video(const boot_video_info_t *video) {
  g_video = video;
}
