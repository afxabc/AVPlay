#pragma once

#include "base\log.h"

class IDrawWndHandle
{
public:
	virtual BOOL IsValid() = 0;
	virtual void Cleanup() = 0;
	virtual BOOL CreateDevice(HWND hwnd) = 0;

	virtual void UpdateCoordinate(float scale, float rotate, POINT pos, SIZE szFrm, SIZE szWnd) = 0;

	virtual void OnFrmSizeChange(int cx, int cy) = 0;
	virtual void DrawFrame(const BYTE* pSrc, int width, int height) = 0;
	virtual void Render() = 0;
};

