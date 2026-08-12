#include "raylib.h"
#include "raymath.h"
#include "resource_dir.h" // utility header for SearchAndSetResourceDir

#define RAYLIB_TMX_IMPLEMENTATION
#include "raylib-tmx.h"

typedef enum {
    LEFT,
    RIGHT,
} Direction;

typedef struct Player {
    Vector2 position;
    Vector2 size;
    Vector2 speed;
    Rectangle bounds;
    Direction direction;
} Player;
typedef struct Zombie {
    Vector2 position;
    Vector2 size;
    Vector2 speed;
    Rectangle bounds;
    Direction direction;
} Zombie;

// frameCount is the number of pictures in the spritesheet. framesSpeed is the
// number of frames(i.e. pictures) showns per second
void animatePixels(int *framesCounter, int *currentFrame, Rectangle *frameRec, Texture2D spritesheet, int frameCount, int framesSpeed) {
    if (*framesCounter >= (60 / framesSpeed)) {
        *framesCounter = 0;

        (*currentFrame)++;

        if (*currentFrame >= frameCount)
            *currentFrame = 0;

        frameRec->x = (float)*currentFrame * (float)spritesheet.width / frameCount;
    }
    return;
}

int main(int argc, char *argv[]) {
    // Initialization
    //--------------------------------------------------------------------------------------
    // Make sure we're running in the correct directory.
    ChangeDirectory(GetDirectoryPath(argv[0]));
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI); // Tell the window to use vsync and work on high DPI displays
    SearchAndSetResourceDir("resources");                  // Utility function from resource_dir.h to find the resources folder

    // Create the window and OpenGL context
    const int screenWidth = 960;
    const int screenHeight = 640;
    InitWindow(screenWidth, screenHeight, "Za Aruku Deddo");

    // Load a texture from the resources directory
    Texture2D playerWalk = LoadTexture("Raider_1/Walk.png");
    Image playerWalkOrigin = LoadImage("Raider_1/Walk.png");
    ImageFormat(&playerWalkOrigin, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    ImageFlipHorizontal(&playerWalkOrigin);
    Rectangle playerWalkFrameRec = {0.0f, 0.0f, (float)playerWalk.width / 8, (float)playerWalk.height};
    Player player = {0};
    player.position = (Vector2){0.0f, 200.0f};
    player.size = (Vector2){(float)playerWalk.width / 8.0f, (float)playerWalk.height};
    player.speed = (Vector2){50.0f, 50.0f};
    player.bounds = (Rectangle){player.position.x, player.position.y, player.size.x, player.size.y};
    player.direction = RIGHT;

    Texture2D wildZombieWalk = LoadTexture("wild_zombie/Walk.png");
    Rectangle wildZombieWalkFrameRec = {0.0f, 0.0f, (float)wildZombieWalk.width / 10, (float)wildZombieWalk.height};
    Zombie wildZombie = {0};
    wildZombie.position = (Vector2){0.0f, 400.0f};
    wildZombie.size = (Vector2){(float)wildZombieWalk.width / 10.0f, (float)wildZombieWalk.height};
    wildZombie.speed = (Vector2){50.0f, 50.0f};
    wildZombie.bounds = (Rectangle){wildZombie.position.x, wildZombie.position.y, wildZombie.size.x, wildZombie.size.y};
    wildZombie.direction = LEFT;

    SetTargetFPS(60);
    int playerCurrentFrame = 0;
    int playerFramesCounter = 0;
    int wildZombieCurrentFrame = 0;
    int wildZombieFramesCounter = 0;

    tmx_map *map = LoadTMX(argc > 1 ? argv[1] : "tilemaps/village/level1.tmx");
    Vector2 position = {0, 0};
    // Game Loop
    while (!WindowShouldClose()) {
        // Update
        float deltaTime = GetFrameTime();
        playerFramesCounter++;
        wildZombieFramesCounter++;

        Vector2 direction = {0};
        if (IsKeyDown(KEY_RIGHT)) {
            direction.x += 1;
            player.direction = RIGHT;
        }
        if (IsKeyDown(KEY_LEFT)) {
            direction.x -= 1;
            player.direction = LEFT;
        }
        if (IsKeyDown(KEY_UP)) {
            direction.y -= 1;
        }
        if (IsKeyDown(KEY_DOWN)) {
            direction.y += 1;
        }
        if (direction.x != 0 || direction.y != 0) {
            direction = Vector2Normalize(direction);
            player.position.x += direction.x * player.speed.x * deltaTime;
            player.position.y += direction.y * player.speed.y * deltaTime;
        }

        if (IsKeyUp(KEY_RIGHT) && IsKeyUp(KEY_UP) && IsKeyUp(KEY_LEFT) && IsKeyUp(KEY_DOWN))
            playerCurrentFrame = 0;

        animatePixels(&playerFramesCounter, &playerCurrentFrame, &playerWalkFrameRec, playerWalk, 8, 8);
        animatePixels(&wildZombieFramesCounter, &wildZombieCurrentFrame, &wildZombieWalkFrameRec, wildZombieWalk, 10, 10);

        wildZombie.position.x += wildZombie.speed.x * deltaTime;

        // Draw
        BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawTMX(map, position.x, position.y, WHITE, 1.0);
        switch (player.direction) {
        case RIGHT:
            playerWalkFrameRec.width = (float)playerWalk.width / 8;
            break;
        case LEFT:
            playerWalkFrameRec.width = -(float)playerWalk.width / 8;
            break;
        }
        DrawTextureRec(playerWalk, playerWalkFrameRec, player.position, WHITE);
        DrawTextureRec(wildZombieWalk, wildZombieWalkFrameRec, wildZombie.position, WHITE);
        EndDrawing();
    }

    // Unload our textures so it can be cleaned up. Destroy the window and cleanup the OpenGL context
    UnloadTexture(playerWalk);
    UnloadTexture(wildZombieWalk);
    CloseWindow();
    return 0;
}
