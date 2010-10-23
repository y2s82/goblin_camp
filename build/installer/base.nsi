; Goblin Camp installer
; Use mkinstaller.py to build it.

SetCompressor /SOLID lzma

; Macros
!define GC_VERSION       "%%_GC_VERSION_%%"
!define GC_UNINST_REGKEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\Goblin Camp"

; Includes
!define MULTIUSER_INSTALLMODE_DEFAULT_REGISTRY_KEY       "${GC_UNINST_REGKEY}"
!define MULTIUSER_INSTALLMODE_DEFAULT_REGISTRY_VALUENAME "InstallLocation"
!define MULTIUSER_INSTALLMODE_INSTDIR_REGISTRY_KEY       "${GC_UNINST_REGKEY}"
!define MULTIUSER_INSTALLMODE_INSTDIR_REGISTRY_VALUENAME "InstallLocation"
!define MULTIUSER_INSTALLMODE_INSTDIR                    "Goblin Camp"
!define MULTIUSER_EXECUTIONLEVEL                         Highest
!define MULTIUSER_MUI
!define MULTIUSER_INSTALLMODE_COMMANDLINE
!include "MultiUser.nsh"
!include "MUI2.nsh"

; Installer settings
Name              "Goblin Camp ${GC_VERSION}"
OutFile           "GoblinCamp-${GC_VERSION}-Setup.exe"
CRCCheck          force
InstProgressFlags smooth
XPStyle           on
ShowInstDetails   show
ShowUnInstDetails show

; Modern UI settings
!define MUI_ABORTWARNING
!define MUI_UNABORTWARNING
!define MUI_FINISHPAGE_NOAUTOCLOSE
!define MUI_UNFINISHPAGE_NOAUTOCLOSE

; Installer pages
var ICONS_GROUP
!define MUI_FINISHPAGE_RUN "$INSTDIR\goblin-camp.exe"
!define MUI_FINISHPAGE_SHOWREADME "$INSTDIR\README.txt"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "src\COPYING.txt"
!insertmacro MULTIUSER_PAGE_INSTALLMODE
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_STARTMENU Application $ICONS_GROUP
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

; Uninstaller pages
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

; Languages
!insertmacro MUI_LANGUAGE "English"

; Sections
Section "!Goblin Camp" EXEC_SEC
    SetOutPath "$INSTDIR"
    SectionIn  RO
    
    %%_GC_INSTALL_MANIFEST_%%
    
    ; Shortcuts
    !insertmacro MUI_STARTMENU_WRITE_BEGIN Application
        CreateDirectory "$SMPROGRAMS\$ICONS_GROUP"
        CreateShortCut  "$SMPROGRAMS\$ICONS_GROUP\Goblin Camp.lnk" "$INSTDIR\goblin-camp.exe"
        CreateShortCut  "$DESKTOP\Goblin Camp.lnk" "$INSTDIR\goblin-camp.exe"
    !insertmacro MUI_STARTMENU_WRITE_END
SectionEnd

Section /o "Program Database" PDB_SEC
    SetOutPath "$INSTDIR"
    File "src\goblin-camp.pdb"
SectionEnd

Section "Visual C++ %%_GC_VCREDIST_VERSION_%% Runtime" VCREDIST_SEC
    SetOutPath "$TEMP"
    File       "src\vcredist_x86.exe"
    ExecWait   '"$TEMP\vcredist_x86.exe" /q'
    Delete     "$TEMP\vcredist_x86.exe"
SectionEnd

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${EXEC_SEC}     "Goblin Camp ${GC_VERSION}."
    !insertmacro MUI_DESCRIPTION_TEXT ${PDB_SEC}      "Debugging symbols for the GC executable."
    !insertmacro MUI_DESCRIPTION_TEXT ${VCREDIST_SEC} "Visual C++ Runtime redistributable package."
!insertmacro MUI_FUNCTION_DESCRIPTION_END

Section -AdditionalIcons
    !insertmacro MUI_STARTMENU_WRITE_BEGIN Application
        WriteIniStr    "$INSTDIR\website.url" "InternetShortcut" "URL" "http://goblincamp.com"
        CreateShortCut "$SMPROGRAMS\$ICONS_GROUP\Website.lnk"   "$INSTDIR\website.url"
        CreateShortCut "$SMPROGRAMS\$ICONS_GROUP\Uninstall.lnk" "$INSTDIR\uninst.exe"
    !insertmacro MUI_STARTMENU_WRITE_END
SectionEnd

Section -Post
    WriteUninstaller "$INSTDIR\uninst.exe"
    WriteRegStr SHCTX "${GC_UNINST_REGKEY}" "DisplayName"     "$(^Name)"
    WriteRegStr SHCTX "${GC_UNINST_REGKEY}" "UninstallString" "$INSTDIR\uninst.exe"
    WriteRegStr SHCTX "${GC_UNINST_REGKEY}" "DisplayIcon"     "$INSTDIR\goblin-camp.exe"
    WriteRegStr SHCTX "${GC_UNINST_REGKEY}" "DisplayVersion"  "${GC_VERSION}"
    WriteRegStr SHCTX "${GC_UNINST_REGKEY}" "URLInfoAbout"    "http://goblincamp.com"
    WriteRegStr SHCTX "${GC_UNINST_REGKEY}" "URLUpdateAbout"  "http://goblincamp.com"
    WriteRegStr SHCTX "${GC_UNINST_REGKEY}" "InstallLocation" "$INSTDIR"
    
    WriteRegDWORD SHCTX "${GC_UNINST_REGKEY}" "NoModify" 1
    WriteRegDWORD SHCTX "${GC_UNINST_REGKEY}" "NoRepair" 1
SectionEnd

Function .onInit
    !insertmacro MULTIUSER_INIT
FunctionEnd

Function un.onInit
    !insertmacro MULTIUSER_UNINIT
FunctionEnd

Section Uninstall
    !insertmacro MUI_STARTMENU_GETFOLDER "Application" $ICONS_GROUP
    
    Delete "$INSTDIR\goblin-camp.pdb"
    Delete "$INSTDIR\uninst.exe"
    Delete "$INSTDIR\website.url"
    
    %%_GC_UNINSTALL_MANIFEST_%%
    
    Delete "$SMPROGRAMS\$ICONS_GROUP\Uninstall.lnk"
    Delete "$SMPROGRAMS\$ICONS_GROUP\Website.lnk"
    Delete "$DESKTOP\Goblin Camp.lnk"
    Delete "$SMPROGRAMS\$ICONS_GROUP\Goblin Camp.lnk"
    
    RMDir "$SMPROGRAMS\$ICONS_GROUP"
    RMDir "$INSTDIR"
    
    DeleteRegKey SHCTX "${GC_UNINST_REGKEY}"
SectionEnd
