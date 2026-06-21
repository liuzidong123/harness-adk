#ifndef SNAKESHOT_BITMAPFONT_H
#define SNAKESHOT_BITMAPFONT_H

#include <cstdint>
#include <string>
#include <vector>

class BitmapFont {
public:
    static std::vector<uint8_t> renderText(
            const std::string &text,
            int padding, int &outW, int &outH,
            int scale = 2);
};

#endif
