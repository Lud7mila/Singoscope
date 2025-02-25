
# Чтобы собрать версию без отладочного кода, в командной строке наберите команду
#  nmake RELEASE=

!IFDEF RELEASE
OUTDIR=Release
!ELSE
OUTDIR=Debug
CL_OPTIONS=/D "DEBUGLOG"
!ENDIF

CL_OPTIONS=$(CL_OPTIONS) /MT /O2 /Gr /fp:fast /QIfist /GR- /GS- /W3 /c\
 /D "WIN32" /D "_WIN32_WINNT=0x0500" /D "UNICODE" /D "_UNICODE" /nologo /errorReport:none

LINK_OPTIONS=/subsystem:windows /nodefaultlib /incremental:no /nologo /errorReport:none\
 /ltcg fp10.obj nothrownew.obj libcmt.lib\
 kernel32.lib user32.lib gdi32.lib advapi32.lib comdlg32.lib winmm.lib msimg32.lib\
 ddraw.lib dxguid.lib

$(OUTDIR)\Singoscope.exe:	$(OUTDIR)\FrameWnd.obj\
                            $(OUTDIR)\Log.obj\
							$(OUTDIR)\Main.obj\
							$(OUTDIR)\MidiFile.obj\
							$(OUTDIR)\MidiLibrary.obj\
							$(OUTDIR)\MidiLyric.obj\
							$(OUTDIR)\MidiPart.obj\
							$(OUTDIR)\MidiSong.obj\
							$(OUTDIR)\MidiTrack.obj\
							$(OUTDIR)\ScrollbarWnd.obj\
                            $(OUTDIR)\ShowError.obj\
                            $(OUTDIR)\SimpleStaveFast.obj\
                            $(OUTDIR)\Song.obj\
                            $(OUTDIR)\SongFile.obj\
                            $(OUTDIR)\StaveWnd.obj\
                            $(OUTDIR)\Statistics.obj\
                            $(OUTDIR)\TextMessages.obj\
                            $(OUTDIR)\ToolbarWnd.obj\
                            $(OUTDIR)\Video.obj\
                            $(OUTDIR)\Resources.res
	link $(LINK_OPTIONS) /out:$@ $**

.cpp{$(OUTDIR)}.obj:
	cl $(CL_OPTIONS) /Fo"$(OUTDIR)/" $**

.rc{$(OUTDIR)}.res:
	rc /fo $@ $**

*.cpp: *.h

*.rc: *.h *.manifest
