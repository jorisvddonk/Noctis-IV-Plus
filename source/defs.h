#ifndef DEFSH
#define DEFSH
#ifdef WINDOWS
#define _argv __argv
#endif

#define WIDTH 320
#define HEIGHT 200
#define DISPLAYBYTES (WIDTH*HEIGHT)
//See noctis-2.cpp for DebugSqrt.
//#define SQRT(x) DebugSqrt((x), __FILE__, __LINE__)
#define SQRT(x) sqrt(x)

#define WEIRDDOSHILLS
	

#ifdef WINDOWS
	#define DOBUFFERCHECK
	
	#define Word short
	#define Uword unsigned short
	#define Dword int
	#define Udword unsigned int
	#define Uchar unsigned char
	#define maybefar 
	#define maybehuge
	#define Bool unsigned char
	#define FastBool int
	#define True 1
	#define False 0
	#define Either 2
	#define DOUBLE_ float
	
	#define FILEPTR FILE*
	#define NOFILE NULL
	#define OPEN_RB "rb"
	#define OPEN_RPLUS "r+b"
	//#error "32-bit mode"
	
	//extra ones
	#define Qword long long
	#define Uqword unsigned long long
#else
	//#define DOBUFFERCHECK
	
	#define Word int
	#define Uword unsigned
	#define Dword long
	#define Udword unsigned long
	#define Uchar unsigned char
	#define maybefar far
	#define maybehuge huge
	#define Bool unsigned char
	#define FastBool int
	#define True 1
	#define False 0
	#define DOUBLE_ double
	
	#define FILEPTR Word
	#define NOFILE -1
	#define OPEN_RB 0
	#define OPEN_RPLUS 4
	//#error "16-bit mode"
#endif

#ifdef WINDOWS
	#define REP_MOVSD rep movsd
	#define SHL_EAX_16 shl eax, 16
	#define REP_STOSD rep stosd
	#define SHL_EAX_1	shl eax, 1
	#define SHL_EAX_8	shl eax, 8
	#define SHL_EDX_1	shl edx, 1
	#define SHR_EDX_2	shr edx, 2
	#define SHR_EDX_16	shr edx, 16
	#define SHL_EBX_16 	shl ebx, 16
	#define SHR_EBX_16	shr ebx, 16
	#define SHL_DWORD_PTR shl dword ptr
	#define SAL_DWORD_PTR sal dword ptr
	#define DB_0x66  
	#define WONLY_BYTE_PTR byte ptr
	#define FSTP_ST fstp st(1)
	#define PUSHAD pushad
	#define POPAD popad
	
	#define MAYBE_EBP ebp
	#define MAYBE_ESI esi
	#define MAYBE_EDI edi
	#define MAYBE_EDX edx
	#define MAYBE_ECX ecx
	#define MAYBE_EBX ebx
	#define MAYBE_EAX eax
	#define MAYBE_DWORD_PTR dword ptr
	
	#define MOV_EAX_DWORD_PTR mov eax, dword ptr
	#define MOV_EBX_DWORD_PTR mov ebx, dword ptr
	#define MOV_ECX_DWORD_PTR mov ecx, dword ptr
	#define MOV_EDX_DWORD_PTR mov edx, dword ptr
	#define MOV_ESI_DWORD_PTR mov esi, dword ptr
	#define MOV_EDI_DWORD_PTR mov edi, dword ptr
	#define MOV_EBP_DWORD_PTR mov ebp, dword ptr
	#define SUB_ESI_DWORD_PTR sub esi, dword ptr
	#define SUB_EAX_DWORD_PTR sub eax, dword ptr
	#define MOV_DWORD_PTR mov dword ptr
	#define CMP_EAX_DWORD_PTR cmp eax, dword ptr
	#define INC_DWORD_PTR inc dword ptr
	#define SHL_DWORD_PTR shl dword ptr
	#define NEG_DWORD_PTR neg dword ptr
	#define ADD_DWORD_PTR add dword ptr
	#define CMP_DWORD_PTR cmp dword ptr
	
	//L_DWORD_PTR loads a pointer to var into seg:reg in DOS or just reg in windows. You'd use MAYBE_EDI or the like for reg.
	#define L_DWORD_PTR(seg, reg, var) mov reg, [var]
	//SEGVAR refers to a variable 
	#define SEGVAR(seg, index) [index]
	#define RAWSEGVAR(seg, var, index) [var+index]
	#define RIGA(index) dword ptr riga[index*2]
	#define CMP cmp
	#define SUB sub
	#define SAR sar
	
	#define FMEMMOVE(x, y, z) memmove(x,y,z)
	#define FMEMCPY(x, y, z) memcpy(x,y,z)
	#define FMEMSET(x, y, z) memset(x,y,z)
	
	#define READFILE(fh, ptr, len) fread(ptr, len, 1, fh)
	#define WRITEFILE(fh, ptr, len) fwrite(ptr, len, 1, fh)
	#define CLOSEFILE(fh) fclose(fh)
	#define OPENFILE(filename, openType) fopen(filename, openType)
	#define FILESEEK(fh, offset, type) fseek(fh, offset, type)
	#define IS_FILESEEK_VALID ==0
	#define CREATEFILE(filename) fopen(filename, "wb")
	#define FILELENGTH(outvar,file) FileLength(outvar, file)
	#define FARMALLOC(a, b) customMalloc(a, b) 
	#ifdef DOBUFFERCHECK
		#define BUFFERCHECK bufferCheck(__FILE__, __LINE__);
	#else
		#define BUFFERCHECK
	#endif
	#define FARFREE free
	#define FILETELL ftell
	#define ISFILEVALID !=NULL
	#define ISREADLEN(num) ==1
	#define MOVZX_MAYBE_ESI movzx esi
	#define MOVZX_MAYBE_EDI movzx edi
	#define MOVZX_MAYBE_EDX movzx edx
	#define MOVZX_MAYBE_ECX movzx ecx
	#define MOVZX_MAYBE_EBX movzx ebx
	#define MOVZX_MAYBE_EAX movzx eax
	#define MOVSX_MAYBE_ESI movsx esi
	#define MOVSX_MAYBE_EDI movsx edi
	#define MOVSX_MAYBE_EDX movsx edx
	#define MOVSX_MAYBE_ECX movsx ecx
	#define MOVSX_MAYBE_EBX movsx ebx
	#define MOVSX_MAYBE_EAX movsx eax
	#define IMUL_ECX_EDX imul ecx, edx
	#define IMUL_EDX imul edx
	#define AND_EDX_0x0001FFFF and edx, 0x0001FFFF
	
	#define FEOF(f) feof(f)
	
	#define LOCALVAR ebp+
	
	#define RAND my_rand
	#define RANDOM my_random
	#define SRAND my_srand
	//CLOCK() for windows is defined in noctis-2.h
#else
	#define REP_MOVSD 	db 0xf3; db 0x66; db 0xa5
	#define SHL_EAX_16	db 0x66; db 0xc1; db 0xe0; db 0x10
	#define REP_STOSD	db 0xf3; db 0x66; db 0xab
	#define SHL_EAX_1	db 0x66; shl ax, 1
	#define SHL_EAX_8	db 0x66; shl ax, 8
	#define SHL_EDX_1	db 0x66; shl dx, 1
	#define SHR_EDX_2	db 0x66; shr dx, 2
	#define SHR_EDX_16	db 0x66; shr dx, 16
	#define SHL_EBX_16	db 0x66; shl bx, 16
	#define SHR_EBX_16	db 0x66; shr bx, 16
	#define SHL_DWORD_PTR db 0x66; shl word ptr
	#define SAL_DWORD_PTR db 0x66; sal word ptr
	#define PUSHAD db 0x66; pusha
	#define POPAD db 0x66; popa
	#ifdef DOBUFFERCHECK
		#define BUFFERCHECK DebugPrintf(0, "bufferCheck(%s, %i)\n", (__FILE__), (__LINE__));
	#else
		#define BUFFERCHECK
	#endif
	
	#define DB_0x66 db 0x66
	#define WONLY_BYTE_PTR  
	#define FSTP_ST fstp st
			
	#define MAYBE_EBP bp
	#define MAYBE_ESI si
	#define MAYBE_EDI di
	#define MAYBE_EDX dx
	#define MAYBE_ECX cx
	#define MAYBE_EBX bx
	#define MAYBE_EAX ax
	#define MAYBE_DWORD_PTR word ptr
	
	#define MOV_EAX_DWORD_PTR db 0x66; mov ax, word ptr
	#define MOV_EBX_DWORD_PTR db 0x66; mov bx, word ptr
	#define MOV_ECX_DWORD_PTR db 0x66; mov cx, word ptr
	#define MOV_EDX_DWORD_PTR db 0x66; mov dx, word ptr
	#define MOV_ESI_DWORD_PTR db 0x66; mov si, word ptr
	#define MOV_EDI_DWORD_PTR db 0x66; mov di, word ptr
	#define MOV_EBP_DWORD_PTR db 0x66; mov bp, word ptr
	#define SUB_ESI_DWORD_PTR db 0x66; sub si, word ptr
	#define SUB_EAX_DWORD_PTR db 0x66; sub ax, word ptr
	#define MOV_DWORD_PTR db 0x66; mov word ptr
	#define CMP_EAX_DWORD_PTR db 0x66; cmp ax, word ptr
	#define INC_DWORD_PTR db 0x66; inc word ptr
	#define SHL_DWORD_PTR db 0x66; shl word ptr
	#define NEG_DWORD_PTR db 0x66; neg word ptr
	#define ADD_DWORD_PTR db 0x66; add word ptr
	#define CMP_DWORD_PTR db 0x66; cmp word ptr
	#define L_DWORD_PTR(seg, reg, var) l##seg reg, dword ptr var
	#define SEGVAR(seg, index) seg:[index]
	#define RAWSEGVAR(seg, var, index) seg:[index+4]
	#define RIGA(index) word ptr riga[index]
	#define CMP db 0x66; cmp
	#define SUB db 0x66; sub
	#define SAR db 0x66; sar
	
	#define FMEMMOVE(x, y, z) _fmemmove(x,y,z)
	#define FMEMCPY(x, y, z) _fmemcpy(x,y,z)
	#define FMEMSET(x, y, z) _fmemset(x,y,z)
	
	#define READFILE(fh, ptr, len) _read(fh, ptr, len)
	#define WRITEFILE(fh, ptr, len) _write(fh, ptr, len)
	#define CLOSEFILE(fh) _close(fh)
	#define OPENFILE(filename, openType) _open(filename, openType)
	#define FILESEEK(fh, offset, type) lseek(fh, offset, type)
	#define IS_FILESEEK_VALID >-1
	#define CREATEFILE(filename) _creat(filename, 0)
	#define FILELENGTH(outvar,file) outvar=filelength(file)
	#define FARMALLOC(a,b) farmalloc(a)
	#define FARFREE farfree
	#define FILETELL tell
	#define ISFILEVALID >-1
	#define ISREADLEN(num) ==num
	#define MOVZX_MAYBE_ESI mov si
	#define MOVZX_MAYBE_EDI mov di
	#define MOVZX_MAYBE_EDX mov dx
	#define MOVZX_MAYBE_ECX mov cx
	#define MOVZX_MAYBE_EBX mov bx
	#define MOVZX_MAYBE_EAX mov ax
	#define MOVSX_MAYBE_ESI mov si
	#define MOVSX_MAYBE_EDI mov di
	#define MOVSX_MAYBE_EDX mov dx
	#define MOVSX_MAYBE_ECX mov cx
	#define MOVSX_MAYBE_EBX mov bx
	#define MOVSX_MAYBE_EAX mov ax
	#define IMUL_ECX_EDX db 0x66, 0x0F, 0xAF, 0xCA // imul ecx, edx
	#define IMUL_EDX db 0x66; imul dx
	#define AND_EDX_0x0001FFFF db 0x66, 0x81, 0xE2, 0xFF, 0xFF, 0x01, 0x00 // and edx, 0x0001FFFF
	
	#define FEOF(f) eof(f)
	#define LOCALVAR 
	
	#define RAND rand
	#define RANDOM random
	#define SRAND srand
	#define CLOCK() clock()
#endif

#endif