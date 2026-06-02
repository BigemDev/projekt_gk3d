#ifndef FILEEXPLORER_H
#define FILEEXPLORER_H

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <map>

typedef struct {
    std::string name;
    std::string fullPath;
    std::string permissions;
    std::string owner;
    std::string group;
    long size;
    std::string date;
    bool isDirectory;
} FileEntry;

struct FileBar {
    std::string name;
    long size;
    float height;
    glm::vec3 color;
    glm::vec3 position;
    GLuint labelTex;
    int labelWidth;
    int labelHeight;
};

class FileChart {
public:
    std::vector<FileBar> bars;
    GLuint vao;
    GLuint vbo;
    int barCount;
    long maxSize;
    
    FileChart();
    ~FileChart();
    void updateData(const std::vector<FileEntry>& entries, class FileExplorer* explorer);
    void draw(glm::mat4 P, glm::mat4 V, glm::vec3 centerPos, glm::vec3 cameraPos);
    void cleanup();
};

class FileExplorer {
public:
    FileExplorer();
    ~FileExplorer();

    void init(const char* fontPath, int texWidth, int texHeight);
    void handleKey(int key);
    void update();
    GLuint makeUiTexture(std::string);
    bool isFilePanelOpen() { return !viewingFile.empty(); }
    GLuint getTexture() { return screenTex; }
    
    std::string getSelectedFilePath() const { 
        if (selectedIndex >= 0 && selectedIndex < (int)entries.size()) {
            return entries[selectedIndex].fullPath;
        }
        return "";
    }
    
    bool isDirectorySelected() const {
        if (selectedIndex >= 0 && selectedIndex < (int)entries.size()) {
            return entries[selectedIndex].isDirectory;
        }
        return false;
    }
    
    void setViewingFile(const std::string& path) {
        viewingFile = path;
        fileViewerScrollOffset = 0;
        redrawFileViewer();
    }
    
    std::string getCurrentViewingFile() const { return viewingFile; }
    
    FileChart* getChart() { return &chart; }
    
    void updateChart();
    std::string getCurrentPath() const { return currentPath; }
    
    void* getCharData() const { return charData; }
    unsigned char* getFontBitmap() const { return fontBitmap; }
    int getBitmapW() const { return bitmapW; }
    
    void getTextDimensions(const std::string& text, int& width, int& height);
    void renderTextToBuffer(const std::string& text, unsigned char* buffer, int bufWidth, int bufHeight);

private:
    std::string currentPath;
    std::vector<FileEntry> entries;
    std::map<std::string, std::string> cachedFileContents;
    
    int selectedIndex;
    int scrollOffset;
    int visibleLines;
    
    std::string viewingFile;
    int fileViewerScrollOffset;
    int totalFileLines;

    unsigned char* fontBitmap;
    int bitmapW, bitmapH;
    void* charData;

    GLuint screenTex;
    int texW, texH;
    unsigned char* pixels;

    void listDir(const std::string& path);
    std::string readFileContent(const std::string& filepath);
    bool isTextFile(const std::string& filepath);
    
    void clearPixels();
    void drawChar(int& x, int& y, char c, unsigned char r, unsigned char g, unsigned char b);
    void drawText(int x, int y, const std::string& text, unsigned char r, unsigned char g, unsigned char b);
    void redraw();
    void redrawFileViewer();
    int countLines(const std::string& content);
    std::vector<std::string> splitLines(const std::string& content);
    FileChart chart;
};
    
#endif