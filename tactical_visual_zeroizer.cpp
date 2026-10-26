#include "raylib.h"
#include "raymath.h"
#include <vector>
#include <string>
#include <random>
#include <cmath>
#include <chrono>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <portaudio.h>

// --- TACTICAL SECURITY CLASSIFICATIONS ---
enum SecurityTier {
    TIER_RESTRICTED = 1,
    TIER_CONFIDENTIAL = 2,
    TIER_SECRET_MOD = 3,
    TIER_TOP_SECRET_SAG = 4
};

struct TacticalPacket {
    uint64_t packetID;
    SecurityTier tier;
    Vector3 position;
    Vector3 velocity;
    Color color;
    float size;
    uint8_t secureBuffer[128];
    bool isZeroized;
};

// Reduced to 400 for clean, distinct visual tracking
const int MAX_PACKETS = 400; 
const float EVENT_HORIZON_RADIUS = 1.5f;
const float MIC_GAIN = 5.0f; // High sensitivity multiplier for real-time speech/noise

// Shared global volatile audio amplitude updated in real-time by PortAudio hardware thread
static volatile float g_micAmplitude = 0.0f;

// --- PORTAUDIO HARDWARE AUDIO STREAM CALLBACK ---
static int PortAudioCallback(const void *inputBuffer, void *outputBuffer,
                             unsigned long framesPerBuffer,
                             const PaStreamCallbackTimeInfo* timeInfo,
                             PaStreamCallbackFlags statusFlags,
                             void *userData) {
    const float *in = (const float*)inputBuffer;
    float peak = 0.0f;
    
    if (in != nullptr) {
        for (unsigned long i = 0; i < framesPerBuffer; i++) {
            float val = fabsf(in[i]);
            if (val > peak) {
                peak = val;
            }
        }
    }
    // Apply hardware gain boost so normal voice/claps trigger instantly
    g_micAmplitude = peak * MIC_GAIN;
    if (g_micAmplitude > 1.0f) g_micAmplitude = 1.0f;
    
    return paContinue;
}

void ResetTacticalPacket(TacticalPacket& pkt, uint64_t id) {
    float angle = (float)(rand() % 360) * DEG2RAD;
    float distance = 1.8f + ((float)(rand() % 50) / 20.0f); 
    
    pkt.packetID = id;
    pkt.tier = static_cast<SecurityTier>((rand() % 4) + 1);
    pkt.position = (Vector3){ cosf(angle) * distance, ((float)(rand() % 100)/100.0f - 0.5f) * 0.4f, sinf(angle) * distance };
    
    float speed = 0.10f + ((float)(rand() % 20) / 200.0f);
    pkt.velocity = (Vector3){ -sinf(angle) * speed, 0.0f, cosf(angle) * speed };
    
    pkt.isZeroized = false;
    pkt.size = 0.07f + ((float)(rand() % 20) / 400.0f); // Slightly larger for better visibility

    switch(pkt.tier) {
        case TIER_RESTRICTED:     pkt.color = SKYBLUE; break;
        case TIER_CONFIDENTIAL:   pkt.color = GREEN; break;
        case TIER_SECRET_MOD:     pkt.color = ORANGE; break;
        case TIER_TOP_SECRET_SAG: pkt.color = RED; break;
    }

    std::memset(pkt.secureBuffer, 0xAB, sizeof(pkt.secureBuffer));
}

// --- DRDO-SAG 3-PASS CRYPTOGRAPHIC SANITIZATION ---
void ExecuteDRDOSAGSanitization(TacticalPacket& pkt) {
    // Pass 1: Zero-fill
    std::memset(pkt.secureBuffer, 0x00, sizeof(pkt.secureBuffer));
    // Pass 2: Complement-fill
    std::memset(pkt.secureBuffer, 0xFF, sizeof(pkt.secureBuffer));
    // Pass 3: Cryptographic Pseudo-Random Overwrite
    for (size_t i = 0; i < sizeof(pkt.secureBuffer); ++i) {
        pkt.secureBuffer[i] = static_cast<uint8_t>(rand() % 256);
    }
    pkt.isZeroized = true;
}

int main() {
    const int screenWidth = 1280;
    const int screenHeight = 720;
    InitWindow(screenWidth, screenHeight, "DRDO-SAG // FUNCTIONAL REAL-TIME ACOUSTIC ZEROIZATION CONSOLE");

    // --- INITIALIZE PORTAUDIO MICROPHONE STREAM ---
    PaError paErr = Pa_Initialize();
    PaStream *paStream = nullptr;
    bool portAudioActive = false;
    if (paErr == paNoError) {
        paErr = Pa_OpenDefaultStream(&paStream, 1, 0, paFloat32, 44100, 256, PortAudioCallback, nullptr);
        if (paErr == paNoError) {
            Pa_StartStream(paStream);
            portAudioActive = true;
        }
    }

    Camera3D camera = { 0 };
    camera.position = (Vector3){ 0.0f, 4.0f, 7.0f };
    camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    std::vector<TacticalPacket> tacticalBuffer(MAX_PACKETS);
    for (int i = 0; i < MAX_PACKETS; i++) {
        ResetTacticalPacket(tacticalBuffer[i], 10000 + i);
    }

    SetTargetFPS(60);

    float simTime = 0.0f;
    unsigned long long totalZeroizedBytes = 0;
    unsigned long long packetsProcessed = 0;
    float smoothedAmplitude = 0.0f;

    std::vector<float> waveHistory(120, 0.0f);

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        simTime += dt;

        // Fetch live microphone audio amplitude from PortAudio thread
        float liveMic = portAudioActive ? g_micAmplitude : 0.05f;

        // Manual override: holding Left Mouse Click or pressing [S] forces an immediate acoustic surge
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsKeyPressed(KEY_S)) {
            liveMic = 0.95f;
        }

        smoothedAmplitude += (liveMic - smoothedAmplitude) * 0.25f;

        for (size_t i = 0; i < waveHistory.size() - 1; i++) waveHistory[i] = waveHistory[i + 1];
        waveHistory.back() = smoothedAmplitude;

        // --- 8X NORMAL GRAVITY ENGINE (DYNAMICALLY DRIVEN BY MIC) ---
        float baseGravity = 8.0f; 
        float dynamicGravity = baseGravity + (smoothedAmplitude * 40.0f);

        for (int i = 0; i < MAX_PACKETS; i++) {
            TacticalPacket& pkt = tacticalBuffer[i];

            Vector3 center = { 0.0f, 0.0f, 0.0f };
            Vector3 dir = Vector3Subtract(center, pkt.position);
            float dist = Vector3Length(dir);
            Vector3 normDir = Vector3Normalize(dir);

            // Gravitational pull straight to event horizon + vortex swirl
            float radialAccel = dynamicGravity / (dist * dist + 0.01f);
            Vector3 tangent = { -normDir.z, 0.0f, normDir.x };
            float vortexSwirl = 4.0f / (dist + 0.05f);

            pkt.velocity = Vector3Add(pkt.velocity, Vector3Scale(normDir, radialAccel * dt));
            pkt.velocity = Vector3Add(pkt.velocity, Vector3Scale(tangent, vortexSwirl * dt));
            
            pkt.velocity = Vector3Scale(pkt.velocity, 0.94f); // Air damping
            pkt.position = Vector3Add(pkt.position, pkt.velocity);

            // --- ZEROIZATION EVENT HORIZON BREACH ---
            if (dist < EVENT_HORIZON_RADIUS && !pkt.isZeroized) {
                ExecuteDRDOSAGSanitization(pkt);
                totalZeroizedBytes += sizeof(pkt.secureBuffer);
                packetsProcessed++;
                
                // Instantly respawn destroyed packets back into accretion orbit
                ResetTacticalPacket(pkt, 10000 + (rand() % 5000));
            }
        }

        float camAngle = simTime * 0.05f;
        camera.position.x = sinf(camAngle) * 8.0f;
        camera.position.z = cosf(camAngle) * 8.0f;

        // --- RENDER PIPELINE ---
        BeginDrawing();
            Color bgCol = (smoothedAmplitude > 0.12f) ? (Color){ 40, 3, 10, 255 } : (Color){ 2, 4, 8, 255 };
            ClearBackground(bgCol);

            BeginMode3D(camera);
                DrawGrid(20, 1.0f);

                // Black Hole Core Singularity (Event Horizon)
                DrawSphere(Vector3Zero(), EVENT_HORIZON_RADIUS, BLACK);
                
                // Corona Rings (reacts live to microphone voice/claps)
                Color coronaCol = (smoothedAmplitude > 0.12f) ? RED : (Color){ 0, 255, 170, 240 };
                DrawSphereWires(Vector3Zero(), EVENT_HORIZON_RADIUS + 0.04f, 32, 32, coronaCol);
                DrawSphereWires(Vector3Zero(), EVENT_HORIZON_RADIUS + 0.18f, 16, 16, (Color){ 0, 110, 250, 120 });

                if (smoothedAmplitude > 0.12f) {
                    float shockwave = EVENT_HORIZON_RADIUS + fmodf(simTime * 4.0f, 2.2f);
                    DrawSphereWires(Vector3Zero(), shockwave, 24, 24, (Color){ 255, 50, 50, 160 });
                }

                // Render 400 Distinct Data Cubes
                for (int i = 0; i < MAX_PACKETS; i++) {
                    DrawCubeV(tacticalBuffer[i].position, (Vector3){ tacticalBuffer[i].size, tacticalBuffer[i].size, tacticalBuffer[i].size }, tacticalBuffer[i].color);
                    DrawCubeWiresV(tacticalBuffer[i].position, (Vector3){ tacticalBuffer[i].size, tacticalBuffer[i].size, tacticalBuffer[i].size }, BLACK);
                }
            EndMode3D();

            // --- MILITARY COMMAND HUD OVERLAY ---
            DrawRectangle(15, 15, 580, 310, (Color){ 3, 6, 12, 235 });
            Color hudBorder = (smoothedAmplitude > 0.12f) ? RED : (Color){ 0, 255, 140, 255 };
            DrawRectangleLines(15, 15, 580, 310, hudBorder);

            DrawText("MINISTRY OF DEFENCE // LIVE MIC ACOUSTIC ZEROIZER", 25, 25, 13, GREEN);
            DrawText("COMPLIANCE PROTOCOL: DRDO-SAG 3-Pass Crypto Sanitization", 25, 45, 12, SKYBLUE);
            
            std::string statusStr = "STATUS: MIC ACTIVE // 8X BASE GRAVITY";
            Color statusCol = GREEN;
            if (smoothedAmplitude > 0.12f) {
                statusStr = "ALERT: AUDIO DETECTED // GRAVITY SURGED";
                statusCol = RED;
            }
            DrawText(statusStr.c_str(), 25, 68, 13, statusCol);

            DrawText(TextFormat("Microphone Input Level: %.2f [Gain Boost Active]", smoothedAmplitude), 25, 95, 12, RAYWHITE);
            DrawText(TextFormat("Total Data Sanitized: %llu Bytes", totalZeroizedBytes), 25, 118, 12, RAYWHITE);
            DrawText(TextFormat("Sectors Zeroized & Re-created: %llu", packetsProcessed), 25, 141, 12, RAYWHITE);

            // Real-Time Oscilloscope Monitor Box
            DrawRectangle(25, 170, 560, 42, (Color){ 6, 10, 18, 255 });
            DrawRectangleLines(25, 170, 560, 42, DARKGREEN);
            for (size_t i = 0; i < waveHistory.size() - 1; i++) {
                int x1 = 30 + (int)(i * 4.4f);
                int y1 = 202 - (int)(waveHistory[i] * 28.0f);
                int x2 = 30 + (int)((i + 1) * 4.4f);
                int y2 = 202 - (int)(waveHistory[i + 1] * 28.0f);
                DrawLine(x1, y1, x2, y2, GREEN);
            }
            DrawText("PORTAUDIO REAL-TIME OSCILLOSCOPE MONITOR", 35, 176, 9, DARKGREEN);

            // Functional Instructions Box
            DrawRectangle(25, 224, 560, 85, (Color){ 10, 15, 25, 255 });
            DrawText("LIVE ACOUSTIC INTERACTION:", 35, 232, 11, YELLOW);
            DrawText("• SPEAK / CLAP INTO MIC: Instantly spikes gravity to shred packets.", 35, 250, 10, RAYWHITE);
            DrawText("• 3-PASS WIPE: Every cube entering event horizon is cryptographically wiped.", 35, 268, 10, GREEN);

            // Bottom technical classification banner
            DrawRectangle(15, screenHeight - 45, screenWidth - 30, 32, (Color){ 5, 8, 14, 240 });
            DrawText("CLASSIFICATION: TOP SECRET // INDIGENOUS DEFENSE FRAMEWORK // RESTRICTED", 25, screenHeight - 35, 12, RED);

        EndDrawing();
    }

    if (portAudioActive) {
        Pa_StopStream(paStream);
        Pa_CloseStream(paStream);
        Pa_Terminate();
    }
    CloseWindow();
    return 0;
}