#include "ShellExtension.h"
#include <shlwapi.h>
#include <strsafe.h>
#include <atomic>
#include <new>

#pragma comment(lib, "shlwapi.lib")

// ── DLL globals ───────────────────────────────────────────
static HINSTANCE         g_hInst = nullptr;
static std::atomic<LONG> g_lockCount{ 0 };

// ── Command IDs (offsets from idCmdFirst) ─────────────────
static constexpr UINT CMD_CONVERT_TO_PDF = 0;

// ── Extension lists ───────────────────────────────────────
// Files we show "Convert to PDF" on
static const wchar_t* kConvertibleExts[] = {
    L".jpg", L".jpeg", L".png", L".bmp",
    L".tiff", L".tif", L".webp",
    L".docx", L".doc", L".odt", L".rtf", L".txt",
    L".pptx", L".ppt", L".odp",
    L".xlsx", L".xls", L".ods",
    L".html", L".htm",
    nullptr
};

static bool IsConvertibleExt(const wchar_t* path)
{
    const wchar_t* ext = PathFindExtensionW(path);
    if (!ext || !*ext) return false;
    for (int i = 0; kConvertibleExts[i]; ++i)
        if (_wcsicmp(ext, kConvertibleExts[i]) == 0)
            return true;
    return false;
}

// ── ShellExtension : IUnknown ─────────────────────────────

namespace zc {

    ShellExtension::ShellExtension()
    {
        g_lockCount++;
    }

    STDMETHODIMP ShellExtension::QueryInterface(REFIID riid, void** ppv)
    {
        if (!ppv) return E_POINTER;
        *ppv = nullptr;

        if (IsEqualIID(riid, IID_IUnknown) ||
            IsEqualIID(riid, IID_IShellExtInit))
        {
            *ppv = static_cast<IShellExtInit*>(this);
        }
        else if (IsEqualIID(riid, IID_IContextMenu))
        {
            *ppv = static_cast<IContextMenu*>(this);
        }
        else return E_NOINTERFACE;

        AddRef();
        return S_OK;
    }

    STDMETHODIMP_(ULONG) ShellExtension::AddRef()
    {
        return ++refCount_;
    }

    STDMETHODIMP_(ULONG) ShellExtension::Release()
    {
        ULONG ref = --refCount_;
        if (ref == 0) delete this;
        return ref;
    }

    // ── IShellExtInit ─────────────────────────────────────────

    STDMETHODIMP ShellExtension::Initialize(
        PCIDLIST_ABSOLUTE /*pidlFolder*/,
        IDataObject* pDataObj,
        HKEY              /*hKeyProgID*/)
    {
        if (!pDataObj) return E_INVALIDARG;

        FORMATETC fe = {
            CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL
        };
        STGMEDIUM stm{};

        HRESULT hr = pDataObj->GetData(&fe, &stm);
        if (FAILED(hr)) return hr;

        HDROP hDrop = static_cast<HDROP>(GlobalLock(stm.hGlobal));
        if (hDrop) {
            UINT fileCount = DragQueryFileW(hDrop, 0xFFFFFFFF, nullptr, 0);
            selectedFiles_.clear();
            selectedFiles_.reserve(fileCount);

            hasConvertibleSelection_ = true;

            for (UINT i = 0; i < fileCount; ++i) {
                wchar_t path[MAX_PATH]{};
                DragQueryFileW(hDrop, i, path, MAX_PATH);

                if (!IsConvertibleExt(path)) {
                    selectedFiles_.clear();
                    hasConvertibleSelection_ = false;
                    break;
                }

                selectedFiles_.emplace_back(path);
            }

            GlobalUnlock(stm.hGlobal);
        }
        ReleaseStgMedium(&stm);

        return hasConvertibleSelection_ && !selectedFiles_.empty()
            ? S_OK
            : E_FAIL;
    }

    // ── IContextMenu ──────────────────────────────────────────

    STDMETHODIMP ShellExtension::QueryContextMenu(
        HMENU hmenu,
        UINT  indexMenu,
        UINT  idCmdFirst,
        UINT  /*idCmdLast*/,
        UINT  uFlags)
    {
        if (uFlags & CMF_DEFAULTONLY)
            return MAKE_HRESULT(SEVERITY_SUCCESS, 0, 0);

        // Single item: "Convert to PDF"
        MENUITEMINFOW mii{};
        mii.cbSize = sizeof(mii);
        mii.fMask = MIIM_STRING | MIIM_ID | MIIM_STATE;
        mii.wID = idCmdFirst + CMD_CONVERT_TO_PDF;
        mii.fState = MFS_ENABLED;
        mii.dwTypeData = const_cast<LPWSTR>(L"Convert to PDF");

        InsertMenuItemW(hmenu, indexMenu, TRUE, &mii);

        // Add a separator after our item for visual clarity
        MENUITEMINFOW sep{};
        sep.cbSize = sizeof(sep);
        sep.fMask = MIIM_TYPE;
        sep.fType = MFT_SEPARATOR;
        InsertMenuItemW(hmenu, indexMenu + 1, TRUE, &sep);

        return MAKE_HRESULT(SEVERITY_SUCCESS, 0, CMD_CONVERT_TO_PDF + 1);
    }

    STDMETHODIMP ShellExtension::InvokeCommand(
        LPCMINVOKECOMMANDINFO pici)
    {
        // Accept both string verb and ordinal
        UINT idCmd = 0;
        if (IS_INTRESOURCE(pici->lpVerb))
            idCmd = LOWORD(pici->lpVerb);
        else
            return E_INVALIDARG; // we don't support string verbs

        if (idCmd != CMD_CONVERT_TO_PDF)
            return E_INVALIDARG;

        if (selectedFiles_.empty())
            return E_FAIL;

        // Find zconverter.exe next to this DLL
        wchar_t dllDir[MAX_PATH]{};
        GetModuleFileNameW(g_hInst, dllDir, MAX_PATH);
        PathRemoveFileSpecW(dllDir);

        wchar_t exePath[MAX_PATH]{};
        StringCchPrintfW(exePath, MAX_PATH,
            L"%s\\zconverter.exe", dllDir);

        // Verify EXE exists before launching
        if (GetFileAttributesW(exePath) == INVALID_FILE_ATTRIBUTES)
            return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);

        // Build command line for all selected files.
        std::wstring cmdLine = L"\"";
        cmdLine += exePath;
        cmdLine += L"\" --to pdf";

        for (const auto& file : selectedFiles_) {
            cmdLine += L" \"";
            cmdLine += file;
            cmdLine += L"\"";
        }

        STARTUPINFOW si{};
        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE;

        PROCESS_INFORMATION pi{};
        BOOL ok = CreateProcessW(
            nullptr, cmdLine.data(),
            nullptr, nullptr,
            FALSE, CREATE_NO_WINDOW,
            nullptr, nullptr,
            &si, &pi);

        if (!ok)
            return HRESULT_FROM_WIN32(GetLastError());

        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return S_OK;
    }

    STDMETHODIMP ShellExtension::GetCommandString(
        UINT_PTR idCmd,
        UINT     uType,
        UINT*    /*pReserved*/,
        CHAR* pszName,
        UINT     cchMax)
    {
        if (idCmd != CMD_CONVERT_TO_PDF)
            return E_INVALIDARG;

        if (uType == GCS_HELPTEXTW) {
            StringCchCopyW(
                reinterpret_cast<wchar_t*>(pszName), cchMax,
                L"Convert the selected file(s) to PDF with zconverter");
            return S_OK;
        }
        if (uType == GCS_VERBW) {
            StringCchCopyW(
                reinterpret_cast<wchar_t*>(pszName), cchMax,
                L"ConvertToPDF");
            return S_OK;
        }
        return E_NOTIMPL;
    }

    // ── ShellExtensionFactory ─────────────────────────────────

    STDMETHODIMP ShellExtensionFactory::QueryInterface(
        REFIID riid, void** ppv)
    {
        if (!ppv) return E_POINTER;
        *ppv = nullptr;
        if (IsEqualIID(riid, IID_IUnknown) ||
            IsEqualIID(riid, IID_IClassFactory))
        {
            *ppv = static_cast<IClassFactory*>(this);
            return S_OK;
        }
        return E_NOINTERFACE;
    }

    STDMETHODIMP ShellExtensionFactory::CreateInstance(
        IUnknown* pUnkOuter, REFIID riid, void** ppv)
    {
        if (pUnkOuter) return CLASS_E_NOAGGREGATION;
        auto* ext = new (std::nothrow) ShellExtension();
        if (!ext) return E_OUTOFMEMORY;
        HRESULT hr = ext->QueryInterface(riid, ppv);
        ext->Release();
        return hr;
    }

} // namespace zc

// ── DLL entry points ──────────────────────────────────────

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH) {
        g_hInst = hModule;
        DisableThreadLibraryCalls(hModule);
    }
    return TRUE;
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void** ppv)
{
    if (!IsEqualCLSID(rclsid, CLSID_ZConverterShellExt))
        return CLASS_E_CLASSNOTAVAILABLE;
    static zc::ShellExtensionFactory factory;
    return factory.QueryInterface(riid, ppv);
}

STDAPI DllCanUnloadNow()
{
    return (g_lockCount == 0) ? S_OK : S_FALSE;
}

// ── DllRegisterServer ─────────────────────────────────────

STDAPI DllRegisterServer()
{
    wchar_t dllPath[MAX_PATH]{};
    GetModuleFileNameW(g_hInst, dllPath, MAX_PATH);

    const wchar_t* clsidStr =
        L"{A1B2C3D4-E5F6-7890-ABCD-EF1234567890}";

    // 1. Register CLSID → InprocServer32
    {
        wchar_t key[256]{};
        StringCchPrintfW(key, 256,
            L"CLSID\\%s\\InprocServer32", clsidStr);

        HKEY hk = nullptr;
        RegCreateKeyExW(HKEY_LOCAL_MACHINE, key,
            0, nullptr, 0, KEY_WRITE, nullptr, &hk, nullptr);

        RegSetValueExW(hk, nullptr, 0, REG_SZ,
            reinterpret_cast<const BYTE*>(dllPath),
            static_cast<DWORD>(
                (wcslen(dllPath) + 1) * sizeof(wchar_t)));

        const wchar_t* apt = L"Apartment";
        RegSetValueExW(hk, L"ThreadingModel", 0, REG_SZ,
            reinterpret_cast<const BYTE*>(apt),
            static_cast<DWORD>(
                (wcslen(apt) + 1) * sizeof(wchar_t)));

        RegCloseKey(hk);
    }

    // 2. Approved extensions list (required on some Windows configs)
    {
        wchar_t key[256]{};
        StringCchPrintfW(key, 256,
            L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\"
            L"Shell Extensions\\Approved");

        HKEY hk = nullptr;
        RegCreateKeyExW(HKEY_LOCAL_MACHINE, key,
            0, nullptr, 0, KEY_WRITE, nullptr, &hk, nullptr);

        const wchar_t* desc = L"zconverter Shell Extension";
        RegSetValueExW(hk, clsidStr, 0, REG_SZ,
            reinterpret_cast<const BYTE*>(desc),
            static_cast<DWORD>(
                (wcslen(desc) + 1) * sizeof(wchar_t)));

        RegCloseKey(hk);
    }

    // 3. Register handler for every supported extension
    const wchar_t* exts[] = {
        L".jpg", L".jpeg", L".png", L".bmp",
        L".tiff", L".tif", L".webp",
        L".docx", L".doc", L".odt", L".rtf", L".txt",
        L".pptx", L".ppt", L".odp",
        L".xlsx", L".xls", L".ods",
        L".html", L".htm",
        nullptr
    };

    for (int i = 0; exts[i]; ++i) {
        wchar_t extKey[256]{};
        StringCchPrintfW(extKey, 256,
            L"SOFTWARE\\Classes\\SystemFileAssociations\\%s"
            L"\\shellex\\ContextMenuHandlers\\zconverter",
            exts[i]);

        HKEY hk = nullptr;
        RegCreateKeyExW(HKEY_LOCAL_MACHINE, extKey,
            0, nullptr, 0, KEY_WRITE, nullptr, &hk, nullptr);

        RegSetValueExW(hk, nullptr, 0, REG_SZ,
            reinterpret_cast<const BYTE*>(clsidStr),
            static_cast<DWORD>(
                (wcslen(clsidStr) + 1) * sizeof(wchar_t)));

        RegCloseKey(hk);
    }

    // 4. Windows 11 classic context menu fix
    // This makes ALL context menu extensions visible without
    // "Show more options" — one key, affects all users
    {
        const wchar_t* win11Key =
            L"CLSID\\{86ca1aa0-34aa-4e8b-a509-50c905bae2a2}"
            L"\\InprocServer32";

        HKEY hk = nullptr;
        LONG result = RegCreateKeyExW(
            HKEY_LOCAL_MACHINE, win11Key,
            0, nullptr, 0, KEY_WRITE, nullptr, &hk, nullptr);

        if (result == ERROR_SUCCESS) {
            // Empty default value = restore classic menu
            RegSetValueExW(hk, nullptr, 0, REG_SZ,
                reinterpret_cast<const BYTE*>(L""),
                sizeof(wchar_t));
            RegCloseKey(hk);
        }
    }

    return S_OK;
}

// ── DllUnregisterServer ───────────────────────────────────

STDAPI DllUnregisterServer()
{
    const wchar_t* clsidStr =
        L"{A1B2C3D4-E5F6-7890-ABCD-EF1234567890}";

    // Remove CLSID
    {
        wchar_t key[256]{};
        StringCchPrintfW(key, 256, L"CLSID\\%s", clsidStr);
        RegDeleteTreeW(HKEY_LOCAL_MACHINE, key);
    }

    // Remove from approved list
    {
        HKEY hk = nullptr;
        RegOpenKeyExW(HKEY_LOCAL_MACHINE,
            L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\"
            L"Shell Extensions\\Approved",
            0, KEY_WRITE, &hk);
        if (hk) {
            RegDeleteValueW(hk, clsidStr);
            RegCloseKey(hk);
        }
    }

    // Remove handler from every extension
    const wchar_t* exts[] = {
        L".jpg", L".jpeg", L".png", L".bmp",
        L".tiff", L".tif", L".webp",
        L".docx", L".doc", L".odt", L".rtf", L".txt",
        L".pptx", L".ppt", L".odp",
        L".xlsx", L".xls", L".ods",
        L".html", L".htm",
        nullptr
    };

    for (int i = 0; exts[i]; ++i) {
        wchar_t extKey[256]{};
        StringCchPrintfW(extKey, 256,
            L"SOFTWARE\\Classes\\SystemFileAssociations\\%s"
            L"\\shellex\\ContextMenuHandlers\\zconverter",
            exts[i]);
        RegDeleteTreeW(HKEY_LOCAL_MACHINE, extKey);
    }

    // NOTE: we intentionally do NOT remove the Win11 classic
    // menu key on uninstall — it's a system preference the
    // user may want to keep. The installer can offer to remove
    // it as an option.

    return S_OK;
}