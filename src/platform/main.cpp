#include <raylib.h>

// Step 1 placeholder: proves raylib is fetched, built and linked. Replaced by App in step 2.
int main() {
    InitWindow(1280, 720, "Gridiron Sim");
    SetTargetFPS(60);
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(Color{34, 102, 51, 255});
        DrawText("Gridiron Sim - build OK", 40, 40, 32, RAYWHITE);
        EndDrawing();
    }
    CloseWindow();
    return 0;
}
