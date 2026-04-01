#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <dirent.h>
#include <sys/stat.h>
#include <algorithm>
#include <cstring>
#include <cstdio>
#include <string>
#include <vector>

#include "fileexplorer.h"

static const int FONT_SIZE = 18;
static const int CHAR_H    = 22;

FileExplorer::FileExplorer()
    : selectedIndex(0), scrollOffset(0), visibleLines(0),
      fontBitmap(nullptr), bitmapW(512), bitmapH(512),
      charData(nullptr), screenTex(0),
      texW(0), texH(0), pixels(nullptr)
{
    currentPath = ".";
}

FileExplorer::~FileExplorer() {
    delete[] fontBitmap;
    delete[] (stbtt_bakedchar*)charData;
    delete[] pixels;
    if (screenTex) glDeleteTextures(1, &screenTex);
}

void FileExplorer::init(const char* fontPath, int texWidth, int texHeight) {
    texW = texWidth;
    texH = texHeight;
    visibleLines = texH / CHAR_H - 2;

    FILE* f = fopen(fontPath, "rb");
    if (!f) { fprintf(stderr, "FileExplorer: nie można otworzyć fontu '%s'\n", fontPath); return; }
    fseek(f, 0, SEEK_END);
    int size = ftell(f);
    fseek(f, 0, SEEK_SET);
    unsigned char* ttfBuf = new unsigned char[size];
    fread(ttfBuf, 1, size, f);
    fclose(f);

    fontBitmap = new unsigned char[bitmapW * bitmapH];
    charData   = new stbtt_bakedchar[96];
    stbtt_BakeFontBitmap(ttfBuf, 0, FONT_SIZE, fontBitmap, bitmapW, bitmapH, 32, 96,(stbtt_bakedchar*)charData);
    delete[] ttfBuf;

    pixels = new unsigned char[texW * texH * 4];
    memset(pixels, 0, texW * texH * 4);

    glGenTextures(1, &screenTex);
    glBindTexture(GL_TEXTURE_2D, screenTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texW, texH, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    listDir(currentPath);
    redraw();
}

void FileExplorer::listDir(const std::string& path) {
    entries.clear();
    entries.push_back("..");

    DIR* dir = opendir(path.c_str());
    if (!dir) return;

    std::vector<std::string> dirs, files;
    struct dirent* ent;
    while ((ent = readdir(dir)) != nullptr) {
        std::string name = ent->d_name;
        if (name == "." || name == "..") continue;
        std::string full = path + "/" + name;
        struct stat st;
        stat(full.c_str(), &st);
        if (S_ISDIR(st.st_mode))
            dirs.push_back(name + "/");
        else
            files.push_back(name);
    }
    closedir(dir);

    std::sort(dirs.begin(),  dirs.end());
    std::sort(files.begin(), files.end());
    for (auto& d : dirs)  entries.push_back(d);
    for (auto& fi : files) entries.push_back(fi);

    selectedIndex = 0;
    scrollOffset  = 0;
}

void FileExplorer::handleKey(int key) {
    if (key == GLFW_KEY_UP) {
        if (selectedIndex > 0) selectedIndex--;
        if (selectedIndex < scrollOffset) scrollOffset--;
    }
    else if (key == GLFW_KEY_DOWN) {
        if (selectedIndex < (int)entries.size() - 1) selectedIndex++;
        if (selectedIndex >= scrollOffset + visibleLines) scrollOffset++;
    }
    else if (key == GLFW_KEY_ENTER || key == GLFW_KEY_KP_ENTER) {
        std::string selected = entries[selectedIndex];
        if (selected == "..") {
            size_t pos = currentPath.rfind('/');
            if (pos != std::string::npos && pos > 0)
                currentPath = currentPath.substr(0, pos);
            else
                currentPath = "/";
        } else if (selected.back() == '/') {
            if (currentPath == "/")
                currentPath = "/" + selected.substr(0, selected.size() - 1);
            else
                currentPath += "/" + selected.substr(0, selected.size() - 1);
        }
        listDir(currentPath);
    }
    redraw();
}

void FileExplorer::clearPixels() {
    for (int i = 0; i < texW * texH; i++) {
        pixels[i*4+0] = 20;
        pixels[i*4+1] = 20;
        pixels[i*4+2] = 30;
        pixels[i*4+3] = 255;
    }
}

void FileExplorer::drawChar(int& x, int& y, char c, unsigned char r, unsigned char g, unsigned char b) {
    if (c < 32 || c > 127) return;
    stbtt_bakedchar* bc = &((stbtt_bakedchar*)charData)[c - 32];

    int x0 = (int)bc->x0, y0 = (int)bc->y0;
    int x1 = (int)bc->x1, y1 = (int)bc->y1;
    int cw = x1 - x0;
    int ch = y1 - y0;

    int destX = x + (int)bc->xoff;
    int destY = y + (int)bc->yoff + FONT_SIZE;

    for (int row = 0; row < ch; row++) {
        for (int col = 0; col < cw; col++) {
            int px = destX + col;
            int py = destY + row;
            if (px < 0 || px >= texW || py < 0 || py >= texH) continue;
            unsigned char alpha = fontBitmap[(y0 + row) * bitmapW + (x0 + col)];
            if (alpha == 0) continue;
            int idx = (py * texW + px) * 4;
            pixels[idx+0] = r;
            pixels[idx+1] = g;
            pixels[idx+2] = b;
            pixels[idx+3] = alpha;
        }
    }
    x += (int)bc->xadvance;
}

void FileExplorer::drawText(int x, int y, const std::string& text, unsigned char r, unsigned char g, unsigned char b) {
    for (char c : text) drawChar(x, y, c, r, g, b);
}

void FileExplorer::redraw() {
    clearPixels();

    drawText(4, 2, currentPath, 100, 200, 255);

    // linia pod nagłówkiem
    for (int x = 0; x < texW; x++) {
        int idx = (CHAR_H * texW + x) * 4;
        pixels[idx+0] = 80; pixels[idx+1] = 80;
        pixels[idx+2] = 120; pixels[idx+3] = 255;
    }

    for (int i = 0; i < visibleLines; i++) {
        int entIdx = scrollOffset + i;
        if (entIdx >= (int)entries.size()) break;

        int y = (i + 1) * CHAR_H + 4;
        bool isSelected = (entIdx == selectedIndex);
        bool isDir = entries[entIdx].back() == '/' || entries[entIdx] == "..";

        if (isSelected) {
            for (int x = 0; x < texW; x++) {
                int idx = ((y + 2) * texW + x) * 4;
                pixels[idx+0] = 50; pixels[idx+1] = 50;
                pixels[idx+2] = 90; pixels[idx+3] = 255;
            }
            drawText(4, y, entries[entIdx], 255, 255, 100);
        } else if (isDir) {
            drawText(4, y, entries[entIdx], 100, 180, 255);
        } else {
            drawText(4, y, entries[entIdx], 200, 200, 200);
        }
    }

    glBindTexture(GL_TEXTURE_2D, screenTex);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texW, texH, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
}