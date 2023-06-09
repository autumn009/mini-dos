
?alsp	=	0
?alptr	=	0
	if1

;�haza86.y�h�̏o�̓t�@�C���p�}�N�����C�u����

;  ���̃}�N�����C�u�����ł͈ȉ��̃}�N�����`���Ă���
;    LJNC,LJC,LJNZ,LJZ,LJNP,LJP  (,LJE,LJNE)
;    CALLNC,CALLC,CALLNZ,CALLZ,CALLNP,CALLP
;    RETNC,RETC,RETNZ,RETZ,RETNP,RETP
;    SRV,ALOOP,LEND
;    LDI,LDIR,LDD,LDDR
;�{���́A�ق��ɁACPI�Ȃǂ��K�v

srv	macro	adr
	call	adr
	endm

;  DEFINE LJxx / CALLxx INSTRUCTIONS
lj_def	macro	mnem,jncond
mnem	macro	adr
	local	skip
	jncond	skip
	jmp	adr
skip:
	endm
	endm
lc_def	macro	mnem,jncond
mnem	macro	adr
	local	skip
	jncond	skip
	call	adr
skip:
	endm
	endm
lr_def	macro	mnem,jncond
mnem	macro
	local	skip
	jncond	skip
	ret
skip:
	endm
	endm
	lr_def	<retnc>,<jc>
	lr_def	<retc>,<jnc>
	lr_def	<retnz>,<jz>
	lr_def	<retz>,<jnz>
	lr_def	<retp>,<jnp>
	lr_def	<retnp>,<jp>
	lc_def	<callnc>,<jc>
	lc_def	<callc>,<jnc>
	lc_def	<callnz>,<jz>
	lc_def	<callz>,<jnz>
	lc_def	<callp>,<jnp>
	lc_def	<callnp>,<jp>
	lj_def	<ljnc>,<jc>
	lj_def	<ljc>,<jnc>
	lj_def	<ljnz>,<jz>
	lj_def	<ljz>,<jnz>
	lj_def	<ljp>,<jnp>
	lj_def	<ljnp>,<jp>


; ���܂�
lje	macro	adr
	ljz	adr
	endm
ljne	macro	adr
	ljnz	adr
	endm
;

;
; �i�`�k�n�n�o�Ƃk�d�m�c�̃y�A�͂`�����̂k�n�n�o�^�k�d�m�c�ɑ�������j
;
;�@�`�i�m�d�͂i�m�d�Ƃk�i�m�d���������肷��C�������O���Q�ƂɌ���
ajne	macro	adr
	local	nowptr
nowptr	label	near
	if	adr-(nowptr+2) ge -128
	jne	adr
	else
	ljne	adr
	endif
	endm
defmac0	macro	nam,reg
nam	macro
	dec	reg
	endm
	endm
adefmac	macro	nam,offs,reg
	defmac0	nam&offs,reg
	endm
synmnm	macro	nam,offs
	nam&offs
	endm
delmac	macro	nam,offs
	purge	nam&offs
	endm
adeflbl	macro	nam,cnt
nam&cnt	label	near
	endm
asetvar	macro	nam,offs,val
nam&offs	=	val
	endm
ardval	macro	nam,offs
?alnum	=	nam&offs
	endm
; ##########################
; # �h�`�k�n�n�o�h�̒�`   #
; ##########################
aloop	macro	reg,cnt
;"cnt"�����݂��Ă���΂����"reg"�ɃZ�b�g���閽�߂𐶐�����
	ifnb	<cnt>
	mov	reg,cnt
	endif
;���̃A�h���X���J��Ԃ��Ŗ߂��Ă���ꏊ�i���`����j
	adeflbl	?ala,%?alptr
;�k�d�m�c�łc�d�b���閽�߂����}�N�����߂����
	adefmac	?alr,%?alptr,reg
;�X�^�b�N�փ}�N���ƃ��x���̔ԍ����Z�b�g����
	asetvar	?alp,%?alsp,%?alptr
;�X�^�b�N���X�V���āC���x���ƃ}�N���̔ԍ����X�V����
?alsp	=	?alsp+1
?alptr	=	?alptr+1
	endm

; ##########################
; # �h�k�d�m�c�h�̒�`     #
; ##########################
lend	macro
;�X�^�b�N�|�C���^�[���P���x���߂�
?alsp	=	?alsp-1
;�߂邽�߂̃��x���ƃ}�N���̊i�[����Ă���ԍ������o��
	ardval	?alp,%?alsp	;?alnum=?ALP&%alsp
;�j���j�b�N�����}�N���ɂ���Ăc�d�b���߂𐶐�����
	synmnm	?alr,%?alnum	;DEC reg
;�w��񐔂ɂȂ��Ă��Ȃ���ΌJ��Ԃ�
;	ajne	?ala&%?alnum
	ljne	?ala&%?alnum
;�}�N����`���폜����
	delmac	?alr,%?alnum
	endm


;�@�u���b�N�]�����߂̒�`
ldi	macro
	xchg	si,bx
	xchg	di,dx
	cld
	movsb
	dec	cx
	xchg	si,bx
	xchg	di,dx
	endm

ldir	macro
	xchg	si,bx
	xchg	di,dx
	cld
	rep movsb
	xchg	si,bx
	xchg	di,dx
	endm

ldd	macro
	xchg	si,bx
	xchg	di,dx
	std
	movsb
	dec	cx
	xchg	si,bx
	xchg	di,dx
	endm

lddr	macro
	xchg	si,bx
	xchg	di,dx
	std
	rep movsb
	xchg	si,bx
	xchg	di,dx
	endm


	endif	;{if 1}
