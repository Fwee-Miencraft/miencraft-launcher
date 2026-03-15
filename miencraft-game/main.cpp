#include <iostream>
#include <cmath>
#include <vector>
#include <unordered_map>
#include <string>
#include <sstream>
#include <tuple>
#include <random>
#include <cstdint>
#include <future>
#include <atomic>
#include <queue>
#include <algorithm>

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

// GLAD – must be included before any OpenGL calls
#include "glad/glad.h"

// GLM for matrices and vectors
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace std;

// ─── Globals ────────────────────────────────────────────────────────────────

float playerX = 0.0f;
float playerY = 10.0f;
float playerZ = 5.0f;

float yaw   = -90.0f;   // starts looking along -Z
float pitch = 0.0f;

float speed = 10.0f;
float mouseSensitivity = 0.15f;

float breakCooldown = 0.0f;
const float BREAK_COOLDOWN_TIME = 0.3f;  // seconds

GLuint hudShaderProgram = 0;
GLuint hotbarVAO = 0;
GLuint hotbarVBO = 0;
GLint hotbarTextureID = 0;

unordered_map<string, string> worldBlocks;
unordered_map<string, GLuint> Textures;
mutex worldBlocksMutex;
SDL_AudioStream* BackGroundMusic = nullptr;
Uint8* musicBuffer = nullptr;
Uint32 musicLength = 0;

vector<tuple<string, string, string>> TextureAtlas = {
    {"grass_top.png", "grass.png",     "dirt.png"},
    {"dirt.png",      "dirt.png",      "dirt.png"},
    {"stone.png",     "stone.png",     "stone.png"},
    {"log_top.png", "wood.png", "log_top.png"},
    {"leaves.png", "leaves.png", "leaves.png"},
    {"error.png",     "error.png",     "error.png"}
};

unordered_map<string, int> KeyMapper = {
    {"error", 0},
    {"grass", 1},
    {"dirt",  2},
    {"stone", 3},
    {"wood", 4},
    {"leaves", 5}
};

mt19937 rng;

queue<pair<int,int>> chunkQueue;

vector<string> playlist = {
    "Assets/sound/minecraft.wav",
    "Assets/sound/c418_subwoofer_lullaby.wav",
    "Assets/sound/mice_on_venus.wav",
    "Assets/sound/c418_living_mice.wav"
};

int currentSong = 0;

SDL_AudioDeviceID device;

// ─── Helper Functions ───────────────────────────────────────────────────────


bool PlayMusic(SDL_AudioDeviceID device, const char* file)
{
    SDL_AudioSpec spec;
    Uint8* buffer;
    Uint32 length;

    if (!SDL_LoadWAV(file, &spec, &buffer, &length))
    {
        cout << "Failed to load WAV: " << SDL_GetError() << endl;
        return false;
    }

    BackGroundMusic = SDL_CreateAudioStream(&spec, &spec);

    if (!BackGroundMusic)
    {
        cout << "Stream failed: " << SDL_GetError() << endl;
        return false;
    }

    SDL_PutAudioStreamData(BackGroundMusic, buffer, length);

    SDL_BindAudioStream(device, BackGroundMusic);

    SDL_free(buffer);

    return true;
}

bool LoadSong(const string& file, SDL_AudioSpec& spec)
{
    if (musicBuffer)
        SDL_free(musicBuffer);

    if (!SDL_LoadWAV(file.c_str(), &spec, &musicBuffer, &musicLength))
    {
        cout << "Failed to load: " << SDL_GetError() << endl;
        return false;
    }

    return true;
}

bool PlayNextSong(SDL_AudioDeviceID device, SDL_AudioSpec& spec)
{
    currentSong++;

    if (currentSong >= playlist.size())
        currentSong = 0;

    if (!LoadSong(playlist[currentSong], spec))
        return false;

    SDL_ClearAudioStream(BackGroundMusic);
    SDL_PutAudioStreamData(BackGroundMusic, musicBuffer, musicLength);

    return true;
}

string posKey(int x, int y, int z) {
    return to_string(x) + "_" + to_string(y) + "_" + to_string(z);
}

bool isSolid(int x, int y, int z) {
    auto it = worldBlocks.find(posKey(x, y, z));
    return (it != worldBlocks.end()) && (it->second != "air");
}

tuple<int,int,int> ParseCoords(const string& str) {
    stringstream ss(str);
    string token;
    int x,y,z;
    getline(ss, token, '_'); x = stoi(token);
    getline(ss, token, '_'); y = stoi(token);
    getline(ss, token, '_'); z = stoi(token);
    return {x, y, z};
}

tuple<string,string,string> Find_tuple(const string& name) {
    auto it = KeyMapper.find(name);
    if (it == KeyMapper.end()) {
        cout << "Unknown block: " << name << endl;
        return {"grass_top.png", "grass.png", "dirt.png"};
    }

    int target = it->second;  // 1=grass, 2=dirt, 3=stone

    int idx = 1;  // ← start at 1 to skip error entry at 0
    for (const auto& tup : TextureAtlas) {
        if (idx == target) return tup;
        idx++;
    }

    cout << "Index not found for " << name << " (value=" << target << ")" << endl;
    return {"grass_top.png", "grass.png", "dirt.png"};
}

// ─── Texture Loading ────────────────────────────────────────────────────────

GLuint LoadTexture(const string& filename) {
    string fullpath = string(SDL_GetBasePath()) + "Assets/textures/" + filename;
    SDL_Surface* raw = IMG_Load(fullpath.c_str());
    if (!raw) {
        cout << "Failed to load " << filename << ": " << SDL_GetError() << endl;
        raw = IMG_Load((string(SDL_GetBasePath()) + "Assets/error.png").c_str());
    }

    SDL_Surface* converted = SDL_ConvertSurface(raw, SDL_PIXELFORMAT_RGBA32);
    SDL_DestroySurface(raw);

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, converted->w, converted->h, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, converted->pixels);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    SDL_DestroySurface(converted);

    cout << "Loaded texture '" << filename << "' as ID " << tex << endl;

    return tex;
}

// ─── Shader ─────────────────────────────────────────────────────────────────

GLuint shaderProgram;

void createShader() {
    const char* vs = R"(
    #version 330 core
    layout(location = 0) in vec3 aPos;
    layout(location = 1) in vec2 aTex;
    uniform mat4 uMVP;
    out vec2 TexCoord;
    void main() {
        gl_Position = uMVP * vec4(aPos, 1.0);
        TexCoord = aTex;
    }
    )";

    const char* fs = R"(
    #version 330 core
    in vec2 TexCoord;
    out vec4 FragColor;
    uniform sampler2D uTexture;
    void main() {
        FragColor = texture(uTexture, TexCoord);
    }
    )";

    GLuint vshader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vshader, 1, &vs, nullptr);
    glCompileShader(vshader);

    GLuint fshader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fshader, 1, &fs, nullptr);
    glCompileShader(fshader);

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vshader);
    glAttachShader(shaderProgram, fshader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vshader);
    glDeleteShader(fshader);
}

void createHudShader() {
    const char* vs = R"(
    #version 330 core
    layout(location = 0) in vec2 aPos;
    layout(location = 1) in vec2 aTex;
    uniform mat4 uOrtho;
    out vec2 TexCoord;
    void main() {
        gl_Position = uOrtho * vec4(aPos, 0.0, 1.0);
        TexCoord = aTex;
    }
    )";

    const char* fs = R"(
    #version 330 core
    in vec2 TexCoord;
    out vec4 FragColor;
    uniform sampler2D uTexture;
    void main() {
        FragColor = texture(uTexture, TexCoord);
    }
    )";

    GLuint vshader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vshader, 1, &vs, nullptr);
    glCompileShader(vshader);

    GLuint fshader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fshader, 1, &fs, nullptr);
    glCompileShader(fshader);

    hudShaderProgram = glCreateProgram();
    glAttachShader(hudShaderProgram, vshader);
    glAttachShader(hudShaderProgram, fshader);
    glLinkProgram(hudShaderProgram);

    glDeleteShader(vshader);
    glDeleteShader(fshader);
}

// ─── Chunk & Mesh ───────────────────────────────────────────────────────────

struct Vertex { float x,y,z, u,v; };

struct Chunk {
    int cx, cz;
    unordered_map<GLuint, GLuint> vaos;
    unordered_map<GLuint, GLuint> vbos;
    unordered_map<GLuint, GLsizei> counts;
    bool dirty = true;

    // Add this constructor
    Chunk(int cx_, int cz_) : cx(cx_), cz(cz_), dirty(true) {
        // vaos/vbos/counts are default-constructed (empty)
    }

    // Optional: default constructor if needed elsewhere
    Chunk() = default;
};
unordered_map<string, Chunk> chunks;

string chunkKey(int cx, int cz) {
    return to_string(cx) + "_" + to_string(cz);
}

void buildChunkMesh(Chunk& chunk) {
    // Clean up old data
    for (auto& p : chunk.vaos) glDeleteVertexArrays(1, &p.second);
    for (auto& p : chunk.vbos) glDeleteBuffers(1, &p.second);
    chunk.vaos.clear();
    chunk.vbos.clear();
    chunk.counts.clear();

    unordered_map<GLuint, vector<Vertex>> vertexGroups;

    for (int lx = 0; lx < 16; ++lx) {
        for (int ly = -30; ly <= 30; ++ly)  {
            for (int lz = 0; lz < 16; ++lz) {
                int wx = chunk.cx * 16 + lx;
                int wy = ly;
                int wz = chunk.cz * 16 + lz;

                string key = posKey(wx, wy, wz);
                if (worldBlocks.find(key) == worldBlocks.end()) continue;

                string type = worldBlocks[key];
                auto [topF, sideF, botF] = Find_tuple(type);

                GLuint topTex  = Textures[topF];
                GLuint sideTex = Textures[sideF];
                GLuint botTex  = Textures[botF];

                // Front (+Z) - use side texture
                if (!isSolid(wx, wy, wz + 1)) {
                    vertexGroups[sideTex].push_back({wx-0.5f, wy-0.5f, wz+0.5f, 0,1});
                    vertexGroups[sideTex].push_back({wx+0.5f, wy-0.5f, wz+0.5f, 1,1});
                    vertexGroups[sideTex].push_back({wx+0.5f, wy+0.5f, wz+0.5f, 1,0});
                    vertexGroups[sideTex].push_back({wx-0.5f, wy-0.5f, wz+0.5f, 0,1});
                    vertexGroups[sideTex].push_back({wx+0.5f, wy+0.5f, wz+0.5f, 1,0});
                    vertexGroups[sideTex].push_back({wx-0.5f, wy+0.5f, wz+0.5f, 0,0});
                }

                // Back (-Z) - side texture
                if (!isSolid(wx, wy, wz - 1)) {
                    vertexGroups[sideTex].push_back({wx+0.5f, wy-0.5f, wz-0.5f, 0,1});
                    vertexGroups[sideTex].push_back({wx-0.5f, wy-0.5f, wz-0.5f, 1,1});
                    vertexGroups[sideTex].push_back({wx-0.5f, wy+0.5f, wz-0.5f, 1,0});
                    vertexGroups[sideTex].push_back({wx+0.5f, wy-0.5f, wz-0.5f, 0,1});
                    vertexGroups[sideTex].push_back({wx-0.5f, wy+0.5f, wz-0.5f, 1,0});
                    vertexGroups[sideTex].push_back({wx+0.5f, wy+0.5f, wz-0.5f, 0,0});
                }

                // Left (-X) - side texture
                if (!isSolid(wx - 1, wy, wz)) {
                    vertexGroups[sideTex].push_back({wx-0.5f, wy-0.5f, wz-0.5f, 0,1});
                    vertexGroups[sideTex].push_back({wx-0.5f, wy-0.5f, wz+0.5f, 1,1});
                    vertexGroups[sideTex].push_back({wx-0.5f, wy+0.5f, wz+0.5f, 1,0});
                    vertexGroups[sideTex].push_back({wx-0.5f, wy-0.5f, wz-0.5f, 0,1});
                    vertexGroups[sideTex].push_back({wx-0.5f, wy+0.5f, wz+0.5f, 1,0});
                    vertexGroups[sideTex].push_back({wx-0.5f, wy+0.5f, wz-0.5f, 0,0});
                }

                // Right (+X) - side texture
                if (!isSolid(wx + 1, wy, wz)) {
                    vertexGroups[sideTex].push_back({wx+0.5f, wy-0.5f, wz+0.5f, 0,1});
                    vertexGroups[sideTex].push_back({wx+0.5f, wy-0.5f, wz-0.5f, 1,1});
                    vertexGroups[sideTex].push_back({wx+0.5f, wy+0.5f, wz-0.5f, 1,0});
                    vertexGroups[sideTex].push_back({wx+0.5f, wy-0.5f, wz+0.5f, 0,1});
                    vertexGroups[sideTex].push_back({wx+0.5f, wy+0.5f, wz-0.5f, 1,0});
                    vertexGroups[sideTex].push_back({wx+0.5f, wy+0.5f, wz+0.5f, 0,0});
                }

                // Top (+Y) - top texture
                if (!isSolid(wx, wy + 1, wz)) {
                    vertexGroups[topTex].push_back({wx-0.5f, wy+0.5f, wz+0.5f, 0,1});
                    vertexGroups[topTex].push_back({wx+0.5f, wy+0.5f, wz+0.5f, 1,1});
                    vertexGroups[topTex].push_back({wx+0.5f, wy+0.5f, wz-0.5f, 1,0});
                    vertexGroups[topTex].push_back({wx-0.5f, wy+0.5f, wz+0.5f, 0,1});
                    vertexGroups[topTex].push_back({wx+0.5f, wy+0.5f, wz-0.5f, 1,0});
                    vertexGroups[topTex].push_back({wx-0.5f, wy+0.5f, wz-0.5f, 0,0});
                }

                // Bottom (-Y) - bottom texture
                if (!isSolid(wx, wy - 1, wz)) {
                    //cout << "Bottom face of " << type << " at (" << wx << "," << wy << "," << wz << ") using texture ID " << botTex << endl;
                    vertexGroups[botTex].push_back({wx-0.5f, wy-0.5f, wz-0.5f, 0,1});
                    vertexGroups[botTex].push_back({wx+0.5f, wy-0.5f, wz-0.5f, 1,1});
                    vertexGroups[botTex].push_back({wx+0.5f, wy-0.5f, wz+0.5f, 1,0});
                    vertexGroups[botTex].push_back({wx-0.5f, wy-0.5f, wz-0.5f, 0,1});
                    vertexGroups[botTex].push_back({wx+0.5f, wy-0.5f, wz+0.5f, 1,0});
                    vertexGroups[botTex].push_back({wx-0.5f, wy-0.5f, wz+0.5f, 0,0});
                }
            }
        }
    }

    // Create one VAO/VBO per texture group
    for (auto& [texID, verts] : vertexGroups) {
        if (verts.empty()) continue;

        GLuint vao, vbo;
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(Vertex), verts.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3*sizeof(float)));
        glEnableVertexAttribArray(1);

        chunk.vaos[texID] = vao;
        chunk.vbos[texID] = vbo;
        chunk.counts[texID] = verts.size();
    }

    chunk.dirty = false;
}

class Perlin {
private:
    vector<int> p;  // permutation table

    float fade(float t) const { return t * t * t * (t * (t * 6 - 15) + 10); }
    float lerp(float t, float a, float b) const { return a + t * (b - a); }
    float grad(int hash, float x, float y) const {
        int h = hash & 15;
        float u = h < 8 ? x : y;
        float v = h < 4 ? y : h == 12 || h == 14 ? x : 0;
        return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
    }

public:
    Perlin(unsigned int seed = 0) {
        p.resize(512);
        vector<int> permutation = {
            151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,
            8,99,37,240,21,10,23,190,6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,
            35,11,32,57,177,33,88,237,149,56,87,174,20,125,136,171,168,68,175,74,165,71,
            134,139,48,27,166,77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,
            55,46,245,40,244,102,143,54,65,25,63,161,1,216,80,73,209,76,132,187,208,89,
            18,169,200,196,135,130,116,188,159,86,164,100,109,198,173,186,3,64,52,217,226,
            250,124,123,5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,
            189,28,42,223,183,170,213,119,248,152,2,44,154,163,70,221,153,101,155,167,43,
            172,9,129,22,39,253,19,98,108,110,79,113,224,232,178,185,112,104,218,246,97,
            228,251,34,242,193,238,210,144,12,191,179,162,241,81,51,145,235,249,14,239,
            107,49,192,214,31,181,199,106,157,184,84,204,176,115,121,50,45,127,4,150,254,
            138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180
        };

        // Shuffle with seed
        mt19937 rng(seed);
        shuffle(permutation.begin(), permutation.end(), rng);

        for (int i = 0; i < 256; ++i) {
            p[256 + i] = p[i] = permutation[i];
        }
    }

    float noise(float x, float y) const {
        int X = static_cast<int>(floor(x)) & 255;
        int Y = static_cast<int>(floor(y)) & 255;

        x -= floor(x);
        y -= floor(y);

        float u = fade(x);
        float v = fade(y);

        int A = p[X] + Y, AA = p[A], AB = p[A + 1];
        int B = p[X + 1] + Y, BA = p[B], BB = p[B + 1];

        return lerp(v,
            lerp(u, grad(p[AA], x, y), grad(p[BA], x-1, y)),
            lerp(u, grad(p[AB], x, y-1), grad(p[BB], x-1, y-1))
        );
    }

    // Fractional Brownian Motion (multiple octaves for detail)
    float fbm(float x, float y, int octaves = 6, float persistence = 0.5f) const {
        float total = 0;
        float frequency = 1.0f;
        float amplitude = 1.0f;
        float maxValue = 0;

        for (int i = 0; i < octaves; ++i) {
            total += noise(x * frequency, y * frequency) * amplitude;
            maxValue += amplitude;
            amplitude *= persistence;
            frequency *= 2.0f;
        }

        return total / maxValue;
    }
};

// ─── Chunk Generation ──────────────────────────────────────────────────────

void AddBlock(int x, int y, int z, string type, bool Overwrite = false) {
    string key = posKey(x, y, z);

    lock_guard<mutex> lock(worldBlocksMutex);  // protect map

    if (Overwrite || worldBlocks.count(key) == 0) {
        worldBlocks[key] = type;

        // Your existing dirty marking...
        int cx = x / 16;
        int cz = z / 16;

        for (int dx = -1; dx <= 1; ++dx) {
            for (int dz = -1; dz <= 1; ++dz) {
                string nkey = chunkKey(cx + dx, cz + dz);
                auto it = chunks.find(nkey);
                if (it != chunks.end()) {
                    it->second.dirty = true;
                }
            }
        }
    }
}

void GenerateTreeBaseAt(int x, int y, int z){
    int height = SDL_rand(2) + 1;
    AddBlock(x, y + height - 2, z, "wood", false);
    AddBlock(x, y + height - 1, z, "wood", false);
    AddBlock(x, y + height, z, "wood", true);
    AddBlock(x, y + height + 1, z, "wood", true);
    AddBlock(x, y + height + 2, z, "wood", true);
    AddBlock(x, y + height + 3, z, "wood", true);
    AddBlock(x, y + height + 4, z, "leaves", true);
    AddBlock(x, y + height + 3, z + 1, "leaves", true);
    AddBlock(x, y + height + 3, z - 1, "leaves", true);
    AddBlock(x + 1, y + height + 3, z, "leaves", true);
    AddBlock(x - 1, y + height + 3, z, "leaves", true);
    AddBlock(x, y + height + 2, z + 1, "leaves", true);
    AddBlock(x, y + height + 2, z - 1, "leaves", true);
    AddBlock(x + 1, y + height + 2, z, "leaves", true);
    AddBlock(x - 1, y + height + 2, z, "leaves", true);
    AddBlock(x + 1, y + height + 2, z + 1, "leaves", true);
    AddBlock(x + 1, y + height + 2, z - 1, "leaves", true);
    AddBlock(x - 1, y + height + 2, z - 1, "leaves", true);
    AddBlock(x - 1, y + height + 2, z + 1, "leaves", true);
    AddBlock(x, y + height + 1, z + 1, "leaves", true);
    AddBlock(x, y + height + 1, z - 1, "leaves", true);
    AddBlock(x + 1, y + height + 1, z, "leaves", true);
    AddBlock(x - 1, y + height + 1, z, "leaves", true);
    AddBlock(x + 1, y + height + 1, z + 1, "leaves", true);
    AddBlock(x + 1, y + height + 1, z - 1, "leaves", true);
    AddBlock(x - 1, y + height + 1, z - 1, "leaves", true);
    AddBlock(x - 1, y + height + 1, z + 1, "leaves", true);
    AddBlock(x, y + height + 1, z + 1, "leaves", true);
    AddBlock(x, y + height + 1, z - 1, "leaves", true);
    AddBlock(x + 2, y + height + 1, z, "leaves", true);
    AddBlock(x - 2, y + height + 1, z, "leaves", true);
    AddBlock(x + 2, y + height + 1, z + 2, "leaves", true);
    AddBlock(x + 2, y + height + 1, z - 2, "leaves", true);
    AddBlock(x - 2, y + height + 1, z - 2, "leaves", true);
    AddBlock(x - 2, y + height + 1, z + 2, "leaves", true);
    AddBlock(x + 1, y + height + 1, z + 2, "leaves", true);
    AddBlock(x + 1, y + height + 1, z - 2, "leaves", true);
    AddBlock(x - 1, y + height + 1, z - 2, "leaves", true);
    AddBlock(x - 1, y + height + 1, z + 2, "leaves", true);
    AddBlock(x + 2, y + height + 1, z + 1, "leaves", true);
    AddBlock(x + 2, y + height + 1, z - 1, "leaves", true);
    AddBlock(x - 2, y + height + 1, z - 1, "leaves", true);
    AddBlock(x - 2, y + height + 1, z + 1, "leaves", true);
    AddBlock(x, y + height + 1, z - 2, "leaves", true);
    AddBlock(x, y + height + 1, z + 2, "leaves", true);
}

void AddLotsOfBlocks(int startX, int startY, int startZ, int len, int height, int width, string type) {
    for (int dz = 0; dz < len; ++dz) {
        for (int dy = 0; dy < height; ++dy) {
            for (int dx = 0; dx < width; ++dx) {
                int x = startX + dx;
                int y = startY - dy;
                int z = startZ + dz;
                AddBlock(x, y, z, type, false);
            }
        }
    }
}

uint64_t hash_coords(int x, int z, uint64_t seed = 123456789ULL) {
    uint64_t h = seed;
    h ^= static_cast<uint64_t>(x) + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
    h ^= static_cast<uint64_t>(z) + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
    return h;
}

void GenerateChunk(int cx, int cz, uint64_t seed = 123456789ULL) {
    string ckey = chunkKey(cx, cz);

    // Use try_emplace to construct in-place (no copy)
    auto [it, inserted] = chunks.try_emplace(ckey, cx, cz);
    if (!inserted) return;  // already exists

    Chunk& ch = it->second;
    ch.dirty = true;

    Perlin perlin(seed);

    for (int lx = 0; lx < 16; ++lx) {
        for (int lz = 0; lz < 16; ++lz) {
            int wx = cx * 16 + lx;
            int wz = cz * 16 + lz;

            float fx = wx * 0.02f;
            float fz = wz * 0.02f;

            float noiseValue = perlin.fbm(fx, fz, 6, 0.5f);
            int height = static_cast<int>(noiseValue * 30.0f + 10.0f);

            for (int wy = -30; wy <= height; ++wy) {
                string type = "stone";
                if (wy == height)     type = "grass";
                else if (wy > height - 4) type = "dirt";
                AddBlock(wx, wy, wz, type, false);
            }
        }
    }

    // Trees...
    int num_trees = (rng() % 5);
    for (int i = 0; i < num_trees; ++i) {
        int tx = cx * 16 + (rng() % 16);
        int tz = cz * 16 + (rng() % 16);

        // Find the highest solid block that is grass or dirt
        int surfaceY = -31;  // impossible low value

        for (int y = -30; y <= 30; ++y) {
            string below = worldBlocks[posKey(tx, y, tz)];
            if (below == "grass" || below == "dirt") {
                surfaceY = y;  // update to the highest one
                break;
            }
        }

        if (surfaceY >= -30) {
            GenerateTreeBaseAt(tx, surfaceY + 1, tz);
        }
    }
}


void UpdateChunks() {
    int px = floor(playerX / 16.0f);
    int pz = floor(playerZ / 16.0f);

    const int LOAD_RADIUS = 4;

    for (int dx = -LOAD_RADIUS; dx <= LOAD_RADIUS; ++dx) {
        for (int dz = -LOAD_RADIUS; dz <= LOAD_RADIUS; ++dz) {
            int cx = px + dx;
            int cz = pz + dz;
            string ckey = chunkKey(cx, cz);

            if (chunks.find(ckey) == chunks.end()) {
                // Launch async generation
            auto _ = async(launch::async, [cx, cz]() {
                GenerateChunk(cx, cz);
            });
            }
        }
    }
}

void tryBreakInFront() {
    if (breakCooldown > 0.0f) {
        cout << "Break on cooldown (" << breakCooldown << "s left)" << endl;
        return;
    }

    cout << "Trying to break block in front" << endl;

    // Look direction
    float radYaw   = glm::radians(yaw);
    float radPitch = glm::radians(pitch);
    glm::vec3 dir(
        cos(radYaw) * cos(radPitch),
        sin(radPitch),
        sin(radYaw) * cos(radPitch)
    );
    dir = glm::normalize(dir);

    glm::vec3 start(playerX, playerY, playerZ);

    for (int step = 1; step <= 5; ++step) {
        glm::vec3 pos = start + dir * (float)step;

        int bx = floor(pos.x);
        int by = floor(pos.y);
        int bz = floor(pos.z);

        string key = posKey(bx, by, bz);
        if (worldBlocks.erase(key)) {
            cout << "Broke block at (" << bx << ", " << by << ", " << bz << ")" << endl;

            int cx = bx / 16;
            int cz = bz / 16;
            for (int dx = -1; dx <= 1; ++dx) {
                for (int dz = -1; dz <= 1; ++dz) {
                    string ckey = chunkKey(cx + dx, cz + dz);
                    auto it = chunks.find(ckey);
                    if (it != chunks.end()) {
                        it->second.dirty = true;
                    }
                }
            }
            breakCooldown = BREAK_COOLDOWN_TIME;
            return;
        }
    }

    cout << "No breakable block in front" << endl;
    breakCooldown = BREAK_COOLDOWN_TIME / 2;
}



// ─── Main ──────────────────────────────────────────────────────────────────

int main(int argc, char* argv[]) {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        cout << "SDL init failed: " << SDL_GetError() << endl;
        return 1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetSwapInterval(1);

    SDL_Window* window = SDL_CreateWindow("Minecraft", 1280, 720, SDL_WINDOW_OPENGL);
    if (!window) {
        cout << "Window failed: " << SDL_GetError() << endl;
        SDL_Quit();
        return 1;
    }

    SDL_GL_CreateContext(window);

    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        cout << "GLAD failed\n";
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Mouse setup - critical for macOS
    SDL_SetWindowRelativeMouseMode(window, true);
    SDL_HideCursor();
    SDL_RaiseWindow(window);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    createShader();
    createHudShader();
    glUseProgram(shaderProgram);

    // Load textures
    Textures["grass_top.png"] = LoadTexture("grass_top.png");
    Textures["grass.png"]     = LoadTexture("grass.png");
    Textures["dirt.png"]      = LoadTexture("dirt.png");
    Textures["stone.png"]     = LoadTexture("stone.png");
    Textures["error.png"]     = LoadTexture("error.png");
    Textures["wood.png"]      = LoadTexture("wood.png");
    Textures["log_top.png"]   = LoadTexture("log_top.png");
    Textures["leaves.png"]    = LoadTexture("leaves.png");
    Textures["hotbar.png"]    = LoadTexture("hotbar.png");

    // Create HUD shader (you already have createHudShader(), good)
    createHudShader();

// Load hotbar texture
Textures["hotbar.png"] = LoadTexture("hotbar.png");
GLuint hotbarTex = Textures["hotbar.png"];

// Create hotbar quad VAO/VBO
float hotbarWidth = 364.0f;   // adjust to your actual hotbar.png width
float hotbarHeight = 44.0f;
float scale = 2.0f;           // 2× size looks good on 1280×720

float screenW = 1280.0f;
float screenH = 720.0f;

float x = (screenW - hotbarWidth * scale) / 2.0f;      // center
float y = screenH - hotbarHeight * scale - 625.0f;      // near bottom

float vertices[] = {
    //   x          y         u     v
    x,             y,         0.0f, 1.0f,   // bottom-left
    x + hotbarWidth*scale, y,         1.0f, 1.0f,   // bottom-right
    x + hotbarWidth*scale, y + hotbarHeight*scale, 1.0f, 0.0f, // top-right

    x,             y,         0.0f, 1.0f,   // bottom-left
    x + hotbarWidth*scale, y + hotbarHeight*scale, 1.0f, 0.0f, // top-right
    x,             y + hotbarHeight*scale, 0.0f, 0.0f    // top-left
};

glGenVertexArrays(1, &hotbarVAO);
glGenBuffers(1, &hotbarVBO);

glBindVertexArray(hotbarVAO);
glBindBuffer(GL_ARRAY_BUFFER, hotbarVBO);
glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

// Position (location 0)
glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)0);
glEnableVertexAttribArray(0);
// TexCoord (location 1)
glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)(2*sizeof(float)));
glEnableVertexAttribArray(1);

glBindVertexArray(0);

    GenerateChunk(0, 0);

    bool running = true;
    SDL_Event e;

    Uint64 lastTime = SDL_GetPerformanceCounter();

SDL_AudioSpec spec;
LoadSong(playlist[currentSong], spec);

BackGroundMusic = SDL_CreateAudioStream(&spec, &spec);
device = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec);

SDL_PutAudioStreamData(BackGroundMusic, musicBuffer, musicLength);
SDL_BindAudioStream(device, BackGroundMusic);
SDL_ResumeAudioDevice(device);

    SDL_ResumeAudioDevice(device);

    if (!device)
    {
        cout << "Audio device failed: " << SDL_GetError() << endl;
        return 1;
    }

    while (running) {
        Uint64 now = SDL_GetPerformanceCounter();
        double dt = (double)(now - lastTime) / SDL_GetPerformanceFrequency();
        lastTime = now;

        breakCooldown -= (float)dt;
        if (breakCooldown < 0.0f) breakCooldown = 0.0f;

        // Event loop
        while (SDL_PollEvent(&e)) {

            if (e.type == SDL_EVENT_QUIT) {
                running = false;
            }

            if (e.type == SDL_EVENT_KEY_DOWN) {
                if (e.key.key == SDLK_ESCAPE) running = false;

                // Break on E
                if (e.key.key == SDLK_E) {
                    cout << "E pressed - trying to break" << endl;
                    tryBreakInFront();
                }
            }

            if (e.type == SDL_EVENT_MOUSE_MOTION) {
                yaw   += e.motion.xrel * mouseSensitivity;
                pitch -= e.motion.yrel * mouseSensitivity;
                if (pitch > 89.0f) pitch = 89.0f;
                if (pitch < -89.0f) pitch = -89.0f;
            }

            if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                cout << "[MOUSE DOWN] Button: " << (int)e.button.button << endl;
                if (e.button.button == SDL_BUTTON_LEFT) {
                    cout << "Left click - trying to break" << endl;
                    tryBreakInFront();
                }
            }
        }

        // Poll mouse button state every frame (backup for event drop)
        float mx, my;
        Uint32 mouseState = SDL_GetMouseState(&mx, &my);

        if ((mouseState & SDL_BUTTON_LMASK) && breakCooldown <= 0.0f) {
            cout << "Mouse held - breaking" << endl;
            tryBreakInFront();
        }

        // Old free-fly movement
        const bool* keys = SDL_GetKeyboardState(nullptr);

        float rad = glm::radians(yaw);
        glm::vec3 forward(cos(rad), 0.0f, sin(rad));
        forward = glm::normalize(forward);
        glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0,1,0)));

        glm::vec3 moveDir(0.0f);

        if (keys[SDL_SCANCODE_W]) moveDir += forward;
        if (keys[SDL_SCANCODE_S]) moveDir -= forward;
        if (keys[SDL_SCANCODE_A]) moveDir -= right;
        if (keys[SDL_SCANCODE_D]) moveDir += right;

        moveDir *= speed * (float)dt;

        playerX += moveDir.x;
        playerZ += moveDir.z;

        if (keys[SDL_SCANCODE_SPACE]) playerY += speed * (float)dt;
        if (keys[SDL_SCANCODE_LSHIFT]) playerY -= speed * (float)dt;

UpdateChunks();

        glClearColor(0.53f, 0.81f, 0.92f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 proj = glm::perspective(glm::radians(70.0f), 1280.0f / 720.0f, 0.1f, 200.0f);

        glm::vec3 direction(
            cos(glm::radians(yaw)) * cos(glm::radians(pitch)),
            sin(glm::radians(pitch)),
            sin(glm::radians(yaw)) * cos(glm::radians(pitch))
        );

        glm::mat4 view = glm::lookAt(
            glm::vec3(playerX, playerY, playerZ),
            glm::vec3(playerX, playerY, playerZ) + direction,
            glm::vec3(0,1,0)
        );

        glm::mat4 mvp = proj * view;
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "uMVP"), 1, GL_FALSE, glm::value_ptr(mvp));

        glUseProgram(shaderProgram);
        int rebuiltThisFrame = 0;
        const int MAX_REBUILDS_PER_FRAME = 1;

        for (auto& pair : chunks) {
            Chunk& chunk = pair.second;

            if (chunk.dirty && rebuiltThisFrame < MAX_REBUILDS_PER_FRAME) {
                if (SDL_rand(2)+1 == 1){
                    buildChunkMesh(chunk);
                    rebuiltThisFrame++;
                }
            }

            if (chunk.counts.empty()) continue;

            for (const auto& [texID, vao] : chunk.vaos) {
                glBindTexture(GL_TEXTURE_2D, texID);
                glBindVertexArray(vao);
                glDrawArrays(GL_TRIANGLES, 0, chunk.counts.at(texID));
            }
        }
        if (SDL_GetAudioStreamQueued(BackGroundMusic) == 0)
{
    PlayNextSong(device, spec);
}
// --- Render HUD (hotbar) ---
glUseProgram(hudShaderProgram);

// Set ortho matrix (bottom-left origin, y up)
glm::mat4 ortho = glm::ortho(0.0f, 1280.0f, 0.0f, 720.0f, -1.0f, 0.0f);
glUniformMatrix4fv(glGetUniformLocation(hudShaderProgram, "uOrtho"), 1, GL_FALSE, glm::value_ptr(ortho));

// Disable depth test + write for 2D overlay
glDisable(GL_DEPTH_TEST);
glDepthMask(GL_FALSE);

// Enable blending if hotbar has transparency
glEnable(GL_BLEND);
glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

// Bind hotbar texture and VAO
glActiveTexture(GL_TEXTURE0);
glBindTexture(GL_TEXTURE_2D, hotbarTex);
glUniform1i(glGetUniformLocation(hudShaderProgram, "uTexture"), 0);

glBindVertexArray(hotbarVAO);
glDrawArrays(GL_TRIANGLES, 0, 6);

glBindVertexArray(0);

// Restore state for next frame
glDisable(GL_BLEND);
glDepthMask(GL_TRUE);
glEnable(GL_DEPTH_TEST);

// Switch back to world shader
glUseProgram(shaderProgram);
        SDL_GL_SwapWindow(window);
    }
    SDL_CloseAudioDevice(device);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}