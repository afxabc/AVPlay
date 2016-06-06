#pragma once

#include "DrawWndHandle.h"

#include <d3d9.h>
#include <d3dx9.h>

class CDrawWndSprite : public IDrawWndHandle
{
public:
	CDrawWndSprite();
	virtual ~CDrawWndSprite();

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
		ResetTexture(cx, cy);
	}

protected:
	BOOL ResetTexture(int width, int height);

protected:
	IDirect3D9* pDirect3D_;
	D3DPRESENT_PARAMETERS d3dpp_;
	IDirect3DDevice9* pDirect3DDevice_;

	ID3DXSprite* pDirect3DSprite_;
	D3DXMATRIX matSprite_;
	IDirect3DTexture9* pDirect3DTexture_;

};

