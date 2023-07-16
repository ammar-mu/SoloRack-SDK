#ifndef __TestModules__
#define __TestModules__

#ifndef __Modules__
#include "Modules.h"
#endif

//---------------------------------------------
// TestMixer

//#define TESTMIXER_NUM_KNOBS		4
class TestMixer : public Module
{
public:

	TestMixer(CFrame *pParent, CControlListener *listener, const SynthComm *psynth_comm, const int vvoice=0);

	//~TestMixer();

	static void Initialize();
	static void End();
	static const int GetType();
	static Product *Activate(char *fullname, char *email, char *serial);				// Activate Licence
	static bool IsActive();	
	static TestMixer *Constructor(CFrame *pParent, CControlListener *listener, const SynthComm *psynth_comm, const int vvoice=0);										// Used to construct modules that are imported for Dll files. Each sub class should define it's own Constructor.
	static const char *GetProductName();
	const char *GetName();
	const int GetNameLen();
	const char *GetVendorName();
	const int GetVendorNameLen();
	void ProcessSample();
	int GetVersion() { return 1000; }														// Is Licence activated ?
	Product *InstanceActivate(char *fullname, char *email, char *serial);		// Virtual Version
	bool InstanceIsActive();													// Virtual Version
	void ValueChanged(CControl* pControl);

	static Product *pproduct;
	static char *name;					// Module title name that will be displayed in the menu
	static int name_len;				// Length of the name string excluding terminating NULL.
	static char *vendorname;			// Module vendor name
	static int vendorname_len;			// Length of the vendor name string excluding terminating NULL.

protected:
	static CBitmap *panel;

private:
	// Inputs
	//float in1, in2, in3, in4;
	
	// Input connections (cables)
	//float *pin1,*pin2,*pin3,*pin4;

	// Outputs
	//float out;
	
	// Knobs
	ModuleKnob *kin1,*kin2,*kin3, *kout;

	// Knobs. If you want to use a change pool, you'll have to define them as ModuleKnobEx
	//ModuleKnobEx *kin1,*kin2,*kin3, *kout;

	// Patch points
	PatchPoint *ppin1,*ppin2,*ppin3,*ppin4,*ppin5,*ppout;

	// Switches
	CVerticalSwitch *sw1;

	// Screws
	CMovieBitmap *screw1,*screw2;

	// knob change pool
	//ModuleKnobExPool<TESTMIXER_NUM_KNOBS> chpool;

	// Other
	bool invert;

};


//---------------------------------------------
// TestTempoFromDAW
class TestTempoFromDAW : public Module
{
public:

	TestTempoFromDAW(CFrame *pParent, CControlListener *listener, const SynthComm *psynth_comm, const int vvoice=0);

	//~TestTempoFromDAW();

	static void Initialize();
	static void End();
	static TestTempoFromDAW *Constructor(CFrame *pParent, CControlListener *listener, const SynthComm *psynth_comm, const int vvoice=0);										// Used to construct modules that are imported for Dll files. Each sub class should define it's own Constructor.
	static const char *GetProductName();
	static Product *Activate(char *fullname, char *email, char *serial);		// Activate Licence
	static bool IsActive();
	static const int GetType();
	const char *GetName();
	const int GetNameLen();
	const char *GetVendorName();
	const int GetVendorNameLen();
	virtual void SetSampleRate(float sr);
	virtual void LoadPreset(void *pdata, int size, int version);
	void ProcessSample();
	void StartOfBlock(int sample_frames);
	virtual int GetVersion() { return 1000; }
	virtual void ValueChanged(CControl* pControl);
	virtual bool GetAlwaysONDefault() { return true; }	
	virtual Product *InstanceActivate(char *fullname, char *email, char *serial);		// Virtual Version
	virtual bool InstanceIsActive();													// Virtual Version

	static char *name;					// Module title name that will be displayed in the menu
	static int name_len;				// Length of the name string excluding terminating NULL.
	static char *vendorname;			// Module vendor name
	static int vendorname_len;			// Length of the vendor name string excluding terminating NULL.
	static Product *pproduct;
	
	//CMouseEventResult onMouseDown (CPoint &where, const long& buttons);
	//CMouseEventResult onMouseUp (CPoint &where, const long& buttons);
	//CMouseEventResult onMouseMoved (CPoint &where, const long& buttons);

protected:
	static CBitmap *panel;

private:
	// Knobs
	ModuleKnob *kdivisor;

	// Patch points
	PatchPoint *ppclock,*pprestart;

	// Leds
	CMovieBitmap *lclock;

	// Screws
	CMovieBitmap *screw1,*screw2;

	// Events array (Queue)
	int pspc,chigh;							// pspc is a Percentage * Samples Per Clock
	float mult,last_restart;
	long divisor;

	double ppqpos;							// Position Per Quarter note (after division). BUT this will not go more than 1.0
	double tempo,tempo_smp;					// tempo is BPM, tempo_smp is BPS (beats per sample)
};

#endif	// __TestModules__