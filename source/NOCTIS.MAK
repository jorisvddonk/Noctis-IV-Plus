.AUTODEPEND

.PATH.obj = .\BIN

#		*Translator Definitions*
CC = bcc +NOCTIS.CFG
TASM = tasm
TLIB = tlib
TLINK = tlink
LIBPATH = C:\BC.31\LIB
INCLUDEPATH = C:\BC.31\INCLUDE


#		*Implicit Rules*
.c.obj:
  $(CC) -c {$< }

.cpp.obj:
  $(CC) -c {$< }

#		*List Macros*


EXE_dependencies =  \
 noctis-0.obj \
 noctis-1.obj \
 noctis.obj

#		*Explicit Rules*
turbo assembler: noctis.cfg $(EXE_dependencies)
  $(TLINK) /x/i/n/c/P-/A=16/L$(LIBPATH) @&&|
c0l.obj+
.\bin\noctis-0.obj+
.\bin\noctis-1.obj+
.\bin\noctis.obj
.\bin\noctis
		# no map file
fp87.lib+
mathl.lib+
cl.lib
|


#		*Individual File Dependencies*
noctis-0.obj: noctis.cfg noctis-0.cpp 

noctis-1.obj: noctis.cfg noctis-1.cpp 

noctis.obj: noctis.cfg noctis.cpp 

#		*Compiler Configuration File*
noctis.cfg: noctis.mak
  copy &&|
-ml
-3
-f287
-O
-Oe
-Ob
-Z
-k-
-h
-vi-
-wamp
-wuse
-weas
-wpre
-n.\BIN
-I$(INCLUDEPATH)
-L$(LIBPATH)
| noctis.cfg


