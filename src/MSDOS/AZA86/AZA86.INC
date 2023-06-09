
?alsp	=	0
?alptr	=	0
	if1

;”aza86.y”の出力ファイル用マクロライブラリ

;  このマクロライブラリでは以下のマクロを定義している
;    LJNC,LJC,LJNZ,LJZ,LJNP,LJP  (,LJE,LJNE)
;    CALLNC,CALLC,CALLNZ,CALLZ,CALLNP,CALLP
;    RETNC,RETC,RETNZ,RETZ,RETNP,RETP
;    SRV,ALOOP,LEND
;    LDI,LDIR,LDD,LDDR
;本来は、ほかに、CPIなども必要

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


; おまけ
lje	macro	adr
	ljz	adr
	endm
ljne	macro	adr
	ljnz	adr
	endm
;

;
; （ＡＬＯＯＰとＬＥＮＤのペアはＡｚａのＬＯＯＰ／ＬＥＮＤに相当する）
;
;　ＡＪＮＥはＪＮＥとＬＪＮＥを自動判定する，ただし前方参照に限る
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
; # ”ＡＬＯＯＰ”の定義   #
; ##########################
aloop	macro	reg,cnt
;"cnt"が存在していればそれを"reg"にセットする命令を生成する
	ifnb	<cnt>
	mov	reg,cnt
	endif
;このアドレスが繰り返しで戻ってくる場所（を定義する）
	adeflbl	?ala,%?alptr
;ＬＥＮＤでＤＥＣする命令を持つマクロ命令を作る
	adefmac	?alr,%?alptr,reg
;スタックへマクロとラベルの番号をセットする
	asetvar	?alp,%?alsp,%?alptr
;スタックを更新して，ラベルとマクロの番号も更新する
?alsp	=	?alsp+1
?alptr	=	?alptr+1
	endm

; ##########################
; # ”ＬＥＮＤ”の定義     #
; ##########################
lend	macro
;スタックポインターを１レベル戻す
?alsp	=	?alsp-1
;戻るためのラベルとマクロの格納されている番号を取り出す
	ardval	?alp,%?alsp	;?alnum=?ALP&%alsp
;ニモニック合成マクロによってＤＥＣ命令を生成する
	synmnm	?alr,%?alnum	;DEC reg
;指定回数になっていなければ繰り返す
;	ajne	?ala&%?alnum
	ljne	?ala&%?alnum
;マクロ定義を削除する
	delmac	?alr,%?alnum
	endm


;　ブロック転送命令の定義
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