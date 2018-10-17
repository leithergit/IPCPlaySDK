#include "DxSurface.h"
WndSurfaceMap	CDxSurface::m_WndSurfaceMap;
CD3D9Helper    g_D3D9Helper;
// HMODULE		CDxSurface::m_hD3D9 = nullptr;
// IDirect3D9*  CDxSurface::m_pDirect3D9 = nullptr;
// IDirect3D9Ex*  CDxSurfaceEx::m_pDirect3D9Ex = nullptr;

CCriticalSectionProxyPtr CDxSurface::m_WndSurfaceMapcs = shared_ptr<CCriticalSectionProxy>(new CCriticalSectionProxy());

int	CDxSurface::m_nObjectCount = 0;
CCriticalSectionProxyPtr CDxSurface::m_csObjectCount = shared_ptr<CCriticalSectionProxy>(new CCriticalSectionProxy());
