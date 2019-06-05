#include "DxSurface.h"
WndSurfaceMap	CDxSurface::m_WndSurfaceMap;
CD3D9Helper    g_pD3D9Helper;
// HMODULE		CDxSurface::m_hD3D9 = nullptr;
// IDirect3D9*  CDxSurface::m_pDirect3D9 = nullptr;
// IDirect3D9Ex*  CDxSurfaceEx::m_pDirect3D9Ex = nullptr;

CCriticalSectionAgentPtr CDxSurface::m_WndSurfaceMapcs = shared_ptr<CCriticalSectionAgent>(new CCriticalSectionAgent());

int	CDxSurface::m_nObjectCount = 0;
CCriticalSectionAgentPtr CDxSurface::m_csObjectCount = shared_ptr<CCriticalSectionAgent>(new CCriticalSectionAgent());
