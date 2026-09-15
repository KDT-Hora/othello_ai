#pragma once
#include <cstdint>
#include <string>

namespace dxlib {

// Mock DXLib API — for unit tests when real DXLib is unavailable.
// In production, link against the real dxlib.lib and use the real headers.

inline bool DXOpen(int width, int height, const char* title) { return true; }
inline void DXClose() {}
inline void DXSetBackColor(unsigned char r, unsigned char g, unsigned char b) {}
inline void DXClear() {}
inline void DXSetColor(unsigned char r, unsigned char g, unsigned char b) {}
inline void DXSetTextColor(unsigned char r, unsigned char g, unsigned char b) {}
inline void DXDrawLine(int x1, int y1, int x2, int y2) {}
inline void DXDrawEllipse(float cx, float cy, float radius) {}
inline void DXFillEllipse(float cx, float cy, float radius) {}
inline void DXSetFillColor(unsigned char r, unsigned char g, unsigned char b) {}
inline void DXSetLineColor(unsigned char r, unsigned char g, unsigned char b) {}
inline void DXDrawTextA(const char* text, int x, int y, int font) {}

enum { KEY_ESCAPE = 27, KEY_R = 'r', KEY_0 = '0' };
inline int DXGetKey() { return 0; }   // mock: no input in unit tests
} // namespace dxlib
