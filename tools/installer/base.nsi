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
OutFile           "GoblinCamp-${GC_VERSION}-Setup-%%_GC_PLATFORM_%%.exe"
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
!insertmacro MUI_PAGE_LICENSE "%%_GC_SOURCE_%%\COPYING.txt"
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

!macro VCRedist Version
    !ifdef GC_BUNDLE_MSVC${Version}
        Section "Visual C++ ${Version} Runtime (%%_GC_PLATFORM_%%)" VCRED${Version}_SEC
            SetOutPath "$TEMP"
            File       "%%_GC_REDIST_%%\vc${Version}\vcredist_%%_GC_PLATFORM_%%.exe"
            ExecWait   '"$TEMP\vcredist_%%_GC_PLATFORM_%%.exe" /q'
            Delete     "$TEMP\vcredist_%%_GC_PLATFORM_%%.exe"
        SectionEnd
    !endif
!macroend

!insertmacro VCRedist 2008
!insertmacro VCRedist 2010

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${EXEC_SEC} "Goblin Camp ${GC_VERSION} (%%_GC_PLATFORM_%% build)."
    !ifdef GC_BUNDLE_MSVC2008
        !insertmacro MUI_DESCRIPTION_TEXT ${VCRED2008_SEC} "Visual C++ 2008 Runtime redistributable package."
    !endif
    !ifdef GC_BUNDLE_MSVC2010
        !insertmacro MUI_DESCRIPTION_TEXT ${VCRED2010_SEC} "Visual C++ 2010 Runtime redistributable package."
    !endif
!insertmacro MUI_FUNCTION_DESCRIPTION_END

Section -AdditionalIcons
    !insertmacro MUI_STARTMENU_WRITE_BEGIN Application
        WriteIniStr    "$SMPROGRAMS\$ICONS_GROUP\Website.url"   "InternetShortcut" "URL" "http://www.goblincamp.com/"
        WriteIniStr    "$SMPROGRAMS\$ICONS_GROUP\Forums.url"    "InternetShortcut" "URL" "http://www.goblincamp.com/forum/"
        CreateShortCut "$SMPROGRAMS\$ICONS_GROUP\Uninstall.lnk" "$INSTDIR\uninst.exe"
    !insertmacro MUI_STARTMENU_WRITE_END
SectionEnd

Section -Post
    WriteUninstaller "$INSTDIR\uninst.exe"
    WriteRegStr SHCTX "${GC_UNINST_REGKEY}" "DisplayName"     "$(^Name)"
    WriteRegStr SHCTX "${GC_UNINST_REGKEY}" "UninstallString" "$INSTDIR\uninst.exe"
    WriteRegStr SHCTX "${GC_UNINST_REGKEY}" "DisplayIcon"     "$INSTDIR\goblin-camp.exe"
    WriteRegStr SHCTX "${GC_UNINST_REGKEY}" "DisplayVersion"  "${GC_VERSION}"
    WriteRegStr SHCTX "${GC_UNINST_REGKEY}" "URLInfoAbout"    "http://www.goblincamp.com"
    WriteRegStr SHCTX "${GC_UNINST_REGKEY}" "URLUpdateAbout"  "http://www.goblincamp.com"
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
    
    Delete "$INSTDIR\uninst.exe"
    
    %%_GC_UNINSTALL_MANIFEST_%%
    %%_GC_UNINSTALL_MANIFEST_DIRS_%%
    
    Delete "$SMPROGRAMS\$ICONS_GROUP\Uninstall.lnk"
    Delete "$SMPROGRAMS\$ICONS_GROUP\Website.url"
    Delete "$SMPROGRAMS\$ICONS_GROUP\Forums.url"
    Delete "$DESKTOP\Goblin Camp.lnk"
    Delete "$SMPROGRAMS\$ICONS_GROUP\Goblin Camp.lnk"
    
    RMDir "$SMPROGRAMS\$ICONS_GROUP"
    RMDir "$INSTDIR"
    
    DeleteRegKey SHCTX "${GC_UNINST_REGKEY}"
SectionEnd
