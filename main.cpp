#include "raylib.h"
#define RAYGUI_IMPLEMENTATION 
#include "raygui.h"           
#include "rlgl.h"
#include "FastNoiseLite.h"
#include "raymath.h"
#include <vector>

struct SpaceParticle {
    Vector3 position;
    Color color;
    float baseAlpha; 
};

std::vector<SpaceParticle> particles;
FastNoiseLite noise;
const int cubeSize = 240; 
const int sampleStep = 3;  
float maxRadius = cubeSize / 1.8f;

void GenerateUniverse(float densityThreshold) {
    particles.clear();

    for (int x = -cubeSize/2; x < cubeSize/2; x += sampleStep) {
        for (int y = -cubeSize/2; y < cubeSize/2; y += sampleStep) {
            for (int z = -cubeSize/2; z < cubeSize/2; z += sampleStep) {
                
                float distFromCenter = sqrtf(x*x + y*y + z*z);
                if (distFromCenter > maxRadius + GetRandomValue(-15, 15)) continue;

                float n = noise.GetNoise((float)x, (float)y, (float)z);

                if (n > densityThreshold) { 
                    SpaceParticle p;
                    p.position = Vector3{
                        x + ((float)GetRandomValue(-5, 5) / 10.0f),
                        y + ((float)GetRandomValue(-5, 5) / 10.0f),
                        z + ((float)GetRandomValue(-5, 5) / 10.0f)
                    };

                    if (n > 0.45f) {
                        p.color = Color{ 255, 255, 255, 255 }; 
                        p.baseAlpha = 180.0f; 
                    } else if (n > 0.28f) {
                        p.color = Color{ 0, 120, 255, 255 };   
                        p.baseAlpha = 90.0f;  
                    } else {
                        p.color = Color{ 140, 0, 255, 255 };   
                        p.baseAlpha = 45.0f;  
                    }
                    particles.push_back(p);
                }
            }
        }
    }
}

int main() {
    const int screenWidth = 1280;
    const int screenHeight = 720;
    InitWindow(screenWidth, screenHeight, "Cosmic Web 3D - Density & Glow Controls");

    Camera3D camera = { 0 };
    camera.position = Vector3{ 0.0f, 20.0f, 150.0f }; 
    camera.target = Vector3{ 0.0f, 0.0f, 0.0f };      
    camera.up = Vector3{ 0.0f, 1.0f, 0.0f };          
    camera.fovy = 65.0f;                               
    camera.projection = CAMERA_PERSPECTIVE;

    bool menuMode = false;
    DisableCursor(); 
    SetTargetFPS(60); 

    noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noise.SetFractalType(FastNoiseLite::FractalType_FBm);
    noise.SetFractalOctaves(3); 
    noise.SetFrequency(0.018f); 

    float glowMultiplier = 1.0f; 
    float universeDensitySetting = 0.5f;
    
    float currentThreshold = 0.26f - (universeDensitySetting * 0.20f); 
    
    GenerateUniverse(currentThreshold);

    int renderedCount = 0; 

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_TAB)) {
            menuMode = !menuMode;
            if (menuMode) EnableCursor(); else DisableCursor();
        }

        if (!menuMode) {
            Vector3 forward = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
            Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, camera.up));

            float speed = 40.0f; 
            float dtSpeed = speed * GetFrameTime();

            if (IsKeyDown(KEY_W)) {
                camera.position = Vector3Add(camera.position, Vector3Scale(forward, dtSpeed));
                camera.target = Vector3Add(camera.target, Vector3Scale(forward, dtSpeed));
            }
            if (IsKeyDown(KEY_S)) {
                camera.position = Vector3Subtract(camera.position, Vector3Scale(forward, dtSpeed));
                camera.target = Vector3Subtract(camera.target, Vector3Scale(forward, dtSpeed));
            }
            if (IsKeyDown(KEY_D)) {
                camera.position = Vector3Add(camera.position, Vector3Scale(right, dtSpeed));
                camera.target = Vector3Add(camera.target, Vector3Scale(right, dtSpeed));
            }
            if (IsKeyDown(KEY_A)) {
                camera.position = Vector3Subtract(camera.position, Vector3Scale(right, dtSpeed));
                camera.target = Vector3Subtract(camera.target, Vector3Scale(right, dtSpeed));
            }

            if (IsKeyDown(KEY_SPACE)) { 
                camera.position.y += dtSpeed;
                camera.target.y += dtSpeed;
            }
            if (IsKeyDown(KEY_LEFT_SHIFT)) { 
                camera.position.y -= dtSpeed;
                camera.target.y -= dtSpeed;
            }

            UpdateCamera(&camera, CAMERA_FREE);
        }

        BeginDrawing();
            ClearBackground(BLACK); 

            BeginMode3D(camera);
                
                rlSetBlendMode(BLEND_ADDITIVE);
                rlSetLineWidth(1.0f + (glowMultiplier * 0.8f)); 

                rlBegin(RL_LINES);
                renderedCount = 0;

                Vector3 currentForward = Vector3Normalize(Vector3Subtract(camera.target, camera.position));

                for (const auto& p : particles) {
                    Vector3 toParticle = Vector3Normalize(Vector3Subtract(p.position, camera.position));
                    float dotProduct = Vector3DotProduct(currentForward, toParticle);

                    if (dotProduct < 0.50f) continue; 

                    renderedCount++;
                    
                    float dynamicAlpha = p.baseAlpha * glowMultiplier;
                    if (dynamicAlpha > 255.0f) dynamicAlpha = 255.0f; 
                    
                    rlColor4ub(p.color.r, p.color.g, p.color.b, (unsigned char)dynamicAlpha);
                    
                    float size = 0.45f;
                    rlVertex3f(p.position.x, p.position.y - size, p.position.z);
                    rlVertex3f(p.position.x, p.position.y + size, p.position.z);
                    
                    rlVertex3f(p.position.x - size, p.position.y, p.position.z);
                    rlVertex3f(p.position.x + size, p.position.y, p.position.z);

                    rlVertex3f(p.position.x, p.position.y, p.position.z - size);
                    rlVertex3f(p.position.x, p.position.y, p.position.z + size);
                }
                rlEnd();

                rlSetLineWidth(1.0f);
                rlSetBlendMode(BLEND_ALPHA);

            EndMode3D();

            DrawFPS(10, 10);
            DrawText(TextFormat("Total galaxies: %i", (int)particles.size()), 10, 35, 20, SKYBLUE);
            DrawText(TextFormat("Visible now: %i", renderedCount), 10, 60, 20, GREEN);
            
            if (menuMode) {
                DrawText("MENU ACTIVE - Adjust settings below", 10, 95, 16, YELLOW);
                DrawText("Press TAB to resume flight", 10, 115, 16, ORANGE);
                
                GuiSlider(Rectangle{ 250, 15, 180, 20 }, "Glow Intensity", NULL, &glowMultiplier, 0.1f, 3.0f);
                
                GuiSlider(Rectangle{ 250, 45, 180, 20 }, "Galaxies Amount", NULL, &universeDensitySetting, 0.05f, 1.0f);
                
                if (GuiButton(Rectangle{ 450, 45, 140, 20 }, "Apply & Regenerate")) {
                    float newThreshold = 0.26f - (universeDensitySetting * 0.20f); 
                    GenerateUniverse(newThreshold);
                }
            } else {
                DrawText("Flight Controls: W,A,S,D | Move Up: SPACE | Move Down: L-SHIFT", 10, 95, 16, GRAY);
                DrawText("Press TAB to open Menu & adjust settings", 10, 115, 16, LIGHTGRAY);
            }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
