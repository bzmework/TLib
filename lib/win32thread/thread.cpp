
#include "thread.h"

//
// 函数：Win32Thread
// 功能：线程构造函数,初始化线程资源。
// 参数：无
// 返回：无
//
Win32Thread::Win32Thread()
{
	m_hThreadHandle = NULL;	
	m_dwThreadId = 0;
    m_lpfnFunc = NULL;
    m_lpvArg = NULL; 
	m_bIsStopping = FALSE;
}

//
// 函数：~Win32Thread
// 功能：线程析构函数,结束线程,并释放线程资源。
// 参数：无
// 返回：无
//
Win32Thread::~Win32Thread()
{
	this->Destroy();
}

DWORD CALLBACK Win32Thread::ThreadExecuteFunction(LPVOID lpvArg)
{
    Win32Thread* thread = (Win32Thread*) lpvArg;
    return thread->InternalThreadExecuteFunction();
}

DWORD Win32Thread::InternalThreadExecuteFunction()
{
    m_lpfnFunc(m_lpvArg);
	/*
	线程函数返回，线程终止运行，该线程堆栈的内存被释放。
	注意：
	1、线程函数必须返回一个值，它将成为该线程的退出代码。
	2、应该始终将线程处理函数设计成能正常返回，因为这是确保所有线程资源被正确地清除的唯一办法。如果线程能够返回，就可以确保下列事项的实现：
	• 在线程函数中创建的所有C++对象均将通过它们的撤消函数正确地撤消。
	• 操作系统将正确地释放线程堆栈使用的内存。
	• 系统将线程的退出代码（在线程的内核对象中维护）设置为线程函数的返回值。
	• 系统将递减线程内核对象的使用计数。
	*/
    return 0;
}

//
// 函数：Create
// 功能：创建新的线程，线程必须通过本函数进行创建。
// 参数：function，线程执行函数，该函数通常应该设置成调用对象的静态成员函数。
//		 arg，线程参数，该参数通常应该设置成当前调用对象(this)。
//		 immeditExcute，是否立即调度执行。false意味着将创建一个暂停的线程，必须调用Resume后才能恢复并启动执行线程。
// 返回：创建成功返回true，,失败返回false。
//
BOOL Win32Thread::Create(THREAD_EXECUTE_FUNCTION lpfnFunc, LPVOID lpvArg, BOOL bImmeditExcute)
{
	BOOL bResult = FALSE;
	
	//如果线程已经存在应先将其销毁，防止资源泄漏。
	if(m_hThreadHandle != NULL)
	{
		this->Destroy();
	}
	
	//参数赋值
	m_bIsStopping = FALSE;
	m_lpfnFunc = lpfnFunc;
	m_lpvArg = lpvArg;
	
	/*
	在线程内核对象的内部有一个值，用于指明线程的暂停计数。
	当调用CreateThread函数时，就创建了线程的内核对象，并且它的暂停计数被初始化为1。
	这可以防止线程被调度到CPU中。当然，这是很有用的，因为线程的初始化需要时间，
	你不希望在系统做好充分的准备之前就开始执行线程。
	当线程完全初始化好了之后，CreateThread要查看是否已经传递了CREATE_SUSPENDED标志。
	如果已经传递了这个标志，那么这些函数就返回，同时新线程处于暂停状态。
	如果尚未传递该标志，那么该函数将线程的暂停计数递减为0。
	当线程的暂停计数是0的时候，除非线程正在等待其他某种事情的发生，否则该线程就处于可调度状态。
	创建一个暂停线程(在CreateThread中设置了CREATE_SUSPENDED标志)，就能够在线程有机会执行任何代码之前改变线程的运行环境(如优先级)。
	一旦改变了线程的环境，必须使线程成为可调度线程。要进行这项操作，可以调用ResumeThread。
	*/
	
	m_hThreadHandle = ::CreateThread(NULL,
									0,
									Win32Thread::ThreadExecuteFunction,
									this,
									((bImmeditExcute)?0:CREATE_SUSPENDED),
									&m_dwThreadId);
	
	/*m_threadHandle = (HANDLE) _beginthreadex(
									NULL,
									0,
									(unsigned int (__stdcall *)(void *))internalThreadFunction,
									this,
									((immeditExcute)?0:CREATE_SUSPENDED),
									(unsigned int *)&m_threadId);*/
	if(m_hThreadHandle != NULL) bResult = true;

	return bResult;
}

//
// 函数：Destroy
// 功能：销毁线程。
// 参数：无
// 返回：无
//
VOID Win32Thread::Destroy()
{
	//终结线程执行
	this->Stop();

	//一个终结线程将会继续存在直到该线程用CloseHandle释放了线程的内核对象。
	//如果不关闭线程的handle，那么这个进程可能最终有数百甚至数千个开启的“线程内核对象”
	//留给操作系统去清理。这样造成的资源泄漏可能对效率带来负面的影响。
	CloseHandle(m_hThreadHandle);
	m_hThreadHandle = NULL;
}

//
// 函数：Stop
// 功能：停止线程执行。
// 参数：immeditExcute，是否立即停止，无论线程是否在运行，将立即终止执行。
//       millisecondsTimeout，等待线程停止的超时毫秒数，指定INFINITE将无限期等待，直到线程终止为止。
// 备注：如果你的线程执行函数中有长时间执行的代码(例如查找文件)，应该始终调用IsStopping检查线程是否
//       正在请求停止执行，以保证线程能正常地终止执行。
// 返回：无
//
VOID Win32Thread::Stop(BOOL bImmeditExcute, DWORD dwMillisecondsTimeout)
{
	/*
	TerminateThread终止任何正在运行的线程。
	注意TerminateThread函数是异步运行的函数，它告诉系统你想要线程终止运行，但是，当函数返回时，不能保证线程被撤消。
	设计良好的应用程序从来不使用这个函数， 因为被终止运行的线程收不到它被撤消的通知，线程不能正确地清除。
	即，如果使用TerminateThread，那么在拥有线程的进程终止运行之前，系统不撤消该线程的堆栈。
	当使用返回或调用ExitThread方法撤消线程(在线程执行函数中调用)时，该线程的内存堆栈也被撤消。
	当线程正常地终止运行时，会发生下列操作：
	• 线程的退出代码从STILL_ACTIVE改为传递给ExitThread或TerminateThread的代码。
	• 如果线程是进程中最后一个活动线程，系统也将进程视为已经终止运行。
	• 线程内核对象的使用计数递减1。
	*/
	this->m_bIsStopping = TRUE;
	if(bImmeditExcute)
	{
		TerminateThread(m_hThreadHandle, 0);
	}
	else
	{
		if(this->IsRunning())
		{
			this->Join(dwMillisecondsTimeout);
		}
		TerminateThread(m_hThreadHandle, 0);
	}
	this->m_bIsStopping = FALSE;
}

//
// 函数：Pause
// 功能：暂停当前线程的执行
// 参数：无
// 返回：无
//
VOID Win32Thread::Pause()
{
	/*
	调用SuspendThread意味着当前线程暂停运行，系统不应该给它安排任何CPU时间，进行调度。
	注意：系统中的大多数线程是不可调度的线程，例如：
	通过调用使用CREATE_SUSPENDED(0x00000004)标志的CreateThread函数创建的一个暂停线程。
	除了暂停的线程外，其他许多线程也是不可调度的线程，因为它们正在等待某些事情的发生。
	例如，如果运行Notepad，但是并不键入任何数据，那么Notepad的线程就没有什么事情要做。系统不给无事可做的线程分配CPU时间。
	当移动Notepad的窗口时，或者Notepad的窗口需要刷新它的内容，或者将数据键入Notepad，系统就会自动使Notepad的线程成为可调度的线程。
	这并不意味着Notepad的线程立即获得了CPU时间。它只是表示Notepad的线程有事情可做，系统将设法在某个时间（不久的将来）对它进行调度。

	任何线程都可以调用该函数来暂停另一个线程的运行（只要拥有线程的句柄）。
	线程可以自行暂停运行，但是不能自行恢复运行。SuspendThread返回的是线程的前一个暂停计数。线程暂停的最多次数可以是
	MAXIMUM_SUSPEND_COUNT次（在WinNT.h中定义为127）。如果一个线程暂停了3次，则它必须恢复3次，然后它才可以被分配给一个CPU。
	注意，SuspendThread与内核方式的执行是异步进行的。

	在实际环境中，调用SuspendThread时必须小心，因为你不知道在暂停线程时它在进行什么操作，执行什么代码。
	如果线程试图从堆栈中分配内存(因为同一时刻只能有一个线程进行堆栈内存分配，例如调用malloc)，那么该线程将在该堆栈上设置一个锁。
	当其他线程试图访问该堆栈时，这些线程的访问就被停止，直到第一个线程恢复运行。
	又或者你在其他线程正在执行Thread::CreateThread函数时暂停它，则试图使用该类的其他线程将被阻止，这样很容易发生死锁。
	因此，只有确切知道目标线程是什么（或者目标线程正在做什么），并且采取强有力的措施来避免因暂停线程的运行而带来的问题(例如死锁)，
	SuspendThread才是安全的。
	*/
	SuspendThread(m_hThreadHandle);
}

//
// 函数：Resume
// 功能：恢复执行已暂停的当前线程
// 参数：无
// 返回：无
//
VOID Win32Thread::Resume()
{
	//如果ResumeThread函数运行成功，它将返回线程的前一个暂停计数，否则返回0xFFFFFFFF。
	ResumeThread(m_hThreadHandle);
}

//
// 函数：Join
// 功能：阻塞当前线程，直到线程终止为止。
//		 如果线程不终止，则调用将无限期阻塞。
//		 如果调用Join时该线程已终止，此方法将立即返回。
// 参数：无
// 返回：无
//
VOID Win32Thread::Join()
{
	WaitForSingleObject(m_hThreadHandle, INFINITE);
}

//
// 函数：Join
// 功能：阻塞当前线程，直到线程终止或超时为止。
//		 如果millisecondsTimeout = INFINITE(0xFFFFFFFF)，则调用将无限期阻塞，直到线程终止为止。
//		 如果调用Join时该线程已终止，此方法将立即返回。
// 参数：millisecondsTimeout,阻塞的超时毫秒数。指定INFINITE以无限期阻止线程，直到线程终止为止。
// 返回：如果线程已终止，则返回true；如果线程在经过了millisecondsTimeout参数指定的时间量后未终止，则返回false。
//
BOOL Win32Thread::Join(DWORD dwMillisecondsTimeout)
{
	return (WaitForSingleObject(m_hThreadHandle, dwMillisecondsTimeout) == WAIT_OBJECT_0);
}

//
// 函数：Sleep
// 功能：将当前线程阻塞指定的毫秒数，然后继续执行。
// 参数：millisecondsTimeout，线程被阻塞的毫秒数。指定零(0)以指示应暂停此线程以使其他等待线程能够执行，指定INFINITE(0xFFFFFFFF)将无限期阻止线程执行。
// 返回：无
//
VOID Win32Thread::Sleep(DWORD dwMillisecondsTimeout)
{
	/*
	Sleep函数可使线程暂停自己的运行，直到millisecondsTimeout过去为止。
	关于Sleep函数，有下面几个重要问题值得注意：
	• 调用Sleep，可使线程自愿放弃它剩余的时间片。
	• 系统将在大约的指定毫秒数内使线程不可调度。不错，如果告诉系统，想睡眠100ms，
	  那么可以睡眠大约这么长时间，但是也可能睡眠数秒钟或者数分钟。记住，Windows不是个实时操作系统。
	  虽然线程可能在规定的时间被唤醒，但是它能否做到，取决于系统中还有什么操作正在进行。
	• 可以调用Sleep，并且为millisecondsTimeout参数传递INFINITE。这将告诉系统永远不要调度
	  该线程。这不是一件值得去做的事情。最好是让线程退出，并还原它的堆栈和内核对象。
	• 可以将0传递给Sleep。这将告诉系统，调用线程将释放剩余的时间片，并迫使系统调度另一个线程。
	  但是，系统可以对刚刚调用Sleep的线程重新调度。如果不存在多个拥有相同优先级的可调度线程，就会出现这种情况。
	*/
	::Sleep(dwMillisecondsTimeout);
}

//
// 函数：IsRunning
// 功能：检查当前线程是否正在运行
// 参数：无
// 返回：如果当前线程已启动并且尚未正常终止，则返回true；否则返回false。
//
BOOL Win32Thread::IsRunning() const
{
	BOOL bResult = FALSE;
	DWORD dwExitCode = 0; 

	//GetExitCodeThread用来检查由hThread标识的线程是否已经终止运行。
	//如果调用GetExitCodeThread时线程尚未终止运行，该函数就用STILL_ACTIVE标识符（定义为0x103）填入lpExitCode。
	if(GetExitCodeThread(m_hThreadHandle, &dwExitCode) != 0)
	{
		bResult = (dwExitCode == STILL_ACTIVE);
	}
	return bResult;
}

//
// 函数：IsStopping
// 功能：检查当前线程是否正在停止运行
// 参数：无
// 返回：是则返回true；否则返回false。
//
BOOL Win32Thread::IsStopping() const
{
	return m_bIsStopping;
}

//
// 函数：GetPriority
// 功能：获取线程的优先级。
// 参数：无
// 返回：返回优先级。
//
int Win32Thread::GetPriority() const
{
    return GetThreadPriority(m_hThreadHandle);
}

//
// 函数：SetPriority
// 功能：设置线程的优先级。
// 参数：priority，线程优先级。
// 返回：返回原来的优先级。
//
int Win32Thread::SetPriority(THREAD_PRIORITY_LEVEL enmPriority)
{
	int nOldPriority = GetThreadPriority(m_hThreadHandle);
	SetThreadPriority(m_hThreadHandle, enmPriority);
	return nOldPriority;
}

//
// 函数：SetPriority
// 功能：设置线程的优先级。
// 参数：priority，线程优先级。
// 返回：返回原来的优先级。
//
int Win32Thread::SetPriority(int nPriority)
{
	int nOldPriority = GetThreadPriority(m_hThreadHandle);
	SetThreadPriority(m_hThreadHandle, nPriority);
	return nOldPriority;
}
