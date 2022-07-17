// pch.cpp: source file corresponding to the pre-compiled header

#include "pch.h"

// When you are using pre-compiled headers, this source file is necessary for compilation to succeed.
#define MAX_PATH 512
#include "dbghelp.h"

#pragma comment(lib,"Dbghelp.lib")

void printStack( const char* pTempPath, CONTEXT* ctx) //Prints stack trace based on context record
{
    constexpr auto MaxNameLen = 256;

    BOOL    result;
    HANDLE  process;
    HANDLE  thread;
    HMODULE hModule;

    STACKFRAME64        stack;
    ULONG               frame;
    DWORD64             displacement;

    DWORD disp;
    IMAGEHLP_LINE64* line;

    char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
    char module[MaxNameLen];
    PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)buffer;

    memset(&stack, 0, sizeof(STACKFRAME64));

    process = GetCurrentProcess();
    thread  = GetCurrentThread();
    displacement = 0;
#if !defined(_M_AMD64)
    stack.AddrPC.Offset = (*ctx).Eip;
    stack.AddrPC.Mode = AddrModeFlat;
    stack.AddrStack.Offset = (*ctx).Esp;
    stack.AddrStack.Mode = AddrModeFlat;
    stack.AddrFrame.Offset = (*ctx).Ebp;
    stack.AddrFrame.Mode = AddrModeFlat;
#endif

   // SymSetSearchPath(process, pTempPath);
    SymSetOptions( SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES | SYMOPT_DEBUG ); // 

    if (!SymInitialize(process, NULL, TRUE))
    {
        // SymInitialize failed
        auto error = GetLastError();
        printf("SymInitialize returned error : %d\n", error);
        return;
    }

   // SymInitialize(process, NULL, TRUE); //load symbols

    for (frame = 0; ; frame++)
    {
        //get next call from stack
        result = StackWalk64
        (
#if defined(_M_AMD64)
            IMAGE_FILE_MACHINE_AMD64
#else
            IMAGE_FILE_MACHINE_I386
#endif
            ,
            process,
            thread,
            &stack,
            ctx,
            NULL,
            SymFunctionTableAccess64,
            SymGetModuleBase64,
            NULL
        );

        if (!result) break;

        //get symbol name for address
        pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        pSymbol->MaxNameLen = MAX_SYM_NAME;
        SymFromAddr(process, (ULONG64)stack.AddrPC.Offset, &displacement, pSymbol);

        line = (IMAGEHLP_LINE64*)malloc(sizeof(IMAGEHLP_LINE64));
        line->SizeOfStruct = sizeof(IMAGEHLP_LINE64);

        //try to get line
        if (SymGetLineFromAddr64(process, stack.AddrPC.Offset, &disp, line))
        {
            printf("\tat %s in %s: line: %lu: address: 0x%0llX\n", pSymbol->Name, line->FileName, line->LineNumber, pSymbol->Address);
        }
        else
        {
            //failed to get line
            printf("\tat %s, address 0x%0llX.\n", pSymbol->Name, pSymbol->Address);
            hModule = NULL;
            lstrcpyA(module, "");
            GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                (LPCTSTR)(stack.AddrPC.Offset), &hModule);

            //at least print module name
            if (hModule != NULL)GetModuleFileNameA(hModule, module, MaxNameLen);

            printf("in %s\n", module);
        }

        free(line);
        line = NULL;
    }
}
