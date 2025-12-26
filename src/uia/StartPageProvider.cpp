/* Copyright 2022 the GurupiaReader project authors (see AUTHORS file).
   License: GPLv3 */

#include "utils/BaseUtil.h"
#include <UIAutomationCore.h>
#include <UIAutomationCoreApi.h>
#include "uia/StartPageProvider.h"
#include "uia/Constants.h"
#include "uia/Provider.h"

ReaderUIAutomationStartPageProvider::ReaderUIAutomationStartPageProvider(HWND canvasHwnd,
                                                                           ReaderUIAutomationProvider* root)
    : refCount(1), canvasHwnd(canvasHwnd), root(root) {
    // root->AddRef(); Don't add refs to our parent & owner.
}

ReaderUIAutomationStartPageProvider::~ReaderUIAutomationStartPageProvider() = default;

// IUnknown
HRESULT STDMETHODCALLTYPE ReaderUIAutomationStartPageProvider::QueryInterface(REFIID riid, void** ppv) {
    static const QITAB qit[] = {QITABENT(ReaderUIAutomationStartPageProvider, IRawElementProviderSimple),
                                QITABENT(ReaderUIAutomationStartPageProvider, IRawElementProviderFragment),
                                {nullptr}};
    return QISearch(this, qit, riid, ppv);
}

ULONG STDMETHODCALLTYPE ReaderUIAutomationStartPageProvider::AddRef() {
    return InterlockedIncrement(&refCount);
}

ULONG STDMETHODCALLTYPE ReaderUIAutomationStartPageProvider::Release() {
    LONG res = InterlockedDecrement(&refCount);
    ReportIf(res < 0);
    if (0 == res) {
        delete this;
    }
    return res;
}

HRESULT STDMETHODCALLTYPE ReaderUIAutomationStartPageProvider::Navigate(enum NavigateDirection direction,
                                                                         IRawElementProviderFragment** pRetVal) {
    if (pRetVal == nullptr) {
        return E_POINTER;
    }

    *pRetVal = nullptr;
    // no siblings, no children
    if (direction == NavigateDirection_NextSibling || direction == NavigateDirection_PreviousSibling ||
        direction == NavigateDirection_FirstChild || direction == NavigateDirection_LastChild) {
        return S_OK;
    } else if (direction == NavigateDirection_Parent) {
        *pRetVal = root;
        (*pRetVal)->AddRef();
        return S_OK;
    } else {
        return E_INVALIDARG;
    }
}

HRESULT STDMETHODCALLTYPE ReaderUIAutomationStartPageProvider::GetRuntimeId(SAFEARRAY** pRetVal) {
    if (pRetVal == nullptr) {
        return E_POINTER;
    }

    SAFEARRAY* psa = SafeArrayCreateVector(VT_I4, 0, 2);
    if (!psa) {
        return E_OUTOFMEMORY;
    }

    // RuntimeID magic, use hwnd to differentiate providers of different windows
    int rId[] = {(int)canvasHwnd, SUMATRA_UIA_STARTPAGE_RUNTIME_ID};
    for (LONG i = 0; i < 2; i++) {
        HRESULT hr = SafeArrayPutElement(psa, &i, (void*)&(rId[i]));
        ReportIf(FAILED(hr));
    }

    *pRetVal = psa;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE ReaderUIAutomationStartPageProvider::GetEmbeddedFragmentRoots(SAFEARRAY** pRetVal) {
    if (pRetVal == nullptr) {
        return E_POINTER;
    }

    // no other roots => return nullptr
    *pRetVal = nullptr;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE ReaderUIAutomationStartPageProvider::SetFocus() {
    return S_OK;
}

HRESULT STDMETHODCALLTYPE ReaderUIAutomationStartPageProvider::get_BoundingRectangle(struct UiaRect* pRetVal) {
    // share area with the canvas uia provider
    return root->get_BoundingRectangle(pRetVal);
}

HRESULT STDMETHODCALLTYPE
ReaderUIAutomationStartPageProvider::get_FragmentRoot(IRawElementProviderFragmentRoot** pRetVal) {
    if (pRetVal == nullptr) {
        return E_POINTER;
    }

    *pRetVal = root;
    root->AddRef();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE ReaderUIAutomationStartPageProvider::GetPatternProvider(__unused PATTERNID patternId,
                                                                                   IUnknown** pRetVal) {
    *pRetVal = nullptr;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE ReaderUIAutomationStartPageProvider::GetPropertyValue(PROPERTYID propertyId,
                                                                                 VARIANT* pRetVal) {
    if (propertyId == UIA_NamePropertyId) {
        pRetVal->vt = VT_BSTR;
        pRetVal->bstrVal = SysAllocString(L"Start Page");
        return S_OK;
    }

    pRetVal->vt = VT_EMPTY;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE
ReaderUIAutomationStartPageProvider::get_HostRawElementProvider(IRawElementProviderSimple** pRetVal) {
    if (pRetVal == nullptr) {
        return E_POINTER;
    }
    *pRetVal = nullptr;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE ReaderUIAutomationStartPageProvider::get_ProviderOptions(ProviderOptions* pRetVal) {
    if (pRetVal == nullptr) {
        return E_POINTER;
    }
    *pRetVal = ProviderOptions_ServerSideProvider;
    return S_OK;
}
