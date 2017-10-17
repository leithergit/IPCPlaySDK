#include "DxSurface.h"
WndSurfaceMap	CDxSurface::m_WndSurfaceMap;
//WNDPROC	CDxSurface::m_pOldWndProc = NULL;
CCriticalSectionProxyPtr CDxSurface::m_WndSurfaceMapcs = shared_ptr<CCriticalSectionProxy>(new CCriticalSectionProxy());

int	CDxSurface::m_nObjectCount = 0;
CCriticalSectionProxyPtr CDxSurface::m_csObjectCount = shared_ptr<CCriticalSectionProxy>(new CCriticalSectionProxy());
