/*	SoloRack SDK v0.11 Beta
	Copyright 2017-2022 Ammar Muqaddas
*/

#include "Modules.h"

//----------------------------------------------------------
// Exported functions
extern "C"
{

__declspec(dllexport) bool GetDllModule(DllModule *mi, char *vendorname)
{	// mi->name and mi->vendorname are inputs to this function.
	// They are read by SoloRack from the xxxx.ini file, were xxxx.dll is the dll contaniing this requested module
	// Returnes true if the operation is successful, otherwise false.

	if (strcmp(mi->name,TestMixer::name)==0 && strcmp(vendorname,TestMixer::vendorname)==0)
	{	mi->sdk_version = SDK_VERSION;
		mi->name_len = TestMixer::name_len;		// Not neccessary.
		
		mi->InitializePtr = TestMixer::Initialize;
		mi->EndPtr = TestMixer::End;
		mi->Constructor = (Module *(*)(CFrame *,CControlListener *, const SynthComm*, const int)) TestMixer::Constructor;				// (Module *(__cdecl *)(CFrame *,CControlListener *)) 
		mi->GetTypePtr = TestMixer::GetType;
		mi->ActivatePtr = TestMixer::Activate;
		mi->IsActivePtr = TestMixer::IsActive;
		mi->GetProductNamePtr = TestMixer::GetProductName;

		return true;
	}
	else if (strcmp(mi->name,TestTempoFromDAW::name)==0 && strcmp(vendorname,TestTempoFromDAW::vendorname)==0)
	{	mi->sdk_version = SDK_VERSION;
		mi->name_len = TestTempoFromDAW::name_len;		// Not neccessary.
		
		mi->InitializePtr = TestTempoFromDAW::Initialize;
		mi->EndPtr = TestTempoFromDAW::End;
		mi->Constructor = (Module *(*)(CFrame *,CControlListener *, const SynthComm*, const int)) TestTempoFromDAW::Constructor;				// (Module *(__cdecl *)(CFrame *,CControlListener *)) 
		mi->GetTypePtr = TestTempoFromDAW::GetType;
		mi->ActivatePtr = TestTempoFromDAW::Activate;
		mi->IsActivePtr = TestTempoFromDAW::IsActive;
		mi->GetProductNamePtr = TestTempoFromDAW::GetProductName;

		return true;
	}

	else return false;
}

// The following functions are used to enumerate all modules in the Dll, they are only used when scanning the dll instead of reading an .INI file. Which is not the default behavior.
// You should provide an accurate .INI file for your dll.
__declspec(dllexport) int GetDllNumberOfModules()
{	// Returned total number of modules in this Dll.
	return 2;
}

__declspec(dllexport) const char *GetDllVendorName()
{	return "SoloStuff";
}

__declspec(dllexport) int GetDllModuleNameLenByIndex(DllModule *mi, int index)
{	// index range is from 0 to (Number Of Modules-1)
	// Returnes the module name length (exclusing terminating null).
	// If the module is not found, -1 is returned

	switch (index)
	{	case 0:
			return TestMixer::name_len;	
		case 1:
			return TestTempoFromDAW::name_len;	
		default:
			return -1;
	}
}

__declspec(dllexport) bool GetDllModuleByIndex(DllModule *mi, int index)
{	// index range is from 0 to (Number Of Modules-1)
	// Returnes true if the operation is successful, otherwise false.
	// Note, caller (SoloRack) should allocate the memory for name. enough space can be ensured by calling GetDllModuleNameLenByIndex() 

	switch (index)
	{	case 0:
			mi->sdk_version = SDK_VERSION;
			strcpy(mi->name,TestMixer::name);
			mi->name_len = TestMixer::name_len;	

			mi->InitializePtr = TestMixer::Initialize;
			mi->EndPtr = TestMixer::End;
			mi->Constructor = (Module *(*)(CFrame *,CControlListener *, const SynthComm*, const int)) TestMixer::Constructor;				// (Module *(__cdecl *)(CFrame *,CControlListener *)) 
			mi->GetTypePtr = TestMixer::GetType;
			mi->ActivatePtr = TestMixer::Activate;
			mi->IsActivePtr = TestMixer::IsActive;
			mi->GetProductNamePtr = TestMixer::GetProductName;
			break;

		case 1:
			mi->sdk_version = SDK_VERSION;
			strcpy(mi->name,TestTempoFromDAW::name);
			mi->name_len = TestTempoFromDAW::name_len;	

			mi->InitializePtr = TestTempoFromDAW::Initialize;
			mi->EndPtr = TestTempoFromDAW::End;
			mi->Constructor = (Module *(*)(CFrame *,CControlListener *, const SynthComm*, const int)) TestTempoFromDAW::Constructor;				// (Module *(__cdecl *)(CFrame *,CControlListener *)) 
			mi->GetTypePtr = TestTempoFromDAW::GetType;
			mi->ActivatePtr = TestTempoFromDAW::Activate;
			mi->IsActivePtr = TestTempoFromDAW::IsActive;
			mi->GetProductNamePtr = TestTempoFromDAW::GetProductName;
			break;

		default:
			return false;
	}
	return true;
}

__declspec(dllexport) bool DllInitialize(const DllInit *init)
{	int i, len;

	// Setup some global variables
	// Skin and UI size variable
	Module::skindir = (char *) malloc((strlen(init->skindir)+2)*sizeof(*(init->skindir)));
	strcpy((char *) Module::skindir,init->skindir);
	Module::defskindir = (char *) malloc((strlen(init->defskindir)+2)*sizeof(*(init->defskindir)));
	strcpy((char *) Module::defskindir,init->defskindir);
	Module::vp = init->vp;
	Module::vp_5 = Module::vp/5; Module::vp_3 = Module::vp/3;
	Module::uiscale = init->uiscale;
	Module::hp = Module::uiscale*BASE_HP;

	// Choose dll skin size based on solorack's skin size (indicated by the name of the skin folder Small/Medium/Large
	// Alternativly you can use a diffrent way, for example, you can base the size upon the Module::uiscale or the Module::vp
	i = strlen(Module::skindir)-2;
	while (i>=0)
	{	if (Module::skindir[i]=='\\') 
		{	Module::dllskindir = (char *) malloc((strlen(Module::dlldatadir)+strlen("Skin\\")+strlen(&(Module::skindir[i+1]))+2)*sizeof(*Module::dllskindir));
			strcpy(Module::dllskindir,Module::dlldatadir);
			strcat(Module::dllskindir,"Skin\\");
			strcat(Module::dllskindir,&(Module::skindir[i+1])); 
			break; 
		}
		i--;
	}
	
	Module::Initialize();
	TestMixer::Initialize();
	TestTempoFromDAW::Initialize();
	// Remmember to call your Initialize() here for each of your modules
	// ....
	return true;
}
}