#include <algorithm>
#include <cstdint>
#include <iostream>
#include <vector>

#ifdef _WIN32
#include <cstdio>
#include <io.h>
#include <fcntl.h>
#endif

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "Noise.h"

void write_data(void *context, void *data, int size) {
    std::cout.write((char*)data, size);
}

int main(int argc, char **argv) {
    int w = 1000, h = 1000;
    std::shared_ptr<NoiseFunction> n = std::make_shared<Perlin>();
    // n = std::make_shared<Turbulence>(n, 4);
    std::vector<uint8_t> image(w*h ,0);

    for (int i = 0; i < w; ++i) {
        for (int j = 0; j < h; ++j) {
            double x = (static_cast<double>(i) / w) * 16.0;
            double y = (static_cast<double>(j) / h) * 16.0;
            double v = (*n)(x, -1.3, y);
            image[(j*w) + i] = std::clamp(
                static_cast<uint8_t>(((v + 1.0) / 2.0) * 256),
                static_cast<uint8_t>(0),
                static_cast<uint8_t>(255)
            );
        }
    }

#ifdef _WIN32
    setmode(fileno(stdout), O_BINARY);
#endif

    stbi_write_png_to_func(&write_data, nullptr, w, h, 1, image.data(), w);

    return 0;
}
