#pragma once
#include <streams.h>
#include "Registry.h"

class CVideoFilter : public CSource
{
public:
	DECLARE_IUNKNOWN;
	static CUnknown * WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);
	IFilterGraph *GetGraph() { return m_pGraph; }
	FILTER_STATE GetState(){ return m_State; }

private:
	CVideoFilter(LPUNKNOWN lpunk, HRESULT *phr);
};

