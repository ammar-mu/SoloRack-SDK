#include "TestModules.h"

// This is the one in VSTGUI. 
extern OSVERSIONINFOEX	gSystemVersion;	

// MIDI Frequencies Table
extern float mfreqtab[128];

// The folloiwng are just X,Y offsets that we use for pannels and positioning of controls. You can remove them if you don't need
#define FIRST_KY	55
#define FIRST_KY_DOWN	FIRST_KY+18
#define FIRST_SKY_DANGLING		FIRST_KY-11
#define FIRST_PPY	56
#define FIRST_PPY_DOWN	FIRST_PPY+17
#define FIRST_RIGHT_KX	62
#define FIRST_LEFT_KX	32
#define FIRST_LEFT_SKX	31
#define FIRST_LEFT_PPX	16
#define FIRST_RIGHT_PPX	72
#define FIRST_RIGHT_PPX2	63
#define XOFFSET 48
#define YOFFSET	43
#define YOFFSET_PP_SQWEEZE	32
#define YOFFSET_SK_SQWEEZE	32
#define PP_XOFFSET 31

//---------------------------------------------
// TestMixer
CBitmap *TestMixer::panel = NULL;
char *TestMixer::name = "Test Mixer";
int TestMixer::name_len=0;
char *TestMixer::vendorname = "SoloStuff";
int TestMixer::vendorname_len=0;
Product *TestMixer::pproduct = NULL;

TestMixer::TestMixer(CFrame *pParent, CControlListener *listener,const SynthComm *psynth_comm, const int vvoice)
: Module(CRect(0, 0, panel->getWidth(), panel->getHeight()),pParent,panel,psynth_comm,vvoice)
{	int i;
	long tag;
	PatchPoint *temp[5];

	// Create The Knobs
	kin1 = AddModuleKnob(FIRST_RIGHT_KX,FIRST_KY,mcbits[knobit],MAIN_KNOB_IMAGES,false,listener);
	kin1->setValue(0.5); kin1->svalue=0.5; //chpool->AddPoolKnob(kin1);

	kin2 = AddModuleKnob(FIRST_RIGHT_KX,FIRST_KY+YOFFSET,mcbits[knobit],MAIN_KNOB_IMAGES,false,listener);
	kin2->setValue(0.5); kin2->svalue=0.5; //chpool->AddPoolKnob(kin2);

	kin3 = AddModuleKnob(FIRST_RIGHT_KX,FIRST_KY+2*YOFFSET,mcbits[knobit],MAIN_KNOB_IMAGES,false,listener);
	kin3->setValue(0.5); kin3->svalue=0.5; //chpool->AddPoolKnob(kin3);

	kout = AddModuleKnob(FIRST_RIGHT_KX,FIRST_KY+4*YOFFSET,mcbits[knobit],MAIN_KNOB_IMAGES,false,listener);
	kout->setValue(1.0); kout->svalue=1.0; //chpool->AddPoolKnob(kout);

	// Create Patch Points
	for (i=0; i<5; i++)
		temp[i] = AddPatchPoint(FIRST_LEFT_PPX,FIRST_PPY+i*YOFFSET,ppTypeInput,ppbit,RAND_BITMAP,listener);
	ppin1=temp[0]; ppin2=temp[1]; ppin3=temp[2]; ppin4=temp[3]; ppout=temp[4];
	ppout->SetType(ppTypeOutput);

	ppin5 = AddPatchPoint(62,FIRST_PPY+3*YOFFSET,ppTypeInput,ppbit,RAND_BITMAP,listener);

	// Create inverter switch
	//CRect r = CRect(0,0,1,1);
	//r.moveTo(36-mcbits[vert_swbit]->getWidth()/2, 256-mcbits[vert_swbit]->getHeight()/4);
	//r.setSize(CPoint(mcbits[vert_swbit]->getWidth(),mcbits[vert_swbit]->getHeight()/2));
	//sw1 = new CVerticalSwitch(r,listener,tag=GetFreeTag(),mcbits[vert_swbit]); addView(sw1); RegisterTag(sw1,tag);
	sw1 = AddVerticalSwitch(36,255,mcbits[vert_swbit],2,listener);

	// Put some screws
	PutLeftScrews(screw1,screw2,listener);

	InitPatchPoints(0.0);

	// Pretend that the switch value has changed so that correct settings are applied
	ValueChanged(sw1);
}

//TestMixer::~TestMixer()
//{	
//	
//}


// SoloRack calls this. It can't directly access rhe constructor since this is a Dll and pointers to constructor can not be easily achieved.
TestMixer *TestMixer::Constructor(CFrame *pParent, CControlListener *listener,const SynthComm *psynth_comm, const int vvoice)
{	return new TestMixer(pParent,listener,psynth_comm,vvoice);
}

void TestMixer::Initialize()
{	char *stemp;
	
	panel = new CBitmap(dllskindir,NULL,"TestMixer.png");
	name_len = strlen(name); vendorname_len = strlen(vendorname);
}

void TestMixer::End()
{	panel->forget();
	if (pproduct!=NULL) delete pproduct;
}

const char * TestMixer::GetName()
{	return name;
}

const int TestMixer::GetNameLen()
{	return name_len;
}

const char * TestMixer::GetVendorName()
{	return vendorname;
}

const int TestMixer::GetVendorNameLen()
{	return vendorname_len;
}

const int TestMixer::GetType()
{	return kMixerPanner;
}

Product *TestMixer::Activate(char *fullname, char *email, char *serial)
{	// Replace this with your own implementation
	// This should return a pointer to the activated product. Or NULL if activation fails
	// In our TestMixer, NULL is retuned because the module is already active all the time, so no need to activate.
	return NULL;

	// Here is a simplistic example.
	//if (pproduct!=NULL) delete pproduct;
	//pproduct = new Product(0,1,NULL,false,"SoloStuff TestMixer");		// Product names has to include vendor name to ensure uniquenes
	//return pproduct->Activate(fullname,email,serial);
}

bool TestMixer::IsActive()
{	// This test module is Active. Replace this with your own check. 
	// It's better not to store this active/inactive status in an obvious place in memory, like a data member of the module or like that.
	// It's even better if the status is not stored at all, but rather a sophisticated test is done
	return true;

	// simple example
	//if (pproduct!=NULL) return pproduct->IsActive(); else return false;
}

Product *TestMixer::InstanceActivate(char *fullname, char *email, char *serial)
{	return this->Activate(fullname,email,serial);
}

bool TestMixer::InstanceIsActive()
{	return this->IsActive();
}

const char *TestMixer::GetProductName()
{	// Change this to your own product name. Product names should include the vendor name to avoid conflicts with other vendor modules in the config.ini
	return "SoloStuff TestMixer";
}

void TestMixer::ValueChanged(CControl* pControl)
{	if (pControl==sw1)
		invert = (sw1->value>=0.5);		// 0.5 and above visualy displays as image 1, anything less than 0.5, displayes as image 0
	
	// If a knob change pool is defined
	/*else
	{	if (chpool->IsKnobInPool(pControl))
			chpool->LockPoolKnobValueChanged(pControl,this);
	}*/
		
}

inline void TestMixer::ProcessSample()
{	float temp;
	
	// Update smooth values.
	// If there are too many of those smoothed knobs, then its more CPU efficient to use a knob change pool using the ModuleKnobExPool class
	// You do this by calling chpool->UpdatePoolSValues() here.
	kin1->UpdateSValue();
	kin2->UpdateSValue();
	kin3->UpdateSValue();
	kout->UpdateSValue();

	// Mixer
	temp = kout->svalue*(ppin1->in*kin1->svalue+ppin2->in*kin2->svalue+ppin3->in*kin3->svalue+ppin4->in+ppin5->in);
	if (invert)
		ppout->out = -temp;
	else 
		ppout->out = temp;
}


//---------------------------------------------
// TestTempoFromDAW
// This module is identical to our SA04 except for the GUI.
CBitmap *TestTempoFromDAW::panel = NULL;
char *TestTempoFromDAW::name = "Test Tempo From DAW";
int TestTempoFromDAW::name_len=0;
char *TestTempoFromDAW::vendorname = "SoloStuff";
int TestTempoFromDAW::vendorname_len=0;

TestTempoFromDAW::TestTempoFromDAW(CFrame *pParent, CControlListener *listener,const SynthComm *psynth_comm, const int vvoice)
: Module(CRect(0, 0, panel->getWidth(), panel->getHeight()),pParent,panel,psynth_comm,vvoice)
{	int i, hw;
	long tag;

	hw = 23;

	// Create Patch Points
	ppclock = AddPatchPoint(hw,198,ppTypeOutput,ppbit,RAND_BITMAP,listener);

	pprestart = AddPatchPoint(hw,75,ppTypeInput,ppbit,RAND_BITMAP,listener);

	// Create divisor knob
	kdivisor = this->AddModuleKnob(hw,157,mcbits[sknobit_black5],5,true,listener);
	kdivisor->setValue(1.0);

	// Put leds
	lclock = AddMovieBitmap(hw,222,mcbits[led_blue],LED_BLUE_IMAGES,listener);

	// Put some screws
	PutLeftScrews(screw1,screw2,listener);

	InitPatchPoints(0.0);

	// Simulate the value change to initialize divisor value
	ValueChanged(kdivisor);

	// Tell SoloRack that this module needs a call to StartOfBlock() at the start of each block
	CallStartOfBlock();

	ppqpos=0; pspc=0; chigh=MAX_INT, last_restart=0; 

}

//TestTempoFromDAW::~TestTempoFromDAW()
//{
//}

// SoloRack calls this. It can't directly access rhe constructor since this is a Dll and pointers to constructor can not be easily achieved.
TestTempoFromDAW *TestTempoFromDAW::Constructor(CFrame *pParent, CControlListener *listener,const SynthComm *psynth_comm, const int vvoice)
{	return new TestTempoFromDAW(pParent,listener,psynth_comm,vvoice);
}

const char *TestTempoFromDAW::GetProductName()
{	// Change this to your own product name. It's recommended that Product names include the vendor name to avoid conflicts 
	// with other vendor modules in the config.ini. But this is not a must if you know what you are doing.
	// Product names are used for license activation. This is meant to separate licensing from actual module names 
	// which may or may not change in the future.
	return "SoloStuff Test Tempo From DAW";
}

void TestTempoFromDAW::Initialize()
{	panel = new CBitmap(dllskindir,NULL,"TestTempoFromDAW.png");
	name_len = strlen(name); vendorname_len = strlen(vendorname);
}

void TestTempoFromDAW::End()
{	panel->forget();
}

const char * TestTempoFromDAW::GetName()
{	return name;
}

const int TestTempoFromDAW::GetNameLen()
{	return name_len;
}

const char * TestTempoFromDAW::GetVendorName()
{	return vendorname;
}

const int TestTempoFromDAW::GetVendorNameLen()
{	return vendorname_len;
}


const int TestTempoFromDAW::GetType()
{	return kFromToDAW;
}

Product *TestTempoFromDAW::Activate(char *fullname, char *email, char *serial)
{	// Replace this with your own implementation
	// This should return a pointer to the activated product. Or NULL if activation fails
	// In our TestTempoFromDAW, NULL is retuned because the module is already active all the time, so no need to activate.
	return NULL;

	// Here is a simplistic example.
	//if (pproduct!=NULL) delete pproduct;
	//pproduct = new Product(0,1,NULL,false,"SoloStuff Test Tempo From DAW");		// Product names has to include vendor name to ensure uniquenes
	//return pproduct->Activate(fullname,email,serial);
}

bool TestTempoFromDAW::IsActive()
{	// This test module is Active. Replace this with your own check. 
	// It's better not to store this active/inactive status in an obvious place in memory, like a data member of the module or like that.
	// It's even better if the status is not stored at all, but a rather sophisticated test is done
	return true;

	// simple example
	//if (pproduct!=NULL) return pproduct->IsActive(); else return false;
}


Product *TestTempoFromDAW::InstanceActivate(char *fullname, char *email, char *serial)
{	return this->Activate(fullname,email,serial);
}

bool TestTempoFromDAW::InstanceIsActive()
{	return this->IsActive();
}

void TestTempoFromDAW::ValueChanged(CControl* pControl)
{	//if (pControl==kdivisor) kdivisor->UpdateQValue();
	if (pControl==kdivisor) divisor = kdivisor->GetCurrentStep();
}

void TestTempoFromDAW::SetSampleRate(float sr)
{	Module::SetSampleRate(sr);
	
	tempo_smp = tempo/sample_rate60;
	//pspc=CLOCK_WIDTH*sample_rate60/(tempo*mult);
	pspc=CLOCK_WIDTH/(tempo_smp*mult);
}

void TestTempoFromDAW::LoadPreset(void *pdata, int size, int version)
{	LoadControlsValues(pdata,size);

	// Make sure LEDs are not affected incorrectly.
	lclock->setValue(ppclock->out);
}

inline void TestTempoFromDAW::ProcessSample()
{	
	// Reset phase if restart is high cv
	if (last_restart<HIGH_CV && pprestart->in>=HIGH_CV) 
	{	// If the clock is already high, make it low, then make it high in the next sample.
		if (ppclock->out>=HIGH_CV)
		{	ppqpos=1; ppclock->out=0;
			
			lclock->value=0.0; //lclock->setDirty(true);
			/* Note: there is no use of setdirty() here. VSTGUI will detect change in value and update the GUI latter.
			Most importantly, it's better not to use Invalid() in ProcessSample() unless you absolutly have to
			Invalid() will trigger an imidiate change to the update region of the control's GUI, which will be very bad for CPU when 
			done at audio rate.
			*/
		}
		else 
		{	ppqpos=0; ppclock->out=1; chigh=pspc; 
			
			lclock->value=1.0; //lclock->setDirty(true);
		}
		last_restart=1; return;
	}
	else 
	{	ppqpos+=tempo_smp;
		if (ppqpos>=1.0)
		{	ppqpos-=1.0;
			ppclock->out=1; chigh=pspc; lclock->value=1.0; //lclock->setDirty(true);
			last_restart=pprestart->in; return;
		}
	}
	last_restart=pprestart->in;

	
	// So there is no clock now. Setting chigh=MAX_INT to eventually avoid doing two if statements. Wierd, I know, but works.
	chigh--;
	if (chigh==0) { ppclock->out=0; chigh=MAX_INT; lclock->value=0.0; /*lclock->setDirty(true);*/ }
}


void TestTempoFromDAW::StartOfBlock(int sample_frames)
{	static float mul[5] = {24, 8, 4, 2, 1};			// Divisor = 24/mul[]
	
	// Get DAW tempo
	DAWTime pt;
	synth_comm.GetDAWTime(peditor,&pt);
	mult = mul[divisor];
	tempo_smp = mult*(tempo=pt.tempo)/sample_rate60;
	pspc=CLOCK_WIDTH/tempo_smp;	
}