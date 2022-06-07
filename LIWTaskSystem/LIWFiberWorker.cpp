#include "LIWFiberWorker.h"
#include "LIWFiberMain.h"

//
// Win32
//
#ifdef _WIN32
LIW::LIWFiberWorker::LIWFiberWorker()
{
	LIWFiberWorker(-1);
}
LIW::LIWFiberWorker::LIWFiberWorker(int id):
	m_id(id),
	m_isRunning(true)
{
	m_sysFiber = CreateFiber(0, InternalFiberRun, this);
}
LIW::LIWFiberWorker::~LIWFiberWorker()
{
	DeleteFiber(m_sysFiber);
}
inline void LIW::LIWFiberWorker::YieldToMain()
{
	SwitchToFiber(m_fiberMain->m_sysFiber);
}
#endif 


