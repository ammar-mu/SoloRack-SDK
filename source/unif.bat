rename Modules.h Modules_org.h
rename Modules.cpp Modules_org.cpp
unifdef -DIS_DLL_MODULES -oModules.h Modules_org.h
unifdef -DIS_DLL_MODULES -oModules.cpp Modules_org.cpp
