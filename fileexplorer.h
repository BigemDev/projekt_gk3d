
#ifndef FILEEXPLORER_H
#define FILEEXPLORER_H

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <string>
#include <vector>

// typedef struct stbtt_bakedchar;

class FileExplorer {
public:
    FileExplorer();
    ~FileExplorer();

    void init(const char* fontPath, int texWidth, int texHeight);
    void handleKey(int key);
    GLuint getTexture() { return screenTex; }

private:
    std::string currentPath;
    std::vector<std::string> entries;
    int selectedIndex;
    int scrollOffset;
    int visibleLines;

    unsigned char* fontBitmap;
    int bitmapW, bitmapH;
    void* charData;

    GLuint screenTex;
    int texW, texH;
    unsigned char* pixels;

    void listDir(const std::string& path);
    void clearPixels();
    void drawChar(int& x, int& y, char c, unsigned char r, unsigned char g, unsigned char b);
    void drawText(int x, int y, const std::string& text, unsigned char r, unsigned char g, unsigned char b);
    void redraw();
};

#endif