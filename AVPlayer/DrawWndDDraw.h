#pragma once

#include "IDrawWndHandle.h"

#include <ddraw.h>

class CDrawWndDDraw : public IDrawWndHandle
{
public:
	CDrawWndDDraw();
	~CDrawWndDDraw();

	virtual BOOL IsValid()
	{
		return (lpSurface_ != NULL);
	}
	virtual void Cleanup();
	virtual BOOL CreateDevice(HWND hwnd);
	virtual void UpdateCoordinate(float scale, float rotate, POINT pos, SIZE szFrm, SIZE szWnd);
	virtual void DrawFrame(const BYTE* pSrc, int width, int height);
	virtual void Render();

	virtual void OnFrmSizeChange(int cx, int cy)
	{
		ResetSurface(cx, cy);
	}

protected:
	BOOL ResetSurface(int width, int height);

protected:

	RECT rect_;

	//directx
	int width_;
	int height_;
	LPDIRECTDRAW lpDirectDraw_;	LPDIRECTDRAWSURFACE lpSurface_, lpBkSurface_;	LPDIRECTDRAWCLIPPER lpClipper_;
};

