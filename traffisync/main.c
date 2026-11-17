#include "raylib.h"

#include <stdbool.h>

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 800
#define ROAD_AREA_WIDTH 930
#define MAX_VEHICLES 120

#define ROAD_WIDTH 156.0f
#define STOP_OFFSET 58.0f
#define CAR_LENGTH 30.0f
#define CAR_GAP 18.0f

typedef enum {
    DIR_NORTH,
    DIR_SOUTH,
    DIR_EAST,
    DIR_WEST,
    DIR_COUNT
} Direction;

typedef struct {
    bool active;
    Direction direction;
    float progress;
    float speed;
    Color color;
} Vehicle;

static const Color backgroundColor = { 27, 35, 44, 255 };
static const Color roadColor = { 54, 63, 72, 255 };
static const Color laneColor = { 119, 130, 137, 255 };
static const Color panelColor = { 34, 43, 53, 255 };
static const Color accentColor = { 68, 190, 170, 255 };
static const Color textColor = { 232, 238, 241, 255 };
static const Color mutedTextColor = { 155, 169, 178, 255 };
static const Color vehicleColors[] = {
    { 239, 112, 90, 255 },
    { 245, 184, 74, 255 },
    { 104, 170, 232, 255 },
    { 181, 139, 218, 255 },
    { 90, 198, 139, 255 }
};

static bool IsVertical(Direction direction)
{
    return direction == DIR_NORTH || direction == DIR_SOUTH;
}

static float GetStopProgress(Direction direction)
{
    float pathLength = IsVertical(direction) ? SCREEN_HEIGHT : ROAD_AREA_WIDTH;
    return pathLength / 2.0f - STOP_OFFSET;
}

static bool IsGreen(Direction direction, int phase)
{
    return IsVertical(direction) ? phase == 0 : phase == 1;
}

static int CountQueue(const Vehicle vehicles[], Direction first, Direction second)
{
    int count = 0;
    for (int i = 0; i < MAX_VEHICLES; i++) {
        if (!vehicles[i].active) continue;

        float stopProgress = GetStopProgress(vehicles[i].direction);
        if ((vehicles[i].direction == first || vehicles[i].direction == second) &&
            vehicles[i].progress < stopProgress + 20.0f) {
            count++;
        }
    }

    return count;
}

static void SpawnVehicle(Vehicle vehicles[], Direction direction)
{
    for (int i = 0; i < MAX_VEHICLES; i++) {
        if (!vehicles[i].active) {
            vehicles[i].active = true;
            vehicles[i].direction = direction;
            vehicles[i].progress = -CAR_LENGTH - CAR_GAP;
            vehicles[i].speed = (float)GetRandomValue(100, 145);
            vehicles[i].color = vehicleColors[GetRandomValue(0, 4)];
            return;
        }
    }
}

static void ResetSimulation(Vehicle vehicles[], float spawnTimers[],
                            float *elapsedTime, int *completedVehicles,
                            int *phase, float *phaseTimer)
{
    for (int i = 0; i < MAX_VEHICLES; i++) {
        vehicles[i].active = false;
    }

    spawnTimers[DIR_NORTH] = 0.5f;
    spawnTimers[DIR_SOUTH] = 1.2f;
    spawnTimers[DIR_EAST] = 0.8f;
    spawnTimers[DIR_WEST] = 1.6f;
    *elapsedTime = 0.0f;
    *completedVehicles = 0;
    *phase = 0;
    *phaseTimer = 0.0f;
}

static float GetPathCoordinate(Direction direction, float progress)
{
    float middle = ROAD_AREA_WIDTH / 2.0f;
    switch (direction) {
        case DIR_NORTH: return SCREEN_HEIGHT - progress;
        case DIR_SOUTH: return progress;
        case DIR_EAST: return progress;
        case DIR_WEST: return ROAD_AREA_WIDTH - progress;
        default: return middle;
    }
}

static Vector2 GetVehiclePosition(const Vehicle *vehicle)
{
    float centerX = ROAD_AREA_WIDTH / 2.0f;
    float centerY = SCREEN_HEIGHT / 2.0f;
    float coordinate = GetPathCoordinate(vehicle->direction, vehicle->progress);

    switch (vehicle->direction) {
        case DIR_NORTH: return (Vector2){ centerX + 37.0f, coordinate };
        case DIR_SOUTH: return (Vector2){ centerX - 37.0f, coordinate };
        case DIR_EAST: return (Vector2){ coordinate, centerY - 37.0f };
        case DIR_WEST: return (Vector2){ coordinate, centerY + 37.0f };
        default: return (Vector2){ centerX, centerY };
    }
}

static int UpdateVehicles(Vehicle vehicles[], float deltaTime, int phase)
{
    int completedVehicles = 0;
    for (int i = 0; i < MAX_VEHICLES; i++) {
        Vehicle *vehicle = &vehicles[i];
        if (!vehicle->active) continue;

        float stopProgress = GetStopProgress(vehicle->direction);
        float nextProgress = vehicle->progress + vehicle->speed * deltaTime;
        if (!IsGreen(vehicle->direction, phase) &&
            vehicle->progress <= stopProgress &&
            nextProgress > stopProgress) {
            nextProgress = stopProgress;
        }

        for (int j = 0; j < MAX_VEHICLES; j++) {
            if (i == j || !vehicles[j].active ||
                vehicles[j].direction != vehicle->direction ||
                vehicles[j].progress <= vehicle->progress) {
                continue;
            }

            float maxProgress = vehicles[j].progress - CAR_LENGTH - CAR_GAP;
            if (nextProgress > maxProgress) nextProgress = maxProgress;
        }

        vehicle->progress = nextProgress;
        float pathLength = IsVertical(vehicle->direction) ? SCREEN_HEIGHT : ROAD_AREA_WIDTH;
        if (vehicle->progress > pathLength + CAR_LENGTH) {
            vehicle->active = false;
            completedVehicles++;
        }
    }

    return completedVehicles;
}

static void DrawRoad(void)
{
    float centerX = ROAD_AREA_WIDTH / 2.0f;
    float centerY = SCREEN_HEIGHT / 2.0f;
    float halfRoad = ROAD_WIDTH / 2.0f;

    DrawRectangle(0, (int)(centerY - halfRoad), ROAD_AREA_WIDTH,
                  (int)ROAD_WIDTH, roadColor);
    DrawRectangle((int)(centerX - halfRoad), 0, (int)ROAD_WIDTH,
                  SCREEN_HEIGHT, roadColor);

    for (int x = 14; x < ROAD_AREA_WIDTH; x += 48) {
        if (x > centerX - halfRoad - 18 && x < centerX + halfRoad + 18) continue;
        DrawRectangle(x, (int)centerY - 2, 25, 4, laneColor);
    }
    for (int y = 14; y < SCREEN_HEIGHT; y += 48) {
        if (y > centerY - halfRoad - 18 && y < centerY + halfRoad + 18) continue;
        DrawRectangle((int)centerX - 2, y, 4, 25, laneColor);
    }

    DrawLine((int)(centerX - halfRoad), (int)(centerY - 2),
             (int)(centerX + halfRoad), (int)(centerY - 2), laneColor);
    DrawLine((int)(centerX - 2), (int)(centerY - halfRoad),
             (int)(centerX - 2), (int)(centerY + halfRoad), laneColor);

    DrawLine((int)(centerX - halfRoad), (int)(centerY - STOP_OFFSET),
             (int)(centerX - 32), (int)(centerY - STOP_OFFSET), RAYWHITE);
    DrawLine((int)(centerX + 32), (int)(centerY + STOP_OFFSET),
             (int)(centerX + halfRoad), (int)(centerY + STOP_OFFSET), RAYWHITE);
    DrawLine((int)(centerX - STOP_OFFSET), (int)(centerY - halfRoad),
             (int)(centerX - STOP_OFFSET), (int)(centerY - 32), RAYWHITE);
    DrawLine((int)(centerX + STOP_OFFSET), (int)(centerY + 32),
             (int)(centerX + STOP_OFFSET), (int)(centerY + halfRoad), RAYWHITE);

    DrawText("N", (int)centerX - 8, 24, 18, mutedTextColor);
    DrawText("S", (int)centerX - 8, SCREEN_HEIGHT - 44, 18, mutedTextColor);
    DrawText("W", 18, (int)centerY - 8, 18, mutedTextColor);
    DrawText("E", ROAD_AREA_WIDTH - 34, (int)centerY - 8, 18, mutedTextColor);
}

static void DrawSignal(int phase)
{
    float centerX = ROAD_AREA_WIDTH / 2.0f;
    float centerY = SCREEN_HEIGHT / 2.0f;
    Color red = { 224, 82, 76, 255 };
    Color green = { 70, 207, 129, 255 };

    DrawCircle((int)(centerX - 105), (int)(centerY - 105), 15,
               phase == 0 ? green : red);
    DrawCircle((int)(centerX + 105), (int)(centerY + 105), 15,
               phase == 0 ? green : red);
    DrawCircle((int)(centerX + 105), (int)(centerY - 105), 15,
               phase == 1 ? green : red);
    DrawCircle((int)(centerX - 105), (int)(centerY + 105), 15,
               phase == 1 ? green : red);
}

static void DrawVehicles(const Vehicle vehicles[])
{
    for (int i = 0; i < MAX_VEHICLES; i++) {
        if (!vehicles[i].active) continue;

        Vector2 position = GetVehiclePosition(&vehicles[i]);
        if (IsVertical(vehicles[i].direction)) {
            DrawRectangleRounded((Rectangle){ position.x - 8, position.y - 15, 16, 30 },
                                 0.25f, 4, vehicles[i].color);
            DrawRectangle(position.x - 5, position.y - 10, 10, 5,
                          (Color){ 225, 233, 236, 255 });
        } else {
            DrawRectangleRounded((Rectangle){ position.x - 15, position.y - 8, 30, 16 },
                                 0.25f, 4, vehicles[i].color);
            DrawRectangle(position.x + 4, position.y - 5, 6, 10,
                          (Color){ 225, 233, 236, 255 });
        }
    }
}

static bool DrawButton(Rectangle bounds, const char *label, bool active)
{
    Vector2 mouse = GetMousePosition();
    bool hovered = CheckCollisionPointRec(mouse, bounds);
    Color fill = active ? accentColor : (hovered ? (Color){ 70, 84, 96, 255 } :
                                         (Color){ 51, 63, 74, 255 });
    DrawRectangleRounded(bounds, 0.18f, 6, fill);
    int textWidth = MeasureText(label, 18);
    DrawText(label, (int)(bounds.x + (bounds.width - textWidth) / 2),
             (int)(bounds.y + (bounds.height - 18) / 2), 18,
             active ? backgroundColor : textColor);
    return hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

static void DrawPanel(const Vehicle vehicles[], bool autoMode, bool paused,
                      int phase, float phaseTimer, int density,
                      float elapsedTime, int completedVehicles)
{
    int verticalQueue = CountQueue(vehicles, DIR_NORTH, DIR_SOUTH);
    int horizontalQueue = CountQueue(vehicles, DIR_EAST, DIR_WEST);
    int total = 0;
    for (int i = 0; i < MAX_VEHICLES; i++) {
        if (vehicles[i].active) total++;
    }

    DrawRectangle(ROAD_AREA_WIDTH, 0, SCREEN_WIDTH - ROAD_AREA_WIDTH,
                  SCREEN_HEIGHT, panelColor);
    DrawText("TRAFFISYNC", 970, 32, 30, accentColor);
    DrawText("Intersection control", 970, 70, 18, mutedTextColor);
    DrawLine(970, 108, 1240, 108, (Color){ 75, 88, 98, 255 });

    DrawText("SIGNAL PHASE", 970, 132, 15, mutedTextColor);
    DrawText(phase == 0 ? "NORTH / SOUTH" : "EAST / WEST",
             970, 158, 23, textColor);
    DrawText(phase == 0 ? "GREEN" : "RED", 970, 188, 16,
             phase == 0 ? (Color){ 87, 220, 147, 255 } :
                          (Color){ 235, 105, 98, 255 });
    DrawText(phase == 1 ? "GREEN" : "RED", 1100, 188, 16,
             phase == 1 ? (Color){ 87, 220, 147, 255 } :
                          (Color){ 235, 105, 98, 255 });

    DrawText("LIVE QUEUE", 970, 238, 15, mutedTextColor);
    DrawText(TextFormat("North / South     %02i", verticalQueue), 970, 268, 18, textColor);
    DrawText(TextFormat("East / West        %02i", horizontalQueue), 970, 296, 18, textColor);
    DrawText(TextFormat("Vehicles on road   %02i", total), 970, 324, 18, textColor);
    int elapsedSeconds = (int)elapsedTime;
    DrawText(TextFormat("Session time       %02i:%02i",
                        elapsedSeconds / 60, elapsedSeconds % 60),
             970, 352, 18, textColor);
    DrawText(TextFormat("Vehicles cleared   %02i", completedVehicles),
             970, 380, 18, textColor);

    DrawText(autoMode ? "ADAPTIVE CONTROL" : "MANUAL CONTROL",
             970, 420, 16, mutedTextColor);
    DrawText(autoMode ? TextFormat("Phase time  %.1f s", phaseTimer) :
                        "Select a green phase", 970, 445, 18, textColor);

    DrawButton((Rectangle){ 970, 478, 270, 44 },
               autoMode ? "Switch to manual" : "Switch to adaptive",
               autoMode);

    DrawText(TextFormat("Traffic density  %i%%", density * 25), 970, 546, 18, textColor);
    DrawButton((Rectangle){ 970, 576, 126, 40 }, "Less", false);
    DrawButton((Rectangle){ 1114, 576, 126, 40 }, "More", false);

    DrawButton((Rectangle){ 970, 636, 126, 40 },
               paused ? "Resume" : "Pause", paused);
    DrawButton((Rectangle){ 1114, 636, 126, 40 }, "Reset", false);

    DrawText("SPACE: pause/resume     R: reset", 970, 694, 14, mutedTextColor);
    DrawText("A: adaptive/manual       1 / 2: phase", 970, 716, 14, mutedTextColor);
    DrawText("Simulation only - not connected to", 970, 752, 14, mutedTextColor);
    DrawText("real traffic infrastructure.", 970, 772, 14, mutedTextColor);

    if (paused) {
        DrawRectangle(0, 0, ROAD_AREA_WIDTH, 38, (Color){ 20, 27, 34, 220 });
        DrawText("PAUSED", 18, 10, 18, textColor);
    }
}

int main(void)
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "TraffiSync - Traffic Control");
    SetTargetFPS(60);
    SetRandomSeed((unsigned int)GetTime());

    Vehicle vehicles[MAX_VEHICLES] = { 0 };
    float spawnTimers[DIR_COUNT] = { 0.5f, 1.2f, 0.8f, 1.6f };
    bool autoMode = true;
    bool paused = false;
    int phase = 0;
    int density = 2;
    float phaseTimer = 0.0f;
    float elapsedTime = 0.0f;
    int completedVehicles = 0;

    while (!WindowShouldClose()) {
        float deltaTime = GetFrameTime();
        if (deltaTime > 0.05f) deltaTime = 0.05f;

        if (IsKeyPressed(KEY_SPACE)) paused = !paused;
        if (IsKeyPressed(KEY_A)) autoMode = !autoMode;
        if (IsKeyPressed(KEY_R)) {
            ResetSimulation(vehicles, spawnTimers, &elapsedTime,
                            &completedVehicles, &phase, &phaseTimer);
        }
        if (IsKeyPressed(KEY_ONE) && !autoMode) {
            phase = 0;
            phaseTimer = 0.0f;
        }
        if (IsKeyPressed(KEY_TWO) && !autoMode) {
            phase = 1;
            phaseTimer = 0.0f;
        }

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            Vector2 mouse = GetMousePosition();
            if (CheckCollisionPointRec(mouse, (Rectangle){ 970, 576, 126, 40 }) && density > 1) {
                density--;
            } else if (CheckCollisionPointRec(mouse, (Rectangle){ 1114, 576, 126, 40 }) && density < 4) {
                density++;
            } else if (CheckCollisionPointRec(mouse, (Rectangle){ 970, 478, 270, 44 })) {
                autoMode = !autoMode;
            } else if (!autoMode &&
                       CheckCollisionPointRec(mouse, (Rectangle){ 970, 188, 128, 28 })) {
                phase = 0;
                phaseTimer = 0.0f;
            } else if (!autoMode &&
                       CheckCollisionPointRec(mouse, (Rectangle){ 1100, 188, 130, 28 })) {
                phase = 1;
                phaseTimer = 0.0f;
            } else if (CheckCollisionPointRec(mouse, (Rectangle){ 970, 636, 126, 40 })) {
                paused = !paused;
            } else if (CheckCollisionPointRec(mouse, (Rectangle){ 1114, 636, 126, 40 })) {
                ResetSimulation(vehicles, spawnTimers, &elapsedTime,
                                &completedVehicles, &phase, &phaseTimer);
            }
        }

        if (!paused) {
            phaseTimer += deltaTime;
            elapsedTime += deltaTime;

            if (autoMode) {
                int activeQueue = phase == 0
                    ? CountQueue(vehicles, DIR_NORTH, DIR_SOUTH)
                    : CountQueue(vehicles, DIR_EAST, DIR_WEST);
                int waitingQueue = phase == 0
                    ? CountQueue(vehicles, DIR_EAST, DIR_WEST)
                    : CountQueue(vehicles, DIR_NORTH, DIR_SOUTH);

                if (phaseTimer >= 12.0f ||
                    (phaseTimer >= 5.0f && waitingQueue > activeQueue + 1)) {
                    phase = 1 - phase;
                    phaseTimer = 0.0f;
                }
            }

            float spawnInterval = 3.5f - (float)density * 0.65f;
            for (int i = 0; i < DIR_COUNT; i++) {
                spawnTimers[i] -= deltaTime;
                if (spawnTimers[i] <= 0.0f) {
                    SpawnVehicle(vehicles, (Direction)i);
                    spawnTimers[i] = spawnInterval *
                        (float)GetRandomValue(75, 125) / 100.0f;
                }
            }
            completedVehicles += UpdateVehicles(vehicles, deltaTime, phase);
        }

        BeginDrawing();
        ClearBackground(backgroundColor);
        DrawRoad();
        DrawVehicles(vehicles);
        DrawSignal(phase);
        DrawPanel(vehicles, autoMode, paused, phase, phaseTimer, density,
                  elapsedTime, completedVehicles);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
