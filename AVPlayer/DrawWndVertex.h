#pragma once

#include "DrawWndHandle.h"

#include <d3d9.h>

class CDrawWndVertex : public IDrawWndHandle
{
public:
	CDrawWndVertex();
	~CDrawWndVertex();

	virtual BOOL IsValid()
	{
		return (pDirect3DDevice_ != NULL);
	}
	virtual void Cleanup();
	virtual BOOL CreateDevice(HWND hwnd);
	virtual void UpdateCoordinate(float scale, ROTATIONTYPE rotate, POINT pos, SIZE szFrm, SIZE szWnd);
	virtual void DrawFrame(const BYTE* pSrc, int width, int height);
	virtual void Render();

	virtual void OnFrmSizeChange(int cx, int cy)
	{
		ResetTexture(cx, cy);
	}

protected:
	BOOL ResetTexture(int width, int height);

protected:
	IDirect3D9* pDirect3D_;
	D3DPRESENT_PARAMETERS d3dpp_;
	IDirect3DDevice9* pDirect3DDevice_;

	IDirect3DVertexBuffer9* pDirect3DVertexBuffer_;
	IDirect3DTexture9* pDirect3DTexture_;

};

