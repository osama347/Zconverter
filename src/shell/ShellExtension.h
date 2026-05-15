#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlobj.h>
#include <atomic>
#include <vector>
#include <string>

// {A1B2C3D4-E5F6-7890-ABCD-EF1234567890}
static constexpr GUID CLSID_ZConverterShellExt = {
    0xa1b2c3d4, 0xe5f6, 0x7890,
    { 0xab, 0xcd, 0xef, 0x12, 0x34, 0x56, 0x78, 0x90 }
};

namespace zc {

    class ShellExtension
        : public IShellExtInit
        , public IContextMenu
    {
    public:
        ShellExtension();
        virtual ~ShellExtension() = default;

        // IUnknown
        STDMETHOD(QueryInterface)(REFIID riid, void** ppv) override;
        STDMETHOD_(ULONG, AddRef)()  override;
        STDMETHOD_(ULONG, Release)() override;

        // IShellExtInit
        STDMETHOD(Initialize)(
            PCIDLIST_ABSOLUTE pidlFolder,
            IDataObject* pDataObj,
            HKEY              hKeyProgID) override;

        // IContextMenu
        STDMETHOD(QueryContextMenu)(
            HMENU hmenu,
            UINT  indexMenu,
            UINT  idCmdFirst,
            UINT  idCmdLast,
            UINT  uFlags) override;

        STDMETHOD(InvokeCommand)(
            LPCMINVOKECOMMANDINFO pici) override;

        STDMETHOD(GetCommandString)(
            UINT_PTR idCmd,
            UINT     uType,
            UINT* pReserved,
            CHAR* pszName,
            UINT     cchMax) override;

    private:
        std::atomic<ULONG> refCount_{ 1 };
        std::vector<std::wstring> selectedFiles_;
        bool                      hasConvertibleSelection_{ false };
    };

    class ShellExtensionFactory : public IClassFactory
    {
    public:
        STDMETHOD(QueryInterface)(REFIID riid, void** ppv) override;
        STDMETHOD_(ULONG, AddRef)()  override { return 2; }
        STDMETHOD_(ULONG, Release)() override { return 1; }

        STDMETHOD(CreateInstance)(
            IUnknown* pUnkOuter,
            REFIID    riid,
            void** ppv) override;

        STDMETHOD(LockServer)(BOOL) override { return S_OK; }
    };

} // namespace zc