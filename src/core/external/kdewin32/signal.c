/*
   This file is part of the KDE libraries
   Copyright (C) 2003-2008 Jaroslaw Staniek <js@iidea.pl>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include <windows.h>
#include <tlhelp32.h>

#include "kdewin32/sys/types.h"
#include "kdewin32/errno.h"
#include "kdewin32/signal.h"

/* used in kill() */
int handle_kill_result(HANDLE h)
{
  if (GetLastError() == ERROR_ACCESS_DENIED)
    errno = EPERM;
  else if (GetLastError() == ERROR_NO_MORE_FILES)
    errno = ESRCH;
  else
    errno = EINVAL;
  CloseHandle(h);
  return -1;
}

int kill(pid_t pid, int sig)
{
  HANDLE h;
  HANDLE h_thread;
  DWORD thread_id;
  PROCESSENTRY32 pe32;
  if (pid <= 0 || sig < 0) {
    errno = EINVAL;
    return -1;
  }
  if (sig == 0) { /* we just wanted to know if the process exists  */
    h = CreateToolhelp32Snapshot(0, pid);
    if (h == INVALID_HANDLE_VALUE) {
      errno = ESRCH;
      return -1;
    }
    pe32.dwSize = sizeof(PROCESSENTRY32);
    if (!Process32First( h, &pe32 ))
      return handle_kill_result(h);
    CloseHandle(h);
    return 0;
  }
  h = OpenProcess(
    sig == 0 ? PROCESS_QUERY_INFORMATION|PROCESS_VM_READ : PROCESS_ALL_ACCESS, 
    FALSE, (DWORD)pid);
  if (!h) {
    CloseHandle(h);
    errno = ESRCH;
    return -1;
  }
  switch (sig) {
  case SIGINT:
    if (!GenerateConsoleCtrlEvent(CTRL_C_EVENT, (DWORD)pid))
      return handle_kill_result(h);
    break;
  case SIGQUIT:
    if (!GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, (DWORD)pid))
      return handle_kill_result(h);
    break;
  case SIGKILL:
    if (!TerminateProcess(h, sig))
      return handle_kill_result(h);
    break;
  default:
    h_thread = CreateRemoteThread(
      h, NULL, 0,
      (LPTHREAD_START_ROUTINE)(GetProcAddress(GetModuleHandleA("KERNEL32.DLL"), "ExitProcess")),
      0, 0, &thread_id);
    if (h_thread)
      WaitForSingleObject(h_thread, 5);
    else
      return handle_kill_result(h);
  }
  CloseHandle(h);
  return 0;
}

pid_t waitpid(pid_t p, int *a, int b)
{
  HANDLE h = OpenProcess(PROCESS_TERMINATE, FALSE, (DWORD)p);
  if( h ) {
    DWORD dw;
    WaitForSingleObject(h,INFINITE);
    GetExitCodeProcess(h,&dw);
    CloseHandle(h);
    return 0;
  }
  errno = ECHILD;
  return -1;
}

sighandler_t kdewin32_signal(int signum, sighandler_t handler)
{
  if (signum==SIGABRT
    || signum==SIGFPE
    || signum==SIGILL
    || signum==SIGINT
    || signum==SIGSEGV
    || signum==SIGTERM)
    return signal(signum, handler);
  return SIG_ERR;
}
