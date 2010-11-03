!define VERSION "0.4.1"

Name "MultiVNC ${VERSION}"

OutFile "multivnc_${VERSION}-setup.exe"

InstallDir $PROGRAMFILES\MultiVNC

Page license
LicenseData COPYING.TXT

Page directory

Page instfiles

Section ""

  SetOutPath $INSTDIR

  File src\multivnc.exe
  File src\mingwm10.dll
  File contrib\WinVNC.exe
  File contrib\VNCHooks.dll
  File contrib\README-contrib.txt
  File NEWS.TXT	
  File README.TXT
  File TODO.TXT

  writeUninstaller $INSTDIR\multivnc-uninstall.exe

  # registry entries so this shows up in "Software"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${INSTALLATIONNAME}" "DisplayName" "MultiVNC"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${INSTALLATIONNAME}" "DisplayIcon" '"$INSTDIR\multivnc.exe"'
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${INSTALLATIONNAME}" "UninstallString" '"$INSTDIR\multivnc-uninstall.exe"'
  # just display "Remove" button
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${INSTALLATIONNAME}" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${INSTALLATIONNAME}" "NoRepair" 1


  # now the shortcuts
  CreateDirectory "$SMPROGRAMS\MultiVNC"
  createShortCut  "$SMPROGRAMS\MultiVNC\Multivnc.lnk" "$INSTDIR\multivnc.exe"
  createShortCut  "$SMPROGRAMS\MultiVNC\Readme.lnk" "$INSTDIR\README.TXT"
  createShortCut  "$SMPROGRAMS\MultiVNC\News.lnk" "$INSTDIR\NEWS.TXT"
  createShortCut  "$SMPROGRAMS\MultiVNC\Todo.lnk" "$INSTDIR\TODO.TXT"
  createShortCut  "$SMPROGRAMS\MultiVNC\Uninstall MultiVNC.lnk" "$INSTDIR\multivnc-uninstall.exe"

SectionEnd

Function .onInstSuccess
    MessageBox MB_YESNO "Installation successful! Would you like to start MultiVNC now?" IDYES NoAbort
      Abort ; causes uninstaller to quit.
    NoAbort: Exec '"$INSTDIR\MultiVNC.exe"'
FunctionEnd

section "Uninstall"
 
  # Always delete uninstaller first
  delete $INSTDIR\multivnc-uninstall.exe

  # now delete installed files
  delete $INSTDIR\multivnc.exe
  delete $INSTDIR\mingwm10.dll
  delete $INSTDIR\WinVNC.exe
  delete $INSTDIR\VNCHooks.dll
  delete $INSTDIR\README-contrib.txt
  delete $INSTDIR\NEWS.TXT
  delete $INSTDIR\README.TXT
  delete $INSTDIR\TODO.TXT
  RMDir  $INSTDIR

  # delete registry entries
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${INSTALLATIONNAME}"
  
  # delete shortcuts
  delete "$SMPROGRAMS\MultiVNC\Multivnc.lnk"
  delete "$SMPROGRAMS\MultiVNC\Readme.lnk"
  delete "$SMPROGRAMS\MultiVNC\News.lnk"
  delete "$SMPROGRAMS\MultiVNC\Todo.lnk"
  delete "$SMPROGRAMS\MultiVNC\Uninstall MultiVNC.lnk"
  RMDir  "$SMPROGRAMS\MultiVNC"
  
sectionEnd

Function un.onInit
    MessageBox MB_YESNO "This will uninstall MultiVNC. Continue?" IDYES NoAbort
      Abort ; causes uninstaller to quit.
    NoAbort:
FunctionEnd
