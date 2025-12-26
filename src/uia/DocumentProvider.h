/* Copyright 2022 the GurupiaReader project authors (see AUTHORS file).
   License: GPLv3 */

struct DisplayModel;
class ReaderUIAutomationProvider;
class ReaderUIAutomationPageProvider;
class ReaderUIAutomationTextRange;

class ReaderUIAutomationDocumentProvider : public IRawElementProviderFragment,
                                            public IRawElementProviderSimple,
                                            public ITextProvider,
                                            public IAccIdentity {
    LONG refCount;
    HWND canvasHwnd;
    ReaderUIAutomationProvider* root;
    bool released;

    ReaderUIAutomationPageProvider* child_first;
    ReaderUIAutomationPageProvider* child_last;

    DisplayModel* dm;

  public:
    ReaderUIAutomationDocumentProvider(HWND canvasHwnd, ReaderUIAutomationProvider* root);
    ~ReaderUIAutomationDocumentProvider();

    // reads page count and creates a child element for each page
    void LoadDocument(DisplayModel* dm);
    void FreeDocument();
    bool IsDocumentLoaded() const;

    // GetDM() must not be called if IsDocumentLoaded()==FALSE
    DisplayModel* GetDM();

    ReaderUIAutomationPageProvider* GetFirstPage();
    ReaderUIAutomationPageProvider* GetLastPage();

    // IUnknown
    HRESULT STDMETHODCALLTYPE QueryInterface(const IID&, void**);
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();

    // IRawElementProviderFragment
    HRESULT STDMETHODCALLTYPE Navigate(enum NavigateDirection direction, IRawElementProviderFragment** pRetVal);
    HRESULT STDMETHODCALLTYPE GetRuntimeId(SAFEARRAY** pRetVal);
    HRESULT STDMETHODCALLTYPE GetEmbeddedFragmentRoots(SAFEARRAY** pRetVal);
    HRESULT STDMETHODCALLTYPE SetFocus();
    HRESULT STDMETHODCALLTYPE get_BoundingRectangle(struct UiaRect* pRetVal);
    HRESULT STDMETHODCALLTYPE get_FragmentRoot(IRawElementProviderFragmentRoot** pRetVal);

    // IRawElementProviderSimple
    HRESULT STDMETHODCALLTYPE GetPatternProvider(PATTERNID patternId, IUnknown** pRetVal);
    HRESULT STDMETHODCALLTYPE GetPropertyValue(PROPERTYID propertyId, VARIANT* pRetVal);
    HRESULT STDMETHODCALLTYPE get_HostRawElementProvider(IRawElementProviderSimple** pRetVal);
    HRESULT STDMETHODCALLTYPE get_ProviderOptions(ProviderOptions* pRetVal);

    // ITextProvider
    HRESULT STDMETHODCALLTYPE GetSelection(SAFEARRAY** pRetVal);
    HRESULT STDMETHODCALLTYPE GetVisibleRanges(SAFEARRAY** pRetVal);
    HRESULT STDMETHODCALLTYPE RangeFromChild(IRawElementProviderSimple* childElement, ITextRangeProvider** pRetVal);
    HRESULT STDMETHODCALLTYPE RangeFromPoint(struct UiaPoint point, ITextRangeProvider** pRetVal);
    HRESULT STDMETHODCALLTYPE get_DocumentRange(ITextRangeProvider** pRetVal);
    HRESULT STDMETHODCALLTYPE get_SupportedTextSelection(enum SupportedTextSelection* pRetVal);

    // IAccIdentity
    HRESULT STDMETHODCALLTYPE GetIdentityString(DWORD dwIDChild, BYTE** ppIDString, DWORD* pdwIDStringLen);
};
