#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace {
struct Image {
  int width = 0;
  int height = 0;
  stbi_uc *pixels = nullptr;
  ~Image() { stbi_image_free(pixels); }
};

bool Load(const char *path, Image &image) {
  int channels = 0;
  image.pixels = stbi_load(path, &image.width, &image.height, &channels, 4);
  if (!image.pixels) std::cerr << "cannot decode " << path << ": " << stbi_failure_reason() << '\n';
  return image.pixels != nullptr;
}
}  // namespace

int main(int argc, char **argv) {
  if (argc != 3) return 2;
  Image actual, expected;
  if (!Load(argv[1], actual) || !Load(argv[2], expected)) return 1;
  if (actual.width != expected.width || actual.height != expected.height ||
      actual.width <= 0 || actual.height <= 0 || actual.width > 4096 || actual.height > 4096) {
    std::cerr << "image dimensions differ\n";
    return 1;
  }

  std::size_t matched = 0;
  std::uint64_t absolute_error = 0;
  const std::size_t count = static_cast<std::size_t>(actual.width) * actual.height;
  for (int y = 0; y < actual.height; ++y) {
    for (int x = 0; x < actual.width; ++x) {
      const std::size_t expected_index = (static_cast<std::size_t>(y) * actual.width + x) * 4;
      bool found = false;
      int best_error = 256 * 4;
      for (int oy = -1; oy <= 1; ++oy) {
        for (int ox = -1; ox <= 1; ++ox) {
          const int px = x + ox, py = y + oy;
          if (px < 0 || py < 0 || px >= actual.width || py >= actual.height) continue;
          const std::size_t actual_index = (static_cast<std::size_t>(py) * actual.width + px) * 4;
          int error = 0;
          bool close = actual.pixels[actual_index + 3] == 255;
          for (int channel = 0; channel < 3; ++channel) {
            const int delta = std::abs(int(expected.pixels[expected_index + channel]) -
                                       int(actual.pixels[actual_index + channel]));
            error += delta;
            if (delta > 4) close = false;
          }
          best_error = std::min(best_error, error);
          found = found || close;
        }
      }
      matched += found;
      absolute_error += static_cast<std::uint64_t>(best_error);
    }
  }
  const double ratio = static_cast<double>(matched) / count;
  const double mean_error = static_cast<double>(absolute_error) / (count * 3);
  std::cout << "matched=" << ratio << " mean_rgb_error=" << mean_error << '\n';
  return ratio >= 0.99 && mean_error <= 2.0 ? 0 : 1;
}
