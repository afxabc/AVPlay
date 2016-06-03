#pragma once

#include "base\log.h"

typedef enum _ROTATIONTYPE {
	ROTATION_0,
	ROTATION_90,
	ROTATION_180,
	ROTATION_270,
	ROTATION_N,
} ROTATIONTYPE;

class IDrawWndHandle
{
public:
	virtual ~IDrawWndHandle() {}

	virtual BOOL IsValid() = 0;
	virtual void Cleanup() = 0;
	virtual BOOL CreateDevice(HWND hwnd) = 0;

	virtual void UpdateCoordinate(float scale, ROTATIONTYPE rotate, POINT pos, SIZE szFrm, SIZE szWnd) = 0;

	virtual void OnFrmSizeChange(int cx, int cy) = 0;
	virtual void DrawFrame(const BYTE* pSrc, int width, int height) = 0;
	virtual void Render() = 0;
};

