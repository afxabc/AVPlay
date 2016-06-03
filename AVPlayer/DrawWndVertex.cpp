#include "stdafx.h"
#include "DrawWndVertex.h"



typedef struct
{
	FLOAT x, y, z;      // vertex untransformed position
	FLOAT rhw;      // vertex untransformed position
	FLOAT tu, tv;     // texture relative coordinates
} CUSTOMVERTEX;

// Custom flexible vertex format (FVF), which describes custom vertex structure
#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZRHW|D3DFVF_TEX1)


CDrawWndVertex::CDrawWndVertex()
	: pDirect3D_(NULL)
	, pDirect3DDevice_(NULL)
	, pDirect3DVertexBuffer_(NULL)
	, pDirect3DTexture_(NULL)
{
}

CDrawWndVertex::~CDrawWndVertex()
{
	Cleanup();
}

void CDrawWndVertex::Cleanup()
{
	if (pDirect3DTexture_)
		pDirect3DTexture_->Release(), pDirect3DTexture_ = NULL;

	if (pDirect3DVertexBuffer_)
		pDirect3DVertexBuffer_->Release(), pDirect3DVertexBuffer_ = NULL;

	if (pDirect3DDevice_)
		pDirect3DDevice_->Release(), pDirect3DDevice_ = NULL;

	if (pDirect3D_)
		pDirect3D_->Release(), pDirect3D_ = NULL;
}

BOOL CDrawWndVertex::CreateDevice(HWND hwnd)
{
	if (!pDirect3D_)
	{
		pDirect3D_ = ::Direct3DCreate9(D3D_SDK_VERSION);
		if (pDirect3D_ == NULL)
		{
			LOGE("Direct3DCreate9 failed !!");
			return FALSE;
		}
	}

	ZeroMemory(&d3dpp_, sizeof(d3dpp_));
	d3dpp_.Windowed = TRUE;
	d3dpp_.BackBufferCount = 1;
	d3dpp_.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp_.BackBufferFormat = D3DFMT_UNKNOWN;

	HRESULT ret = pDirect3D_->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd,
		D3DCREATE_MIXED_VERTEXPROCESSING | D3DCREATE_NOWINDOWCHANGES,
		&d3dpp_, &pDirect3DDevice_);
	if (FAILED(ret))
	{
		LOGE("pDirect3D_->CreateDevice failed !!");
		//		Cleanup();
		return FALSE;
	}
	pDirect3DDevice_->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);

	ret = pDirect3DDevice_->CreateVertexBuffer(4 * sizeof(CUSTOMVERTEX),
		0, D3DFVF_CUSTOMVERTEX,
		D3DPOOL_DEFAULT,
		&pDirect3DVertexBuffer_, NULL);
	if (FAILED(ret))
	{
		LOGE("pDirect3DDevice_->CreateVertexBuffer !!");
		return FALSE;
	}

	float x = 0.0f;
	float y = 0.0f;
	float z = 0.0f;
	float rhw = 10.0f;
	CUSTOMVERTEX vertices[] = {
		{ x, y, z, rhw, 0.0f, 0.0f },
		{ x, y, z, rhw, 1.0f, 0.0f },
		{ x, y, z, rhw, 1.0f, 1.0f },
		{ x, y, z, rhw, 0.0f, 1.0f }
	};

	// Fill Vertex Buffer
	CUSTOMVERTEX *pVertex;
	ret = pDirect3DVertexBuffer_->Lock(0, 4 * sizeof(CUSTOMVERTEX), (void**)&pVertex, 0);
	if (!FAILED(ret))
	{
		memcpy(pVertex, vertices, sizeof(vertices));
		pDirect3DVertexBuffer_->Unlock();
	}
	else LOGE("pDirect3DVertexBuffer_->Lock !!");

	return TRUE;
}

void CDrawWndVertex::UpdateCoordinate(float scale, ROTATIONTYPE rotate, POINT pos, SIZE szFrm, SIZE szWnd)
{
	if (pDirect3DVertexBuffer_ == NULL)
		return;

	CUSTOMVERTEX *vertex;
	HRESULT ret = pDirect3DVertexBuffer_->Lock(0, 4 * sizeof(CUSTOMVERTEX), (void**)&vertex, 0);
	if (FAILED(ret))
	{
		LOGE("pDirect3DVertexBuffer_->Lock !!");
		return;
	}

	float x = pos.x;
	float y = pos.y;

	float scalex = scale;// *szFrm.cx / szWnd.cx;// *1.07563f;
	float scaley = scale;// *szFrm.cy / szWnd.cy;// *0.951f;
	float WIDTH = (rotate%2==0)?(float)szFrm.cx*scalex:(float)szFrm.cy*scalex;
	float HEIGHT = (rotate%2==0)?(float)szFrm.cy*scaley:(float)szFrm.cx*scaley;

	int p0 = rotate;
	int p1 = (rotate + 1) % ROTATION_N;
	int p2 = (rotate + 2) % ROTATION_N;
	int p3 = (rotate + 3) % ROTATION_N;

	vertex[p0].x = x; vertex[p0].y = y;				//(0,0)
	vertex[p1].x = x+WIDTH; vertex[p1].y = y;			//(1,0)
	vertex[p2].x = x+WIDTH; vertex[p2].y = y+HEIGHT;	//(1,1)
	vertex[p3].x = x; vertex[p3].y = y+HEIGHT;		//(0,1)

	pDirect3DVertexBuffer_->Unlock();
}

void CDrawWndVertex::DrawFrame(const BYTE * pSrc, int width, int height)
{
	if (pDirect3DTexture_ == NULL && !ResetTexture(width, height))
		return;

	D3DLOCKED_RECT d3d_rect;
	//Locks a rectangle on a texture resource.
	//And then we can manipulate pixel data in it.
	HRESULT ret = pDirect3DTexture_->LockRect(0, &d3d_rect, NULL, D3DLOCK_DISCARD);
	if (FAILED(ret))
	{
		LOGE("pDirect3DTexture_->LockRect !!");
		return;
	}

	// Copy pixel data to texture
	byte *pDest = (byte *)d3d_rect.pBits;
	int stride = d3d_rect.Pitch;
	unsigned long i = 0;

#if LOAD_YUV420P
	for (i = 0; i < height_; i++) {
		memcpy(pDest + i * stride, pSrc + i * width_, width_);
	}
	for (i = 0; i < height_ / 2; i++) {
		memcpy(pDest + stride * height_ + i * stride / 2, pSrc + width_ * height_ + width_ * height_ / 4 + i * width_ / 2, width_ / 2);
	}
	for (i = 0; i < height_ / 2; i++) {
		memcpy(pDest + stride * height_ + stride * height_ / 4 + i * stride / 2, pSrc + width_ * height_ + i * width_ / 2, width_ / 2);
	}

#else
	int pixel_w_size = width * 4;
//	if (stride != pixel_w_size)
	{
		for (i = 0; i< height; i++)
		{
			memcpy(pDest, pSrc, pixel_w_size);
			pDest += stride;
			pSrc += pixel_w_size;
		}
	}
//	else memcpy(pDest, pSrc, pixel_w_size*height);
#endif

	pDirect3DTexture_->UnlockRect(0);

	Render();
}

void CDrawWndVertex::Render()
{
	if (pDirect3DDevice_ == NULL)
		return;

	if (pDirect3DDevice_->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
	{
		pDirect3DDevice_->Reset(&d3dpp_);
	}

	pDirect3DDevice_->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
	pDirect3DDevice_->BeginScene();

	if (pDirect3DTexture_ != NULL && pDirect3DVertexBuffer_ != NULL)
	{
		pDirect3DDevice_->SetTexture(0, pDirect3DTexture_);
		pDirect3DDevice_->SetStreamSource(0, pDirect3DVertexBuffer_, 0, sizeof(CUSTOMVERTEX));
		pDirect3DDevice_->SetFVF(D3DFVF_CUSTOMVERTEX);
		pDirect3DDevice_->DrawPrimitive(D3DPT_TRIANGLEFAN, 0, 2);
	}
	
	pDirect3DDevice_->EndScene();
	pDirect3DDevice_->Present(NULL, NULL, NULL, NULL);
}

BOOL CDrawWndVertex::ResetTexture(int width, int height)
{
	if (pDirect3DDevice_ == NULL)
		return FALSE;

	if (pDirect3DTexture_)
		pDirect3DTexture_->Release(), pDirect3DTexture_ = NULL;

	D3DFORMAT format;
#if LOAD_YUV420P
	format = (D3DFORMAT)MAKEFOURCC('Y', 'V', '1', '2');
#else
	format = D3DFMT_X8R8G8B8;
#endif
	HRESULT ret = pDirect3DDevice_->CreateTexture(width, height, 1,
		0,
		format,
		D3DPOOL_MANAGED,
		&pDirect3DTexture_, NULL);
	if (FAILED(ret))
	{
		LOGE("pDirect3DDevice_->CreateTexture !!");
		return FALSE;
	}

	return TRUE;
}
