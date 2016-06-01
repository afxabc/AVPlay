#pragma once

class IChildViewCallback
{
public:
	virtual void OnResetSize(int width, int height) = 0;
	virtual void ReportParams(float x, float y, float scale, float rotate, int width, int height, const RECT& r) = 0;
};

/////////////////////////////////////////////////////

#include "FrameData.h"
#include <d3d9.h>
#include <d3dx9.h>

// CDrawWndD3d
class CDrawWndD3d : public CWnd
{
	DECLARE_DYNAMIC(CDrawWndD3d)

public:
	CDrawWndD3d();
	virtual ~CDrawWndD3d();

	void setCallback(IChildViewCallback* cb)
	{
		cb_ = cb;
	}
	void ResetRect() {};
	void DrawFrame(const FrameData& f);
	void ResetCoordinate();

protected:
	void Cleanup();
	BOOL CreateDevice();

	void RecalculateRect(BOOL render = FALSE);
	void ResetSurfaceBk();
	void ResetSurface(int width, int height);
	BOOL CreateSurface(int width, int height, IDirect3DSurface9** pSurface);
	void RenderSurface();
	void DrawSurface(const FrameData& f);

	void UpdateMatSprite(BOOL render = FALSE);
	void ResetTexture(int width, int height);
	void RenderTexture();
	void DrawTexture(const FrameData& f);

protected:
	IChildViewCallback* cb_;

	float width_;
	float height_;
	IDirect3D9* pDirect3D_;
	D3DPRESENT_PARAMETERS d3dpp_;
	IDirect3DDevice9* pDirect3DDevice_;

	int scale_;	//szPercent_/100
	int xPos_;
	int yPos_;
	POINT posMove_;

	//USAGE_SURFACE
	RECT rectDst_;
	RECT rectSrc_;
	IDirect3DSurface9* pDirect3DSurfaceRender_;
	IDirect3DSurface9* pDirect3DSurfaceBk_;

	//USAGE_TEXTURE
	float rotation_;
	ID3DXSprite* pDirect3DSprite_;
	D3DXMATRIX matSprite_;
	IDirect3DTexture9* pDirect3DTexture_;

	// 生成的消息映射函数
protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnRotateAngle();
	afx_msg void OnZoomIn();
	afx_msg void OnZoomOut();
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnInitSize();
};


