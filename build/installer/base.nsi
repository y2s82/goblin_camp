; Goblin Camp installer
; Use mkinstaller.py to build it.

!include "MUI2.nsh"

; Macros
!define GC_VERSION       "%%_GC_VERSION_%%"
!define GC_UNINST_REGKEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\Goblin Camp"

; Installer settings
SetCompressor     /SOLID lzma
Name              "Goblin Camp ${GC_VERSION}"
OutFile           "GoblinCamp-${GC_VERSION}.exe"
InstallDir        "$PROGRAMFILES\Goblin Camp"
InstallDirRegKey  HKLM "$GC_UNINST_REGKEY" "InstallLocation"
CRCCheck          force
InstProgressFlags smooth
ShowInstDetails   show
ShowUnInstDetails show

; Modern UI settings
!define      MUI_ABORTWARNING
!define      MUI_UNABORTWARNING
!define      MUI_FINISHPAGE_NOAUTOCLOSE
!define      MUI_UNFINISHPAGE_NOAUTOCLOSE

; Installer pages
var ICONS_GROUP
!define MUI_FINISHPAGE_RUN "$INSTDIR\goblin-camp.exe"
!define MUI_FINISHPAGE_SHOWREADME "$INSTDIR\README.txt"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "src\COPYING.txt"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_STARTMENU Application $ICONS_GROUP
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

; Uninstaller pages
!insertmacro MUI_UNPAGE_INSTFILES

; Languages
!insertmacro MUI_LANGUAGE "English"

; Sections
Section "!Goblin Camp" EXEC_SEC
    SetOutPath "$INSTDIR"
    SectionIn  RO
    
    File "src\dbghelp.dll"
    
    %%_GC_EXECUTABLES_MANIFEST_%%
    
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

Section "Data files" DATA_SEC
    SetOutPath   "$INSTDIR"
    SetOverwrite ifnewer
    
    %%_GC_DATA_FILES_MANIFEST_%%
SectionEnd

Section "Visual C++ %%_GC_VCREDIST_VERSION_%% Runtime" VCREDIST_SEC
    SetOutPath "$TEMP"
    File       "src\vcredist_x86.exe"
    ExecWait   '"$TEMP\vcredist_x86.exe" /q'
    Delete     "$TEMP\vcredist_x86.exe"
SectionEnd

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${EXEC_SEC}     "Main application."
    !insertmacro MUI_DESCRIPTION_TEXT ${PDB_SEC}      "Debugging symbols for GC executable."
    !insertmacro MUI_DESCRIPTION_TEXT ${DATA_SEC}     "Raws - may overwrite your changes."
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
    WriteRegStr HKLM "${GC_UNINST_REGKEY}" "DisplayName"     "$(^Name)"
    WriteRegStr HKLM "${GC_UNINST_REGKEY}" "UninstallString" "$INSTDIR\uninst.exe"
    WriteRegStr HKLM "${GC_UNINST_REGKEY}" "DisplayIcon"     "$INSTDIR\goblin-camp.exe"
    WriteRegStr HKLM "${GC_UNINST_REGKEY}" "DisplayVersion"  "${GC_VERSION}"
    WriteRegStr HKLM "${GC_UNINST_REGKEY}" "URLInfoAbout"    "http://goblincamp.com"
    WriteRegStr HKLM "${GC_UNINST_REGKEY}" "URLUpdateAbout"  "http://goblincamp.com"
    WriteRegStr HKLM "${GC_UNINST_REGKEY}" "InstallLocation" "$INSTDIR"
    
    WriteRegDWORD HKLM "${GC_UNINST_REGKEY}" "NoModify" 1
    WriteRegDWORD HKLM "${GC_UNINST_REGKEY}" "NoRepair" 1
SectionEnd

Function un.onUninstSuccess
    HideWindow
    MessageBox MB_ICONINFORMATION|MB_OK "$(^Name) was successfully removed from your computer."
FunctionEnd

Function un.onInit
    MessageBox MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2 "Are you sure you want to completely remove $(^Name)?" IDYES +2
        Abort
FunctionEnd

Section Uninstall
    !insertmacro MUI_STARTMENU_GETFOLDER "Application" $ICONS_GROUP
    
    Delete "$INSTDIR\dbghelp.dll"
    Delete "$INSTDIR\goblin-camp.pdb"
    Delete "$INSTDIR\Log.txt"
    Delete "$INSTDIR\uninst.exe"
    Delete "$INSTDIR\website.url"
    
    MessageBox MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2 "Do you want to REMOVE your saved games?" IDNO +2
        RMDir /r "$INSTDIR\saves"
    
    %%_GC_UNINSTALL_MANIFEST_%%
    
    Delete "$SMPROGRAMS\$ICONS_GROUP\Uninstall.lnk"
    Delete "$SMPROGRAMS\$ICONS_GROUP\Website.lnk"
    Delete "$DESKTOP\Goblin Camp.lnk"
    Delete "$SMPROGRAMS\$ICONS_GROUP\Goblin Camp.lnk"
    
    RMDir "$SMPROGRAMS\$ICONS_GROUP"
    RMDir "$INSTDIR"
    
    DeleteRegKey HKLM "${GC_UNINST_REGKEY}"
SectionEnd
