#include "raylib.h"
#include "raymath.h"
#include "resource_dir.h" // utility header for SearchAndSetResourceDir
#include "string.h"
#include <stdio.h>
#include <tmx.h>

#define RAYLIB_TMX_IMPLEMENTATION
#include "raylib-tmx.h"

typedef enum {
    LEFT,
    RIGHT,
} Direction;

typedef struct Player {
    Vector2 position;
    Vector2 spriteSize;
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
typedef struct Bullet {
    Vector2 position;
    Vector2 speed;
    float radius;
} Bullet;

//----------------------------------------------------------------------------------
// Helper Functions
//----------------------------------------------------------------------------------
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
void UpdatePlayerBounds(Player *player) { player->bounds = (Rectangle){player->position.x + 7, player->position.y + 52, 14, 12}; }
bool PlayerCollides(Rectangle bounds, tmx_layer *collisionLayer) {
    if (collisionLayer == NULL || collisionLayer->type != L_OBJGR) {
        return false;
    }

    tmx_object *object = collisionLayer->content.objgr->head;

    while (object != NULL) {
        if (object->obj_type == OT_SQUARE) {
            RaylibTMXCollision collision = HandleTMXCollision(object);

            if (CheckCollisionRecs(bounds, collision.rect)) {
                return true;
            }
        }

        object = object->next;
    }

    return false;
}
int compare(const void *a, const void *b) {
    const tmx_object *objA = *(const tmx_object **)a;
    const tmx_object *objB = *(const tmx_object **)b;

    if (objA->y < objB->y)
        return -1;

    if (objA->y > objB->y)
        return 1;

    return 0;
}
void sortByY(tmx_map *map, tmx_layer *objectsLayer, Player *player, Texture2D playerWalk, Rectangle playerWalkFrameRec) {
    int count = 0;

    // Count buildings
    tmx_object *object = objectsLayer->content.objgr->head;

    while (object != NULL) {
        count++;
        object = object->next;
    }

    // Make array
    tmx_object **collection = malloc(count * sizeof(tmx_object *));

    if (collection == NULL)
        return;

    // Fill array
    object = objectsLayer->content.objgr->head;
    int i = 0;

    while (object != NULL) {
        collection[i] = object;
        i++;
        object = object->next;
    }

    // Sort buildings by Y
    qsort(collection, count, sizeof(tmx_object *), compare);

    bool playerDrawn = false;
    float playerY = player->position.y + player->spriteSize.y;
    for (int j = 0; j < count; j++) {
        object = collection[j];

        // Draw player when we reach an object below him
        if (!playerDrawn && playerY < object->y) {
            DrawTextureRec(playerWalk, playerWalkFrameRec, player->position, WHITE);

            playerDrawn = true;
        }

        // Draw this building
        if (object->obj_type == OT_TILE) {
            int baseGid = object->content.gid;
            int gid = baseGid & TMX_FLIP_BITS_REMOVAL;

            tmx_tile *tile = map->tiles[gid];

            if (tile != NULL) {
                Rectangle dest = {object->x, object->y, object->width, object->height};

                DrawTMXObjectTile(tile, baseGid, dest, object->rotation, WHITE);
            }
        }
    }

    // Player is below all buildings
    if (!playerDrawn) {
        DrawTextureRec(playerWalk, playerWalkFrameRec, player->position, WHITE);
    }

    free(collection);
}

//----------------------------------------------------------------------------------
// Main function
//----------------------------------------------------------------------------------

int main(int argc, char *argv[]) {
    //----------------------------------------------------------------------------------
    // Initialization
    //----------------------------------------------------------------------------------

    ChangeDirectory(GetDirectoryPath(argv[0]));
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
    SearchAndSetResourceDir("resources");

    const int screenWidth = 960;
    const int screenHeight = 640;
    InitWindow(screenWidth, screenHeight, "Za Aruku Deddo");

    Texture2D playerWalk = LoadTexture("Raider_1/Walk.png");
    float playerFrameWidth = (float)playerWalk.width / 8.0f; // 28
    float playerFrameHeight = (float)playerWalk.height;      // 67
    Rectangle playerWalkFrameRec = {0.0f, 0.0f, playerFrameWidth, playerFrameHeight};

    Player player = {0};
    player.position = (Vector2){0.0f, 200.0f};
    player.spriteSize = (Vector2){playerFrameWidth, playerFrameHeight};
    player.speed = (Vector2){250.0f, 250.0f};
    player.bounds = (Rectangle){player.position.x, player.position.y, player.spriteSize.x, player.spriteSize.y};
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

    tmx_layer *groundLayer = tmx_find_layer_by_name(map, "Ground");
    tmx_layer *collisionLayer = tmx_find_layer_by_name(map, "Collisions");
    tmx_layer *objectsLayer = tmx_find_layer_by_name(map, "Buildings");

    int bulletCount = 5;
    Bullet **bullets = calloc(bulletCount, sizeof(Bullet *));
    if (bullets == NULL) {
        printf("Memory allocation failed.");
        return 1;
    }
    //----------------------------------------------------------------------------------
    // Gameplay Loop
    //----------------------------------------------------------------------------------
    while (!WindowShouldClose()) {

        //----------------------------------------------------------------------------------
        // Update
        //----------------------------------------------------------------------------------
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

            float moveX = direction.x * player.speed.x * deltaTime;
            float moveY = direction.y * player.speed.y * deltaTime;

            // Try X movement
            player.position.x += moveX;

            UpdatePlayerBounds(&player);

            if (PlayerCollides(player.bounds, collisionLayer)) {
                player.position.x -= moveX;
            }

            // Try Y movement
            player.position.y += moveY;

            UpdatePlayerBounds(&player);

            if (PlayerCollides(player.bounds, collisionLayer)) {
                player.position.y -= moveY;
            }

            UpdatePlayerBounds(&player);
        }
        switch (player.direction) {
        case RIGHT:
            playerWalkFrameRec.width = (float)playerWalk.width / 8;
            break;
        case LEFT:
            playerWalkFrameRec.width = -(float)playerWalk.width / 8;
            break;
        }

        UpdatePlayerBounds(&player);

        if (IsKeyUp(KEY_RIGHT) && IsKeyUp(KEY_UP) && IsKeyUp(KEY_LEFT) && IsKeyUp(KEY_DOWN))
            playerCurrentFrame = 0;

        animatePixels(&playerFramesCounter, &playerCurrentFrame, &playerWalkFrameRec, playerWalk, 8, 8);
        animatePixels(&wildZombieFramesCounter, &wildZombieCurrentFrame, &wildZombieWalkFrameRec, wildZombieWalk, 10, 10);

        wildZombie.position.x += wildZombie.speed.x * deltaTime;

        if (IsKeyPressed(KEY_A)) {
            Bullet bullet = {0};
            bullet.position = player.position;
            bullet.radius = 10.0f;
            bullet.speed = (Vector2){20.0f, 0.0f};

            // Add bullet to bullets array
            for (int i = 0; i < bulletCount; i++) {
                if (bullets[i] != NULL) {
                    printf("count #%d\n", i);
                    continue;
                } else {
                    printf("count #%d\n", i);
                    bullets[i] = &bullet;
                    break;
                }
            }
        }
        int count = 0;
        while (count < bulletCount) {
            if (bullets[count] == NULL) {
                break;
            }
            bullets[count]->position.x += bullets[count]->speed.x * deltaTime;
            count++;
        }
        //----------------------------------------------------------------------------------
        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();
        ClearBackground(RAYWHITE);

        // 1. Draw all map layers (tiles, building objects, images) except "Collisions"
        tmx_layer *layer = map->ly_head;
        while (layer != NULL) {
            if (layer->visible && strcmp(layer->name, "Collisions") != 0 && strcmp(layer->name, "Buildings") != 0) {
                DrawTMXLayer(map, layer, position.x, position.y, WHITE, 1.0f);
            }
            layer = layer->next;
        }
        count = 0;
        while (count < bulletCount) {
            if (bullets[count] == NULL) {
                break;
            }
            DrawCircleV((bullets[count])->position, bullets[count]->radius, RED);
            count++;
        }

        // 2. Draw buildings + player in Y order
        sortByY(map, objectsLayer, &player, playerWalk, playerWalkFrameRec);

        // Zombie
        DrawTextureRec(wildZombieWalk, wildZombieWalkFrameRec, wildZombie.position, WHITE);
        // 3. Debug collision bounds (RED)
        if (collisionLayer != NULL && collisionLayer->type == L_OBJGR) {
            tmx_object *obj = collisionLayer->content.objgr->head;
            while (obj != NULL) {
                if (obj->obj_type == OT_SQUARE) {
                    DrawRectangleLines(obj->x, obj->y, obj->width, obj->height, RED);
                }
                obj = obj->next;
            }
        }
        DrawRectangleLinesEx(player.bounds, 1.0, GREEN);
        EndDrawing();
    }

    //----------------------------------------------------------------------------------
    // Cleaup
    //----------------------------------------------------------------------------------
    UnloadTexture(playerWalk);
    UnloadTexture(wildZombieWalk);
    UnloadTMX(map);
    CloseWindow();
    return 0;
}
