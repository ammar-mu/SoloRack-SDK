/* Please DO NOT modify this file at all. This is what maintains API/ABI compatibility with SoloRack
for virtual functions. Even the order in which these members/functions appear is relavent
future versions of the SDK will only add members/functions to the END (tail) of this file. Deprecated
members/functions will not be removed. Thier newer counter parts will be added to the end of this file.

If you want to add your own members/functions. Add them to the Module class or add to your classes that are
derived from the Module class.

TODO:
Compilers other than VC++ are not ABI compatible when using destructors. I have a few ideas of how to mitigate
this but it's not ready yet. Needs allot of testing. But basically it should involve defining the NONE_CPP_DESTRUCTOR 
directive, then implementing the Destructor() instead of the ~() function. Operators may also be incompatible across compilers.
*/

class ModuleWrapper : public CViewContainer
{
friend class SynEditor;
friend class SoloRack;
friend class ModuleKnob;
friend class CMovieBitmapEx;
public:
	virtual int GetSDKVersion()=0;
	ModuleWrapper(const CRect &size, CFrame *pParent, CBitmap *pBackground);
	virtual ~ModuleWrapper();
	//static void Module::Initialize();
	//static void Module::End();
	virtual bool forget()=0;

	virtual CMouseEventResult onMouseDown (CPoint &where, const long& buttons)=0;
	virtual CMouseEventResult onMouseUp (CPoint &where, const long& buttons)=0;
	virtual CMouseEventResult onMouseMoved (CPoint &where, const long& buttons)=0;
	virtual CMouseEventResult OnMouseMovedObserve(CPoint &where, const long& buttons)=0;

	virtual void PutLeftScrews(CMovieBitmap *&top_screw,CMovieBitmap *&bottom_screw, CControlListener *listener)=0;
	virtual void PutRightScrews(CMovieBitmap *&top_screw,CMovieBitmap *&bottom_screw, CControlListener *listener)=0;
	
	virtual void InitPatchPoints(float init)=0;
	virtual void ProcessSample()=0;				// For internal modules
	virtual const char *GetName()=0;
	virtual const int GetNameLen()=0;
	virtual const char *GetVendorName()=0;
	virtual const int GetVendorNameLen()=0;
	//static const int GetType();
	virtual long GetFreeTag()=0;
	virtual bool RegisterTag(CControl *pcon, long tag)=0;
	virtual bool UnRegisterTag(CControl *pcon)=0;
	//void ForceUnRegisterTag(CControl *pcon, long tag)=0;
	virtual bool CallProcessEvents()=0;
	virtual bool CallStartOfBlock()=0;
	virtual bool CallMouseObserve()=0;
	virtual bool ClearIfOrphaned()=0;
	virtual bool GetAlwaysONDefault()=0;
	virtual bool GetIsMonoDefault()=0;
	virtual bool IsAudioToDAW()=0;
	virtual bool IsPolyManager()=0;
	virtual inline void SendAudioToDAW(float left, float right)=0;
	virtual inline void SendAudioToDAW(PatchPoint **pps_outputs)=0;
	virtual inline void SendAudioToDAW(PatchPoint **pps_outputs, int first_output)=0;
	virtual inline void SendAudioToDAW(float *outputs, int last_output)=0;
	virtual inline void SendAudioToDAW(float *outputs, int first_output, int last_output)=0;

	virtual inline void ReceiveAudioFromDAW(float *left, float *right)=0;
	virtual inline void ReceiveAudioFromDAW(PatchPoint **pps_inputs)=0;
	virtual inline void ReceiveAudioFromDAW(PatchPoint **pps_inputs, int first_input)=0;
	virtual inline void ReceiveAudioFromDAW(float *inputs, int last_input)=0;
	virtual inline void ReceiveAudioFromDAW(float *inputs, int first_input, int last_input)=0;

	//inline void ReceiveAudioFromDAW(PatchPoint *inputs)=0;
	virtual inline int GetNumberOfAudioFromDAW()=0;
	virtual inline int GetNumberOfAudioToDAW()=0;
	virtual void ProcessEvents(const EventsHandle ev)=0;
	virtual void StartOfBlock(int sample_frames)=0;

	//static void TakeDAWOuts(float *output1,float *output2);
	//static void NextSample();

	virtual void CableConnected(PatchPoint *pp)=0;
	virtual void CableDisconnected(PatchPoint *pp)=0;
	virtual void ValueChanged(CControl* pControl)=0;
	virtual void CountControlsAndPatchPoints()=0;

	virtual void ResetNbControlsAndPatchPoints()=0;							// Must be called if number of controls in module changes for GetControlsValuesSize to work properly
	virtual void SetPPTags(long &ctag)=0;
	virtual void ConstructTags2PP(PatchPoint **tags2pp, long &ctag, int nb_pp)=0;
	virtual int GetControlsValuesSize()=0;
	virtual void SaveControlsValues(void *pdata)=0;
	virtual void LoadControlsValues(void *pdata, int size)=0;
	virtual int GetControlsTagsSize()=0;
	virtual void SaveControlsTags(void *pdata)=0;
	virtual void LoadControlsTags(void *pdata, int size)=0;
	virtual void UnRegisterControlsTags()=0;
	virtual int GetPresetSize()=0;
	virtual void SavePreset(void *pdata, int size)=0;
	virtual void LoadPreset(void *pdata, int size, int version)=0;
	virtual void SetSampleRate(float sr)=0;				// Will not be auto called when a module is first created. sample_rate is set by the contructor.
														// Will only be called when sample rate changes.
														// Your version of SetSampleRate() is responsible for calling SetBandLimit() OR accounting for bandlimit value within it (if you require it by your module). Solorack will not autocall SetBandLimit() for you
														// I do this because SetBandLimit() may include memory allocation for things like Minblep, etc. Which can trigger multiple/redundant allocations if both functions are called when a preset is loaded.
	virtual void SetDAWSampleRate(float daw_sr)=0;		// SetSampleRate() will be imidiatly called after SetDAWSampleRate().
	virtual void SetDAWBlockSize(float blocksize)=0;				// Called when ever DAW block size changes. This usually not neccesary to be implemented except for modules that need to know the DAW block size
	//virtual void SetBlockSize(int bs)=0;
	virtual void SetKnobsSmoothDelay(int del)=0;
	virtual int GetVersion()=0;				// -1 means no version is specified. But modules better override this with a real version number
	virtual bool SetBandLimit(int bndlim)=0;				// Will only be called by solorack when bandlimit is changed/set by the user.
	//static Product *Activate(char *fullname, char *email, char *serial);		// Activate Licence
	//Deactivate() { is_active=false; }					// But will stay active if parent is active
	//static bool IsActive();								// Is Licence activated ?
	virtual Product *InstanceActivate(char *fullname, char *email, char *serial)=0;		// Activate Licence
	virtual bool InstanceIsActive()=0;					// Is Licence activated ?. Same, but called from instance instead of class
	virtual void AddDemoViews(char *msg)=0;
	virtual void SetEnableDemo(bool state)=0;
	virtual void EnterProcessingCriticalSection()=0;
	virtual void LeaveProcessingCriticalSection()=0;
	virtual const char *GetInfoURL()=0;
	virtual void PolyphonyChanged(int voices)=0;
	virtual void SetForceMono(PatchPoint *pp, bool set_is_mono)=0;
	virtual Module *GetVoiceModule(int voice)=0;
	virtual void ConstructionComplete(int voices)=0;
	virtual void DestructionStarted()=0;
	virtual void Destructor()=0;
	virtual PatchPoint *AddPatchPoint(float x, float y, int pptype, CBitmap **ppbit, int bitm_index, CControlListener *listener)=0;
	virtual PatchPoint *AddMIDIPatchPoint(float x, float y, int pptype, CBitmap *bitmap, CControlListener *listener)=0;
	virtual ModuleKnob *AddModuleKnob(float x, float y, CBitmap *bitmap, int num_images, bool is_stepping, CControlListener *listener, long tag=NEW_TAG)=0;
	virtual CVerticalSwitch *AddVerticalSwitch(float x, float y, CBitmap *bitmap, int num_images, CControlListener *listener)=0;
	virtual CMovieBitmap *AddMovieBitmap(float x, float y, CBitmap *bitmap, int num_images, CControlListener *listener, bool centre=true)=0;
	virtual CKickButton *AddKickButton(float x, float y, CBitmap *bitmap, int num_images, CControlListener *listener)=0;
	virtual COnOffButton *AddOnOffButton(float x, float y, CBitmap *bitmap, int num_images, CControlListener *listener, long style, long tag=NEW_TAG)=0;
	virtual CSpecialDigit *AddSpecialDigit(float x, float y, CBitmap *bitmap, int num_images, int inumbers, long tag, CControlListener *listener)=0;
	virtual CSpecialDigitEx *AddSpecialDigitEx(float x, float y, CBitmap *bitmap, int num_images, int inumbers, long tag, CControlListener *listener, CBitmap* blank)=0;
	virtual ModuleKnobEx *AddModuleKnobEx(float x, float y, CBitmap *bitmap, int num_images, bool is_stepping, CControlListener *listener, CControl *vattached1, int vtag1, long tag=NEW_TAG)=0;
	virtual CTextLabel *AddTextLabel(float x, float y, CBitmap *bitmap, long style, float width, float height)=0;
	virtual CTextLabelEx *AddTextLabelEx(float x, float y, CBitmap *bitmap, long style, float width, float height)=0;
	virtual CHorizontalSlider *AddHorizontalSlider(float x, float y, float width, float height, CBitmap *bitmap, CBitmap *background, CControlListener *listener)=0;

	//virtual void DeleteModule()=0;
	//virtual void forget()=0;
	//const char *(__thiscall *GetInfoURL)();
	//static Product GetProduct();
	//static Module *Constructor(CFrame *pParent, CControlListener *listener, const int vvoice=0);				// Used to construct modules that are imported for Dll files. Each sub class should define it's own Constructor.

	//CLASS_METHODS(Module, CViewContainer)
	virtual bool isTypeOf (const char* s) const=0;
	virtual CView* newCopy () const=0;

	// Setters and getters
	virtual float GetSampleRate()=0;
	virtual float GetHalfSampleRate()=0;
	virtual float Get60SampleRate()=0;
	virtual float GetDAWSampleRate()=0;
	virtual int GetDAWBlockSize()=0;

	virtual int GetNBControls()=0;
	virtual void SetNBControls(int n)=0;
	virtual int GetNBPatchPoints()=0;
	virtual void SetNBPatchPoints(int n)=0;
	virtual int GetNBCables()=0;
	virtual void SetNBCables(int n)=0;
	virtual int GetBandLimit()=0;

	virtual SoloRack *GetSynth()=0;
	virtual void SetSynth(SoloRack *p)=0;
	virtual const SynthComm GetSynthComm()=0;
	virtual void SetSynthComm(const SynthComm *p)=0;
	virtual SynEditor *GetSynEditor()=0;
	virtual void SetSynEditor(SynEditor *p)=0;
	
	virtual bool GetInMove()=0;
	virtual void SetInMove(bool b)=0;
	virtual int GetIndex()=0;
	virtual void SetIndex(int i)=0;

	virtual int GetProcIndex()=0;
	virtual void SetProcIndex(int i)=0;
	virtual int GetEvIndex()=0;
	virtual void SetEvIndex(int i)=0;
	virtual int GetSbIndex()=0;
	virtual void SetSbIndex(int i)=0;
	virtual int GetMouseObIndex()=0;
	virtual void SetMouseObIndex(int i)=0;
	virtual float GetClipLevel()=0;
	virtual void SetClipLevel(float v)=0;
	virtual int GetVoice()=0;
	virtual void SetVoice(int vvoice)=0;

};

