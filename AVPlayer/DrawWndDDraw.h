#pragma once

#include "IDrawWndHandle.h"

#include <ddraw.h>

class CDrawWndDDraw : public IDrawWndHandle
{
public:
	CDrawWndDDraw(CWnd* wnd);
	virtual ~CDrawWndDDraw();

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
		ResetSurfaceFrm(cx, cy);
	}

protected:
	BOOL ResetSurfaceBk(const SIZE& szWnd);
	BOOL ResetSurfaceFrm(int width, int height);
	BOOL ResetSurface(int width, int height, LPDIRECTDRAWSURFACE* ppSurface);

protected:
	CWnd* rectWnd_;
	RECT rect_;
	CSize szWnd_;

	LPDIRECTDRAW lpDirectDraw_;	LPDIRECTDRAWSURFACE lpSurface_, lpSurfaceBk_, lpSurfaceFrm_;	LPDIRECTDRAWCLIPPER lpClipper_;
};

