/*
DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
                    Version 2, December 2004

Copyright (C) 2004 Sam Hocevar <sam@hocevar.net>

Everyone is permitted to copy and distribute verbatim or modified
copies of this license document, and changing it is allowed as long
as the name is changed.

DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION

0. You just DO WHAT THE FUCK YOU WANT TO.
*/

#include <pthread.h>

#include "raylib.h" 
#include "httplib.h"

#define WINDOW_WIDTH  1280
#define WINDOW_HEIGHT  960

#define PORT 1337

Image incoming_image;
bool swap_image = false;
pthread_mutex_t lock;

static void put_image(const httplib::Request &req, httplib::Response &res) {
    printf("put image \n");

    auto data = reinterpret_cast<const unsigned char *>(req.body.c_str());

    pthread_mutex_lock(&lock);

    incoming_image = LoadImageFromMemory(".png", data, req.body.size());
    if (incoming_image.data == NULL) {
        printf("can't load incoming image\n");
        UnloadImage(incoming_image);
    } else {
        swap_image = true;
    }

    pthread_mutex_unlock(&lock);
}

static void *http_thread(void *arg) {
    httplib::Server svr;

    if (!svr.set_mount_point("/", "./miniPaint")) {
        printf("AIEEE!! cannot load static files\n");
        return NULL;
    }

    svr.Get("/hi", [](const httplib::Request &, httplib::Response &res) {
        res.set_content("Hello World!", "text/plain");
    });

    svr.Post("/putimage", put_image);
    svr.listen("0.0.0.0", PORT);
    return NULL;
}

int main(int argc, char *argv[]) {
    pthread_t server_thread;

    pthread_mutex_init(&lock, NULL);
    pthread_create(&server_thread, NULL, &http_thread, NULL);

    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Gigiproiettalo");
    SetTargetFPS(60);

    Texture2D texture;

    while (!WindowShouldClose()) {
        pthread_mutex_lock(&lock);
        if (swap_image) {
            UnloadTexture(texture);
            texture = LoadTextureFromImage(incoming_image);

            UnloadImage(incoming_image);
            swap_image = false;
        }

        pthread_mutex_unlock(&lock);

        BeginDrawing();
            ClearBackground(DARKGRAY);
            DrawTexture(texture, 0, 0, WHITE);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}

