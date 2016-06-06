#pragma once

#include "DrawWndHandle.h"

#include <d3d9.h>

class CDrawWndSurface : public IDrawWndHandle
{
public:
	CDrawWndSurface();
	virtual ~CDrawWndSurface();

	virtual BOOL IsValid()
	{
		return (pDirect3DDevice_ != NULL);
	}
	virtual void Cleanup();
	virtual BOOL CreateDevice(HWND hwnd);
	virtual void UpdateCoordinate(float scale, ROTATIONTYPE rotate, POINT pos, const FrameData& frm, HWND hwnd);
	virtual void DrawFrame(const BYTE* pSrc, int width, int height);
	virtual void Render(); 

	virtual void OnFrmSizeChange(int cx, int cy)
	{
		ResetSurfaceRender(cx, cy);
	}

protected:
	BOOL ResetSurfaceBk(int width, int height);
	BOOL ResetSurfaceRender(int width, int height);
	BOOL CreateSurface(int width, int height, IDirect3DSurface9** pSurface);

protected:
	IDirect3D9* pDirect3D_;
	D3DPRESENT_PARAMETERS d3dpp_;
	IDirect3DDevice9* pDirect3DDevice_;
	IDirect3DSurface9* pDirect3DSurfaceRender_;
	IDirect3DSurface9* pDirect3DSurfaceBk_;

	CSize szSurfaceBk_;
	RECT rectDst_;
	RECT rectSrc_;

};

