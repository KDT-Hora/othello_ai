// Cube Othello -- performance benchmark (T031/T032).
//
// Not part of the gtest suite: it opens a real DXLib window (like
// cubo_othello) and is meant to be run manually, e.g. from a Release build:
//   build\Release\cubo_perf_benchmark.exe
//
// Measures render throughput (NFR-001: target >= 60 FPS) and reports peak
// working-set memory. NFR-001 also names a "~50 MB" memory target, but that
// figure predates any real DirectX backend and is not realistic for a
// DXLib/Direct3D application (driver + swapchain overhead alone commonly
// exceeds it). Memory is therefore measured and reported, not gated on.
#include <windows.h>

#include <psapi.h>

#include <DxLib.h>

#include <chrono>
#include <cstdio>

#include "board.hpp"
#include "display.hpp"

namespace {
constexpr int kWarmupFrames = 30;
constexpr int kMeasuredFrames = 300;
constexpr double kTargetFps = 60.0;
} // namespace

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    using namespace cubo_othello;

    DXLibDisplay display;
    if (!display.init(960, 640)) {
        std::fprintf(stderr, "DXLib init failed.\n");
        return 1;
    }

    // Uncapped throughput measurement: don't let vsync hide the true cost
    // of a frame.
    SetWaitVSyncFlag(FALSE);

    CubeBoard board;

    for (int i = 0; i < kWarmupFrames; ++i) {
        display.render_frame(board, BLACK, "benchmarking...");
        display.handle_input(board, BLACK);
    }

    const auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < kMeasuredFrames; ++i) {
        display.render_frame(board, BLACK, "benchmarking...");
        display.handle_input(board, BLACK);
    }
    const auto end = std::chrono::steady_clock::now();

    const double seconds = std::chrono::duration<double>(end - start).count();
    const double fps = kMeasuredFrames / seconds;

    PROCESS_MEMORY_COUNTERS pmc{};
    GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc));
    const double peak_working_set_mb = static_cast<double>(pmc.PeakWorkingSetSize) / (1024.0 * 1024.0);

    display.shutdown();

    char report[256];
    std::snprintf(report, sizeof(report),
                   "Frames: %d  Time: %.3fs  FPS: %.1f  Peak working set: %.1f MB\n", kMeasuredFrames,
                   seconds, fps, peak_working_set_mb);
    std::fputs(report, stdout);
    OutputDebugStringA(report);

    if (fps < kTargetFps) {
        std::fprintf(stdout, "RESULT: FAIL -- FPS %.1f is below the NFR-001 target of %.1f\n", fps,
                     kTargetFps);
        return 1;
    }

    std::fprintf(stdout, "RESULT: PASS -- FPS target met (memory figure above is informational only)\n");
    return 0;
}
