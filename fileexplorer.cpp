#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <algorithm>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <unistd.h>
#include <limits.h>
#include <pwd.h>
#include <grp.h>
#include <ctime>
#include "fileexplorer.h"
#include "shaderprogram.h"

extern ShaderProgram* spLambert;
extern ShaderProgram* spTexture;
extern ShaderProgram* spLabel;

static const int FONT_SIZE = 28;
static const int CHAR_H    = 32;
static const int BITMAP_W  = 1024;
static const int BITMAP_H  = 1024;

FileExplorer::FileExplorer()
    : selectedIndex(0), scrollOffset(0), visibleLines(0),
      fontBitmap(nullptr), bitmapW(BITMAP_W), bitmapH(BITMAP_H),
      charData(nullptr), screenTex(0),
      texW(0), texH(0), pixels(nullptr),
      fileViewerScrollOffset(0), totalFileLines(0)
{
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != nullptr) {
        currentPath = cwd;
    } else {
        currentPath = ".";
    }
    viewingFile = "";
}

FileExplorer::~FileExplorer() {
    delete[] fontBitmap;
    delete[] (stbtt_bakedchar*)charData;
    delete[] pixels;
    if (screenTex) glDeleteTextures(1, &screenTex);
}

void FileExplorer::listDir(const std::string& path) {
    entries.clear();
    
    DIR* dir = opendir(path.c_str());
    if (!dir) {
        fprintf(stderr, "FileExplorer: nie można otworzyć katalogu '%s'\n", path.c_str());
        return;
    }
    
    std::string parentPath = path;
    size_t pos = parentPath.rfind('/');
    if (pos != std::string::npos) {
        if (pos == 0) {
            parentPath = "/";
        } else {
            parentPath = parentPath.substr(0, pos);
        }
    } else {
        parentPath = ".";
    }
    
    FileEntry parentEntry;
    parentEntry.name = "..";
    parentEntry.fullPath = parentPath;
    parentEntry.isDirectory = true;
    parentEntry.size = 0;
    parentEntry.permissions = "";
    parentEntry.owner = "";
    parentEntry.group = "";
    parentEntry.date = "";
    entries.push_back(parentEntry);
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string name = entry->d_name;
        
        if (name == "." || name == "..") continue;
        
        std::string fullPath = path + "/" + name;
        
        struct stat st;
        if (stat(fullPath.c_str(), &st) != 0) {
            continue;
        }
        
        FileEntry fileEntry;
        fileEntry.name = name;
        fileEntry.fullPath = fullPath;
        fileEntry.size = st.st_size;
        fileEntry.isDirectory = S_ISDIR(st.st_mode);
        
        if (fileEntry.isDirectory) {
            fileEntry.name += "/";
        }
        
        fileEntry.permissions = "";
        fileEntry.permissions += (st.st_mode & S_IRUSR) ? 'r' : '-';
        fileEntry.permissions += (st.st_mode & S_IWUSR) ? 'w' : '-';
        fileEntry.permissions += (st.st_mode & S_IXUSR) ? 'x' : '-';
        fileEntry.permissions += (st.st_mode & S_IRGRP) ? 'r' : '-';
        fileEntry.permissions += (st.st_mode & S_IWGRP) ? 'w' : '-';
        fileEntry.permissions += (st.st_mode & S_IXGRP) ? 'x' : '-';
        fileEntry.permissions += (st.st_mode & S_IROTH) ? 'r' : '-';
        fileEntry.permissions += (st.st_mode & S_IWOTH) ? 'w' : '-';
        fileEntry.permissions += (st.st_mode & S_IXOTH) ? 'x' : '-';
        
        struct passwd* pw = getpwuid(st.st_uid);
        struct group* gr = getgrgid(st.st_gid);
        fileEntry.owner = pw ? pw->pw_name : std::to_string(st.st_uid);
        fileEntry.group = gr ? gr->gr_name : std::to_string(st.st_gid);
        
        char timeBuf[100];
        struct tm* tm_info = localtime(&st.st_mtime);
        strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M", tm_info);
        fileEntry.date = timeBuf;
        
        entries.push_back(fileEntry);
    }
    
    closedir(dir);
    
    std::sort(entries.begin() + 1, entries.end(), [](const FileEntry& a, const FileEntry& b) {
        if (a.isDirectory != b.isDirectory) {
            return a.isDirectory > b.isDirectory;
        }
        std::string aName = a.name;
        std::string bName = b.name;
        std::transform(aName.begin(), aName.end(), aName.begin(), ::tolower);
        std::transform(bName.begin(), bName.end(), bName.begin(), ::tolower);
        return aName < bName;
    });
    
    selectedIndex = 0;
    scrollOffset = 0;
}

bool FileExplorer::isTextFile(const std::string& filepath) {
    size_t dotPos = filepath.rfind('.');
    if (dotPos != std::string::npos) {
        std::string ext = filepath.substr(dotPos);
        std::vector<std::string> textExtensions = {
            ".txt", ".cpp", ".c", ".h", ".hpp", ".cc", ".cxx",
            ".py", ".java", ".js", ".html", ".htm", ".css", ".xml",
            ".json", ".yml", ".yaml", ".md", ".rst", ".tex",
            ".glsl", ".vert", ".frag", ".geom", ".sh", ".bash",
            ".cfg", ".conf", ".ini", ".log", ".csv", ".tsv",
            ".rs", ".go", ".rb", ".php", ".pl", ".lua", ".sql"
        };
        for (const auto& textExt : textExtensions) {
            if (ext == textExt) return true;
        }
    }
    
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) return false;
    
    char buffer[512];
    file.read(buffer, sizeof(buffer));
    std::streamsize bytesRead = file.gcount();
    file.close();
    
    if (bytesRead == 0) return true;
    
    for (int i = 0; i < bytesRead; i++) {
        unsigned char c = buffer[i];
        if (c != '\n' && c != '\r' && c != '\t' && (c < 32 || c > 126)) {
            if (c < 128) {
                return false;
            }
        }
    }
    
    return true;
}

std::string FileExplorer::readFileContent(const std::string& filepath) {
    if (cachedFileContents.find(filepath) != cachedFileContents.end()) {
        return cachedFileContents[filepath];
    }
    
    if (!isTextFile(filepath)) {
        return "Plik binarny - nie można wyświetlić zawartości:\n" + filepath +
               "\n\nPliki binarne (jak obrazy, executables, itp.)\nnie mogą być wyświetlone jako tekst.";
    }
    
    std::ifstream file(filepath);
    if (!file.is_open()) {
        return "Nie można otworzyć pliku: " + filepath;
    }
    
    const std::streamsize MAX_FILE_SIZE = 5 * 1024 * 1024;
    file.seekg(0, std::ios::end);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    if (size > MAX_FILE_SIZE) {
        file.close();
        return "Plik jest zbyt duży do wyświetlenia (>5 MB):\n" + filepath;
    }
    
    std::stringstream bufferStream;
    bufferStream << file.rdbuf();
    std::string content = bufferStream.str();
    file.close();
    
    cachedFileContents[filepath] = content;
    return content;
}

int FileExplorer::countLines(const std::string& content) {
    int lines = 1;
    for (char c : content) {
        if (c == '\n') lines++;
    }
    return lines;
}

std::vector<std::string> FileExplorer::splitLines(const std::string& content) {
    std::vector<std::string> lines;
    std::stringstream ss(content);
    std::string line;
    while (std::getline(ss, line)) {
        lines.push_back(line);
    }
    return lines;
}

void FileExplorer::update() {
}

void FileExplorer::handleKey(int key) {
    if (!viewingFile.empty()) {
        if (key == GLFW_KEY_BACKSPACE || key == GLFW_KEY_TAB || key == GLFW_KEY_ESCAPE) {
            viewingFile = "";
            fileViewerScrollOffset = 0;
            redraw();
        }
        else if (key == GLFW_KEY_UP) {
            if (fileViewerScrollOffset > 0) {
                fileViewerScrollOffset--;
                redrawFileViewer();
            }
        }
        else if (key == GLFW_KEY_DOWN) {
            int maxLines = (texH - (CHAR_H * 3 + 16)) / CHAR_H;
            if (fileViewerScrollOffset + maxLines < totalFileLines) {
                fileViewerScrollOffset++;
                redrawFileViewer();
            }
        }
        return;
    }
    
    if (key == GLFW_KEY_UP) {
        if (selectedIndex > 0) selectedIndex--;
        if (selectedIndex < scrollOffset) scrollOffset--;
        redraw();
    }
    else if (key == GLFW_KEY_DOWN) {
        if (selectedIndex < (int)entries.size() - 1) selectedIndex++;
        if (selectedIndex >= scrollOffset + visibleLines) scrollOffset++;
        redraw();
    }
    else if (key == GLFW_KEY_ENTER || key == GLFW_KEY_KP_ENTER) {
        if (selectedIndex >= 0 && selectedIndex < (int)entries.size()) {
            const FileEntry& selected = entries[selectedIndex];
            
            if (selected.isDirectory) {
                currentPath = selected.fullPath;
                listDir(currentPath);
                redraw();
            } else {
                viewingFile = selected.fullPath;
                fileViewerScrollOffset = 0;
                redrawFileViewer();
            }
        }
    }
}

void FileExplorer::init(const char* fontPath, int texWidth, int texHeight) {
    texW = texWidth;
    texH = texHeight;
    visibleLines = texH / CHAR_H - 3;

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
    
    stbtt_BakeFontBitmap(ttfBuf, 0, FONT_SIZE, fontBitmap, bitmapW, bitmapH, 32, 96, (stbtt_bakedchar*)charData);
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
    int originalX = x;
    for (char c : text) {
        if (c == '\n') {
            x = originalX;
            y += CHAR_H;
        } else {
            drawChar(x, y, c, r, g, b);
        }
    }
}

void FileExplorer::redraw() {
    clearPixels();

    drawText(8, 4, "=== PLIKI ===", 100, 200, 255);
    drawText(8, 4 + CHAR_H, "Path: " + currentPath, 100, 200, 255);

    int separatorY = CHAR_H * 2 + 4;
    for (int x = 0; x < texW; x++) {
        int idx = (separatorY * texW + x) * 4;
        pixels[idx+0] = 80;
        pixels[idx+1] = 80;
        pixels[idx+2] = 120;
        pixels[idx+3] = 255;
    }

    for (int i = 0; i < visibleLines; i++) {
        int entIdx = scrollOffset + i;
        if (entIdx >= (int)entries.size()) break;

        int y = (i + 3) * CHAR_H + 4;
        bool isSelected = (entIdx == selectedIndex);
        const FileEntry& entry = entries[entIdx];

        if (isSelected) {
            for (int x = 0; x < texW; x++) {
                int idx = ((y + 2) * texW + x) * 4;
                pixels[idx+0] = 50;
                pixels[idx+1] = 50;
                pixels[idx+2] = 90;
                pixels[idx+3] = 255;
            }
            drawText(8, y, entry.name, 255, 255, 100);
        } 
        else if (entry.isDirectory) {
            drawText(8, y, entry.name, 100, 180, 255);
        } 
        else {
            drawText(8, y, entry.name, 200, 200, 200);
        }
    }

    glBindTexture(GL_TEXTURE_2D, screenTex);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texW, texH, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
}

void FileExplorer::updateChart() {
    chart.updateData(entries, this);
}

void FileExplorer::redrawFileViewer() {
    clearPixels();
    
    std::string content = readFileContent(viewingFile);
    
    drawText(8, 4, "=== PODGLAD PLIKU ===", 100, 200, 255);
    
    std::string displayFilename = viewingFile;
    int maxFilenameLen = (texW / 10) - 5;
    if ((int)displayFilename.length() > maxFilenameLen && maxFilenameLen > 10) {
        displayFilename = "..." + displayFilename.substr(displayFilename.length() - maxFilenameLen + 3);
    }
    drawText(8, 4 + CHAR_H, "Plik: " + displayFilename, 100, 200, 255);
    drawText(8, 4 + CHAR_H * 2, "[BACKSPACE/TAB/ESC] - Zamknij | [UP/DOWN] - Przewijaj", 150, 150, 150);
    
    int separatorY = CHAR_H * 3 + 4;
    for (int x = 0; x < texW; x++) {
        int idx = (separatorY * texW + x) * 4;
        pixels[idx+0] = 80;
        pixels[idx+1] = 80;
        pixels[idx+2] = 120;
        pixels[idx+3] = 255;
    }
    
    std::vector<std::string> wrappedLines;
    int maxCharsPerLine = texW / 9;
    
    std::stringstream ss(content);
    std::string line;
    while (std::getline(ss, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        
        while ((int)line.length() > maxCharsPerLine) {
            int splitPos = maxCharsPerLine;
            for (int i = maxCharsPerLine; i > maxCharsPerLine / 2; i--) {
                if (line[i] == ' ' || line[i] == '\t') {
                    splitPos = i + 1;
                    break;
                }
            }
            wrappedLines.push_back(line.substr(0, splitPos));
            line = line.substr(splitPos);
        }
        if (!line.empty() || (int)line.length() == 0) {
            wrappedLines.push_back(line);
        }
    }
    
    totalFileLines = wrappedLines.size();
    
    int startY = separatorY + CHAR_H;
    int maxLines = (texH - startY) / CHAR_H;
    
    for (int i = 0; i < maxLines && (fileViewerScrollOffset + i) < totalFileLines; i++) {
        int lineNum = fileViewerScrollOffset + i;
        std::string line = wrappedLines[lineNum];
        
        std::string lineNumStr = std::to_string(lineNum + 1) + ":";
        drawText(8, startY + i * CHAR_H, lineNumStr, 150, 150, 150);
        
        int maxContentChars = texW / 9 - (int)lineNumStr.length() - 1;
        if (maxContentChars < 10) maxContentChars = 10;
        if ((int)line.length() > maxContentChars) {
            line = line.substr(0, maxContentChars - 3) + "...";
        }
        drawText(8 + (int)lineNumStr.length() * 9 + 4, startY + i * CHAR_H, line, 200, 200, 200);
    }
    
    if (totalFileLines == 0) {
        drawText(8, startY, "(pusty plik)", 150, 150, 150);
    }
    
    if (totalFileLines > maxLines) {
        std::string scrollInfo = "Linie: " + std::to_string(fileViewerScrollOffset + 1) + 
                                 " - " + std::to_string(std::min(fileViewerScrollOffset + maxLines, totalFileLines)) +
                                 " / " + std::to_string(totalFileLines);
        drawText(8, texH - CHAR_H, scrollInfo, 100, 200, 255);
    }
    
    glBindTexture(GL_TEXTURE_2D, screenTex);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texW, texH, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
}

void FileExplorer::getTextDimensions(const std::string& text, int& width, int& height) {
    width = 0;
    height = CHAR_H;
    for (char c : text) {
        if (c >= 32 && c <= 127) {
            stbtt_bakedchar* bc = &((stbtt_bakedchar*)charData)[c - 32];
            width += (int)bc->xadvance;
        }
    }
    width += 10;
}

void FileExplorer::renderTextToBuffer(const std::string& text, unsigned char* buffer, int bufWidth, int bufHeight) {
    memset(buffer, 0, bufWidth * bufHeight * 4);
    
    int x = 5;
    int y = 5;
    
    for (char c : text) {
        if (c >= 32 && c <= 127) {
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
                    if (px < 0 || px >= bufWidth || py < 0 || py >= bufHeight) continue;
                    unsigned char alpha = fontBitmap[(y0 + row) * bitmapW + (x0 + col)];
                    if (alpha == 0) continue;
                    int idx = (py * bufWidth + px) * 4;
                    buffer[idx+0] = 255;
                    buffer[idx+1] = 255;
                    buffer[idx+2] = 255;
                    buffer[idx+3] = alpha;
                }
            }
            x += (int)bc->xadvance;
        }
    }
}

FileChart::FileChart() : vao(0), vbo(0), barCount(0), maxSize(1) {}

FileChart::~FileChart() {
    cleanup();
}

void FileChart::cleanup() {
    if (vbo) glDeleteBuffers(1, &vbo);
    if (vao) glDeleteVertexArrays(1, &vao);
    vao = vbo = 0;
}

void FileChart::updateData(const std::vector<FileEntry>& entries, FileExplorer* explorer) {
   for (auto& bar : bars) {
        if (bar.labelTex) {
            glDeleteTextures(1, &bar.labelTex);
        }
    }
    bars.clear();
    
    maxSize = 1;
    for (const auto& e : entries) {
        if (!e.isDirectory && e.size > maxSize) {
            maxSize = e.size;
        }
    }
    
    std::vector<FileEntry> files;
    for (const auto& e : entries) {
        if (e.name != ".." && !e.isDirectory && e.size > 0) {
            files.push_back(e);
        }
    }
    
    int fileCount = files.size();
    if (fileCount == 0) return;
    
    float startX = -14.0f;
    float endX = 14.0f;
    float stepX = (endX - startX) / (fileCount - 1);
    float baseY = 6.0f;
    float zPos = 0.0f;
    
    std::vector<glm::vec3> colors = {
        glm::vec3(1.0f, 0.2f, 0.2f),
        glm::vec3(0.2f, 1.0f, 0.2f),
        glm::vec3(0.2f, 0.2f, 1.0f),
        glm::vec3(1.0f, 1.0f, 0.2f),
        glm::vec3(1.0f, 0.2f, 1.0f),
        glm::vec3(0.2f, 1.0f, 1.0f),
        glm::vec3(1.0f, 0.5f, 0.0f),
        glm::vec3(0.5f, 0.0f, 1.0f),
        glm::vec3(0.0f, 0.8f, 0.4f),
        glm::vec3(0.8f, 0.4f, 0.0f),
        glm::vec3(0.8f, 0.0f, 0.8f),
        glm::vec3(0.0f, 0.8f, 0.8f),
        glm::vec3(0.8f, 0.8f, 0.0f),
        glm::vec3(1.0f, 0.3f, 0.6f),
        glm::vec3(0.6f, 0.3f, 1.0f)
    };
    
    for (int i = 0; i < fileCount; i++) {
        const auto& e = files[i];
        FileBar bar;
        bar.name = e.name;
        bar.size = e.size;
        bar.height = 0.5f + (float)e.size / maxSize * 3.5f;
        bar.color = colors[i % colors.size()];
        bar.position = glm::vec3(
            startX + i * stepX,
            baseY + bar.height / 2.0f,
            zPos
        );
        
        bar.labelTex = 0;
        bar.labelWidth = 0;
        bar.labelHeight = 0;
        
        if (explorer) {
            explorer->getTextDimensions(bar.name, bar.labelWidth, bar.labelHeight);
            
            int texW = bar.labelWidth;
            int texH = bar.labelHeight;
            if (texW < 10) texW = 10;
            if (texH < 10) texH = 10;
            
            texW += 10;
            texH += 6;
            
            glGenTextures(1, &bar.labelTex);
            glBindTexture(GL_TEXTURE_2D, bar.labelTex);
            
            unsigned char* buffer = new unsigned char[texW * texH * 4];
            memset(buffer, 0, texW * texH * 4);
            
            int oldX = 5;
            int oldY = 3; 
            
            void* charData = explorer->getCharData();
            unsigned char* fontBitmap = explorer->getFontBitmap();
            int bitmapW = explorer->getBitmapW();
            
            for (char c : bar.name) {
                if (c >= 32 && c <= 127) {
                    stbtt_bakedchar* bc = &((stbtt_bakedchar*)charData)[c - 32];
                    
                    int x0 = (int)bc->x0, y0 = (int)bc->y0;
                    int x1 = (int)bc->x1, y1 = (int)bc->y1;
                    int cw = x1 - x0;
                    int ch = y1 - y0;
                    
                    int destX = oldX + (int)bc->xoff;
                    int destY = oldY + (int)bc->yoff + FONT_SIZE;
                    
                    for (int row = 0; row < ch; row++) {
                        for (int col = 0; col < cw; col++) {
                            int px = destX + col;
                            int py = destY + row;
                            if (px < 0 || px >= texW || py < 0 || py >= texH) continue;
                            unsigned char alpha = fontBitmap[(y0 + row) * bitmapW + (x0 + col)];
                            if (alpha == 0) continue;
                            int idx = (py * texW + px) * 4;
                            buffer[idx+0] = 255;
                            buffer[idx+1] = 255;
                            buffer[idx+2] = 255;
                            buffer[idx+3] = alpha;
                        }
                    }
                    oldX += (int)bc->xadvance;
                }
            }
            
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texW, texH, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            
            delete[] buffer;
        }
        
        bars.push_back(bar);
    }
    cleanup();
    
    std::vector<float> vertexData;
    
    for (const auto& bar : bars) {
        float w = 0.6f;
        float h = bar.height;
        float d = 0.6f;
        float x = bar.position.x;
        float y = bar.position.y - h/2.0f;
        float z = bar.position.z;
        
        glm::vec3 vertices[8] = {
            glm::vec3(x - w/2, y,     z - d/2),
            glm::vec3(x + w/2, y,     z - d/2),
            glm::vec3(x + w/2, y,     z + d/2),
            glm::vec3(x - w/2, y,     z + d/2),
            glm::vec3(x - w/2, y + h, z - d/2),
            glm::vec3(x + w/2, y + h, z - d/2),
            glm::vec3(x + w/2, y + h, z + d/2),
            glm::vec3(x - w/2, y + h, z + d/2)
        };
        
        int indices[] = {
            0,1,2, 0,2,3,
            4,6,5, 4,7,6,
            0,4,1, 1,4,5,
            2,6,3, 3,6,7,
            0,3,4, 3,7,4,
            1,5,2, 2,5,6
        };
        
        for (int idx : indices) {
            vertexData.push_back(vertices[idx].x);
            vertexData.push_back(vertices[idx].y);
            vertexData.push_back(vertices[idx].z);
            vertexData.push_back(bar.color.r);
            vertexData.push_back(bar.color.g);
            vertexData.push_back(bar.color.b);
        }
    }
    
    barCount = vertexData.size() / 6;
    
    if (barCount > 0) {
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), vertexData.data(), GL_STATIC_DRAW);
        
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        
        glBindVertexArray(0);
    }
}

void FileChart::draw(glm::mat4 P, glm::mat4 V, glm::vec3 centerPos, glm::vec3 cameraPos) {
    if (barCount == 0) return;
    
    glm::mat4 M = glm::translate(glm::mat4(1.0f), centerPos);
    
    if (spLambert) {
        spLambert->use();
        glUniformMatrix4fv(spLambert->u("P"), 1, false, glm::value_ptr(P));
        glUniformMatrix4fv(spLambert->u("V"), 1, false, glm::value_ptr(V));
        glUniformMatrix4fv(spLambert->u("M"), 1, false, glm::value_ptr(M));
        
        glUniform4f(spLambert->u("lightDir"), 0.5f, 1.0f, 0.3f, 0.0f);
        glUniform4f(spLambert->u("color"), 1.0f, 1.0f, 1.0f, 1.0f);
        glUniform1i(spLambert->u("shadowMap"), 0);
        
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, barCount);
        glBindVertexArray(0);
    }
    
    if (spLabel) {
        spLabel->use();
        glUniformMatrix4fv(spLabel->u("P"), 1, false, glm::value_ptr(P));
        glUniformMatrix4fv(spLabel->u("V"), 1, false, glm::value_ptr(V));
        
        glActiveTexture(GL_TEXTURE3);
        
        for (const auto& bar : bars) {
            if (bar.labelTex == 0) continue;
            
            float labelW = 1.4f;
            float labelH = 0.45f;
            glm::vec3 labelPos = centerPos + glm::vec3(
                bar.position.x, 
                bar.position.y + bar.height/2.0f + 0.7f, 
                bar.position.z
            );

            glm::vec3 look = glm::normalize(cameraPos - labelPos);
            glm::vec3 right = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), look));
            glm::vec3 up = glm::cross(look, right);
            
            glm::mat4 labelM(1.0f);
            labelM[0] = glm::vec4(right, 0.0f);
            labelM[1] = glm::vec4(up, 0.0f);
            labelM[2] = glm::vec4(look, 0.0f);
            labelM[3] = glm::vec4(labelPos, 1.0f);
            labelM = glm::scale(labelM, glm::vec3(labelW, labelH, 1.0f));
            
            glUniformMatrix4fv(spLabel->u("M"), 1, false, glm::value_ptr(labelM));
            glBindTexture(GL_TEXTURE_2D, bar.labelTex);
            glUniform1i(spLabel->u("tex"), 3);
            
            extern GLuint panelVAO;
            glBindVertexArray(panelVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            glBindVertexArray(0);
        }
        
        glActiveTexture(GL_TEXTURE0);
    }
}