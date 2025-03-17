#include "raylib.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>   
#include <string.h>


Wave alpha[26];
Wave numba[10];

Vector2 PointForSample(int16_t *data, int i, float scale, Vector2 center) {
    Vector2 d = {
        data[i * 2 + 0] / 32768.0f,
        data[i * 2 + 1] / 32768.0f,
    };

    Vector2 position = {
          d.x * scale + center.x,
         -d.y * scale + center.y,
    };

    return position;
}

int main() {
    const int screenWidth = 800;
    const int screenHeight = 600;
    InitWindow(screenWidth, screenHeight, "Vector Scope Oscilloscope");


    for (int i = 0; i < 26; i++) {
        char filename[32];
        snprintf(filename, 32, "oscw/%c.wav", 'A' + i);
        alpha[i] = LoadWave(filename);
        printf("Loading %s\n", filename);
        printf("Loaded %d frames\n", alpha[i].frameCount);

        if (!alpha[i].data) {
            TraceLog(LOG_ERROR, "Failed to load WAV file!");
            CloseWindow();
            return 1;
        }
    }

    for (int i = 0; i < 10; i++) {
        char filename[32];
        
        snprintf(filename, 32,"oscw/%d.wav", i);
        numba[i] = LoadWave(filename);
        printf("Loading %s\n", filename);
        printf("Loaded %d frames\n", numba[i].frameCount);

        if (!numba[i].data) {
            TraceLog(LOG_ERROR, "Failed to load WAV file!");
            CloseWindow();
            return 1;
        }
    }

    Wave PD = LoadWave("/Users/willy/Downloads/Telegram Desktop/osci-write-250217_192250.wav");

    static Wave *w = NULL;
    static int frame = 0;
    static int start = 0;

    w = &PD;

    SetTargetFPS(60);
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground((Color){0, 0, 0, 0});

        int c = GetCharPressed();
        if (c >= 'a' && c <= 'z') {
            w = &alpha[c - 'a'];
            start = 0;
        } else if (c >= '0' && c <= '9') {
            w = &numba[c - '0'];
            start = 0;
        }

        const float scale = fminf(screenWidth, screenHeight) * 0.4f;
        const Vector2 center = {screenWidth/2.0f, screenHeight/2.0f};
        float deltatime = GetFrameTime();
        int end = start + deltatime * w->sampleRate;
        int16_t *data = (int16_t*)w->data;

        int falloff = 1;
        
        for (unsigned int i = start; i < end; i++) {
            if (i >= w->frameCount) {
                break;
            }

            Color c = GREEN;

            Vector2 position = PointForSample(data, i, scale, center);
            if (i > 0) {
                Vector2 last = PointForSample(data, i - 1, scale, center);
                DrawLineV(position, last, c);
            }

            int d = 2;
            DrawCircleV(position, d, c);
        }

        start = end;
        frame++;

        if (start >= w->frameCount) {
            start = 0;
        }

        // Draw UI elements
        DrawCircleLinesV(center, scale, Fade(WHITE, 0.2f));
        DrawText("Vector Scope", 10, 10, 20, LIGHTGRAY);
        DrawFPS(screenWidth - 90, 10);
        EndShaderMode();
        EndDrawing();
    }

    return 0;
}