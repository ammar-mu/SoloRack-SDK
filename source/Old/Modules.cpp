/*	SoloRack SDK v0.11 Beta
	Copyright 2017-2020 Ammar Muqaddas
*/

#include "time.h"

#ifndef __Modules__
#include "Modules.h"
#endif


extern OSVERSIONINFOEX	gSystemVersion;			// This is the one in VSTGUI. 

//unsigned int fp_control_state = _controlfp(_EM_INEXACT | _EM_OVERFLOW | _EM_ZERODIVIDE | _EM_UNDERFLOW | _EM_DENORMAL | _EM_INVALID, _MCW_EM);
//unsigned int fp_control_state = _controlfp(_IC_AFFINE | _IC_PROJECTIVE, _MCW_IC );

// The folloiwng are just X,Y offsets that we use as SoloStuff for pannels and positioning of controls. You can remove them if you don't need
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

// Some more constants
#define SCREW_X	9
#define SCREW_Y	9
#define NUM_PP_BITMAPS	8
#define NUM_SCREW_BITMAPS	10
#define MAIN_KNOB_IMAGES 49
#define MAIN_KNOB_BIG_IMAGES 99
#define SMALL_KNOB_GRAY_IMAGES	31
#define SMALL_KNOB_BLUE_IMAGES	31
#define LED_BLUE_IMAGES	10
#define LED_RED_IMAGES	10
#define DIGITS_RED_IMAGES	10
#define DIGITS_RED_ALPHA_IMAGES	26
#define DIGITS_PLUS_MINUS_IMAGES	3
//#define PWM_CC_SAMPLES		1
//#define PWM_C_SAMPLES		1
#define MIN_PW		0.01
#define RNG_PW		0.98
#define MAX_PW		(MIN_PW+RNG_PW)
#define HALF_PW		((MIN_PW+MAX_PW)/2)
#define CLOCK_WIDTH			0.3					// Percentage of the total clock cycle
#define CLIPTIME 0.2							// Clip led pulse time in seconds
#define TRIGTIME 0.1							// Trigger led pulse time in seconds
#define LMIDITIME	0.1							// MIDI led pulse time in seconds
#define MAX_CLOCK_WIDTH		0.1					// Max clock pulse width is seconds
#define LED_PULSE_TIME		0.1					// Reasonably visible pulse time (sec)
#define SMOOTH_DELAY		35					// (was 45) Time in milliseconds to allow a knob to reach it's value smoothly.
#define HIGH_CV				0.25				
#define LOW_CV				0.15
#define MID_CV				((LOW_CV+HIGH_CV)/2.0)
#define VERY_HIGH_CV		4.0
#define MAX_LEVEL			(clip_level)
#define DEFAULT_MAX_LEVEL	4.0					// 12db
//#define MAX_LEVEL2		22.0				// 26db
#define MIDI_TUNE_FACTOR	10.0				// The factor to achieve 0.1/oct tunning
#define MIDI_MAX_DELAY		0.002				// SMALL. For some Generators. In seconds. Dictates MIDI buffer sizes
#define MIDI_MAX_DELAY2		0.006				// MEDIUM. For mergers and such. In seconds. Dictates MIDI buffer sizes
//#define MAX_NAME_SIZE		50
//#define MAX_PW_PERIOD		2691				// longest period we allow for PWM storage array. Is at note c0(16.352Hz) = 44000/16.352 = 2690.8023 samples. //** increase to allow higher sampling rates.
//#undef USE_REV_SAW							// For S310VCO. Method of square generation.
#define LOG2				0.69314718055994529
#define LOG10				2.30258509299405
#define DENORMAL			0.0000000000001


// Supports automatic 14bit/7bit CC changes regardless of the type of controller used.
#define CCHANGE(midi,msb,last_msb,lsb,conmsb,conlsb,val,drag_lsb,no_hold_lsb)		\
	/* MSB */								\
	case (char)conmsb:						\
		msb=midi.data2;						\
		/* Drag LSB If hi res 14bit MIDI is enabled. Maily to handle erratic jumping or delayed LSB */		\
		if (drag_lsb)									\
		{	if (msb>last_msb) lsb=(char)0;			\
			else if (msb<last_msb) lsb=(char)127;	\
			last_msb = msb;					\
		}									\
		temp=MSBLSB(msb,lsb);				\
		val=temp/16383.0;					\
		break;								\
	/* LSB */								\
	case (char)conlsb:						\
		lsb=midi.data2;						\
		if (no_hold_lsb) { temp=MSBLSB(msb,lsb); val=temp/16383.0;	}				\
		break;	


// MIDI Frequencies Table
float mfreqtab[128];

int gcd(int a, int b) { 
   if (b == 0) 
      return a; 
   return gcd(b, a % b);  
}

// for Park-Miller random generator
unsigned int zgen;

//Product Module::vproduct("M",0,NULL,true);

int IntPow(int base, int exp)
{	int i, out;
	
	out=1;
	for (i=1; i<=exp; i++) out*=base;
	return out;
}

int IntLog(int v, int base)
{	int i, out;

	out=0;
	while (v/=base) out++;
	return out;
}

// Char stack definitions and implementation
template <typename TP>
void InitStack(cstack<TP> &stackv,int size)
{	stackv.top = -1;
	stackv.pbottom = (TP *) malloc(size*sizeof(*(stackv.pbottom)));
}

template <typename TP>
void FreeStack(cstack<TP> &stackv)
{	
	if (stackv.pbottom!=NULL) 
	{	free(stackv.pbottom); stackv.pbottom=NULL;
	}
}

#define PUSH(stackv,value) stackv.pbottom[++stackv.top]=value
#define POP(stackv) stackv.pbottom[stackv.top--]
#define FLUSHPOP(stackv) stackv.top--
#define NOTEMPTY(stackv) stackv.top!=-1
#define QISFULL(evin,evout,qsize)	(evin-evout==-1 || evin-evout==ev_qsize-1)


inline void ForceInside(long &x, long &y, CRect &r, CRect &rpar)
{	// rpar is rect of the container (parent)
	// Forces x,y of module to be inside container
	
	if (x<rpar.left) x=rpar.left;
	else if (x>rpar.right-r.width()) 
		x=rpar.right-r.width();

	if (y<rpar.top) y=rpar.top;
	else if (y>rpar.bottom-r.height()) 
		y=rpar.bottom-r.height();
}

CRect RectSkinFix(CRect r, float factor)
{	// Meant for old code. Scales a Rect to fit variable skin sizes
	//r.moveTo(FRound(factor*(float)r.x),FRound(factor*(float)r.y));
	r.moveTo(FRound(factor*((float)r.x+0.5*r.width())-0.5*r.width()),FRound(factor*((float)r.y+0.5*r.height())-0.5*r.height()));
	//r.setWidth(FRound(factor*(float)r.width()));
	//r.setHeight(FRound(factor*(float)r.height()));
	return r;
}

//---------------------------------------------
// Product Class

Product::Product(void *parameter, int opt, Product *pparent, bool isactive, char* prname) 
{	// Implement your own contructor here. The parameters above are just suggestions, you can remove them and do it your own way
	// parameter: a ppinter to something that is related/defines the module(s) that is being activated
	// pparent: a parent product for this product. If the parent is activated, then all children should become activated too.
	// isactive: indicates if the product should be active by default (like in freeware)
	// prname: Name of the product
	name=NULL;
	SetName(prname);
}

Product *Product::Activate(char *fullname, char *email, char *serial)
{	// This should try to activate the product using the given information.
	// If activation fails. It should try to activate the parent product (which will do the same recursivly)
	// If that fails, it should return NULL.
	// If activation is successful, then a pointer to the activated product should be returned.
	// A pointer to the product should NOT be returnred in ANY other situation, because an un-licensed caller could modify it.
	// Instead, if you want to make a copy of product. Implement something like CopyProduct()
	return NULL;
}

Product::~Product()
{	if (name!=NULL) free(name);
}

void Product::SetName(char* prname)
{	// Copy name
	if (prname!=NULL)
	{	if (prname!=name)
		{	name_len=strlen(prname);
			if (name!=NULL) free(name);
			name=(char *) malloc(sizeof(*name)*(name_len+1));
			strcpy(name,prname);
		}
	} 
	else 
	{	if (name!=NULL) free(name);
		name=NULL; name_len=0; 
	}
}


//PatchPointWrapper::PatchPointWrapper (const CRect& size, CControlListener* listener, long tag, CBitmap* background, const CPoint& offset)
//: CMovieBitmap(size,listener,tag,background)
//{
//}
//
//PatchPointWrapper::~PatchPointWrapper()
//{
//}
//---------------------------------------------
// Patch Point Class
PatchPoint::PatchPoint(const CRect& size, CControlListener* listener, long tag, CBitmap* background, int pptype)
: CMovieBitmap(size,listener,tag,background)
{	type=pptype; active_type = ppTypeUnused; protocol=ppAudioCV;
	cable_in=NULL; pcable=NULL; coff_x = 0; coff_y = 0; pnext=this;
	num_cables=0;
	peditor = (SynEditor *) listener;
}

PatchPoint::PatchPoint(const CRect& size, CControlListener* listener, long tag, CBitmap* background, int pptype, char x, char y)
: CMovieBitmap(size,listener,tag,background)
{	type=pptype; active_type = ppTypeUnused; protocol=ppAudioCV;
	cable_in=NULL; pcable=NULL; coff_x = x; coff_y = y; pnext=this;
	num_cables=0;
	peditor = (SynEditor *) listener;
}

inline void PatchPoint::SetCenterOffset(char x, char y)
{	coff_x = x; coff_y = y;
}

inline void PatchPoint::SetType(int pptype)
{	type=pptype;
}

inline void PatchPoint::SetProtocol(int ppprotocol)
{	protocol=ppprotocol;
}

//inline void PatchPoint::SetPPTag(long tag)
//{	// This functoion is used exclusivly by SoloRack. It should not be called by modules
//	pptag = tag;
//}
//
//inline long PatchPoint::GetPPTag()
//{	return pptag;
//}

//#ifndef IS_DLL_MODULES
CMouseEventResult PatchPoint::onMouseDown (CPoint &where, const long& buttons)
{	// All this mess is for Dll modules. This also assumes a module is the direct parent of the patchpoint
	return ((Module *)getParentView())->synth_comm.PPOnMouseDownPtr(this,where,buttons);
}

//CMouseEventResult PatchPoint2::onMouseDown (CPoint &where, const long& buttons)
//{	//return TTT1(where,buttons);
//	return ((Module *)getParentView())->synth_comm.PPOnMouseDownPtr(this,where,buttons);
//	//return kMouseEventHandled;
//}
//#else
//CMouseEventResult PatchPoint::onMouseDown (CPoint &where, const long& buttons)
//{	return kMouseEventHandled;
//}
//
//CMouseEventResult PatchPoint2::onMouseDown (CPoint &where, const long& buttons)
//{	return TTT1(where,buttons);
//	//return ((Module *)getParentView())->synth_comm.PPOnMouseDownPtr(this,where,buttons);
//}
//#endif
//
//CMouseEventResult PatchPoint::TTT1(CPoint &where, const long& buttons)
//{	return ((Module *)getParentView())->synth_comm.PPOnMouseDownPtr(this,where,buttons);
//}
//
//CMouseEventResult PatchPoint2::TTT1(CPoint &where, const long& buttons)
//{	return ((Module *)getParentView())->synth_comm.PPOnMouseDownPtr(this,where,buttons);
//}
//
//PatchPoint2::PatchPoint2(const CRect& size, CControlListener* listener, long tag, CBitmap* background, int pptype)
//: PatchPoint(size,listener,tag,background,pptype)
//{
//}

CMouseEventResult PatchPoint::onMouseUp (CPoint &where, const long& buttons)
{	// All this mess is for Dll modules. This also assumes a module is the direct parent of the patchpoint
	return ((Module *)getParentView())->synth_comm.PPOnMouseUpPtr(this,where,buttons);
}

CMouseEventResult PatchPoint::onMouseMoved (CPoint &where, const long &buttons)
{	
	return kMouseEventNotHandled;
}

CMessageResult PatchPoint::notify(CBaseObject* sender, const char* message)
{	// Currently, only "Cable Draged In" message is sent here, so we will not check and assume it is the same message.
	// If you want to notify about any other message, you have to check then proccess and NOT call PPnotify if the message was not "Cable Draged In"
	return ((Module *)getParentView())->synth_comm.PPNotify(this,sender,message);
}



//---------------------------------------------
// Module Knob Class - for smooth tweaking
ModuleKnob::ModuleKnob(const CRect& size, CControlListener* listener, long tag, CBitmap* background, Module *parent, const CPoint &offset)
: CAnimKnob(size,listener,tag,background,offset)
, is_stepping(false)
{	svalue=qvalue=value;
	SetSmoothDelay(SMOOTH_DELAY,parent);
	setZoomFactor(5.0);
}

ModuleKnob::ModuleKnob (const CRect& size, CControlListener* listener, long tag, long subPixmaps, CCoord heightOfOneImage, CBitmap* background, Module *parent, const CPoint &offset)
: CAnimKnob(size,listener,tag,subPixmaps,heightOfOneImage,background,offset)
, is_stepping(false)
{	svalue=qvalue=value; 
	SetSmoothDelay(SMOOTH_DELAY,parent);
	setZoomFactor(5.0);
}

ModuleKnob::ModuleKnob (const ModuleKnob& v)
: CAnimKnob(v)
, is_stepping(false)
{	svalue=qvalue=value;
	SetSmoothDelay(SMOOTH_DELAY,NULL);
}

void ModuleKnob::UpdateQValue()
{	// Update Quantized value
	// This has to be called manually like in ValueChanged. Because VSTGUI does not call setValue but changes values directly.
	// I could have done it automatically, but I opted out for performance issues. I will leave to the developer

	float temp = subPixmaps - 1;
	if (bInverseBitmap)
		qvalue = ((float)((long) ((1-value) * temp)))/temp;
	else
		qvalue = ((float)((long) (value * temp)))/temp;

}

void ModuleKnob::setValue(float val)
{	CAnimKnob::setValue(val);
	if (is_stepping) UpdateQValue();
}

CMouseEventResult ModuleKnob::onMouseDown (CPoint& where, const long& buttons)
{	return ((Module *)getParentView())->synth_comm.ModuleKnobOnMouseDownPtr(this,where,buttons);
}

CMouseEventResult ModuleKnob::onMouseMoved(CPoint& where, const long& buttons)
{	CMouseEventResult result = CAnimKnob::onMouseMoved(where,buttons);
	
	//if ((buttons & kLButton) && is_stepping)
	//	UpdateQValue();
	return result;
}


void ModuleKnob::SetSmoothDelay(int del, Module *parent)
{	// getParentView() does not seam to work when the parent is still in it's constructor.
	// So you have to pass it as parent in this situation.
	
	if (del>=0) delay=del;

	if (parent==NULL) parent=(Module *) getParentView();
	//if (parent!=NULL)								//**
		smooth_samples=parent->sample_rate/1000.0*delay;
		//if (smooth_samples>30000) smooth_samples=30000;
	//else
	//	smooth_samples=Module/1000.0*delay;
}

//---------------------------------------------
// Extended Module Knob Class - Has a extra feature. Currently: a pointer to Attached control, and tag.
ModuleKnobEx::ModuleKnobEx(const CRect& size, CControlListener* listener, long tag, CBitmap* background, Module *parent, const CPoint &offset)
	: ModuleKnob(size, listener, tag, background, parent, offset)
	, invalidate(true)
	, auto_zoom(false)
{
}

ModuleKnobEx::ModuleKnobEx(const CRect& size, CControlListener* listener, long tag, long subPixmaps, CCoord heightOfOneImage, CBitmap* background, Module *parent, const CPoint &offset)
	: ModuleKnob(size, listener, tag, subPixmaps, heightOfOneImage, background, parent, offset)
	, invalidate(true)
	, auto_zoom(false)
{
}

ModuleKnobEx::ModuleKnobEx(const ModuleKnobEx& v)
	: ModuleKnob(v)
{
	attached1 = v.attached1;
	invalidate = v.invalidate;
	auto_zoom = v.auto_zoom;
}

CMouseEventResult ModuleKnobEx::onMouseMoved(CPoint& where, const long& buttons)
{	// Same as in CKnob except that it can block invalidation
	// And can use auto_zoom which simulates pressing shift all the time while mooving the knob
	
	//long buttons;
	//if (auto_zoom) buttons = vbuttons | kShift;
	//else buttons = vbuttons;

	if (buttons & kLButton)
	{
		float middle = (vmax - vmin) * 0.5f;

		if (where != lastPoint)
		{
			lastPoint = where;
			if (modeLinear)
			{
				CCoord diff = (firstPoint.v - where.v) + (where.h - firstPoint.h);
				if (buttons != oldButton)
				{
					range = 400.f;			// Ammar: was 200
					if (auto_zoom)
					{	range *= zoomFactor;
						if (buttons & kShift) range *= 4.0;
					}
					else if (buttons & kShift) range *= zoomFactor;

					float coef2 = (vmax - vmin) / range;
					fEntryState += diff * (coef - coef2);
					coef = coef2;
					value = fEntryState + diff * coef;
					oldButton = buttons;
				}
				else value = fEntryState + diff * coef;
				bounceValue();
			}
			else
			{
				where.offset(-size.left, -size.top);
				value = valueFromPoint(where);
				if (startValue - value > middle)
					value = vmax;
				else if (value - startValue > middle)
					value = vmin;
				else
					startValue = value;
			}
			if (value != oldValue && listener)
				listener->valueChanged(this);
			if (invalidate && isDirty())
				invalid();
		}
	}
	return kMouseEventHandled;
}

CMouseEventResult ModuleKnobEx::onMouseDown(CPoint& where, const long& buttons)
{
	beginEdit();
	if (checkDefaultValue(buttons))
	{
		endEdit();
		return kMouseDownEventHandledButDontNeedMovedOrUpEvents;
	}

	firstPoint = where;
	lastPoint(-1, -1);
	startValue = oldValue;

	modeLinear = false;
	fEntryState = value;
	range = 400.f;					// Ammar: was 200
	coef = (vmax - vmin) / range;
	oldButton = buttons;

	long mode = kCircularMode;
	//long newMode = getFrame ()->getKnobMode ();	//Ammar to force kLinearMode
	long newMode = kLinearMode;						//Ammar to force kLinearMode
	if (kLinearMode == newMode)
	{
		if (!(buttons & kAlt))
			mode = newMode;
	}
	else if (buttons & kAlt)
		mode = kLinearMode;

	if (mode == kLinearMode && (buttons & kLButton))
	{
		if (auto_zoom)
		{	range *= zoomFactor;
			if (buttons & kShift) range *= 4.0;
		}
		else if (buttons & kShift) range *= zoomFactor;

		lastPoint = where;
		modeLinear = true;
		coef = (vmax - vmin) / range;
	}
	else
	{
		CPoint where2(where);
		where2.offset(-size.left, -size.top);
		startValue = valueFromPoint(where2);
	}

	return onMouseMoved(where, buttons);
}

//void ModuleKnobEx::setZoomFactor(float val)
//{
//	CPoint where;
//	long buttons;
//
//	zoomFactor = val; 
//	//where = lastPoint; lastPoint = CPoint(0, 0); buttons = oldButton; 
//	if (auto_zoom) oldButton = 0;
//	//onMouseMoved(where, buttons);
//}

// ---------------------------------------------------------------------
// Extended CSpecialDigit Class - Has a extra feature. Currently: a pointer to Attached control
CSpecialDigitEx::CSpecialDigitEx(const CRect& size, CControlListener* listener, long tag, long dwPos, long iNumbers, long* xpos, long* ypos, long width, long height, CBitmap* background, CBitmap* pblank)
: CSpecialDigit(size,listener,tag,dwPos,iNumbers,xpos,ypos,width,height,background)
{	attached1=NULL; tag1=-1;
	blank=pblank;
}

CSpecialDigitEx::CSpecialDigitEx(const CSpecialDigitEx& digit)
: CSpecialDigit(digit)
{	attached1 = digit.attached1;
	tag1=digit.tag1;
	//invalidate = digit.invalidate;
}

void CSpecialDigitEx::draw (CDrawContext *pContext)
{
	CPoint  where;
	CRect   rectDest;
	long    i, j;
	long    dwValue;
	long     one_digit[16];
	bool	leading_zero=true;
  
	if ((long)value >= getMax ()) 
		dwValue = (long)getMax ();
	else if ((long)value < getMin ()) 
		dwValue = (long)getMin ();
	else
		dwValue = (long)value;
	
	for (i = 0, j = ((long)getMax () + 1) / 10; i < iNumbers; i++, j /= 10)
	{
		one_digit[i] = dwValue / j;
		dwValue -= (one_digit[i] * j);
	}
	
	where.h = 0;
	for (i = 0; i < iNumbers; i++)
	{	
		j = one_digit[i];
		if (j > 9)
			j = 9;
		leading_zero = leading_zero && (j==0) && i<iNumbers-1;
		
		rectDest.left   = (CCoord)xpos[i];
		rectDest.top    = (CCoord)ypos[i];
		
		rectDest.right  = rectDest.left + width;
		rectDest.bottom = rectDest.top  + height;		
		
		// where = src from bitmap
		if (blank!=NULL && leading_zero)
		{	where.v = (CCoord)0;
			if (bTransparencyEnabled)
					blank->drawTransparent (pContext, rectDest, where);
				else
					blank->draw (pContext, rectDest, where);
		} else
		{	where.v = (CCoord)j * height;
			if (pBackground)
			{
				if (bTransparencyEnabled)
					pBackground->drawTransparent (pContext, rectDest, where);
				else
					pBackground->draw (pContext, rectDest, where);
			}
		}
	}
		
	setDirty (false);
}

//---------------------------------------------
// Extended CKickButtonEx Class - Has a extra features. Currently: a pointer to Attached control and new onMouseDown and onMouseUp to allow for valuechanged() to be called imidiatly
CKickButtonEx::CKickButtonEx(const CRect& size, CControlListener* listener, long tag, CBitmap* background, const CPoint& offset)
: CKickButton(size,listener,tag,background,offset)
{	attached1=NULL; tag1=-1;
	immediate_valuechanged=true;
}

CKickButtonEx::CKickButtonEx(const CRect& size, CControlListener* listener, long tag, CCoord heightOfOneImage, CBitmap* background, const CPoint& offset)
: CKickButton(size,listener,tag,heightOfOneImage,background,offset)
{	attached1=NULL; tag1=-1;
	immediate_valuechanged=true;
}

CKickButtonEx::CKickButtonEx(const CKickButtonEx& kickButton)
: CKickButton(kickButton)
{	attached1 = kickButton.attached1;
	tag1 = kickButton.tag1;
	immediate_valuechanged = kickButton.immediate_valuechanged;
}

CMouseEventResult CKickButtonEx::onMouseDown (CPoint& where, const long& buttons)
{
	if (!(buttons & kLButton))
		return kMouseEventNotHandled;
	fEntryState = value;
	beginEdit ();

	// Change value imidiatly instead of waiting for mousemove (Changed by Ammar)
	if (where.h >= size.left && where.v >= size.top  &&
			where.h <= size.right && where.v <= size.bottom)
			value = !fEntryState;
		else
			value = fEntryState;

	// This was in onMouseUp. This is the main defrence between CKickButton and CKickButtonEx. (Changed by Ammar)
	if (value && listener)
		listener->valueChanged (this);

	return onMouseMoved (where, buttons);
}

CMouseEventResult CKickButtonEx::onMouseUp (CPoint& where, const long& buttons)
{
	//if (value && listener)
	//	listener->valueChanged (this);
	value = 0.0f;  // set button to UNSELECTED state
	if (listener)
		listener->valueChanged (this);
	if (isDirty ())
		invalid ();
	endEdit ();
	return kMouseEventHandled;
}


//----------------------------------------------------------------------
// Extended CMovieBitmap. Mainly to block setDirty (false); when value is volatile.
CMovieBitmapEx::CMovieBitmapEx (const CRect& size, CControlListener* listener, long tag, CBitmap* background, const CPoint &offset)
: CMovieBitmap (size,listener,tag,background,offset)
{	
	#ifdef USE_NEW_OLD_VERTICAL
	old_vertical=-1000;
	#endif
	subPixmaps_1 = subPixmaps-1; update_count=update_ticks=5;
}

CMovieBitmapEx::CMovieBitmapEx (const CRect& size, CControlListener* listener, long tag, long subPixmaps, CCoord heightOfOneImage, CBitmap* background, const CPoint &offset)
: CMovieBitmap (size,listener,tag,subPixmaps,heightOfOneImage,background,offset)
, dirty_false_after_draw (true)
{	
	#ifdef USE_NEW_OLD_VERTICAL
	old_vertical=-1000;
	#endif
	subPixmaps_1 = subPixmaps-1; update_count=update_ticks=5;
}

CMovieBitmapEx::CMovieBitmapEx (const CMovieBitmapEx& v)
: CMovieBitmap (v)
{
	dirty_false_after_draw = v.dirty_false_after_draw;
	#ifdef USE_NEW_OLD_VERTICAL
	old_vertical=-1000; 
	#endif
	subPixmaps_1 = subPixmaps-1; update_count=update_ticks=5;
}

void CMovieBitmapEx::draw (CDrawContext *pContext)
{	// better for values that change slowly and smoothly in a none stepy fashion
	
	float temp;

	if (value > 1.0f)
		value = 1.0f;

	oldValue=value;

	if (pBackground)
	{	CPoint where (offset.h, offset.v);

 		if (oldValue > 0.0f)
		{	
			#ifdef USE_NEW_OLD_VERTICAL
				old_vertical = (int)(oldValue * subPixmaps_1 + 0.5);
				where.v += heightOfOneImage * old_vertical;
			#else
				where.v += heightOfOneImage * (int)(oldValue * subPixmaps_1 + 0.5);
			#endif
			
		}

		//if (bTransparencyEnabled)
		//	pBackground->drawTransparent (pContext, size, where);  // No need for that since both call alphablend
		//else
			pBackground->draw (pContext, size, where);
	}
	// If value changes rapidly in audio thread, then not use of setDirty (false)
	//if (dirty_false_after_draw) setDirty (false);
}

//void CMovieBitmapEx::setDirty (const bool val)
//{
//	CView::setDirty (val);
//	if (val)
//	{
//		if (value != -1.f)
//			oldValue = -1.f;
//		else
//			oldValue = 0.f;
//	}
//	else
//		oldValue = value;
//}
//


bool CMovieBitmapEx::isDirty () const
{	// Has tricks to lower CPU usage by limiting updating of LEDs and thus preventing updating of GDI+ CPU heavy cables
	return ((Module*)getParentView())->synth_comm.CMovieBitmapExIsDirtyPtr(this);
}



////-----------------------------------------------------------------------------
//// CFileSelectorEx Declaration
//// Extension for the CFileSelector. adds default filter index
////-----------------------------------------------------------------------------
//
//CFileSelectorEx::CFileSelectorEx(void* ptr)
//: CFileSelector(ptr)
//{
//}


//---------------------------------------------
// Base Module Class
CBitmap **Module::mcbits = NULL;			// Bitmap(s) of the main patchpoints
CBitmap **Module::ppbit = NULL;				// Bitmap(s) of the main patchpoints
char *Module::skindir = NULL;
char *Module::defskindir = NULL;
char *Module::dlldatadir = NULL;
char *Module::dllskindir = NULL;
float Module::uiscale = 1.0;
long Module::vp = BASE_MHEIGHT;
long Module::vp_3 = BASE_MHEIGHT/3;
long Module::vp_5 = BASE_MHEIGHT/5;
long Module::hp = BASE_HP;

int Module::GetSDKVersion() 
{	return SDK_VERSION; 
}

Module::Module(const CRect &size, CFrame *pParent, CBitmap *pBackground, const SynthComm *psynth_comm, const int vvoice)
: ModuleWrapper(size,pParent,pBackground) 
{	synth_comm=*psynth_comm;
	in_move = false; evindex=-1; sbindex=-1; procindex=-1; voice=vvoice; is_mono=false;
	nbcontrols=-1; nb_pp=-1; allow_oversample=true;
	nb_cables=0; always_on=GetAlwaysONDefault();

	peditor = (SynEditor *) pParent->getEditor();
	//psynth = peditor->psynth;
	psynth = psynth_comm->GetSynth(peditor);
	DAW_block_size = psynth->getBlockSize();
	DAW_sample_rate = psynth->getSampleRate();

	#ifdef MODULE_OVERSAMPLE
	// Oversampling settings for per mdoule oversampling. Not complete yet.
	ovr.overs=1; ovr.sovers=1; ovr.overs_index=0; ovr.overs_filter=peditor->moversfilter_menu_index[kIIRChamberlin];
	ovr.ovin=NULL; ovr.cof=NULL; ovr.cofs=0; ovr.iovin=0;
	ovr.bp=0; ovr.bp_2=0; ovr.lp=0; ovr.lp_2=0;
	sample_rate = DAW_sample_rate*psynth->sovers*ovr.overs;
	#else
	//sample_rate = DAW_sample_rate*psynth->sovers;
	sample_rate = DAW_sample_rate*psynth_comm->GetOversamplingFactor(psynth);
	#endif
	
	//sample_rate = DAW_sample_rate*psynth->sovers*ovr.overs;
	hsample_rate = sample_rate/2.0;
	sample_rate60 = 60.0*sample_rate;

	// Default to NO Bandlimiting
	Module::SetBandLimit(kNoBandlimiting);
	demolabel=NULL; infourl=NULL;
	clip_level = DEFAULT_MAX_LEVEL;
}

//Module *Module::VConstructor(CFrame *pParent, CControlListener *listener,const SynthComm *psynth_comm, const int vvoice)
//{	return NULL;
//}

Module::~Module()
{	
	#ifdef MODULE_OVERSAMPLE
	free(ovr.ovin); free(ovr.cof);
	#endif
	// removeAll() in CViewContiner will remove and delete all added views from memory
	free(infourl);
}

// Will be called only once by Solorack. Don't call this in a module Initialize()
void Module::Initialize()
{	int i;
	char temp[40];
	char temp2[2];

	SetSeed(time(NULL));

	// Create cbits array
	mcbits = (CBitmap **) malloc(kModuleCBitsCount*sizeof(**mcbits));

	// Main Knob bitmaps
	mcbits[knobit] = new CBitmap(dllskindir,NULL,"main_knob_s.png");

	// Switch bitmaps
	mcbits[vert_swbit] = new CBitmap(skindir,defskindir,"vert_switch_toggle.png");
	mcbits[tr_vert_swbit] = new CBitmap(skindir,defskindir,"vert_triple_switch_toggle.png");

	// Prepare screws bitmap
	mcbits[scrbit] = new CBitmap(skindir,defskindir,"screw.png");

	// MIDI patch point
	mcbits[MIDIppbit] = new CBitmap(skindir,defskindir,"MIDIpp.png");
	//MIDIplugbit = new CBitmap(skindir,defskindir,"MIDIplug.png");
	//MIDIplugconbit = new CBitmap(skindir,defskindir,"MIDIplugcon.png");

	// Prepare patchpoints bitmaps
	ppbit = (CBitmap **) malloc(sizeof(*ppbit)*NUM_PP_BITMAPS);
	for (i=1; i<=NUM_PP_BITMAPS; i++)
	{	strcpy(temp,"patchpoint");
		_itoa(i,temp2,10); strcat(temp,temp2);
		strcat(temp,".png");
		ppbit[i-1] = new CBitmap(skindir,defskindir,temp);
	}

	// leds bitmaps
	//crap = new CBitmap(skindir,defskindir,"sknobit_gray_13s.png");
	mcbits[led_blue] = new CBitmap(skindir,defskindir,"led_blue.png");
	mcbits[led_red] = new CBitmap(skindir,defskindir,"led_red.png");

	
	// small knobs
	mcbits[sknobit_black5] = new CBitmap(dllskindir,NULL,"sknobit_black_5s.png");


	// make frequency (Hz) table
	double k = 1.059463094359;	// 12th root of 2
	double a = 6.875;	// a
	a *= k;	// b
	a *= k;	// bb
	a *= k;	// c, frequency of midi note 0. (note: It's C-1, C at octave -1)

	for (i = 0; i < 128; i++)	// 128 midi notes
	{
		mfreqtab[i] = (float)a;
		a *= k;
	}

	// Get OS version, Required by VST gui. This would have been done if CFrame() was called
	memset(&gSystemVersion, 0, sizeof (gSystemVersion));
	gSystemVersion.dwOSVersionInfoSize = sizeof (gSystemVersion);
	GetVersionEx((OSVERSIONINFO *)&gSystemVersion);

}

void Module::End()
{	// Cleaning
	int i;

	for (i=0; i<NUM_PP_BITMAPS; i++)
		ppbit[i]->forget();
	free(ppbit);

	// Free common bitmaps
	for (i=0; i<kModuleCBitsCount; i++)
		mcbits[i]->forget();

	//mcbits[knobit]->forget(); mcbits[knobit_big]->forget();;

	//// Small knobs
	//mcbits[sknobit_gray13]->forget(); mcbits[sknobit_gray13_fs]->forget();
	//mcbits[sknobit_gray]->forget(); mcbits[sknobit_gray5]->forget();
	//mcbits[sknobit_gray7_2]->forget(); mcbits[sknobit_gray11]->forget();
	//mcbits[sknobit_black]->forget(); mcbits[sknobit_red]->forget(); 

	//// MIDI patch point
	//mcbits[MIDIppbit]->forget(); //delete MIDIplugbit;
	////delete MIDIplugconbit;

	//// Prepare screws bitmap
	//mcbits[scrbit]->forget();

	//// Switch bitmap
	//mcbits[vert_swbit]->forget(); mcbits[tr_vert_swbit]->forget();

	//// leds bitmaps
	//mcbits[led_blue]->forget(); mcbits[led_red]->forget();

	//// Buttons bitmaps
	//mcbits[red_buttonbit]->forget();
	//mcbits[black_buttonbit]->forget();
	//mcbits[white_buttonbit]->forget();

	//// Digits
	//mcbits[up_buttonbit]->forget(); mcbits[down_buttonbit]->forget(); 
	//mcbits[digits_mid_red]->forget();

	free(mcbits);

}

CMouseEventResult Module::onMouseDown (CPoint &where, const long& buttons)
{	// all this mess is for Dll modules
	return synth_comm.ModuleOnMouseDownPtr(this,where,buttons);
}

CMouseEventResult Module::onMouseUp (CPoint &where, const long& buttons)
{	// all this mess is for Dll modules
	return synth_comm.ModuleOnMouseUpPtr(this,where,buttons);
}

CMouseEventResult Module::onMouseMoved (CPoint &where, const long& buttons)
{	// all this mess is for Dll modules
	return synth_comm.ModuleOnMouseMovedPtr(this,where,buttons);
}


void Module::PutLeftScrews(CMovieBitmap *&top_screw,CMovieBitmap *&bottom_screw, CControlListener *listener)
{	float j;
	CRect r;
	// Top left screw
	j=((float) GenRand(0,NUM_SCREW_BITMAPS-1))/(NUM_SCREW_BITMAPS-1.0);
	r.moveTo(this->getViewSize().getTopLeft().x+SkinScale(SCREW_X)-mcbits[scrbit]->getWidth()/2,this->getViewSize().getTopLeft().y+SkinScale(SCREW_Y)-mcbits[scrbit]->getHeight()/NUM_SCREW_BITMAPS/2);
	r.setSize(CPoint(mcbits[scrbit]->getWidth(),mcbits[scrbit]->getHeight()/NUM_SCREW_BITMAPS));
	top_screw = new CMovieBitmap(r,listener,NO_TAG,mcbits[scrbit]); addView(top_screw);
	top_screw->setValue(j);
	// Bottom left screw
	j=((float) GenRand(0,NUM_SCREW_BITMAPS-1))/(NUM_SCREW_BITMAPS-1.0);
	r.moveTo(this->getViewSize().getBottomLeft().x+SkinScale(SCREW_X)-mcbits[scrbit]->getWidth()/2,this->getViewSize().getBottomLeft().y-SkinScale(SCREW_Y)-mcbits[scrbit]->getHeight()/NUM_SCREW_BITMAPS/2);
	r.setSize(CPoint(mcbits[scrbit]->getWidth(),mcbits[scrbit]->getHeight()/NUM_SCREW_BITMAPS));
	bottom_screw = new CMovieBitmap(r,listener,NO_TAG,mcbits[scrbit]); addView(bottom_screw);
	bottom_screw->setValue(j);
}

void Module::PutRightScrews(CMovieBitmap *&top_screw,CMovieBitmap *&bottom_screw, CControlListener *listener)
{	float j;
	CRect r;
	// Top right Screw
	j=((float) GenRand(0,NUM_SCREW_BITMAPS-1))/(NUM_SCREW_BITMAPS-1.0);
	r.moveTo(this->getViewSize().getTopRight().x-SkinScale(SCREW_X)-mcbits[scrbit]->getWidth()/2,this->getViewSize().getTopRight().y+SkinScale(SCREW_Y)-mcbits[scrbit]->getHeight()/NUM_SCREW_BITMAPS/2);
	r.setSize(CPoint(mcbits[scrbit]->getWidth(),mcbits[scrbit]->getHeight()/NUM_SCREW_BITMAPS));
	top_screw = new CMovieBitmap(r,listener,NO_TAG,mcbits[scrbit]); addView(top_screw);
	top_screw->setValue(j);
	// Bottom right Screw
	j=((float) GenRand(0,NUM_SCREW_BITMAPS-1))/(NUM_SCREW_BITMAPS-1.0);
	r.moveTo(this->getViewSize().getBottomRight().x-SkinScale(SCREW_X)-mcbits[scrbit]->getWidth()/2,this->getViewSize().getBottomRight().y-SkinScale(SCREW_Y)-mcbits[scrbit]->getHeight()/NUM_SCREW_BITMAPS/2);
	r.setSize(CPoint(mcbits[scrbit]->getWidth(),mcbits[scrbit]->getHeight()/NUM_SCREW_BITMAPS));
	bottom_screw = new CMovieBitmap(r,listener,NO_TAG,mcbits[scrbit]); addView(bottom_screw);
	bottom_screw->setValue(j);
}

long Module::GetFreeTag()
{	// Returns an unused tag for DAW automation. Should be called by the module, typicaly in the construtor.
	return synth_comm.ModuleGetFreeTagPtr(this);
}

bool Module::RegisterTag(CControl *pcon, long tag)
{	// Associates the given tag with the given control for DAW automation. Should be called by the module, typicaly in the construtor.
	return synth_comm.ModuleRegisterTagPtr(this,pcon,tag);
}

bool Module::UnRegisterTag(CControl *pcon)
{	// Unassociates the given tag with the given control for DAW automation. Should be called by the module
	return synth_comm.ModuleUnRegisterTagPtr(this,pcon);
}

bool Module::CanProcessEvents()
{	// Tells SoloRack to call ProcessEvents for this module, ProcessEvents() is called when ever a DAW event is sent to SoloRack. Should be called by the module, typicaly in the construtor.
	return synth_comm.ModuleCanProcessEventsPtr(this);
}

bool Module::CallStartOfBlock()
{	// Tells SoloRack to call StartOfBlock for this module every time processReplacing() is called in SoloRack. 
	return synth_comm.ModuleCallStartOfBlockPtr(this);
}


bool Module::UnRegisterOldTag(CControl *pcon,long oldtag, long newtag)
{	// This function safely unregisters an old tag if a new tag for the same control has been registered (before calling this function).
	// This is mainly used when loading presets.
	
	return synth_comm.ModuleUnRegisterOldTagPtr(this,pcon,oldtag,newtag);
}


//void Module::ForceUnRegisterTag(CControl *pcon, long tag)
//{	// Unregister that tag even if the tag was not registered properly.
//	// This will make sure both the controls[] array and the .tag are unregistered no matter what the developer did.
//	if (tag<kNumParams)
//	{	peditor->controls[tag]->setTag(NO_TAG);
//		peditor->controls[tag]=NULL;
//	}
//
//	if (pcon->getTag()<kNumParams)
//	{	peditor->controls[pcon->getTag()]=NULL;
//		pcon->setTag(NO_TAG);
//	} 
//}


void Module::CanBandLimit(int default_limit)
{	// Possible values for default_limit:
	// kNoBandlimiting, kAtOversamplingNyquist, kNearDAWNyquist
}

void Module::ProcessEvents(const VstEvents* ev)
{
}

inline void Module::StartOfBlock(int sample_frames)				// Called on the start each block, for all modules than CanProcessEvents
{
}


void Module::InitPatchPoints(float init)
{	long i,n;
	CView *temp;
	n=getNbViews();
	for (i=0; i<n; i++)
	{	temp=getView(i);				//** Use pFirstView and pNext instead. Should be faster.
		if (temp->isTypeOf("PatchPoint"))
		{	((PatchPoint *) temp)->in = init;
			((PatchPoint *) temp)->out = init;
		}
	}
}

inline void Module::ProcessSample()
{
}

void Module::CableConnected(PatchPoint *pp)
{
}

void Module::CableDisconnected(PatchPoint *pp)
{
}

void Module::ValueChanged(CControl* pControl)
{
}

void Module::CountControlsAndPatchPoints()
{	// Calculates the number of controls and patch points and stores them num_controls and num_pp
	nbcontrols=0; nb_pp=0;
	CCView *ptemp=pFirstView;

	while (ptemp)
	{	if (ptemp->pView->isTypeOf("PatchPoint"))
		{	nbcontrols++; nb_pp++;
		}
		else if (ptemp->pView->isTypeOf("CControl"))
				if (((CControl *)(ptemp->pView))->getTag()!=NOSAVE_TAG) nbcontrols++;
		ptemp = ptemp->pNext;
	}
}


int Module::GetControlsTagsSize()
{	// This fuction assumes that all controls tags of type long

	// If we already calculated it before
	if (nbcontrols>=0) return nbcontrols*sizeof(long);				// must be type of CControl::tag

	// Otherwise
	CountControlsAndPatchPoints();
	return nbcontrols*sizeof(long);									// must be type of CControl::tag
}

void Module::SaveControlsTags(void *pdata)
{	
	long *pfdata = (long *) pdata;							// This type must be the same type as tag/getTag()
	CCView *ptemp=pFirstView;

	while (ptemp)
	{	if (ptemp->pView->isTypeOf("CControl"))
		{	if (((CControl *)(ptemp->pView))->getTag()!=NOSAVE_TAG)
			{	*pfdata = ((CControl *)(ptemp->pView))->getTag(); pfdata++;
			}
		}
		ptemp = ptemp->pNext;
	}
}
void Module::LoadControlsTags(void *pdata, int size)
{	//** You could use size to check, just in case the saved preset uses less data
	long *pfdata = (long *) pdata, ttag;							// This type must be the same type as tag/getTag()
	CCView *ptemp=pFirstView;
	CControl *ptemp2;

	while (ptemp && size>=sizeof(*pfdata))		//** && pfdata<pdata+(size/sizeof(*pfdata))
	{	if (ptemp->pView->isTypeOf("PatchPoint"))	
		{	((PatchPoint *)(ptemp->pView))->setTag(*pfdata);
			size-=sizeof(*pfdata); pfdata++;
		}
		else if (ptemp->pView->isTypeOf("CControl"))	
		{	if (((CControl *)(ptemp->pView))->getTag()!=NOSAVE_TAG)
			{	if ((*pfdata)<kNumParams)
				{
					// I'm not doing force untagging any more because a hacker might exploit this to unregister good tags
					//ptemp2 = (CControl *)(ptemp->pView);
					//if (ptemp2->getTag()<kNumParams) peditor->controls[ptemp2->getTag()]=NULL;
					//ptemp2->setTag(NO_TAG);					// No need because were going to put a new tag down

					//// Make sure the target tag is also unregsitered from it's control, if there is one.
					//ptemp2 = peditor->controls[*pfdata];
					//if (ptemp2!=NULL)
					//{	// Again manual unregistration
					//	// UnRegisterTag(peditor->controls[*pfdata]);
					//	ptemp2->setTag(NO_TAG); 
					//	peditor->controls[*pfdata]=NULL;
					//}

					// Try to Register the preset requested tag. If succsefull, unregisted the tag that might alread have been
					// Registered in module constructor. if not keep the contructor tag registered.
					ttag=((CControl *)(ptemp->pView))->getTag();
					if (RegisterTag(((CControl *)(ptemp->pView)),*pfdata))
					{	// Unregister the constructor tag manually if it's different from the preset tag
						UnRegisterOldTag((CControl *)(ptemp->pView),ttag,*pfdata);
						//if (ttag!=*pfdata && ttag<kNumParams && peditor->controls[ttag]==ptemp->pView)		// Just in case the developer forgot to registertag his control or did something different with the tag
						//	peditor->controls[ttag]=NULL;
					}
				}
				//else ((CControl *)(ptemp->pView))->setTag(*pfdata);		// Tag is discarded because it can create conflicts
				size-=sizeof(*pfdata); pfdata++;
			}
		}
		ptemp = ptemp->pNext;
	}
}


void Module::ResetNbControlsAndPatchPoints()
{	nbcontrols=-1; nb_pp=-1;
}

void Module::SetPPTags(long &ctag)
{	// The primary porpus of this it to set the pptag. which shouldn't be changed by developers. These are exlusibly used by SoloRack to save cable connections
	// A side effect is that it will set the tag (VSTGUI) too to the same value. This one will be saved, although it is currently not used. Might be usefull in future
	// if something goes wrong.
	CCView *ptemp=pFirstView;

	while (ptemp)
	{	if (ptemp->pView->isTypeOf("PatchPoint"))
		{	((PatchPoint *)(ptemp->pView))->pptag=ctag; 
			((PatchPoint *)(ptemp->pView))->setTag(ctag);
			ctag++;
		}
		ptemp = ptemp->pNext;
	}
}

void Module::ConstructTags2PP(PatchPoint **tags2pp, long &ctag, int nb_pp)
{	// nb_pp here is the number of patch points in the saved module, not necceesarly the working module.
	// This is because the working one might be in a newer version.
	CCView *ptemp=pFirstView;
	int i=1;

	while (ptemp && i<=nb_pp)
	{	if (ptemp->pView->isTypeOf("PatchPoint"))
		{	tags2pp[ctag] = (PatchPoint *) ptemp->pView;
			ctag++; i++;
		}
		ptemp = ptemp->pNext;
	}
}


int Module::GetControlsValuesSize()
{	// This fuction assumes that all controls tags of type float

	// If we already calculated it before
	if (nbcontrols>=0) return nbcontrols*sizeof(float);					// must be type of CControl::value

	// Otherwise
	CountControlsAndPatchPoints();
	return nbcontrols*sizeof(float);									// must be type of CControl::value
}

void Module::SaveControlsValues(void *pdata)
{	float *pfdata = (float *) pdata;							// This type must be the same type as value/getValue()
	CCView *ptemp=pFirstView;

	while (ptemp)
	{	if (ptemp->pView->isTypeOf("CControl"))
		{	if (((CControl *)(ptemp->pView))->getTag()!=NOSAVE_TAG)
			{	*pfdata = ((CControl *)(ptemp->pView))->getValue(); pfdata++;
			}
		}
		ptemp = ptemp->pNext;
	}
}

void Module::LoadControlsValues(void *pdata, int size)
{	//** Size is used to check, just in case the saved preset uses less data
	float *pfdata = (float *) pdata;							// This type must be the same type as value/getValue()
	CCView *ptemp=pFirstView;

	while (ptemp && size>=sizeof(*pfdata))				//** && pfdata<pdata+(size/sizeof(*pfdata))
	{	if (ptemp->pView->isTypeOf("CControl"))	
		{	if (((CControl *)(ptemp->pView))->getTag()!=NOSAVE_TAG)
			{	//if (((CControl *)(ptemp->pView))->getBackground()!=mcbits[scrbit])
					((CControl *)(ptemp->pView))->setValue(*pfdata);
				// Set svalue, this some times but not always important, like for the case if you want to preserve the saved phase
				if (ptemp->pView->isTypeOf("ModuleKnob"))
					if (((ModuleKnob *)(ptemp->pView))->is_stepping)
						((ModuleKnob *)(ptemp->pView))->svalue=((ModuleKnob *)(ptemp->pView))->qvalue;				// qvalue is already set by setValue above
					else
						((ModuleKnob *)(ptemp->pView))->svalue=((ModuleKnob *)(ptemp->pView))->getValue();
				else
					// Set MIDI patch point value to zero, this will remove plug image. Untill it is connected latter by the preset
					// This is better if preset loading was interupted because in that case it will show a plug without a cable!!
					if (ptemp->pView->isTypeOf("PatchPoint"))
					{	PatchPoint *pp = (PatchPoint *)(ptemp->pView);
						if (pp->protocol==ppMIDI && pp->num_cables==0)
							pp->setValue(0); //pp->invalid();
					}
					
				size-=sizeof(*pfdata); pfdata++;
					
				ValueChanged((CControl *)(ptemp->pView));
			}
		}
		ptemp = ptemp->pNext;
	}
}

int Module::GetPresetSize()
{	return GetControlsValuesSize();
}

void Module::SavePreset(void *pdata, int size)
{	SaveControlsValues(pdata);
}

void Module::LoadPreset(void *pdata, int size, int version)
{	LoadControlsValues(pdata,size);
}

const char * Module::GetName()
{	return "";
}

const int Module::GetNameLen()
{	return -1;
}

const char * Module::GetVendorName()
{	return "";
}

const int Module::GetVendorNameLen()
{	return -1;
}

const int Module::GetType()
{	return -1;
}

void Module::SetKnobsSmoothDelay(int del)
{	//del=-1 will use the same delay already assciated with each knob. This function will be called by solorack when sample rate changes too because variable smooth_samples has to be recalculated
	CCView *ptemp=pFirstView;

	while (ptemp)				//** && pfdata<pdata+(size/sizeof(*pfdata))
	{	if (ptemp->pView->isTypeOf("ModuleKnob"))	
			((ModuleKnob *)(ptemp->pView))->SetSmoothDelay(del,this);
		ptemp = ptemp->pNext;
	}
}

void Module::SetSampleRate(float sr)
{	// Will not be auto called when a module is first created. sample_rate should be set by contructors. Sure you can call this function from inside your contructor
	// Will only be called by SoloRack when sample rate changes.
	// Your version of SetSampleRate() is responsible for calling SetBandLimit() OR accounting for bandlimit value within it (if you require it by your module). Solorack will not autocall SetBandLimit() for you
	// I do this because SetBandLimit() may include memory allocation for things like Minblep, etc. Which can trigger multiple/redundant allocations if both functions are called when a preset is loaded.
	
	sample_rate = sr;
	hsample_rate = sr/2.0;
	sample_rate60 = 60.0*sr;
	
	// Recalculate smoothed delay for all knobs
	SetKnobsSmoothDelay(-1);
}

void Module::SetDAWBlockSize(float blocksize)
{	DAW_block_size=blocksize;
}

//void Module::SetBlockSize(int bs)
//{	block_size=bs;
//}

bool Module::SetBandLimit(int bndlim)
{	// Will only be called by solorack when bandlimit is changed/set by the user.
	bandlimit=bndlim;
	return true;
}

Product *Module::Activate(char *fullname, char *email, char *serial)
{	// No activation done. Base module class is always active.
	return NULL;
}

bool Module::IsActive()
{	// By default, the module is active (licensed)
	return true;
}

Product *Module::InstanceActivate(char *fullname, char *email, char *serial)
{	return this->Activate(fullname,email,serial);
}

bool Module::InstanceIsActive()
{	return this->IsActive();
}

void Module::AddDemoViews(char *msg)
{	// Must set the tag to all added controls to NOSAVE_TAG. Otherwise, values of these controls will be saved
	// And will cause a crash when the preset is opened
	// NOTE: It's recommended that you have your own DIFFERENT version of this function
	
	CRect r;
	CColor cc,cc2;

	r = this->getViewSize();
	r = CRect(CPoint(5,27),CPoint(r.width()-10,r.height()-54));
	demolabel = new CTextLabel(r,msg,NULL,kShadowText);
	demolabel->getFont()->setStyle(kBoldFace);
	//line_height=1.6*demolabel->getFont()->getSize();
	//r.setHeight(line_height); demolabel->setViewSize(r);
	cc = CColor(); cc.blue=50; cc.green=50; cc.red=255; cc.alpha=160;
	demolabel->setBackColor(cc);
	cc.blue=0; cc.green=0; cc.red=0; cc.alpha=160;
	demolabel->setShadowColor(cc);
	//cc2.blue=0; cc2.green=0; cc2.red=0; cc2.alpha=255;
	//demolabel->setFontColor(cc2);
	demolabel->setTag(NOSAVE_TAG); demolabel->setVisible(false);
	addView(demolabel);
}

void Module::SetEnableDemo(bool state)
{	// NOTE: It's recommended that you have your own DIFFERENT version of this function.
	
	CRect r;
	CColor cc,cc2;
	
	if (demolabel==NULL)			// Just in case
	{	AddDemoViews("Demo");
	}
	else if (state)					// Make sure it the right size and color
	{	if (this!=demolabel->getParentView())			// Make sure it's still attached.
		{	if (demolabel->isAttached())
				((CViewContainer *) (demolabel->getParentView()))->removeView(demolabel);
			addView(demolabel);
		}
		r = this->getViewSize();						// Correct size
		r = CRect(CPoint(5,27),CPoint(r.width()-10,r.height()-54));
		demolabel->setViewSize(r);
		cc = CColor(); cc.blue=50; cc.green=50; cc.red=255; cc.alpha=160;
		demolabel->setBackColor(cc);
	}

	demolabel->setVisible(state);
	invalid();

	// Old version
	//if (demolabel!=NULL)
	//{	demolabel->setVisible(state);
	//	invalid();
	//}
}

// The folliwng Add... function are not general purpose. They are meant to make adding controls just a bit quicker.
PatchPoint *Module::AddPatchPoint(CCoord x, CCoord y, int pptype, CBitmap **ppbit, int bitm_index, CControlListener *listener)
{	// x,y are the cordinates for the centre
	// if bitm_index==RAND_BITMAP, then a random bitmap is chosen
	PatchPoint *pptemp;
	CRect r(CPoint(0,0),CPoint(1,1));

	if (bitm_index==RAND_BITMAP) bitm_index = GenRand(0,NUM_PP_BITMAPS-1);
	x=SkinScale(x); y=SkinScale(y);
	r.moveTo(x-ppbit[bitm_index]->getWidth()/2, y-ppbit[bitm_index]->getHeight()/2);
	r.setSize(CPoint(ppbit[bitm_index]->getWidth(),ppbit[bitm_index]->getHeight()));
	pptemp = new PatchPoint(r,listener,NO_TAG,ppbit[bitm_index],pptype); addView(pptemp);
	return pptemp;
}

PatchPoint *Module::AddMIDIPatchPoint(CCoord x, CCoord y, int pptype, CBitmap *bitmap, CControlListener *listener)
{	// x,y are the cordinates for the centre
	PatchPoint *pptemp;
	CRect r(CPoint(0,0),CPoint(1,1));

	// Create MIDI patch port
	x=SkinScale(x); y=SkinScale(y);
	r.moveTo(x-bitmap->getWidth()/2, y-bitmap->getHeight()/4);
	r.setSize(CPoint(bitmap->getWidth(),bitmap->getHeight()/2));
	pptemp = new PatchPoint(r,listener,NO_TAG,bitmap,pptype); addView(pptemp);
	pptemp->SetProtocol(ppMIDI);
	return pptemp;
}

ModuleKnob *Module::AddModuleKnob(CCoord x, CCoord y, CBitmap *bitmap, int num_images, bool is_stepping, CControlListener *listener)
{	// x,y are the cordinates for the centre
	long tag;
	ModuleKnob *ktemp;
	CRect r(CPoint(0,0),CPoint(1,1));

	x=SkinScale(x); y=SkinScale(y);
	r.moveTo(x-bitmap->getWidth()/2, y-bitmap->getHeight()/(2*num_images));
	r.setSize(CPoint(bitmap->getWidth(), bitmap->getHeight()/num_images));
	ktemp = new ModuleKnob(r,listener,tag=GetFreeTag(),bitmap,this); addView(ktemp); RegisterTag(ktemp,tag);
	ktemp->is_stepping=is_stepping;
	return ktemp;
}

CVerticalSwitch *Module::AddVerticalSwitch(CCoord x, CCoord y, CBitmap *bitmap, int num_images, CControlListener *listener)
{	// x,y are the cordinates for the centre
	long tag;
	CVerticalSwitch *swtemp;
	CRect r(CPoint(0,0),CPoint(1,1));

	x=SkinScale(x); y=SkinScale(y);
	r.moveTo(x-bitmap->getWidth()/2, y-bitmap->getHeight()/(2*num_images));
	r.setSize(CPoint(bitmap->getWidth(),bitmap->getHeight()/num_images));
	swtemp = new CVerticalSwitch(r,listener,tag=GetFreeTag(),bitmap); addView(swtemp); RegisterTag(swtemp,tag);
	return swtemp;
}

CMovieBitmap *Module::AddMovieBitmap(CCoord x, CCoord y, CBitmap *bitmap, int num_images, CControlListener *listener, bool centre)
{	// x,y are the cordinates for the centre
	CMovieBitmap *mbtemp;
	CRect r(CPoint(0,0),CPoint(1,1));

	r.setSize(CPoint(bitmap->getWidth(),bitmap->getHeight()/num_images));
	x=SkinScale(x); y=SkinScale(y);
	if (centre) 
		r.moveCentreTo(x,y);
	else
		r.moveTo(x,y);
	mbtemp = new CMovieBitmap(r,listener,NO_TAG,bitmap); addView(mbtemp);
	return mbtemp;
}

CKickButton *Module::AddKickButton(CCoord x, CCoord y, CBitmap *bitmap, int num_images, CControlListener *listener)
{	// x,y are the cordinates for the centre
	long tag;
	CKickButton *btemp;
	CRect r(CPoint(0,0),CPoint(1,1));

	x=SkinScale(x); y=SkinScale(y);
	r.moveTo(x-bitmap->getWidth()/2, y-bitmap->getHeight()/(2*num_images));
	r.setSize(CPoint(bitmap->getWidth(),bitmap->getHeight()/num_images));
	btemp = new CKickButton(r,listener,tag=GetFreeTag(),bitmap); addView(btemp); RegisterTag(btemp,tag);
	return btemp;
}

COnOffButton *Module::AddCOnOffButton(CCoord x, CCoord y, CBitmap *bitmap, int num_images, CControlListener *listener, long style)
{	// x,y are the cordinates for the centre
	long tag;
	COnOffButton *btemp;
	CRect r(CPoint(0,0),CPoint(1,1));

	x=SkinScale(x); y=SkinScale(y);
	r.moveTo(x-bitmap->getWidth()/2, y-bitmap->getHeight()/(2*num_images));
	r.setSize(CPoint(bitmap->getWidth(),bitmap->getHeight()/num_images));
	btemp = new COnOffButton(r,listener,tag=GetFreeTag(),bitmap,style); addView(btemp); RegisterTag(btemp,tag);
	return btemp;
}

CSpecialDigit *Module::AddSpecialDigit(CCoord x, CCoord y, CBitmap *bitmap, int num_images, int inumbers, long tag, CControlListener *listener)
{	// x,y are the cordinates for the centre
	CSpecialDigit *dtemp;
	CRect r(CPoint(0,0),CPoint(1,1));

	x=SkinScale(x); y=SkinScale(y);
	r.moveTo(x, y);
	r.setSize(CPoint(inumbers*bitmap->getWidth(),bitmap->getHeight()/num_images));
	dtemp = new CSpecialDigit(r,listener,tag,0,inumbers,NULL,NULL,bitmap->getWidth(),bitmap->getHeight()/num_images,bitmap); addView(dtemp); //RegisterTag(dch,tag);
	return dtemp;
}

CSpecialDigitEx *Module::AddSpecialDigitEx(CCoord x, CCoord y, CBitmap *bitmap, int num_images, int inumbers, long tag, CControlListener *listener, CBitmap* blank)
{	CSpecialDigitEx *dtemp;
	CRect r(CPoint(0,0),CPoint(1,1));

	x=SkinScale(x); y=SkinScale(y);
	r.moveTo(x, y);
	r.setSize(CPoint(inumbers*bitmap->getWidth(),bitmap->getHeight()/num_images));
	dtemp = new CSpecialDigitEx(r,listener,tag,0,inumbers,NULL,NULL,bitmap->getWidth(),bitmap->getHeight()/num_images,bitmap,blank); addView(dtemp); //RegisterTag(dch,tag);
	return dtemp;
}

ModuleKnobEx *Module::AddModuleKnobEx(CCoord x, CCoord y, CBitmap *bitmap, int num_images, bool is_stepping, CControlListener *listener, CControl *vattached1, int vtag1)
{	// x,y are the cordinates for the centre
	long tag;
	ModuleKnobEx *ktemp;
	CRect r(CPoint(0,0),CPoint(1,1));

	x=SkinScale(x); y=SkinScale(y);
	r.moveTo(x-bitmap->getWidth()/2, y-bitmap->getHeight()/(2*num_images));
	r.setSize(CPoint(bitmap->getWidth(), bitmap->getHeight()/num_images));
	ktemp = new ModuleKnobEx(r,listener,tag=GetFreeTag(),bitmap,this); addView(ktemp); RegisterTag(ktemp,tag);
	ktemp->is_stepping=is_stepping;
	ktemp->attached1=vattached1; ktemp->tag1=vtag1;
	return ktemp;
}


inline void Module::SendAudioToDAW(float left, float right)
{	synth_comm.ModuleSendAudioToDAW1Ptr(this,left,right);
}

inline void Module::SendAudioToDAW(PatchPoint **pps_outputs)
{	// INs of patchpoints will be sent to DAW outs
	// NULL indicates the end of the array. Advantage is that there is no num_inputs to pass.
	synth_comm.ModuleSendAudioToDAW2Ptr(this,pps_outputs);
}

inline void Module::SendAudioToDAW(PatchPoint **pps_outputs, int first_output)
{	// INs of patchpoints will be sent to DAW outs
	// NULL indicated the end of the array. Advantage is that there is no last_output to pass.
	// Sending will start from pps_outputs[first_output]
	synth_comm.ModuleSendAudioToDAW3Ptr(this,pps_outputs,first_output);
}

inline void Module::SendAudioToDAW(float *outputs, int last_output)
{	// Send audio to DAW from an array of float. Sending will stop at outputs[last_output]
	synth_comm.ModuleSendAudioToDAW4Ptr(this,outputs,last_output);
}

inline void Module::SendAudioToDAW(float *outputs, int first_output, int last_output)
{	// Send audio to DAW from an array of float
	// Sending will start from outputs[first_output] and will stop at outputs[last_output]
	synth_comm.ModuleSendAudioToDAW5Ptr(this,outputs,first_output,last_output);
}

inline void Module::ReceiveAudioFromDAW(float *left, float *right)
{	synth_comm.ModuleReceiveAudioFromDAW1Ptr(this,left,right);
}

inline void Module::ReceiveAudioFromDAW(PatchPoint **pps_inputs)
{	// Outs of patch points will be filled with audio from DAW
	// NULL indicates the end of the array. Advantage is that there is no last_output to pass.
	synth_comm.ModuleReceiveAudioFromDAW2Ptr(this,pps_inputs);
}

inline void Module::ReceiveAudioFromDAW(PatchPoint **pps_inputs, int first_input)
{	// Outs of patch points will be filled with audio from DAW
	// NULL indicates the end of the array. Advantage is that there is no num_inputs to pass.
	// Recieving will start from pps_inputs[first_output]
	synth_comm.ModuleReceiveAudioFromDAW3Ptr(this,pps_inputs,first_input);
}

inline void Module::ReceiveAudioFromDAW(float *inputs, int last_input)
{	// Recieve audio from DAW to an array of float. Will stop at inputs[last_input]
	synth_comm.ModuleReceiveAudioFromDAW4Ptr(this,inputs,last_input);
}

inline void Module::ReceiveAudioFromDAW(float *inputs, int first_input, int last_input)
{	// Recieve audio from DAW to an array of float. Receiving will start at inputs[first_output] and will stop at inputs[last_input] 
	synth_comm.ModuleReceiveAudioFromDAW5Ptr(this,inputs,first_input,last_input);
}

inline int Module::GetNumberOfAudioFromDAW()
{	return synth_comm.ModuleGetNumberOfAudioFromDAWPtr(this);
}

inline int Module::GetNumberOfAudioToDAW()
{	return synth_comm.ModuleGetNumberOfAudioToDAWPtr(this);
}

inline void Module::EnterProcessingCriticalSection()
{	// Once called, it will block audio processing until LeaveProcessingCriticalSection() is called.
	// This means that ProcessSample() will not be called for any module while this module has not left the critical section.
	synth_comm.ModuleEnterProcessingCriticalSectionPtr(this);
}

inline void Module::LeaveProcessingCriticalSection()
{	synth_comm.ModuleLeaveProcessingCriticalSectionPtr(this);
}

const char *Module::GetInfoURL()
{	// Should return a string (infourl) containing the complete URL where the user can find more information about this module.
	// SoloRack will browse that URL whwn the user right clicks the module and clicks "Info...".
	return infourl;
}



// This is just a place holder. Although it's static, This function should be implemented in derived classes, vproduct here refers to the one in Module.
const char *Module::GetProductName()
{	return NULL;
}

//// Set module to synth communication object.
//void Module::SetSynthComm(const SynthComm *psynth_comm)
//{	synth_comm=*psynth_comm;
//}

//void DeleteModule()
//{	delete this;
//}

void Module::PolyphonyChanged(int voices)
{	// By default, does nothing
}

float Module::GetSampleRate() {	return sample_rate; }
float Module::GetHalfSampleRate() { return hsample_rate; }
float Module::Get60SampleRate() { return sample_rate60; }
float Module::GetDAWSampleRate() { return DAW_sample_rate; }
int Module::GetDAWBlockSize() { return DAW_block_size; }

int Module::GetNBControls() { return nbcontrols; }
void Module::SetNBControls(int n) { nbcontrols=n; }
int Module::GetNBPatchPoints() { return nb_pp; }
void Module::SetNBPatchPoints(int n) { nb_pp=n; }
int Module::GetNBCables() { return nb_cables; }
void Module::SetNBCables(int n) { nb_cables=n; }
int Module::GetBandLimit() { return bandlimit; }

SoloRack *Module::GetSynth() { return psynth; }
void Module::SetSynth(SoloRack *p) { psynth=p; }
const SynthComm Module::GetSynthComm() { return synth_comm; }
void Module::SetSynthComm(const SynthComm *p) { synth_comm=*p; }
SynEditor *Module::GetSynEditor() { return peditor; }
void Module::SetSynEditor(SynEditor *p) { peditor=p; }

bool Module::GetInMove() { return in_move; }
void Module::SetInMove(bool b) { in_move=b; }
int Module::GetIndex() { return index; }
void Module::SetIndex(int i) { index=i; }

int Module::GetProcIndex() { return procindex; }
void Module::SetProcIndex(int i) { procindex=i; }
int Module::GetEvIndex() { return evindex; }
void Module::SetEvIndex(int i) { evindex=i; }
int Module::GetSbIndex() { return sbindex; }
void Module::SetSbIndex(int i) { sbindex=i; }
float Module::GetClipLevel() { return clip_level; }
void Module::SetClipLevel(float v) { clip_level=v; }
int Module::GetVoice() { return voice; }
void Module::SetVoice(int vvoice) { voice = vvoice; }

CCoord Module::SkinScale(CCoord v)
{	return FRound(Module::uiscale*(float)v);
}

float Module::FSkinScale(float v)
{	return Module::uiscale*(float)v;
}

CRect Module::SkinScale(CRect r)
{	r.x = FRound(uiscale*(float)r.x); r.y = FRound(uiscale*(float)r.y);
	r.x2 = FRound(uiscale*(float)r.x2); r.y2 = FRound(uiscale*(float)r.y2);
	return r;
}

CCoord Module::SkinUnScale(CCoord v)
{	// Does the inverse of SkinScale(). i.e returns v to the base (smallest) skin scale.
	return FRound((float)v/Module::uiscale);
}


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

	// Initialize smoothed knobs change pool.
	// INITKNOBPOOL(chpool,TESTMIXER_NUM_KNOBS);

	// Create The Knobs
	kin1 = AddModuleKnob(FIRST_RIGHT_KX,FIRST_KY,mcbits[knobit],MAIN_KNOB_IMAGES,false,listener);
	kin1->setValue(0.5); kin1->svalue=0.5; //ADDPOOLKNOB(chpool,kin1);

	kin2 = AddModuleKnob(FIRST_RIGHT_KX,FIRST_KY+YOFFSET,mcbits[knobit],MAIN_KNOB_IMAGES,false,listener);
	kin2->setValue(0.5); kin2->svalue=0.5; //ADDPOOLKNOB(chpool,kin2);

	kin3 = AddModuleKnob(FIRST_RIGHT_KX,FIRST_KY+2*YOFFSET,mcbits[knobit],MAIN_KNOB_IMAGES,false,listener);
	kin3->setValue(0.5); kin3->svalue=0.5; //ADDPOOLKNOB(chpool,kin3);

	kout = AddModuleKnob(FIRST_RIGHT_KX,FIRST_KY+4*YOFFSET,mcbits[knobit],MAIN_KNOB_IMAGES,false,listener);
	kout->setValue(1.0); kout->svalue=1.0; //ADDPOOLKNOB(chpool,kout);

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
	{	bool result;
		IS_KNOB_IN_POOL(chpool,pControl,result);
		if (result)
		{	EnterProcessingCriticalSection();
			POOL_KNOB_VALUECHANGED(chpool,pControl);
			LeaveProcessingCriticalSection();
		}
	}*/
		
}

inline void TestMixer::ProcessSample()
{	float temp;
	
	// Update smooth values.
	// If there are too many of those smoothed knobs, then its more CPU efficient to use a knob change pool, and those macros:
	// INITKNOBPOOL, ADDPOOLKNOB, POOL_KNOB_VALUECHANGED, UPDATE_POOL_SVALUES and ModuleKnobExPool to update the svalue.
	// This system of macros ensures that only changed knobs are tested and proccessed.
	// Here is how to update the smoothed values in the pool:
	// UPDATE_POOL_SVALUES(chpool)

	// Otherwise just update all the knobs smoothed values.
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
{	// Change this to your own product name. Product names should include the vendor name to avoid conflicts with other vendor modules.
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
	VstTimeInfo *tinf = psynth->getTimeInfo(kVstTempoValid);
	// kVstTempoValid | kVstPpqPosValid | kVstBarsValid | kVstCyclePosValid | kVstTimeSigValid | kVstSmpteValid | kVstClockValid | kVstTransportChanged | kVstTransportPlaying | kVstTransportCycleActive | kVstNanosValid | kVstTransportRecording | kVstAutomationReading | kVstAutomationWriting
	mult = mul[divisor];
	tempo_smp = mult*(tempo=tinf->tempo)/sample_rate60; //ppqpos=tinf->ppqPos; //temp=ppqpos-last_ppqpos; 
	pspc=CLOCK_WIDTH/tempo_smp;	
}

