
; Really Simple Help Screen for Noctis G.O.E.S"
; Mainly to prefent DOS-HELP from crasing NICE"

name "N_Help_3"

org 100h

jmp start       ; jump over data declaration

msg:	
		db	"ST   SL   DL    PAR  "
		db	"WHERE     CLEAN      "
		db	"CAST CAT  REP   DELE "
		db	"PRI       PRIF       "
		db	"INBOX     OUTBOX     "
		db	"IMPORTGD             "
		db	"REPAIR    X          "
		db	24h

start:  mov     dx, msg  ; load offset of msg into dx.
        mov     ah, 09h  ; print function is 9.
        int     21h      ; do it!
        
ret ; return to operating system.

