!define VERSION "0.2"

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
  File NEWS.TXT	
  File README.TXT

  writeUninstaller $INSTDIR\multivnc-uninstall.exe

  # now the shortcuts
  CreateDirectory "$SMPROGRAMS\MultiVNC"
  createShortCut  "$SMPROGRAMS\MultiVNC\Multivnc.lnk" "$INSTDIR\multivnc.exe"
  createShortCut  "$SMPROGRAMS\MultiVNC\Readme.lnk" "$INSTDIR\README.TXT"
  createShortCut  "$SMPROGRAMS\MultiVNC\News.lnk" "$INSTDIR\NEWS.TXT"
  createShortCut  "$SMPROGRAMS\MultiVNC\Uninstall MultiVNC.lnk" "$INSTDIR\multivnc-uninstall.exe"

SectionEnd 

section "Uninstall"
 
  # Always delete uninstaller first
  delete $INSTDIR\multivnc-uninstall.exe

 
  # now delete installed files
  delete $INSTDIR\multivnc.exe
  delete $INSTDIR\mingwm10.dll
  delete $INSTDIR\NEWS.TXT
  delete $INSTDIR\README.TXT
 
  # delete shortcuts
  delete "$SMPROGRAMS\MultiVNC\Multivnc.lnk"
  delete "$SMPROGRAMS\MultiVNC\Readme.lnk"
  delete "$SMPROGRAMS\MultiVNC\News.lnk"
  delete "$SMPROGRAMS\MultiVNC\Uninstall MultiVNC.lnk"
  delete "$SMPROGRAMS\MultiVNC"
  
sectionEnd

Function un.onInit
    MessageBox MB_YESNO "This will uninstall MultiVNC. Continue?" IDYES NoAbort
      Abort ; causes uninstaller to quit.
    NoAbort:
  FunctionEnd
