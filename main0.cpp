#include "raylib.h"
#include "raymath.h"
#include <cmath>

struct Pen {
    Vector2 pos;
    Vector2 vel;
    float angle;
    float radius;       // সংঘর্ষ গণনার ব্যাসার্ধ
    float length;       // কলমের দৈর্ঘ্য
    float width;        // কলমের প্রস্থ
    Color bodyColor;
    Color capColor;
    bool fallen;
    int score;
};

// টেবিলের পরিধি
const Rectangle table = { 100, 80, 800, 540 };

void ResetRound(Pen &p1, Pen &p2) {
    p1.pos = { table.x + 200, table.y + table.height / 2.0f };
    p1.vel = { 0, 0 };
    p1.angle = 0.0f;
    p1.fallen = false;

    p2.pos = { table.x + table.width - 200, table.y + table.height / 2.0f };
    p2.vel = { 0, 0 };
    p2.angle = 180.0f;
    p2.fallen = false;
}

void DrawPen(const Pen &pen) {
    if (pen.fallen) return;

    Vector2 origin = { pen.length / 2.0f, pen.width / 2.0f };
    Rectangle dest = { pen.pos.x, pen.pos.y, pen.length, pen.width };
    
    // কলমের বডি
    DrawRectanglePro(dest, origin, pen.angle, pen.bodyColor);
    
    // কলমের ক্যাপ
    Rectangle capDest = { pen.pos.x, pen.pos.y, pen.length * 0.35f, pen.width };
    DrawRectanglePro(capDest, origin, pen.angle, pen.capColor);

    // কলমের নিব (টপ সাইড নির্দেশক ডট)
    Vector2 radVec = { cosf(pen.angle * DEG2RAD) * (pen.length / 2.0f), sinf(pen.angle * DEG2RAD) * (pen.length / 2.0f) };
    Vector2 tip = Vector2Add(pen.pos, radVec);
    DrawCircleV(tip, 3.0f, BLACK);
}

int main() {
    const int screenWidth = 1000;
    const int screenHeight = 700;
    InitWindow(screenWidth, screenHeight, "Pen Fight 2D - C++ Game");
    SetTargetFPS(60);

    Pen p1 = { {0, 0}, {0, 0}, 0.0f, 22.0f, 65.0f, 14.0f, RED, MAROON, false, 0 };
    Pen p2 = { {0, 0}, {0, 0}, 180.0f, 22.0f, 65.0f, 14.0f, SKYBLUE, BLUE, false, 0 };
    ResetRound(p1, p2);

    int currentTurn = 1; // ১ = Player 1, ২ = Player 2
    bool isAiming = false;
    Vector2 aimStart = { 0, 0 };
    const float friction = 0.982f;
    const float maxPower = 18.0f;

    while (!WindowShouldClose()) {
        Vector2 mousePos = GetMousePosition();
        bool isMoving = (Vector2Length(p1.vel) > 0.15f || Vector2Length(p2.vel) > 0.15f);

        // ইনপুট ও নিশানা নিয়ন্ত্রণ
        if (!isMoving && !p1.fallen && !p2.fallen) {
            Pen &activePen = (currentTurn == 1) ? p1 : p2;

            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                if (CheckCollisionPointCircle(mousePos, activePen.pos, activePen.radius + 15.0f)) {
                    isAiming = true;
                    aimStart = activePen.pos;
                }
            }

            if (isAiming && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
                Vector2 pullVector = Vector2Subtract(aimStart, mousePos);
                float pullDist = Vector2Length(pullVector);

                if (pullDist > 10.0f) {
                    float power = fminf(pullDist * 0.12f, maxPower);
                    Vector2 dir = Vector2Normalize(pullVector);
                    activePen.vel = Vector2Scale(dir, power);
                    activePen.angle = atan2f(dir.y, dir.x) * RAD2DEG;
                    
                    // পরবর্তী চালের জন্য টার্ন পরিবর্তন
                    currentTurn = (currentTurn == 1) ? 2 : 1;
                }
                isAiming = false;
            }
        }

        // ফিজিক্স আপডেট (গতি ও ঘর্ষণ)
        auto UpdatePhysics = [friction](Pen &p) {
            p.pos = Vector2Add(p.pos, p.vel);
            p.vel = Vector2Scale(p.vel, friction);
            if (Vector2Length(p.vel) < 0.15f) p.vel = { 0, 0 };

            // টেবিল সীমানা পরীক্ষা
            if (!p.fallen) {
                if (p.pos.x < table.x || p.pos.x > table.x + table.width ||
                    p.pos.y < table.y || p.pos.y > table.y + table.height) {
                    p.fallen = true;
                    p.vel = { 0, 0 };
                }
            }
        };

        UpdatePhysics(p1);
        UpdatePhysics(p2);

        // কলম দুটির স্থিতিস্থাপক সংঘর্ষ (Elastic Collision)
        if (!p1.fallen && !p2.fallen) {
            Vector2 delta = Vector2Subtract(p2.pos, p1.pos);
            float dist = Vector2Length(delta);
            float minDist = p1.radius + p2.radius;

            if (dist < minDist && dist > 0.001f) {
                Vector2 normal = Vector2Normalize(delta);
                float overlap = minDist - dist;
                p1.pos = Vector2Subtract(p1.pos, Vector2Scale(normal, overlap * 0.5f));
                p2.pos = Vector2Add(p2.pos, Vector2Scale(normal, overlap * 0.5f));

                Vector2 relVel = Vector2Subtract(p1.vel, p2.vel);
                float velAlongNormal = Vector2DotProduct(relVel, normal);

                if (velAlongNormal > 0) {
                    float restitution = 0.88f; // বাউন্স ফ্যাক্টর
                    float impulse = (1.0f + restitution) * velAlongNormal / 2.0f;
                    Vector2 impulseVec = Vector2Scale(normal, impulse);
                    p1.vel = Vector2Subtract(p1.vel, impulseVec);
                    p2.vel = Vector2Add(p2.vel, impulseVec);

                    p1.angle = atan2f(p1.vel.y, p1.vel.x) * RAD2DEG;
                    p2.angle = atan2f(p2.vel.y, p2.vel.x) * RAD2DEG;
                }
            }
        }

        // স্কোর এবং রাউন্ড রিস্টার্ট (R প্রেস করলে)
        if (IsKeyPressed(KEY_R)) {
            ResetRound(p1, p2);
        }

        // রেন্ডারিং
        BeginDrawing();
        ClearBackground(GetColor(0x1F2421FF)); // মেঝের রঙ

        // টেবিল আঁকা (উডেন ফিনিশ)
        DrawRectangleRec(table, GetColor(0xB08968FF));
        DrawRectangleLinesEx(table, 6, GetColor(0x7F5539FF));

        // গ্রিড/বর্ডার লাইন
        DrawRectangleLinesEx({ table.x + 12, table.y + 12, table.width - 24, table.height - 24 }, 2, Fade(BLACK, 0.15f));

        // নিশানা লাইন
        if (isAiming) {
            Pen &activePen = (currentTurn == 1) ? p1 : p2;
            Vector2 pullVector = Vector2Subtract(aimStart, mousePos);
            Vector2 targetAim = Vector2Add(activePen.pos, Vector2Scale(Vector2Normalize(pullVector), fminf(Vector2Length(pullVector), 120.0f)));
            DrawLineEx(activePen.pos, targetAim, 3.5f, Fade(YELLOW, 0.8f));
            DrawCircleV(targetAim, 5.0f, ORANGE);
        }

        // কলম ড্রয়িং
        DrawPen(p1);
        DrawPen(p2);

        // UI ও স্ট্যাটাস প্রদর্শন
        DrawText("PEN FIGHT 2D", 20, 20, 24, RAYWHITE);
        DrawText(TextFormat("Player 1: %s", (currentTurn == 1 ? "YOUR TURN" : "WAIT")), 100, 45, 18, RED);
        DrawText(TextFormat("Player 2: %s", (currentTurn == 2 ? "YOUR TURN" : "WAIT")), table.x + table.width - 180, 45, 18, SKYBLUE);

        if (p1.fallen) {
            DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.6f));
            DrawText("PLAYER 2 WINS!", screenWidth / 2 - 160, screenHeight / 2 - 30, 40, SKYBLUE);
            DrawText("Press [R] to Play Again", screenWidth / 2 - 120, screenHeight / 2 + 30, 20, RAYWHITE);
        } else if (p2.fallen) {
            DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.6f));
            DrawText("PLAYER 1 WINS!", screenWidth / 2 - 160, screenHeight / 2 - 30, 40, RED);
            DrawText("Press [R] to Play Again", screenWidth / 2 - 120, screenHeight / 2 + 30, 20, RAYWHITE);
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
