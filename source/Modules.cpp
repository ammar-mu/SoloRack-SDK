/*	SoloRack SDK v0.2 Beta
	Copyright 2017-2023 Ammar Muqaddas
*/

#include <time.h>

#ifndef __Modules__
#include "Modules.h"
#endif



// This is the one in VSTGUI. 
extern OSVERSIONINFOEX	gSystemVersion;	

// MIDI Frequencies Table
float mfreqtab[128];

// for Park-Miller random generator
unsigned int zgen;



// Greatest Common Divider
int gcd(int a, int b) 
{	if (b == 0) return a; 
	return gcd(b, a % b);  
}

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


// Stack definitions and implementation
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

inline void ForceInside(long &x, long &y,const CRect &r,const CRect &rpar)
{	// rpar is rect of the container (parent)
	// Forces x,y of module to be inside container
	
	if (x<rpar.left) x=rpar.left;
	else if (x>rpar.right-r.width()) 
		x=rpar.right-r.width();

	if (y<rpar.top) y=rpar.top;
	else if (y>rpar.bottom-r.height()) 
		y=rpar.bottom-r.height();
}

CRect RectSkinFix(CRect r, float factor, float xfix=0.0, float yfix=0.0)
{	// Meant for old code. Scales a Rect to fit variable skin sizes
	//r.moveTo(FRound(factor*(float)r.x),FRound(factor*(float)r.y));
	r.moveTo(FRound(factor*((float)r.x+xfix+0.5*r.width())-0.5*r.width()),FRound(factor*((float)r.y+yfix+0.5*r.height())-0.5*r.height()));
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
	num_cables=0; force_mono=false;
	peditor = (SynEditor *) listener;
}

PatchPoint::PatchPoint(const CRect& size, CControlListener* listener, long tag, CBitmap* background, int pptype, char x, char y)
: CMovieBitmap(size,listener,tag,background)
{	type=pptype; active_type = ppTypeUnused; protocol=ppAudioCV;
	cable_in=NULL; pcable=NULL; coff_x = x; coff_y = y; pnext=this;
	num_cables=0; force_mono=false;
	peditor = (SynEditor *) listener;
}

void PatchPoint::SetCenterOffset(char x, char y)
{	coff_x = x; coff_y = y;
}

void PatchPoint::SetType(int pptype)
{	type=pptype;
}

void PatchPoint::SetProtocol(int ppprotocol)
{	protocol=ppprotocol;
}

//void PatchPoint::SetPPTag(long tag)
//{	// This functoion is used exclusivly by SoloRack. It should not be called by modules
//	pptag = tag;
//}
//
//long PatchPoint::GetPPTag()
//{	return pptag;
//}

CMouseEventResult PatchPoint::onMouseDown (CPoint &where, const long& buttons)
{	// All this mess is for Dll modules. This also assumes a module is the direct parent of the patchpoint
	return ((Module *)getParentView())->synth_comm.PPOnMouseDownPtr(this,where,buttons);
}

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
{	svalue=qvalue=value; mvalue=0.0;
	SetSmoothDelay(SMOOTH_DELAY,parent);
	setZoomFactor(5.0);
}

ModuleKnob::ModuleKnob (const CRect& size, CControlListener* listener, long tag, long subPixmaps, CCoord heightOfOneImage, CBitmap* background, Module *parent, const CPoint &offset)
: CAnimKnob(size,listener,tag,subPixmaps,heightOfOneImage,background,offset)
, is_stepping(false)
{	svalue=qvalue=value; mvalue=0.0;
	SetSmoothDelay(SMOOTH_DELAY,parent);
	setZoomFactor(5.0);
}

ModuleKnob::ModuleKnob (const ModuleKnob& v)
: CAnimKnob(v)
, is_stepping(false)
{	svalue=qvalue=value; mvalue=v.mvalue;
	SetSmoothDelay(SMOOTH_DELAY,NULL);
}

void ModuleKnob::UpdateQValue()
{	// Update Quantized value. (It's beter to just use the newer GetCurrentStep(). Much easier)
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


void ModuleKnob::SetSmoothDelay(float del, Module *parent)
{	// getParentView() does not seam to work when the parent is still in it's constructor.
	// So you have to pass it as parent in this situation.
	
	if (del>=0.f) delay=del;

	if (parent==NULL) parent=(Module *) getParentView();
		// This was the 'reciprocal' prior to SDK version 0.12
		smooth_samples_1 = 1000.0 / (parent->sample_rate * delay);
}

//---------------------------------------------
// Extended Module Knob Class - Has a extra feature. Currently: a pointer to Attached control, and tag.
// License: Some functions in this class like onMouseMoved() and onMouseDown() are derived/modified from VSTGUI originals
// Therefore these functions are licensed under original VSTGUI license
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
					range = 400.f;			// Was 200
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
	if (buttons & kRButton)
	{	// This will handle right click (copy/paste) menu
		return ModuleKnob::onMouseDown(where,buttons);
	}

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
	range = 400.f;					// Was 200
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

// License: This draw() function is derived/modified from the VSTGUI original
// Therefore the license for it is the original VSTGUI license
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
// License: Some functions in this class like onMouseUp() and onMouseDown() are derived/modified from VSTGUI originals
// Therefore these functions are licensed under the original VSTGUI license
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

// License: This draw() function is derived/modified from the VSTGUI original
// Therefore the license for it is the original VSTGUI license
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
	// If value changes rapidly in audio thread, then no use of setDirty (false)
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



// -----------------------------
// The Critical section in the this class only protects against text[] resize/realloc
// Make sure you call module::EnterCriticalSection() and module::Leave.... where ever appropriate.
// From example if you AppendText() in audio thread and call FreeText() in GUI thread.
// You have to manually protect against those situation.
// I have performance reasons for this.
CTextLabelEx::CTextLabelEx(const CRect& size, const char* txt, CBitmap* background, const long style)
: CTextLabel(size,txt,background,style)
, line_offsets(NULL)
, line_offsets_size(0)	
, last_line(-1)
, cur_line(-1)
, max_draw_lines(-1)
, use_critsec(false)
, max_text_size(CTEXTLABEL_MAXTEXT_SIZE)
, max_chars_per_line(1000000)
{
}

CTextLabelEx::CTextLabelEx(const CTextLabelEx& v)
: CTextLabel(v)
, line_offsets(NULL)
, line_offsets_size(0)
, last_line(-1)
, cur_line(-1)
, max_draw_lines(-1)
, use_critsec(false)
, max_text_size(CTEXTLABEL_MAXTEXT_SIZE)
, max_chars_per_line(1000000)
{	
	max_text_size = v.max_text_size;
	max_chars_per_line = v.max_chars_per_line;
	if (v.line_offsets!=NULL) FindLineOffsets();
	if (v.use_critsec) InitCritSec();
}

CTextLabelEx::~CTextLabelEx()
{	freeText();
	DelCritSec();
}

void CTextLabelEx::InitCritSec()
{	InitializeCriticalSection(&critsec);
	use_critsec=true;
}

void CTextLabelEx::DelCritSec()
{	if (use_critsec) DeleteCriticalSection(&critsec);
	use_critsec = false;
}

void CTextLabelEx::EnterCritSec()
{	if (use_critsec) EnterCriticalSection(&critsec);
}

void CTextLabelEx::LeaveCritSec()
{	if (use_critsec) LeaveCriticalSection(&critsec);
}

void CTextLabelEx::setText(const char* txt)
{	if (use_critsec) EnterCriticalSection(&critsec);
	CTextLabel::setText(txt);
	if (use_critsec) LeaveCriticalSection(&critsec);
}

void CTextLabelEx::freeText()
{	
	if (use_critsec) EnterCriticalSection(&critsec);
	CTextLabel::freeText();
		
	if (line_offsets!=NULL) free(line_offsets); line_offsets=NULL;
	line_offsets_size=0; last_line=-1; cur_line=-1; max_draw_lines=-1;
	if (use_critsec) LeaveCriticalSection(&critsec);
}

// txt has to be allocated using malloc, reallloc, calloc, etc...
// This is NOT the best programming practice. But I use it for performance in some cases.
void CTextLabelEx::SetTextPointer(const char* txt, int arsize)
{	
	if (use_critsec) EnterCriticalSection(&critsec);
	freeText();
	text = (char *) txt; text[arsize-1]='\0';
	text_offset=0; text_arsize = arsize; text_len = strlen(text);
	if (use_critsec) LeaveCriticalSection(&critsec);
	setDirty(true);
}

// By Ammar
bool CTextLabelEx::AppendText(const char* txt, bool ignore_limit)				
{	int i, len1;

	if (txt==NULL) return false;
	//if (use_critsec) EnterCriticalSection(&critsec);
	if (text==NULL) setText(txt);
	else
	{	i = text_len; len1 = text_len+strlen(txt)+1; 
		if (len1>text_arsize)
		{	if (len1<=max_text_size)
			{	if (use_critsec) EnterCriticalSection(&critsec);
				text_arsize = len1*2;			// Double the required size to avoid reallocation too many times
				if (text_arsize>=max_text_size) text_arsize=max_text_size; 
				text = (char *) realloc(text,text_arsize*sizeof(*text));
				text[text_arsize-1]='\0';
				if (use_critsec) LeaveCriticalSection(&critsec);
			}
			else if (ignore_limit)
			{	if (use_critsec) EnterCriticalSection(&critsec);
				text_arsize = len1;
				text = (char *) realloc(text,text_arsize*sizeof(*text));
				text[text_arsize-1]='\0';
				if (use_critsec) LeaveCriticalSection(&critsec);
			}
			else { /*if (use_critsec) LeaveCriticalSection(&critsec);*/ return false; }
		}
		// I know it's safer to have the following inside the critical section but since AppendText()
		// could be called at audio rate. I decided not to do it this way for performance.
		// I handle this manually using the Module critical section if freeText() or setText() is called from GUI thread.
		strcat(text+i,txt); text_len=len1-1;		

		if (line_offsets!=NULL) FindLineOffsets(i);
		setDirty(true);
	}
	//if (use_critsec) LeaveCriticalSection(&critsec);
	return true;
}

void CTextLabelEx::FindLineOffsets()
{	// Subsequent calls to AppendText()
	// will automatically maintain the line offsets correctly.
	int i;

	if (use_critsec) EnterCriticalSection(&critsec);
	if (text==NULL) return;
	if (line_offsets!=NULL) free(line_offsets);
	line_offsets_size=20; 
	line_offsets = (int *) malloc(line_offsets_size*sizeof(*line_offsets));
	line_offsets[0]=0; last_line=0; cur_line=0;
	FindLineOffsets(0);
	if (use_critsec) LeaveCriticalSection(&critsec);
}

void CTextLabelEx::FindLineOffsets(int start)
{	int i,linelen, lastspace;

	lastspace=-1; linelen=0; i=start; 
	// Discover current running line length where at without the newly appended charaters
	while (i>=0 && text[i]!='\n') { i--; linelen++; }
	i=start;
	while (text[i]!='\0')
	{	if (text[i]==' ') lastspace=i;
		if (text[i]=='\n') 
		{	
			line_found:
			// adjust size if not enough
			if (last_line+1>=line_offsets_size)
			{	if (use_critsec) EnterCriticalSection(&critsec);
				line_offsets_size = (last_line+1)*2;
				line_offsets = (int *) realloc(line_offsets,line_offsets_size*sizeof(*line_offsets));
				if (use_critsec) LeaveCriticalSection(&critsec);
			}

			i++; if (text[i]=='\r') i++;	// skip carriage  return
			//if (text[i]=='\0') break;
			line_offsets[++last_line] = i; linelen=0;
		}
		else if (linelen>max_chars_per_line && lastspace>line_offsets[last_line])
		{	// Put a new line to word wrap
			text[lastspace] = '\n'; i=lastspace; lastspace=-1; goto line_found;
		}
		else { i++; linelen++; }
	}
}

void CTextLabelEx::SetCurrentLine(int line)
{	//if (use_critsec) EnterCriticalSection(&critsec);
	if (line>=0 && line<=last_line)
	{	cur_line = line;
		SetTextOffset(line_offsets[line]);
	}
	//if (use_critsec) LeaveCriticalSection(&critsec);
}

void CTextLabelEx::draw(CDrawContext *pContext)
{	int i,j;
	char ch; 

	// Ammar: Trick to stop drawing string out of control bounds (ClipRect is not enough in this case) by temporarly inserting null character.
	// At the end of a line. Not the cleanest nor thread safest way, but works fast
	// The proper other alternative is to calcualte the actual size the text takes on screen and limit drawing to that. Probably not CPU lite.
	if (use_critsec) EnterCriticalSection(&critsec);
	if (line_offsets!=NULL && max_draw_lines>0) 
	{	i = cur_line+max_draw_lines;
		if (i<last_line) { j=line_offsets[i]-1; ch=text[j]; text[j]='\0'; } else ch='\0';
		
		if (text_offset<=0)	
			drawText(pContext, text);
		else drawText(pContext, &text[text_offset]);

		if (ch!='\0') text[j]=ch;
	}
	else 
	{	// Just draw the whole string
		if (text_offset<=0)	
			drawText(pContext, text);
		else drawText(pContext, &text[text_offset]);
	}
	if (use_critsec) LeaveCriticalSection(&critsec);
	setDirty (false);
}


//---------------------------------------------
// Base Module Class
CBitmap **Module::mcbits = NULL;			// Main bitmap(s) array for most controls
CBitmap **Module::ppbit = NULL;				// Bitmap(s) of the main patchpoints
char *Module::skindir = NULL;
char *Module::defskindir = NULL;
char *Module::dlldatadir = NULL;
char *Module::dllskindir = NULL;
char *Module::datadir = NULL;
char *Module::plugindir = NULL;
float Module::uiscale = 1.0;
long Module::vp = BASE_MHEIGHT;
long Module::vp_3 = BASE_MHEIGHT/3;
long Module::vp_5 = BASE_MHEIGHT/5;
long Module::hp = BASE_HP;
CColor Module::digit_color = CColor();

int Module::GetSDKVersion() 
{	return SDK_VERSION; 
}

Module::Module(const CRect &size, CFrame *pParent, CBitmap *pBackground, const SynthComm *psynth_comm, const int vvoice)
: ModuleWrapper(size,pParent,pBackground) 
{	synth_comm=*psynth_comm;
	in_move = false; index=-1; evindex=-1; sbindex=-1; mouseobindex=-1; procindex=-1; orphindex=-1;
	voice=vvoice; is_mono=false; enable_is_mono_menu=true;
	nbcontrols=-1; nb_pp=-1; nb_force_mono=0; allow_oversample=true; enable_allow_oversample=true;
	nb_cables=0; always_on=GetAlwaysONDefault();

	//peditor = (SynEditor *) pParent->getEditor();
	peditor = synth_comm.GetEditor();
	psynth = synth_comm.GetSynth(peditor);
	DAW_block_size = synth_comm.GetDAWBlockSize(peditor);
	DAW_sample_rate = synth_comm.GetDAWSampleRate(peditor);

	#ifdef MODULE_OVERSAMPLE
	// Oversampling settings for per mdoule oversampling. Not complete yet.
	ovr.overs=1; ovr.sovers=1; ovr.overs_index=0; ovr.overs_filter=peditor->moversfilter_menu_index[kIIRChamberlin];
	ovr.ovin=NULL; ovr.cof=NULL; ovr.cofs=0; ovr.iovin=0;
	ovr.bp=0; ovr.bp_2=0; ovr.lp=0; ovr.lp_2=0;
	sample_rate = DAW_sample_rate*psynth->sovers*ovr.overs;
	#else
	//sample_rate = DAW_sample_rate*psynth->sovers;
	sample_rate = DAW_sample_rate*psynth_comm->GetOversamplingFactor(peditor);
	#endif
	
	//sample_rate = DAW_sample_rate*psynth->sovers*ovr.overs;
	hsample_rate = sample_rate/2.0;
	sample_rate60 = 60.0*sample_rate;

	// Default to NO Bandlimiting
	Module::SetBandLimit(kNoBandlimiting);
	demolabel=NULL; infourl=NULL;
	clip_level = DEFAULT_MAX_LEVEL;
}

Module::~Module()
{	
	#ifdef MODULE_OVERSAMPLE
	free(ovr.ovin); free(ovr.cof);
	#endif
	// removeAll() in CViewContiner will remove and delete all added views from memory
	free(infourl);
}

// Will be called only once by DllInitialize(). Don't call this in a derived module Initialize()
void Module::Initialize()
{	int i;
	char temp[40];
	char temp2[2];
	char *skin_config, *defskin_config;

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
	mcbits[led_blue] = new CBitmap(skindir,defskindir,"led_blue.png");
	mcbits[led_red] = new CBitmap(skindir,defskindir,"led_red.png");

	
	// small knobs
	mcbits[sknobit_black5] = new CBitmap(dllskindir,NULL,"sknobit_black_5s.png");


	// Make skin config file
	skin_config = (char *) malloc((strlen(skindir)+21)*sizeof(*skin_config));
	defskin_config = (char *) malloc((strlen(defskindir)+21)*sizeof(*defskin_config));
	strcpy(skin_config,skindir); strcat(skin_config,"skin_config.ini");
	strcpy(defskin_config,defskindir); strcat(defskin_config,"skin_config.ini");

	// Skin attributes
	i = GetPrivateProfileIntA("Other","digit_red",MAX_INT,skin_config);
	if (i==MAX_INT) digit_color.red = GetPrivateProfileIntA("Other","digit_red",245,defskin_config);
	else digit_color.red=i;

	i = GetPrivateProfileIntA("Other","digit_blue",MAX_INT,skin_config);
	if (i==MAX_INT) digit_color.blue = GetPrivateProfileIntA("Other","digit_blue",0,defskin_config);
	else digit_color.blue=i;

	i = GetPrivateProfileIntA("Other","digit_green",MAX_INT,skin_config);
	if (i==MAX_INT) digit_color.green = GetPrivateProfileIntA("Other","digit_green",132,defskin_config);
	else digit_color.green=i;

	i = GetPrivateProfileIntA("Other","digit_alpha",MAX_INT,skin_config);
	if (i==MAX_INT) digit_color.alpha = GetPrivateProfileIntA("Other","digit_alpha",255,defskin_config);
	else digit_color.alpha=i;

	free(skin_config); free(defskin_config);

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

	free(mcbits);

}

CMouseEventResult Module::onMouseDown(CPoint &where, const long& buttons)
{	// all this mess is for Dll modules
	return synth_comm.ModuleOnMouseDownPtr(this,where,buttons);
}

CMouseEventResult Module::onMouseUp(CPoint &where, const long& buttons)
{	// all this mess is for Dll modules
	return synth_comm.ModuleOnMouseUpPtr(this,where,buttons);
}

CMouseEventResult Module::onMouseMoved(CPoint &where, const long& buttons)
{	// all this mess is for Dll modules
	return synth_comm.ModuleOnMouseMovedPtr(this,where,buttons);
}

CMouseEventResult Module::OnMouseMovedObserve(CPoint &where, const long& buttons)
{	// This is NOT from VSTGUI.
	// Do nothing by default. This should be overiden by modules that call CallMouseObserve()
	// SoloRack will keep calling this for any mouse movements no matter where they are located, even
	// if they are far off this modules boundaries. This is usefull for example for a theremin module or such things.
	return kMouseEventNotHandled;
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

bool Module::CallProcessEvents()
{	// Tells SoloRack to call ProcessEvents for this module, ProcessEvents() is called when ever a DAW event is sent to SoloRack. Should be called by the module, typicaly in the construtor.
	return synth_comm.ModuleCallProcessEventsPtr(this);
}

bool Module::CallStartOfBlock()
{	// Tells SoloRack to call StartOfBlock for this module every time processReplacing() is called in SoloRack. 
	return synth_comm.ModuleCallStartOfBlockPtr(this);
}

bool Module::CallMouseObserve()
{	// Tells SoloRack to call OnMouseMovedObserve() for this module every time the mouse changes position/moves no matter where the mouse is
	return synth_comm.ModuleCallMouseObservePtr(this);
}

bool Module::ClearIfOrphaned()
{	// Should only be called in the destructor or just before it.
	// In very odd cases, you may not want to imidiatly destruct your module after it's been deleted.
	// For example, with modules that connect to the network or have worker thread actively working.
	// This can be done calling remmember(), then calling forget() when work is finished and you are ready to destruct.
	// However, during this "hanging there period", SoloRack will put the module into an "orphaned" list that will be forced to destruct at synth exit.
	// This function tells SoloRack that this module has gracefully been detructed. So if it's in the orphaned list. SoloRack will remove it and not try to destruct it on synth exit
	// You HAVE TO call this function if you automatically destruct your orphaned modules later. Otherwise SoloRack WILL CRASH on synth exit
	// Because it will try to decontruct your modules which doesn't exist any more in memory.
	return synth_comm.ClearIfOrphaned(this);
}



bool Module::UnRegisterOldTag(CControl *pcon,long oldtag, long newtag)
{	// This function safely unregisters an old tag if a new tag for the same control has been registered (before calling this function).
	// This is mainly used when loading presets.
	
	return synth_comm.ModuleUnRegisterOldTagPtr(this,pcon,oldtag,newtag);
}



void Module::ProcessEvents(const EventsHandle ev)
{
}

void Module::StartOfBlock(int sample_frames)	
{	// Called by SoloRack on the start of each block, for all modules that called CallProcessEvents()
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

void Module::ProcessSample()
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
{	// This fuction assumes that all controls tags are of type long

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
{	// You could use size to check, just in case the saved preset uses less data
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
					{	// This is not needed any more since UnRegisterControlsTags() is laways called
						// Before calling this function
						UnRegisterOldTag((CControl *)(ptemp->pView),ttag,*pfdata);
					}
				}
				//else ((CControl *)(ptemp->pView))->setTag(*pfdata);		// Tag is discarded because it can create conflicts
				size-=sizeof(*pfdata); pfdata++;
			}
		}
		ptemp = ptemp->pNext;
	}
}

void Module::UnRegisterControlsTags()
{	CCView *pv=pFirstView;

	while (pv)
	{	if (pv->pView->isTypeOf("CControl"))
			UnRegisterTag((CControl *)(pv->pView));
		pv = pv->pNext;
	}
}

void Module::ResetNbControlsAndPatchPoints()
{	nbcontrols=-1; nb_pp=-1;
}

void Module::SetPPTags(long &ctag)
{	// The primary porpus of this it to set the pptag. which shouldn't be changed by developers. These are exclusively used by SoloRack to save cable connections
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

	if (this->nb_force_mono>0 && voice!=0)
	{	// Some patchpoints are force_mono=true
		CCView *ptempz = GetVoiceModule(0)->pFirstView;
		while (ptemp && i<=nb_pp)
		{	if (ptemp->pView->isTypeOf("PatchPoint"))
			{	// If it's a forcemono patch point. Then take it from voice zero
				if (((PatchPoint *)ptemp->pView)->force_mono)
					tags2pp[ctag] = (PatchPoint *) ptempz->pView;
				else tags2pp[ctag] = (PatchPoint *) ptemp->pView;
				ctag++; i++;
			}
			ptemp = ptemp->pNext; ptempz = ptempz->pNext;
		}
	}
	else
	{	// Most usual case
		while (ptemp && i<=nb_pp)
		{	if (ptemp->pView->isTypeOf("PatchPoint"))
			{	tags2pp[ctag] = (PatchPoint *) ptemp->pView;
				ctag++; i++;
			}
			ptemp = ptemp->pNext;
		}
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

// Introduced for CLAP host modulation
void Module::ZeroDAWModValues()
{
	CCView *ptemp;
	ModuleKnob *knob;
	float ft;

	if (synth_comm.GetPluginFormat(peditor)==CLAP_FORMAT)
	{	ptemp=pFirstView;
		while (ptemp)
		{	if (ptemp->pView->isTypeOf("ModuleKnob"))
			{	knob = (ModuleKnob*) ptemp->pView;
				if (knob->mvalue!=0.f)
				{	ft = knob->value-knob->mvalue;
					knob->setValue(CLIP(ft,0.f,1.f));
					knob->mvalue=0.0;
					ValueChanged(knob);
				}
			}
			ptemp = ptemp->pNext;
		}
	}
}

void Module::SaveControlsValues(void *pdata)
{	float *pfdata = (float *) pdata;							// This type must be the same type as value/getValue()
	CCView *ptemp=pFirstView;

	ZeroDAWModValues();
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
{	// Size is used to check, just in case the saved preset uses less data
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
						if (pp->protocol==ppMIDI)
							if (pp->num_cables==0) pp->setValue(0); //pp->invalid();
							else pp->setValue(1);
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

void Module::SetKnobsSmoothDelay(float del)
{	//del=-1 will use the same delay already assciated with each knob. This function will be called by solorack when sample rate changes too because variable smooth_samples_1 has to be recalculated
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
{	// Called by solorack
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
// Coordinates here are in float!! this is to allow finer adjustment for larger than 1 skin scale
// Skin scale 1 is the minimum size (small) of the skin. Each 1.0 (x or y) coresponds to 1 pixel in that size.
PatchPoint *Module::AddPatchPoint(float x, float y, int pptype, CBitmap **ppbit, int bitm_index, CControlListener *listener)
{	// x,y are the cordinates for the centre
	// if bitm_index==RAND_BITMAP, then a random bitmap is chosen
	PatchPoint *pptemp;
	CRect r(CPoint(0,0),CPoint(1,1));

	if (bitm_index==RAND_BITMAP) bitm_index = GenRand(0,NUM_PP_BITMAPS-1);
	x=FSkinScale(x); y=FSkinScale(y);
	r.moveTo(FRound(x-(float) (ppbit[bitm_index]->getWidth()/2)), FRound(y-(float) (ppbit[bitm_index]->getHeight()/2)));
	r.setSize(CPoint(ppbit[bitm_index]->getWidth(),ppbit[bitm_index]->getHeight()));
	pptemp = new PatchPoint(r,listener,NO_TAG,ppbit[bitm_index],pptype); addView(pptemp);
	return pptemp;
}

PatchPoint *Module::AddMIDIPatchPoint(float x, float y, int pptype, CBitmap *bitmap, CControlListener *listener)
{	// x,y are the cordinates for the centre
	PatchPoint *pptemp;
	CRect r(CPoint(0,0),CPoint(1,1));

	// Create MIDI patch port
	x=FSkinScale(x); y=FSkinScale(y);
	r.moveTo(FRound(x-(float) (bitmap->getWidth()/2)), FRound(y-(float) (bitmap->getHeight()/4)));
	r.setSize(CPoint(bitmap->getWidth(),bitmap->getHeight()/2));
	pptemp = new PatchPoint(r,listener,NO_TAG,bitmap,pptype); addView(pptemp);
	pptemp->SetProtocol(ppMIDI);
	return pptemp;
}

ModuleKnob *Module::AddModuleKnob(float x, float y, CBitmap *bitmap, int num_images, bool is_stepping, CControlListener *listener, long tag)
{	// x,y are the cordinates for the centre
	ModuleKnob *ktemp;
	CRect r(CPoint(0,0),CPoint(1,1));

	x=FSkinScale(x); y=FSkinScale(y);
	r.moveTo(FRound(x-(float) (bitmap->getWidth()/2)), FRound(y-(float) (bitmap->getHeight()/(2*num_images))));
	r.setSize(CPoint(bitmap->getWidth(), bitmap->getHeight()/num_images));
	if (tag==NEW_TAG) tag = GetFreeTag();
	ktemp = new ModuleKnob(r,listener,tag,bitmap,this); addView(ktemp); RegisterTag(ktemp,tag);
	ktemp->is_stepping=is_stepping;
	return ktemp;
}

CVerticalSwitch *Module::AddVerticalSwitch(float x, float y, CBitmap *bitmap, int num_images, CControlListener *listener)
{	// x,y are the cordinates for the centre
	long tag;
	CVerticalSwitch *swtemp;
	CRect r(CPoint(0,0),CPoint(1,1));

	x=FSkinScale(x); y=FSkinScale(y);
	r.moveTo(FRound(x-(float) (bitmap->getWidth()/2)), FRound(y-(float) (bitmap->getHeight()/(2*num_images))));
	r.setSize(CPoint(bitmap->getWidth(),bitmap->getHeight()/num_images));
	swtemp = new CVerticalSwitch(r,listener,tag=GetFreeTag(),bitmap); addView(swtemp); RegisterTag(swtemp,tag);
	return swtemp;
}

CMovieBitmap *Module::AddMovieBitmap(float x, float y, CBitmap *bitmap, int num_images, CControlListener *listener, bool centre)
{	CMovieBitmap *mbtemp;
	CRect r(CPoint(0,0),CPoint(1,1));

	r.setSize(CPoint(bitmap->getWidth(),bitmap->getHeight()/num_images));
	x=FRound(FSkinScale(x)); y=FRound(FSkinScale(y));
	if (centre) 
		r.moveCentreTo(x,y);
	else
		r.moveTo(x,y);
	mbtemp = new CMovieBitmap(r,listener,NO_TAG,bitmap); addView(mbtemp);
	return mbtemp;
}

CKickButton *Module::AddKickButton(float x, float y, CBitmap *bitmap, int num_images, CControlListener *listener)
{	// x,y are the cordinates for the centre
	long tag;
	CKickButton *btemp;
	CRect r(CPoint(0,0),CPoint(1,1));

	x=FSkinScale(x); y=FSkinScale(y);
	r.moveTo(FRound(x-(float) (bitmap->getWidth()/2)), FRound(y-(float) (bitmap->getHeight()/(2*num_images))));
	r.setSize(CPoint(bitmap->getWidth(),bitmap->getHeight()/num_images));
	btemp = new CKickButton(r,listener,tag=GetFreeTag(),bitmap); addView(btemp); RegisterTag(btemp,tag);
	return btemp;
}

COnOffButton *Module::AddOnOffButton(float x, float y, CBitmap *bitmap, int num_images, CControlListener *listener, long style, long tag)
{	// x,y are the cordinates for the centre
	COnOffButton *btemp;
	CRect r(CPoint(0,0),CPoint(1,1));

	x=FSkinScale(x); y=FSkinScale(y);
	r.moveTo(FRound(x-(float) (bitmap->getWidth()/2)), FRound(y-(float) (bitmap->getHeight()/(2*num_images))));
	r.setSize(CPoint(bitmap->getWidth(),bitmap->getHeight()/num_images));
	if (tag==NEW_TAG) tag = GetFreeTag();
	btemp = new COnOffButton(r,listener,tag,bitmap,style); addView(btemp); RegisterTag(btemp,tag);
	return btemp;
}

CSpecialDigit *Module::AddSpecialDigit(float x, float y, CBitmap *bitmap, int num_images, int inumbers, long tag, CControlListener *listener)
{	CSpecialDigit *dtemp;
	CRect r(CPoint(0,0),CPoint(1,1));

	x=FRound(FSkinScale(x)); y=FRound(FSkinScale(y));
	r.moveTo(x, y);
	r.setSize(CPoint(inumbers*bitmap->getWidth(), bitmap->getHeight()/num_images ));
	dtemp = new CSpecialDigit(r,listener,tag,0,inumbers,NULL,NULL,bitmap->getWidth(),bitmap->getHeight()/num_images,bitmap); addView(dtemp); //RegisterTag(dch,tag);
	return dtemp;
}

CSpecialDigitEx *Module::AddSpecialDigitEx(float x, float y, CBitmap *bitmap, int num_images, int inumbers, long tag, CControlListener *listener, CBitmap* blank)
{	CSpecialDigitEx *dtemp;
	CRect r(CPoint(0,0),CPoint(1,1));

	x=FRound(FSkinScale(x)); y=FRound(FSkinScale(y));
	r.moveTo(x, y);
	r.setSize(CPoint(inumbers*bitmap->getWidth(),bitmap->getHeight()/num_images));
	dtemp = new CSpecialDigitEx(r,listener,tag,0,inumbers,NULL,NULL,bitmap->getWidth(),bitmap->getHeight()/num_images,bitmap,blank); addView(dtemp); //RegisterTag(dch,tag);
	return dtemp;
}

ModuleKnobEx *Module::AddModuleKnobEx(float x, float y, CBitmap *bitmap, int num_images, bool is_stepping, CControlListener *listener, CControl *vattached1, int vtag1, long tag)
{	// x,y are the cordinates for the centre
	ModuleKnobEx *ktemp;
	CRect r(CPoint(0,0),CPoint(1,1));

	x=FSkinScale(x); y=FSkinScale(y);
	r.moveTo(FRound(x-(float) (bitmap->getWidth()/2)), FRound(y-(float) (bitmap->getHeight()/(2*num_images))));
	r.setSize(CPoint(bitmap->getWidth(), bitmap->getHeight()/num_images));
	if (tag==NEW_TAG) tag = GetFreeTag();
	ktemp = new ModuleKnobEx(r,listener,tag,bitmap,this); addView(ktemp); RegisterTag(ktemp,tag);
	ktemp->is_stepping=is_stepping;
	ktemp->attached1=vattached1; ktemp->tag1=vtag1;
	return ktemp;
}

CTextLabel *Module::AddTextLabel(float x, float y, CBitmap *bitmap, long style, float width, float height)
{	long tag;
	CTextLabel *btemp;
	CRect r(CPoint(0,0),CPoint(1,1));

	x=FRound(FSkinScale(x)); y=FRound(FSkinScale(y));
	r.moveTo(x, y);
	if (width>=0.0 || height>=0.0)
	{	r.setSize(CPoint(FRound(FSkinScale(width)),FRound(FSkinScale(height))));
	}
	else
	{	// Bitmap must be not NULL in this case
		r.setSize(CPoint(bitmap->getWidth(),bitmap->getHeight()));
	}
	btemp = new CTextLabel(r,NULL,bitmap,style); addView(btemp);
	btemp->setTag(NO_TAG);
	return btemp;
}

CTextLabelEx *Module::AddTextLabelEx(float x, float y, CBitmap *bitmap, long style, float width, float height)
{	long tag;
	CTextLabelEx *btemp;
	CRect r(CPoint(0,0),CPoint(1,1));

	x=FRound(FSkinScale(x)); y=FRound(FSkinScale(y));
	r.moveTo(x, y);
	if (width>=0.0 || height>=0.0)
	{	r.setSize(CPoint(FRound(FSkinScale(width)),FRound(FSkinScale(height))));
	}
	else
	{	// Bitmap must be not NULL in this case
		r.setSize(CPoint(bitmap->getWidth(),bitmap->getHeight()));
	}
	btemp = new CTextLabelEx(r,NULL,bitmap,style); addView(btemp);
	btemp->setTag(NO_TAG);
	return btemp;
}

CHorizontalSlider *Module::AddHorizontalSlider(float x, float y, float width, float height, CBitmap *bitmap, CBitmap *background, CControlListener *listener)
{	// if a width or height is <0 then it will be taken from background
	long tag;
	CHorizontalSlider *hslider;
	CRect r(CPoint(0,0),CPoint(1,1));

	x=FRound(FSkinScale(x)); y=FRound(FSkinScale(y)); r.moveTo(x, y);
	width = FRound(FSkinScale(width)); height = FRound(FSkinScale(height)); 
	if (width>=0.0 && height>=0.0) r.setSize(CPoint(width,height));
	else if (background!=NULL) r.setSize(CPoint(background->getWidth(),background->getHeight()));
	else r.setSize(CPoint(0,0));
	hslider = new CHorizontalSlider(r, listener, tag=GetFreeTag(), 0, r.getWidth(), bitmap, background,CPoint(0,0),kLeft);
	addView(hslider); RegisterTag(hslider,tag);
	return hslider;
}


void Module::SendAudioToDAW(float left, float right)
{	synth_comm.ModuleSendAudioToDAW1Ptr(this,left,right);
}

void Module::SendAudioToDAW(PatchPoint **pps_outputs)
{	// INs of patchpoints will be sent to DAW outs
	// NULL indicates the end of the array. Advantage is that there is no num_inputs to pass.
	synth_comm.ModuleSendAudioToDAW2Ptr(this,pps_outputs);
}

void Module::SendAudioToDAW(PatchPoint **pps_outputs, int first_output)
{	// INs of patchpoints will be sent to DAW outs
	// NULL indicated the end of the array. Advantage is that there is no last_output to pass.
	// Sending will start from pps_outputs[first_output]
	synth_comm.ModuleSendAudioToDAW3Ptr(this,pps_outputs,first_output);
}

void Module::SendAudioToDAW(float *outputs, int last_output)
{	// Send audio to DAW from an array of float. Sending will stop at outputs[last_output]
	synth_comm.ModuleSendAudioToDAW4Ptr(this,outputs,last_output);
}

void Module::SendAudioToDAW(float *outputs, int first_output, int last_output)
{	// Send audio to DAW from an array of float
	// Sending will start from outputs[first_output] and will stop at outputs[last_output]
	synth_comm.ModuleSendAudioToDAW5Ptr(this,outputs,first_output,last_output);
}

void Module::ReceiveAudioFromDAW(float *left, float *right)
{	synth_comm.ModuleReceiveAudioFromDAW1Ptr(this,left,right);
}

void Module::ReceiveAudioFromDAW(PatchPoint **pps_inputs)
{	// Outs of patch points will be filled with audio from DAW
	// NULL indicates the end of the array. Advantage is that there is no last_output to pass.
	synth_comm.ModuleReceiveAudioFromDAW2Ptr(this,pps_inputs);
}

void Module::ReceiveAudioFromDAW(PatchPoint **pps_inputs, int first_input)
{	// Outs of patch points will be filled with audio from DAW
	// NULL indicates the end of the array. Advantage is that there is no num_inputs to pass.
	// Recieving will start from pps_inputs[first_output]
	synth_comm.ModuleReceiveAudioFromDAW3Ptr(this,pps_inputs,first_input);
}

void Module::ReceiveAudioFromDAW(float *inputs, int last_input)
{	// Recieve audio from DAW to an array of float. Will stop at inputs[last_input]
	synth_comm.ModuleReceiveAudioFromDAW4Ptr(this,inputs,last_input);
}

void Module::ReceiveAudioFromDAW(float *inputs, int first_input, int last_input)
{	// Recieve audio from DAW to an array of float. Receiving will start at inputs[first_output] and will stop at inputs[last_input] 
	synth_comm.ModuleReceiveAudioFromDAW5Ptr(this,inputs,first_input,last_input);
}

int Module::GetNumberOfAudioFromDAW()
{	return synth_comm.ModuleGetNumberOfAudioFromDAWPtr(this);
}

int Module::GetNumberOfAudioToDAW()
{	return synth_comm.ModuleGetNumberOfAudioToDAWPtr(this);
}

void Module::EnterProcessingCriticalSection()
{	// Once called, it will block audio processing until LeaveProcessingCriticalSection() is called.
	// This means that ProcessSample() will not be called for any module while this module has not left the critical section.
	synth_comm.ModuleEnterProcessingCriticalSectionPtr(this);
}

void Module::LeaveProcessingCriticalSection()
{	synth_comm.ModuleLeaveProcessingCriticalSectionPtr(this);
}

const char *Module::GetInfoURL()
{	// Should return a string (infourl) containing the complete URL where the user can find more information about this module.
	// SoloRack will browse that URL whwn the user right clicks the module and clicks "Info...".
	return infourl;
}



// This is just a place holder. Although it's static, This function should be implemented in derived classes.
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
{	// Called By SoloRack when the user changes the number of voices in the menu
	// By default, does nothing
}

void Module::SetForceMono(PatchPoint *pp, bool set_is_mono)
{	// The module must be a parent of pp.
	
	if (!pp->force_mono && set_is_mono) nb_force_mono++;
	else if (pp->force_mono && !set_is_mono) nb_force_mono--;
	pp->force_mono = set_is_mono;
}

Module *Module::GetVoiceModule(int voice)
{	// This usefull if you want different voices of the same module to talk to each other.
	return synth_comm.GetVoiceModule(peditor,this,voice);
}

void Module::ConstructionComplete(int voices)
{	// This function is called by SoloRack after all constructors of all voices of a module is called
	// And after all solorack specific information like index, procindex, etc... has been setup
	// This is where GetVoiceModule() will start working.

}

void Module::DestructionStarted()
{	// This function is called by SoloRack for all voices just before destructors are called of all voice.
	// This will give a chance for odd modules that cannot imidiatly destruct, for example in case the module
	// Is running a high CPU worker thread that needs to be stopped, or is connecting to the network.

	// The idea here is to give all modules a quick heads up, then atempt to destruct them one by one. This allows 
	// the odd modules to end there work WHILE solorack destructs other modules
}

// NONE_CPP_DESTRUCTOR is reserved for future use to resolve ABI compatibility issues with compilers other
// than VC++
#ifdef NONE_CPP_DESTRUCTOR
bool Module::forget()
{	nbReference--; 
	if (nbReference == 0) { this->Destructor(); delete this; return true; } else return false;
}
#else
bool Module::forget()
{	return CBaseObject::forget();
}
#endif

void Module::Destructor()
{	// Reserved for future use. Please do not implement this. It's not ready.
}

// Not a fan of these Setters and getters. But I think it's better for future generic coding and better ABI compatibility in the future.
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
int Module::GetMouseObIndex() { return mouseobindex; }
void Module::SetMouseObIndex(int i) { mouseobindex=i; }
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

float Module::GetClipLevelPreset(int v)
{	switch (v)
	{	case km12db: return 0.25;
		case km6db: return 0.5;
		case km3db: return 0.70794578;
		case k0db: return 1.0;
		case k3db: return 1.41253755;
		case k6db: return 2.0;
		case k12db: default: return 4.0;
	}	
}

