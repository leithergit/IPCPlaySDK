#pragma once
#include <wtypes.h>
#include <memory>
using namespace std::tr1;
#if defined(_M_IX86)
#pragma pack(push,1)
struct WinThunk
{
	unsigned short m_push1;    //push dword ptr [esp] ;push return address
	unsigned short m_push2;
	unsigned short m_mov1;     //mov dword ptr [esp+0x4], pThis ;set our new parameter by replacing old return address
	unsigned char  m_mov2;     //(esp+0x4 is first parameter)
	unsigned long  m_this;     //ptr to our object
	unsigned char  m_jmp;      //jmp WndProc
	unsigned long  m_relproc;  //relative jmp
	static HANDLE  hHeap;		//heap address this thunk will be initialized to
	static bool InitHeap()
	{
		try
		{
			hHeap = HeapCreate(HEAP_CREATE_ENABLE_EXECUTE | HEAP_GENERATE_EXCEPTIONS, 0, 0);
			if (hHeap)
				return true;
			else
				return false;
		}
		catch (...)
		{
			throw;
		}

	}
	WinThunk(void *pThis,DWORD_PTR pProc)
	{
		InitThunk(pThis, pProc);
	}
	bool InitThunk(void* pThis, DWORD_PTR proc)
	{
		m_push1 = 0x34ff; //ff 34 24 push DWORD PTR [esp]
		m_push2 = 0xc724;
		m_mov1 = 0x2444; // c7 44 24 04 mov dword ptr [esp+0x4],
		m_mov2 = 0x04;
		m_this = PtrToUlong(pThis);
		m_jmp = 0xe9;  //jmp
		//calculate relative address of proc to jump to
		m_relproc = unsigned long((INT_PTR)proc - ((INT_PTR)this + sizeof(WinThunk)));
		// write block from data cache and flush from instruction cache
		if (FlushInstructionCache(GetCurrentProcess(), this, sizeof(WinThunk)))
		{ //succeeded
			return TRUE;
		}
		else { //error
			return FALSE;
		}
	}
	//some thunks will dynamically allocate the memory for the code
	WNDPROC GetThunkAddress()
	{
		return (WNDPROC)this;
	}
	void* operator new(size_t)
	{
		//since we can't pass a value with delete operator we need to store
		//our heap address so we can use it later when we need to free this thunk
		return HeapAlloc(hHeap, 0, sizeof(WinThunk));
	}
	void operator delete(void* pThunk)
	{
		HeapFree(hHeap, 0, pThunk);
	}
};
#pragma pack(pop)
#elif defined(_M_AMD64)
#pragma pack(push,2)
struct WinThunk
{
	unsigned short     RaxMov;  //movabs rax, pThis
	unsigned long long RaxImm;
	unsigned long      RspMov;  //mov [rsp+28], rax
	unsigned short     RspMov1;
	unsigned short     Rax2Mov; //movabs rax, proc
	unsigned long long ProcImm;
	unsigned short     RaxJmp;  //jmp rax
	static HANDLE		hHeap;		//heap address this thunk will be initialized to
	static bool InitHeap()
	{
		try
		{
			hHeap = HeapCreate(HEAP_CREATE_ENABLE_EXECUTE | HEAP_GENERATE_EXCEPTIONS, 0, 0);
			if (hHeap)
				return true;
			else
				return false;
		}
		catch (...)
		{
			throw;
		}

	}
	WinThunk(void *pThis, DWORD_PTR pProc)
	{
		InitThunk(pThis, pProc);
	}
	bool InitThunk(void *pThis, DWORD_PTR proc)
	{
		RaxMov = 0xb848;          //movabs rax (48 B8), pThis
		RaxImm = (unsigned long long)pThis; //
		RspMov = 0x24448948;      //mov qword ptr [rsp+28h], rax (48 89 44 24 28)
		RspMov1 = 0x9028;         //to properly byte align the instruction we add a nop (no operation) (90)
		Rax2Mov = 0xb848;         //movabs rax (48 B8), proc
		ProcImm = (unsigned long long)proc;
		RaxJmp = 0xe0ff;          //jmp rax (FF EO)
		if (FlushInstructionCache(GetCurrentProcess(), this, sizeof(WinThunk)))
		{ //error
			return FALSE;
		}
		else
		{//succeeded
			return TRUE;
		}
	}
	//some thunks will dynamically allocate the memory for the code
	WNDPROC GetThunkAddress()
	{
		return (WNDPROC)this;
	}
	void* operator new(size_t)
	{
		return HeapAlloc(hHeap, 0, sizeof(WinThunk));
	}
	void operator delete(void* pThunk)
	{
		HeapFree(hHeap, 0, pThunk);
	}
};
#pragma pack(pop)
#endif

typedef shared_ptr<WinThunk> WinThunkPtr;