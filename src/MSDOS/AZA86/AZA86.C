#include <yaccdefs.h>

#include <alib.h>
#include <string.h>
#define GEN_FATAL_ERROR_HANDLER
#include <sch3.h>
#include <ctype.h>


/*　このマクロをｄｅｆｉｎｅしておくと、ＪＲも１６ｂｉｔオフセットの　*/
/*　ｊｍｐに変換する　*/
#define	ALL_LONG_JUMP

#define ungetchar(ch) ungetc( ch, stdin );

int yyparse( void );

typedef	struct	node	{
	char 	value[80];
	int	number;
	struct node * next;
}	NODE;


typedef NODE * NODE_p;
#define YYSTYPE NODE_p


NODE_p root_of_nodes = NULL;



void charcat( char * buf, int ch )
{
	char * cp;
	cp = strchr( buf, '\0' );
	*(cp+0) = ch;
	*(cp+1) = '\0';
}


NODE_p make_node( void )
{
	NODE_p np;
	np = malloc_with_errchk( sizeof( NODE ) );
	np->next = root_of_nodes;
	root_of_nodes = np;
	return( np );
}


NODE_p make_const_node(char * val)
{
	NODE_p ans;
	ans = make_node();
	strcpy( ans->value, val );
	return( ans );
}


/*　作成したＮＯＤＥを回収する（主枝以外）　*/
void release_nodes_sub( NODE_p free_nodes )
{
	NODE_p np, np_next;
	np = free_nodes;
	while( np != NULL ){
		np_next = np->next;
		free( np );
		np = np_next;
	}
}

/*　作成したＮＯＤＥを回収する（主枝のみ）　*/
void release_nodes( void )
{
	NODE_p np, np_next;
	np = root_of_nodes;
	while( np != NULL ){
		np_next = np->next;
		free( np );
		np = np_next;
	}
	root_of_nodes = NULL;
}



/* ｋｅｅｐ処理用　*/
int number_of_klist;
NODE_p * kptr;
NODE_p klist_buf[10];

typedef struct keep_stack0 {
	NODE_p klist_buf[10];
	int number_of_list;
	NODE_p keep_nodes;
	struct keep_stack0 * last;
} KEEP_STACK;

KEEP_STACK * root_of_keep_stack = NULL;



void push_keep( void )
{
	int i;

	KEEP_STACK * kp;
	kp = malloc_with_errchk( sizeof( KEEP_STACK ) );
	kp->number_of_list = number_of_klist;
	for( i=0; i<number_of_klist; i++ ){
		kp->klist_buf[i] = klist_buf[i];
	}
	kp->last = root_of_keep_stack;
	kp->keep_nodes = root_of_nodes;
	root_of_nodes = NULL;
	root_of_keep_stack = kp;
}

void pop_keep( void )
{
	KEEP_STACK * kp;

	kp = root_of_keep_stack;
	if( kp == NULL ) {
		printf( "ｋｅｅｐ疑似命令スタックがアンダーフローしました\n");
		exit(4);
	}
	release_nodes_sub( kp->keep_nodes );
	root_of_keep_stack = kp->last;
	free( kp );
}




static char expr_buf[256];
static char lbl[256];
static char mnem[256];
static char operand[256];
static char comm[256];
static char elist_buf[256];
static char buf[256];		/*雑バッファ*/
static int id_to_number[NAMES];


void line_putout( void )
{
	int l;

	/*ラベルフィールドの出力を行なう*/
	l = strlen( lbl );
	if( l > 0 ){
		printf( "%s:", lbl );
		l++;
	}
	/*常に、ｔａｂコードを挟んで１６カラムのスペーシングを行なう*/
	putchar( '\t' );
	if( l < 8 ) putchar( '\t' );

	/*命令フィールドの出力を行なう*/
	printf("%s\t",mnem);

	/*オペランドフィールドの出力を行なう*/
	printf("%s",operand);

	/*コメントフィールドの出力を行なう*/
	if( strlen( comm ) > 0 ){
		l = strlen( operand );
		putchar( '\t' );
		if( l < 8 ) putchar( '\t' );
		if( l < 16 ) putchar( '\t' );
		printf("%s", comm );
	}
	putchar('\n');

	/*自動クリアさせる*/
	strcpy( lbl,     "" );
	strcpy( mnem,    "" );
	strcpy( operand, "" );
	strcpy( comm,    "" );
}


void line_putout_with_release_nodes( void )
{
	line_putout();
	release_nodes();
}



/* 変換不能命令に対するコメントの出力 */
void unconvertable( char * instruction, char * comment )
{
	printf(";Unconvertable [%s] %s\n",instruction, comment );
	strcpy( mnem, "" );
	strcpy( operand, "" );
	line_putout_with_release_nodes();
}


/*ｅｑｕだけ、別処理となる（ラベルのあとに「：」を必ず入れない）*/
void equ_putout( NODE_p label, NODE_p val )
{
	int l;

	/*ラベルフィールドの出力を行なう*/
	printf( "%s", label->value );
	/*常に、ｔａｂコードを挟んで１６カラムのスペーシングを行なう*/
	l = strlen( label->value );
	putchar( '\t' );
	if( l < 8 ) putchar( '\t' );

	/*命令フィールドの出力を行なう*/
	printf("EQU\t");

	/*オペランドフィールドの出力を行なう*/
	printf("%s",val->value);

	/*コメントフィールドの出力を行なう*/
	if( strlen( comm ) > 0 ){
		l = strlen( val->value );
		putchar( '\t' );
		if( l < 8 ) putchar( '\t' );
		if( l < 16 ) putchar( '\t' );
		printf("%s", comm );
	}
	putchar('\n');

	/*自動クリアさせる*/
	strcpy( lbl,     "" );
	strcpy( mnem,    "" );
	strcpy( operand, "" );
	strcpy( comm,    "" );
}




char * reg_name( NODE_p node );
char * cond_name( NODE_p cond );
char * mnm_name( NODE_p node );


#ifndef	YYSTYPE
#define	YYSTYPE	int
#endif
#define	YYLNK	YYBASE
#define	YYEOF	(YYBASE + 1)
#define	YYERR	(YYBASE + 2)
#define	label	(YYBASE + 3)
#define	const	(YYBASE + 4)
#define	str_const	(YYBASE + 5)
#define	comment	(YYBASE + 6)
#define	KW_A	(YYBASE + 7)
#define	KW_B	(YYBASE + 8)
#define	KW_C	(YYBASE + 9)
#define	KW_D	(YYBASE + 10)
#define	KW_E	(YYBASE + 11)
#define	KW_H	(YYBASE + 12)
#define	KW_L	(YYBASE + 13)
#define	KW_I	(YYBASE + 14)
#define	KW_R	(YYBASE + 15)
#define	KW_IX	(YYBASE + 16)
#define	KW_IY	(YYBASE + 17)
#define	KW_HL	(YYBASE + 18)
#define	KW_DE	(YYBASE + 19)
#define	KW_BC	(YYBASE + 20)
#define	KW_SP	(YYBASE + 21)
#define	KW_AF	(YYBASE + 22)
#define	KW_AF_DUSH	(YYBASE + 23)
#define	KW_NC	(YYBASE + 24)
#define	KW_Z	(YYBASE + 25)
#define	KW_NZ	(YYBASE + 26)
#define	KW_P	(YYBASE + 27)
#define	KW_M	(YYBASE + 28)
#define	KW_PE	(YYBASE + 29)
#define	KW_PO	(YYBASE + 30)
#define	KW_EQU	(YYBASE + 31)
#define	KW_LDI	(YYBASE + 32)
#define	KW_LDIR	(YYBASE + 33)
#define	KW_LDD	(YYBASE + 34)
#define	KW_LDDR	(YYBASE + 35)
#define	KW_CPI	(YYBASE + 36)
#define	KW_CPIR	(YYBASE + 37)
#define	KW_CPD	(YYBASE + 38)
#define	KW_CPDR	(YYBASE + 39)
#define	KW_LD	(YYBASE + 40)
#define	KW_PUSH	(YYBASE + 41)
#define	KW_POP	(YYBASE + 42)
#define	KW_EX	(YYBASE + 43)
#define	KW_ADD	(YYBASE + 44)
#define	KW_SUB	(YYBASE + 45)
#define	KW_ADC	(YYBASE + 46)
#define	KW_SBC	(YYBASE + 47)
#define	KW_AND	(YYBASE + 48)
#define	KW_OR	(YYBASE + 49)
#define	KW_XOR	(YYBASE + 50)
#define	KW_CP	(YYBASE + 51)
#define	KW_INC	(YYBASE + 52)
#define	KW_DEC	(YYBASE + 53)
#define	KW_DAA	(YYBASE + 54)
#define	KW_CPL	(YYBASE + 55)
#define	KW_NEG	(YYBASE + 56)
#define	KW_CCF	(YYBASE + 57)
#define	KW_SCF	(YYBASE + 58)
#define	KW_NOP	(YYBASE + 59)
#define	KW_HALT	(YYBASE + 60)
#define	KW_DI	(YYBASE + 61)
#define	KW_EI	(YYBASE + 62)
#define	KW_IM	(YYBASE + 63)
#define	KW_RLCA	(YYBASE + 64)
#define	KW_RLA	(YYBASE + 65)
#define	KW_RRCA	(YYBASE + 66)
#define	KW_RRA	(YYBASE + 67)
#define	KW_RLC	(YYBASE + 68)
#define	KW_RL	(YYBASE + 69)
#define	KW_RRC	(YYBASE + 70)
#define	KW_RR	(YYBASE + 71)
#define	KW_SLA	(YYBASE + 72)
#define	KW_SRA	(YYBASE + 73)
#define	KW_SRL	(YYBASE + 74)
#define	KW_RLD	(YYBASE + 75)
#define	KW_RRD	(YYBASE + 76)
#define	KW_BIT	(YYBASE + 77)
#define	KW_SET	(YYBASE + 78)
#define	KW_RES	(YYBASE + 79)
#define	KW_JP	(YYBASE + 80)
#define	KW_JR	(YYBASE + 81)
#define	KW_CALL	(YYBASE + 82)
#define	KW_RET	(YYBASE + 83)
#define	KW_DJNZ	(YYBASE + 84)
#define	KW_RETI	(YYBASE + 85)
#define	KW_RETN	(YYBASE + 86)
#define	KW_RST	(YYBASE + 87)
#define	KW_IN	(YYBASE + 88)
#define	KW_INI	(YYBASE + 89)
#define	KW_INIR	(YYBASE + 90)
#define	KW_IND	(YYBASE + 91)
#define	KW_INDR	(YYBASE + 92)
#define	KW_OUT	(YYBASE + 93)
#define	KW_OUTI	(YYBASE + 94)
#define	KW_OTIR	(YYBASE + 95)
#define	KW_OUTD	(YYBASE + 96)
#define	KW_OTDR	(YYBASE + 97)
#define	KW_DB	(YYBASE + 98)
#define	KW_DW	(YYBASE + 99)
#define	KW_DS	(YYBASE + 100)
#define	KW_DEFB	(YYBASE + 101)
#define	KW_DEFW	(YYBASE + 102)
#define	KW_DEFS	(YYBASE + 103)
#define	KW_ORG	(YYBASE + 104)
#define	KW_END	(YYBASE + 105)
#define	KW_INCLUDE	(YYBASE + 106)
#define	KW_TITLE	(YYBASE + 107)
#define	KW_SUBTTL	(YYBASE + 108)
#define	KW_LOOP	(YYBASE + 109)
#define	KW_LEND	(YYBASE + 110)
#define	KW_KEEP	(YYBASE + 111)
#define	KW_KEEPOUT	(YYBASE + 112)
YYSTYPE	*yyasp, yylval, yyval;
extern	void	yydef();
char	yyerrflag = 0;
#define	yyerrok	yyerrflag = 0
int	yytoken = 0;
#define	yyclearin	yytoken = 0
#define	yya1	yydef
#define	yya2	yydef
void	yya3()
{
		strcpy( mnem, "MOV" );
		sprintf( operand, "%s,%s", reg_name(yyasp[4]), reg_name(yyasp[2]) );
		line_putout_with_release_nodes();
	}
void	yya4()
{
		strcpy( mnem, "MOV" );
		sprintf( operand, "%s,offset %s", reg_name(yyasp[4]), yyasp[2] );
		line_putout_with_release_nodes();
	}
void	yya5()
{
		strcpy( mnem, "MOV" );
		sprintf( operand, "%s,byte ptr %s", reg_name(yyasp[4]), yyasp[2]->value);
		line_putout_with_release_nodes();
	}
void	yya6()
{
		strcpy( mnem, "MOV" );
		sprintf( operand, "byte ptr %s,%s", yyasp[4]->value, reg_name(yyasp[2]) );
		line_putout_with_release_nodes();
	}
void	yya7()
{
		strcpy( mnem, "MOV" );
		sprintf( operand, "byte ptr %s,offset %s", yyasp[4]->value, yyasp[2]->value);
		line_putout_with_release_nodes();
	}
void	yya8()
{
		strcpy( mnem, "XCHG" );
		sprintf( operand, "BX,%s",reg_name( yyasp[3] ) );
		line_putout();
		strcpy( mnem, "MOV" );
		sprintf( operand, "%s,byte ptr [BX]",reg_name( yyasp[6] ) );
		line_putout();
		strcpy( mnem, "XCHG" );
		sprintf( operand, "BX,%s",reg_name( yyasp[3] ) );
		line_putout_with_release_nodes();
	}
void	yya9()
{
		strcpy( mnem, "MOV" );
		sprintf( operand, "%s,byte ptr [%s]", reg_name(yyasp[6]), yyasp[3]->value);
		line_putout_with_release_nodes();
	}
void	yya10()
{
		strcpy( mnem, "XCHG" );
		sprintf( operand, "BX,%s",reg_name( yyasp[5] ) );
		line_putout();
		strcpy( mnem, "MOV" );
		sprintf( operand, "byte ptr [BX],%s",reg_name( yyasp[2] ) );
		line_putout();
		strcpy( mnem, "XCHG" );
		sprintf( operand, "BX,%s",reg_name( yyasp[5] ) );
		line_putout_with_release_nodes();
	}
void	yya11()
{
		strcpy( mnem, "MOV" );
		sprintf( operand, "byte ptr [%s],%s", yyasp[5]->value, reg_name(yyasp[2]) );
		line_putout_with_release_nodes();
	}
void	yya12()
{
		unconvertable( "LD A,I","Ｉレジスタは変換できません" );
	}
void	yya13()
{
		unconvertable( "LD I,A","Ｉレジスタは変換できません" );
	}
void	yya14()
{
		unconvertable( "LD A,R","Ｒレジスタは変換できません" );
	}
void	yya15()
{
		unconvertable( "LD R,A","Ｒレジスタは変換できません" );
	}
void	yya16()
{
		strcpy( mnem, "MOV" );
		sprintf( operand, "%s,offset %s", reg_name(yyasp[4]), yyasp[2] );
		line_putout_with_release_nodes();
	}
void	yya17()
{
		strcpy( mnem, "MOV" );
		sprintf( operand, "%s,word ptr [%s]", reg_name(yyasp[6]), yyasp[3] );
		line_putout_with_release_nodes();
	}
void	yya18()
{
		strcpy( mnem, "MOV" );
		sprintf( operand, "word ptr [%s],%s", yyasp[5], reg_name(yyasp[2]) );
		line_putout_with_release_nodes();
	}
void	yya19()
{
		strcpy( mnem, "MOV" );
		sprintf( operand, "%s,%s", reg_name(yyasp[4]), reg_name(yyasp[2]) );
		line_putout_with_release_nodes();
	}
void	yya20()
{
		strcpy( mnem, "PUSH" );
		strcpy( operand, reg_name(yyasp[2]) );
		line_putout_with_release_nodes();
	}
void	yya21()
{
		strcpy( mnem, "POP" );
		strcpy( operand, reg_name(yyasp[2]) );
		line_putout_with_release_nodes();
	}
void	yya22()
{
		strcpy( mnem, "LAHF" );
		strcpy( operand, "" );
		line_putout();
		strcpy( mnem, "XCHG" );
		strcpy( operand, "AL,AH" );
		line_putout();
		strcpy( mnem, "PUSH" );
		strcpy( operand, "AX" );
		line_putout_with_release_nodes();
	}
void	yya23()
{
		strcpy( mnem, "POP" );
		strcpy( operand, "AX" );
		line_putout();
		strcpy( mnem, "XCHG" );
		strcpy( operand, "AL,AH" );
		line_putout();
		strcpy( mnem, "SAHF" );
		strcpy( operand, "" );
		line_putout_with_release_nodes();
	}
void	yya24()
{
		strcpy( mnem, "XCHG" );
		sprintf( operand, "%s,%s", reg_name(yyasp[4]), reg_name(yyasp[2]) );
		line_putout_with_release_nodes();
	}
void	yya25()
{
		unconvertable( "EX AF,AF\'","裏レジスタは存在しません" );
	}
void	yya26()
{
		strcpy( mnem, "XCHG" );
		sprintf( operand, "word ptr [%s],%s", reg_name(yyasp[5]), reg_name(yyasp[2]) );
		line_putout_with_release_nodes();
	}
void	yya27()
{
		strcpy( mnem, yyasp[2]->value );
		line_putout_with_release_nodes();
	}
void	yya28()
{
		strcpy( mnem, yyasp[5]->value );
		sprintf( operand, "AL,%s", reg_name(yyasp[2]) );
		line_putout_with_release_nodes();
	}
void	yya29()
{
		strcpy( mnem, mnm_name(yyasp[5]) );
		sprintf( operand, "AL,offset %s", yyasp[2]->value );
		line_putout_with_release_nodes();
	}
void	yya30()
{
		strcpy( mnem, mnm_name(yyasp[5]) );
		sprintf( operand, "AL,byte ptr %s", yyasp[2]->value );
		line_putout_with_release_nodes();
	}
void	yya31()
{
		strcpy( mnem, mnm_name(yyasp[3]) );
		sprintf( operand, "AL,%s", reg_name(yyasp[2]) );
		line_putout_with_release_nodes();
	}
void	yya32()
{
		strcpy( mnem, mnm_name(yyasp[3]) );
		sprintf( operand, "AL,offset %s", yyasp[2]->value );
		line_putout_with_release_nodes();
	}
void	yya33()
{
		strcpy( mnem, mnm_name(yyasp[3]) );
		sprintf( operand, "AL,byte ptr %s", yyasp[2]->value );
		line_putout_with_release_nodes();
	}
void	yya34()
{
		strcpy( mnem, mnm_name(yyasp[3]) );
		sprintf( operand, "%s", reg_name(yyasp[2]) );
		line_putout_with_release_nodes();
	}
void	yya35()
{
		strcpy( mnem, mnm_name(yyasp[3]) );
		sprintf( operand, "byte ptr %s", yyasp[2]->value );
		line_putout_with_release_nodes();
	}
void	yya36()
{
		strcpy( mnem, mnm_name(yyasp[5]) );
		sprintf( operand, "%s,%s", reg_name(yyasp[4]), reg_name(yyasp[2]) );
		line_putout_with_release_nodes();
	}
void	yya37()
{
		strcpy( mnem, mnm_name(yyasp[3]) );
		sprintf( operand, "%s", reg_name(yyasp[2]) );
		line_putout_with_release_nodes();
	}
void	yya38()
{
		unconvertable( "DAA","not supported" );
	}
void	yya39()
{
		strcpy( mnem, "NOT" );
		strcpy( operand, "AL" );
		line_putout_with_release_nodes();
	}
void	yya40()
{
		strcpy( mnem, "NEG" );
		strcpy( operand, "AL" );
		line_putout_with_release_nodes();
	}
void	yya41()
{
		strcpy( mnem, "CMC" );
		line_putout_with_release_nodes();
	}
void	yya42()
{
		strcpy( mnem, "STC" );
		line_putout_with_release_nodes();
	}
void	yya43()
{
		strcpy( mnem, "NOP" );
		line_putout_with_release_nodes();
	}
void	yya44()
{
		unconvertable( "HALT","not supported" );
	}
void	yya45()
{
		strcpy( mnem, "CLI" );
		line_putout_with_release_nodes();
	}
void	yya46()
{
		strcpy( mnem, "STI" );
		line_putout_with_release_nodes();
	}
void	yya47()
{
		unconvertable( "IM mode","対応する割り込みモードの概念無し" );
	}
void	yya48()
{
		strcpy( mnem, "ROL" );
		strcpy( operand, "AL,1" );
		line_putout_with_release_nodes();
	}
void	yya49()
{
		strcpy( mnem, "RCL" );
		strcpy( operand, "AL,1" );
		line_putout_with_release_nodes();
	}
void	yya50()
{
		strcpy( mnem, "ROR" );
		strcpy( operand, "AL,1" );
		line_putout_with_release_nodes();
	}
void	yya51()
{
		strcpy( mnem, "RCR" );
		strcpy( operand, "AL,1" );
		line_putout_with_release_nodes();
	}
void	yya52()
{
		strcpy( mnem, mnm_name(yyasp[3]) );
		sprintf( operand, "%s,1", reg_name(yyasp[2]) );
		line_putout_with_release_nodes();
	}
void	yya53()
{
		strcpy( mnem, mnm_name(yyasp[3]) );
		sprintf( operand, "byte ptr %s,1", yyasp[2]->value );
		line_putout_with_release_nodes();
	}
void	yya54()
{
		unconvertable( "RLD","対応する命令無し" );
	}
void	yya55()
{
		unconvertable( "RRD","対応する命令無し" );
	}
void	yya56()
{
		strcpy( mnem, "TEST" );
		sprintf( operand, "%s,1 SHL (%s)", reg_name(yyasp[2]), yyasp[4]->value );
		line_putout_with_release_nodes();
	}
void	yya57()
{
		strcpy( mnem, "TEST" );
		sprintf( operand, "byte ptr %s,1 SHL (%s)", yyasp[2]->value, yyasp[4]->value );
		line_putout_with_release_nodes();
	}
void	yya58()
{
		strcpy( mnem, "PUSHF" );
		line_putout();
		strcpy( mnem, "OR" );
		sprintf( operand, "%s,1 SHL (%s)", reg_name(yyasp[2]), yyasp[4]->value );
		line_putout();
		strcpy( mnem, "POPF" );
		line_putout_with_release_nodes();
	}
void	yya59()
{
		strcpy( mnem, "PUSHF" );
		line_putout();
		strcpy( mnem, "OR" );
		sprintf( operand, "byte ptr %s,1 SHL (%s)", yyasp[2]->value, yyasp[4]->value );
		line_putout();
		strcpy( mnem, "POPF" );
		line_putout_with_release_nodes();
	}
void	yya60()
{
		strcpy( mnem, "PUSHF" );
		line_putout();
		strcpy( mnem, "AND" );
		sprintf( operand, "%s,NOT(1 SHL (%s))", reg_name(yyasp[2]), yyasp[4]->value );
		line_putout();
		strcpy( mnem, "POPF" );
		line_putout_with_release_nodes();
	}
void	yya61()
{
		strcpy( mnem, "PUSHF" );
		line_putout();
		strcpy( mnem, "AND" );
		sprintf( operand, "byte ptr %s,NOT(1 SHL (%s))", yyasp[2]->value, yyasp[4]->value );
		line_putout();
		strcpy( mnem, "POPF" );
		line_putout_with_release_nodes();
	}
void	yya62()
{
		strcpy(mnem,"JMP");
		strcpy(operand, yyasp[2]->value );
		line_putout_with_release_nodes();
	}
void	yya63()
{
		sprintf( mnem, "LJ%s", cond_name( yyasp[4] ) );
		strcpy( operand, yyasp[2]->value );
		line_putout_with_release_nodes();
	}
void	yya64()
{
#ifdef ALL_LONG_JUMP
		strcpy(mnem,"JMP");
		strcpy(operand, yyasp[2]->value );
		line_putout_with_release_nodes();
#else
		strcpy(mnem,"JMP");
		sprintf(operand, "short %s",yyasp[2]->value );
		line_putout_with_release_nodes();
#endif
	}
void	yya65()
{
#ifdef ALL_LONG_JUMP
		sprintf( mnem, "LJ%s", cond_name( yyasp[4] ) );
		strcpy( operand, yyasp[2]->value );
		line_putout_with_release_nodes();
#else
		sprintf( mnem, "J%s", cond_name( yyasp[4] ) );
		strcpy( operand, yyasp[2]->value );
		line_putout_with_release_nodes();
#endif
	}
void	yya66()
{
		strcpy(mnem,"JMP");
		strcpy(operand, yyasp[2]->value );
		line_putout_with_release_nodes();
	}
void	yya67()
{
		strcpy(mnem,"DEC");
		strcpy(operand, "CH" );
		line_putout();
		strcpy(mnem,"JNZ");
		strcpy(operand, yyasp[2]->value );
		line_putout_with_release_nodes();
	}
void	yya68()
{
		strcpy(mnem,"CALL");
		strcpy(operand, yyasp[2]->value );
		line_putout_with_release_nodes();
	}
void	yya69()
{
		sprintf( mnem, "CALL%s", cond_name( yyasp[4] ) );
		strcpy( operand, yyasp[2]->value );
		line_putout_with_release_nodes();
	}
void	yya70()
{
		strcpy(mnem,"RET");
		line_putout_with_release_nodes();
	}
void	yya71()
{
		sprintf( mnem, "RET%s", cond_name( yyasp[2] ) );
		line_putout_with_release_nodes();
	}
void	yya72()
{
		unconvertable( "RETI","１対１で対応する命令無し" );
	}
void	yya73()
{
		unconvertable( "RETN","対応する命令無し" );
	}
void	yya74()
{
		unconvertable( (sprintf(buf, "RST %s", yyasp[2]->value),buf),"対応する命令無し" );
	}
void	yya75()
{
		unconvertable( (sprintf(buf, "IN A,(%s)", yyasp[3]->value),buf),"変換しません" );
	}
void	yya76()
{
		unconvertable( "IN A,(C)", "変換しません" );
	}
void	yya77()
{
		unconvertable( (sprintf(buf, "OUT (%s),A", yyasp[5]->value),buf),"変換しません" );
	}
void	yya78()
{
		unconvertable( "OUT (C),A", "変換しません" );
	}
void	yya79()
{
		strcpy( mnem,"DB");
		strcpy( operand, yyasp[2]->value );
		line_putout_with_release_nodes();
	}
void	yya80()
{
		strcpy( mnem,"DW");
		strcpy( operand, yyasp[2]->value );
		line_putout_with_release_nodes();
	}
void	yya81()
{
		strcpy( mnem,"DB");
		strcpy( operand, yyasp[2]->value );
		line_putout_with_release_nodes();
	}
void	yya82()
{
		strcpy( mnem,"DW");
		strcpy( operand, yyasp[2]->value );
		line_putout_with_release_nodes();
	}
void	yya83()
{
		strcpy( mnem,"DB");
		strcpy( operand, yyasp[2]->value );
		strcat( operand, " DUP(?)" );
		line_putout_with_release_nodes();
	}
void	yya84()
{
		strcpy( mnem,"INCLUDE");
		strcpy( operand, yyasp[2]->value );
		line_putout_with_release_nodes();
	}
void	yya85()
{
		unconvertable( "TITLE","TITLE疑似命令は変換できません" );
	}
void	yya86()
{
		unconvertable( "SUBTTL","SUBTTL疑似命令は変換できません" );
	}
void	yya87()
{
		unconvertable( "ORG","ORG疑似命令は変換できません" );
	}
void	yya88()
{
		strcpy( mnem,"END");
		line_putout_with_release_nodes();
	}
void	yya89()
{
		strcpy( mnem,"ALOOP");
		sprintf( operand, "%s",reg_name(yyasp[2]) );
		line_putout_with_release_nodes();
	}
void	yya90()
{
		strcpy( mnem,"ALOOP");
		sprintf( operand, "%s",reg_name(yyasp[2]) );
		line_putout_with_release_nodes();
	}
void	yya91()
{
		strcpy( mnem,"ALOOP");
		sprintf( operand, "%s,%s",reg_name(yyasp[4]),yyasp[2]->value );
		line_putout_with_release_nodes();
	}
void	yya92()
{
		strcpy( mnem,"ALOOP");
		sprintf( operand, "%s,%s",reg_name(yyasp[4]),yyasp[2]->value );
		line_putout_with_release_nodes();
	}
void	yya93()
{
		strcpy( mnem,"LEND");
		line_putout_with_release_nodes();
	}
void	yya94()
{
		strcpy( mnem,"LEND");
		line_putout_with_release_nodes();
	}
void	yya95()
{
		push_keep();
		{
			int i;
			for(i=0; i<root_of_keep_stack->number_of_list; i++){
				if( root_of_keep_stack->klist_buf[i]->number != KW_AF ){
					strcpy( mnem,"PUSH");
					strcpy( operand, reg_name( root_of_keep_stack->klist_buf[i] ) );
					line_putout_with_release_nodes();
				} else {
					strcpy( mnem, "LAHF" );
					strcpy( operand, "" );
					line_putout();
					strcpy( mnem, "PUSH" );
					strcpy( operand, "AX" );
					line_putout_with_release_nodes();
				}
			}
		}
	}
void	yya96()
{
		if( root_of_keep_stack == NULL ) {
			printf( "ｋｅｅｐ疑似命令スタックがアンダーフローしました\n");
			exit(4);
		}
		{
			int i;
			for(i=root_of_keep_stack->number_of_list-1;i>=0; i--){
				if( root_of_keep_stack->klist_buf[i]->number != KW_AF ){
					strcpy( mnem,"POP");
					strcpy( operand, reg_name( root_of_keep_stack->klist_buf[i] ) );
					line_putout_with_release_nodes();
				} else {
					strcpy( mnem, "POP" );
					strcpy( operand, "AX" );
					line_putout();
					strcpy( mnem, "SAHF" );
					strcpy( operand, "" );
					line_putout_with_release_nodes();
				}
			}
		}
		pop_keep();
	}
void	yya97()
{
		equ_putout( yyasp[5], yyasp[2] );
	}
void	yya98()
{
		equ_putout( yyasp[4], yyasp[2] );
	}
void	yya99()
{
		strcpy( mnem,"");
		strcpy( operand, "" );
		line_putout_with_release_nodes();
	}
void	yya100()
{ strcpy( lbl, yyasp[1]->value ); }
void	yya101()
{ strcpy( lbl, "" ); }
void	yya102()
{ strcpy( comm, yyasp[0]->value ); }
void	yya103()
{ strcpy( comm, "" ); }
#define	yya104	yydef
#define	yya105	yydef
#define	yya106	yydef
#define	yya107	yydef
#define	yya108	yydef
#define	yya109	yydef
#define	yya110	yydef
#define	yya111	yydef
#define	yya112	yydef
#define	yya113	yydef
#define	yya114	yydef
#define	yya115	yydef
#define	yya116	yydef
#define	yya117	yydef
#define	yya118	yydef
#define	yya119	yydef
#define	yya120	yydef
#define	yya121	yydef
#define	yya122	yydef
#define	yya123	yydef
#define	yya124	yydef
#define	yya125	yydef
#define	yya126	yydef
#define	yya127	yydef
#define	yya128	yydef
void	yya129()
{
						yyval = make_node();
						sprintf(yyval->value, "[SI+%s]",yyasp[1] );
					}
void	yya130()
{
						yyval = make_node();
						sprintf(yyval->value,"[DI+%s]",yyasp[1] );
					}
void	yya131()
{
						yyval = make_node();
						sprintf(yyval->value,"[SI-%s]",yyasp[1] );
					}
void	yya132()
{
						yyval = make_node();
						sprintf(yyval->value,"[DI-%s]",yyasp[1] );
					}
void	yya133()
{ yyval = make_const_node("[SI]"); }
void	yya134()
{ yyval = make_const_node("[DI]"); }
void	yya135()
{ yyval = make_const_node("[BX]"); }
#define	yya136	yydef
#define	yya137	yydef
#define	yya138	yydef
#define	yya139	yydef
#define	yya140	yydef
#define	yya141	yydef
#define	yya142	yydef
#define	yya143	yydef
#define	yya144	yydef
#define	yya145	yydef
#define	yya146	yydef
#define	yya147	yydef
#define	yya148	yydef
#define	yya149	yydef
#define	yya150	yydef
#define	yya151	yydef
#define	yya152	yydef
#define	yya153	yydef
#define	yya154	yydef
#define	yya155	yydef
#define	yya156	yydef
#define	yya157	yydef
#define	yya158	yydef
#define	yya159	yydef
void	yya160()
{
				number_of_klist = kptr - klist_buf;
			}
void	yya161()
{
				*kptr++ = yyasp[0];
			}
void	yya162()
{
				kptr = klist_buf;
				*kptr++ = yyasp[0];
			}
void	yya163()
{
				yyval = make_node();
				strcpy( yyval->value, elist_buf );
			}
void	yya164()
{
				strcat( elist_buf, "," );
				strcat( elist_buf, yyasp[0]->value );
			}
void	yya165()
{
				elist_buf[0] = '\0';
				strcat( elist_buf, yyasp[0]->value );
			}
void	yya166()
{
					/*printf("\nanswer=[%s]\n", expr_buf );*/
					yyval = make_node();
					strcpy( yyval->value, expr_buf );
				}
void	yya167()
{
					/*printf("[1:%s]",yyasp[0]->value);*/
					strcat( expr_buf, yyasp[0]->value );
				}
void	yya168()
{
					/*printf("[2:%s]",yyasp[0]->value);*/
					expr_buf[0] = '\0';
					strcat( expr_buf, yyasp[0]->value );
				}
#define	yya169	yydef
#define	yya170	yydef
#define	yya171	yydef
#define	yya172	yydef
#define	yya173	yydef
#define	yya174	yydef
#define	yya175	yydef
#define	yya176	yydef
#define	yya177	yydef
#define	yya178	yydef
#ifdef	YYDEBUG
#define	YYDISPLAY
char	*yypnames[] = {
"S' : program",
"program : program line",
"program : line",
"line : l_def KW_LD reg , reg c_def \\n",
"line : l_def KW_LD reg , expr c_def \\n",
"line : l_def KW_LD reg , idx c_def \\n",
"line : l_def KW_LD idx , reg c_def \\n",
"line : l_def KW_LD idx , expr c_def \\n",
"line : l_def KW_LD reg , ( bc_or_de ) c_def \\n",
"line : l_def KW_LD reg , ( expr ) c_def \\n",
"line : l_def KW_LD ( bc_or_de ) , reg c_def \\n",
"line : l_def KW_LD ( expr ) , reg c_def \\n",
"line : l_def KW_LD reg , KW_I c_def \\n",
"line : l_def KW_LD KW_I , reg c_def \\n",
"line : l_def KW_LD reg , KW_R c_def \\n",
"line : l_def KW_LD KW_R , reg c_def \\n",
"line : l_def KW_LD dd , expr c_def \\n",
"line : l_def KW_LD dd , ( expr ) c_def \\n",
"line : l_def KW_LD ( expr ) , dd c_def \\n",
"line : l_def KW_LD dd , dd c_def \\n",
"line : l_def KW_PUSH dd c_def \\n",
"line : l_def KW_POP dd c_def \\n",
"line : l_def KW_PUSH KW_AF c_def \\n",
"line : l_def KW_POP KW_AF c_def \\n",
"line : l_def KW_EX KW_DE , KW_HL c_def \\n",
"line : l_def KW_EX KW_AF , KW_AF_DUSH c_def \\n",
"line : l_def KW_EX ( KW_SP ) , dd c_def \\n",
"line : l_def strings c_def \\n",
"line : l_def calc_with_src KW_A , reg c_def \\n",
"line : l_def calc_with_src KW_A , expr c_def \\n",
"line : l_def calc_with_src KW_A , idx c_def \\n",
"line : l_def calcs reg c_def \\n",
"line : l_def calcs expr c_def \\n",
"line : l_def calcs idx c_def \\n",
"line : l_def inc_dec reg c_def \\n",
"line : l_def inc_dec idx c_def \\n",
"line : l_def calc_with_src dd , dd c_def \\n",
"line : l_def inc_dec dd c_def \\n",
"line : l_def KW_DAA c_def \\n",
"line : l_def KW_CPL c_def \\n",
"line : l_def KW_NEG c_def \\n",
"line : l_def KW_CCF c_def \\n",
"line : l_def KW_SCF c_def \\n",
"line : l_def KW_NOP c_def \\n",
"line : l_def KW_HALT c_def \\n",
"line : l_def KW_DI c_def \\n",
"line : l_def KW_EI c_def \\n",
"line : l_def KW_IM expr c_def \\n",
"line : l_def KW_RLCA c_def \\n",
"line : l_def KW_RLA c_def \\n",
"line : l_def KW_RRCA c_def \\n",
"line : l_def KW_RRA c_def \\n",
"line : l_def rot_sft reg c_def \\n",
"line : l_def rot_sft idx c_def \\n",
"line : l_def KW_RLD c_def \\n",
"line : l_def KW_RRD c_def \\n",
"line : l_def KW_BIT expr , reg c_def \\n",
"line : l_def KW_BIT expr , idx c_def \\n",
"line : l_def KW_SET expr , reg c_def \\n",
"line : l_def KW_SET expr , idx c_def \\n",
"line : l_def KW_RES expr , reg c_def \\n",
"line : l_def KW_RES expr , idx c_def \\n",
"line : l_def KW_JP expr c_def \\n",
"line : l_def KW_JP cond , expr c_def \\n",
"line : l_def KW_JR expr c_def \\n",
"line : l_def KW_JR cond , expr c_def \\n",
"line : l_def KW_JP idx c_def \\n",
"line : l_def KW_DJNZ expr c_def \\n",
"line : l_def KW_CALL expr c_def \\n",
"line : l_def KW_CALL cond , expr c_def \\n",
"line : l_def KW_RET c_def \\n",
"line : l_def KW_RET cond c_def \\n",
"line : l_def KW_RETI c_def \\n",
"line : l_def KW_RETN c_def \\n",
"line : l_def KW_RST expr c_def \\n",
"line : l_def KW_IN KW_A , ( expr ) c_def \\n",
"line : l_def KW_IN KW_A , ( KW_C ) c_def \\n",
"line : l_def KW_OUT ( expr ) , KW_A c_def \\n",
"line : l_def KW_OUT ( KW_C ) , KW_A c_def \\n",
"line : l_def KW_DB e_list c_def \\n",
"line : l_def KW_DW e_list c_def \\n",
"line : l_def KW_DEFB e_list c_def \\n",
"line : l_def KW_DEFW e_list c_def \\n",
"line : l_def KW_DS expr c_def \\n",
"line : l_def KW_INCLUDE label c_def \\n",
"line : l_def KW_TITLE label c_def \\n",
"line : l_def KW_SUBTTL label c_def \\n",
"line : l_def KW_ORG expr c_def \\n",
"line : l_def KW_END c_def \\n",
"line : l_def KW_LOOP reg c_def \\n",
"line : l_def KW_LOOP dd c_def \\n",
"line : l_def KW_LOOP reg , expr c_def \\n",
"line : l_def KW_LOOP dd , expr c_def \\n",
"line : l_def KW_LEND c_def \\n",
"line : l_def KW_LEND KW_A c_def \\n",
"line : l_def KW_KEEP keep_list c_def \\n",
"line : l_def KW_KEEPOUT c_def \\n",
"line : label : KW_EQU expr c_def \\n",
"line : label KW_EQU expr c_def \\n",
"line : l_def c_def \\n",
"l_def : label :",
"l_def :",
"c_def : comment",
"c_def :",
"strings : KW_LDI",
"strings : KW_LDIR",
"strings : KW_LDD",
"strings : KW_LDDR",
"strings : KW_CPI",
"strings : KW_CPIR",
"strings : KW_CPD",
"strings : KW_CPDR",
"calc_with_src : KW_ADD",
"calc_with_src : KW_ADC",
"calc_with_src : KW_SBC",
"calcs : KW_SUB",
"calcs : KW_AND",
"calcs : KW_OR",
"calcs : KW_XOR",
"calcs : KW_CP",
"inc_dec : KW_INC",
"inc_dec : KW_DEC",
"rot_sft : KW_RLC",
"rot_sft : KW_RL",
"rot_sft : KW_RRC",
"rot_sft : KW_RR",
"rot_sft : KW_SLA",
"rot_sft : KW_SRA",
"rot_sft : KW_SRL",
"idx : ( KW_IX + expr )",
"idx : ( KW_IY + expr )",
"idx : ( KW_IX - expr )",
"idx : ( KW_IY - expr )",
"idx : ( KW_IX )",
"idx : ( KW_IY )",
"idx : ( KW_HL )",
"reg : KW_A",
"reg : KW_B",
"reg : KW_C",
"reg : KW_D",
"reg : KW_E",
"reg : KW_H",
"reg : KW_L",
"dd : bc_or_de",
"dd : KW_SP",
"dd : KW_HL",
"dd : KW_IX",
"dd : KW_IY",
"bc_or_de : KW_BC",
"bc_or_de : KW_DE",
"cond : KW_C",
"cond : KW_NC",
"cond : KW_Z",
"cond : KW_NZ",
"cond : KW_P",
"cond : KW_M",
"cond : KW_PE",
"cond : KW_PO",
"dd_af : dd",
"dd_af : KW_AF",
"keep_list : keep_list0",
"keep_list0 : keep_list0 , dd_af",
"keep_list0 : dd_af",
"e_list : e_list0",
"e_list0 : e_list0 , expr",
"e_list0 : expr",
"expr : expr0",
"expr0 : expr0 e_item",
"expr0 : e_item",
"e_item : label",
"e_item : twoop",
"e_item : const",
"e_item : str_const",
"e_item : [",
"e_item : ]",
"twoop : +",
"twoop : -",
"twoop : *",
"twoop : /",
};
#endif
#ifdef	YYDISPLAY
char	*yytnames[] = {
	"EOF",
	"error",
	"label",
	"const",
	"str_const",
	"comment",
	"KW_A",
	"KW_B",
	"KW_C",
	"KW_D",
	"KW_E",
	"KW_H",
	"KW_L",
	"KW_I",
	"KW_R",
	"KW_IX",
	"KW_IY",
	"KW_HL",
	"KW_DE",
	"KW_BC",
	"KW_SP",
	"KW_AF",
	"KW_AF_DUSH",
	"KW_NC",
	"KW_Z",
	"KW_NZ",
	"KW_P",
	"KW_M",
	"KW_PE",
	"KW_PO",
	"KW_EQU",
	"KW_LDI",
	"KW_LDIR",
	"KW_LDD",
	"KW_LDDR",
	"KW_CPI",
	"KW_CPIR",
	"KW_CPD",
	"KW_CPDR",
	"KW_LD",
	"KW_PUSH",
	"KW_POP",
	"KW_EX",
	"KW_ADD",
	"KW_SUB",
	"KW_ADC",
	"KW_SBC",
	"KW_AND",
	"KW_OR",
	"KW_XOR",
	"KW_CP",
	"KW_INC",
	"KW_DEC",
	"KW_DAA",
	"KW_CPL",
	"KW_NEG",
	"KW_CCF",
	"KW_SCF",
	"KW_NOP",
	"KW_HALT",
	"KW_DI",
	"KW_EI",
	"KW_IM",
	"KW_RLCA",
	"KW_RLA",
	"KW_RRCA",
	"KW_RRA",
	"KW_RLC",
	"KW_RL",
	"KW_RRC",
	"KW_RR",
	"KW_SLA",
	"KW_SRA",
	"KW_SRL",
	"KW_RLD",
	"KW_RRD",
	"KW_BIT",
	"KW_SET",
	"KW_RES",
	"KW_JP",
	"KW_JR",
	"KW_CALL",
	"KW_RET",
	"KW_DJNZ",
	"KW_RETI",
	"KW_RETN",
	"KW_RST",
	"KW_IN",
	"KW_INI",
	"KW_INIR",
	"KW_IND",
	"KW_INDR",
	"KW_OUT",
	"KW_OUTI",
	"KW_OTIR",
	"KW_OUTD",
	"KW_OTDR",
	"KW_DB",
	"KW_DW",
	"KW_DS",
	"KW_DEFB",
	"KW_DEFW",
	"KW_DEFS",
	"KW_ORG",
	"KW_END",
	"KW_INCLUDE",
	"KW_TITLE",
	"KW_SUBTTL",
	"KW_LOOP",
	"KW_LEND",
	"KW_KEEP",
	"KW_KEEPOUT",
};
#endif
typedef	struct {
	YYLEX	input;
	short	num;
} YYACT;
void (*yya[])( int l ) = {
	0,
	yya1,
	yya2,
	yya3,
	yya4,
	yya5,
	yya6,
	yya7,
	yya8,
	yya9,
	yya10,
	yya11,
	yya12,
	yya13,
	yya14,
	yya15,
	yya16,
	yya17,
	yya18,
	yya19,
	yya20,
	yya21,
	yya22,
	yya23,
	yya24,
	yya25,
	yya26,
	yya27,
	yya28,
	yya29,
	yya30,
	yya31,
	yya32,
	yya33,
	yya34,
	yya35,
	yya36,
	yya37,
	yya38,
	yya39,
	yya40,
	yya41,
	yya42,
	yya43,
	yya44,
	yya45,
	yya46,
	yya47,
	yya48,
	yya49,
	yya50,
	yya51,
	yya52,
	yya53,
	yya54,
	yya55,
	yya56,
	yya57,
	yya58,
	yya59,
	yya60,
	yya61,
	yya62,
	yya63,
	yya64,
	yya65,
	yya66,
	yya67,
	yya68,
	yya69,
	yya70,
	yya71,
	yya72,
	yya73,
	yya74,
	yya75,
	yya76,
	yya77,
	yya78,
	yya79,
	yya80,
	yya81,
	yya82,
	yya83,
	yya84,
	yya85,
	yya86,
	yya87,
	yya88,
	yya89,
	yya90,
	yya91,
	yya92,
	yya93,
	yya94,
	yya95,
	yya96,
	yya97,
	yya98,
	yya99,
	yya100,
	yya101,
	yya102,
	yya103,
	yya104,
	yya105,
	yya106,
	yya107,
	yya108,
	yya109,
	yya110,
	yya111,
	yya112,
	yya113,
	yya114,
	yya115,
	yya116,
	yya117,
	yya118,
	yya119,
	yya120,
	yya121,
	yya122,
	yya123,
	yya124,
	yya125,
	yya126,
	yya127,
	yya128,
	yya129,
	yya130,
	yya131,
	yya132,
	yya133,
	yya134,
	yya135,
	yya136,
	yya137,
	yya138,
	yya139,
	yya140,
	yya141,
	yya142,
	yya143,
	yya144,
	yya145,
	yya146,
	yya147,
	yya148,
	yya149,
	yya150,
	yya151,
	yya152,
	yya153,
	yya154,
	yya155,
	yya156,
	yya157,
	yya158,
	yya159,
	yya160,
	yya161,
	yya162,
	yya163,
	yya164,
	yya165,
	yya166,
	yya167,
	yya168,
	yya169,
	yya170,
	yya171,
	yya172,
	yya173,
	yya174,
	yya175,
	yya176,
	yya177,
	yya178,
};
YYACT	yyia[] = {
	0,	20101,	/* 0 */
	label,	2,
	0,	-1,	/* 1 */
	KW_EQU,	83,
	':',	93,
	0,	20101,	/* 2 */
	YYEOF,	20000,
	label,	2,
	0,	20002,	/* 3 */
	0,	20103,	/* 4 */
	KW_LDI,	97,
	KW_LDIR,	98,
	KW_LDD,	99,
	KW_LDDR,	100,
	KW_CPI,	101,
	KW_CPIR,	102,
	KW_CPD,	103,
	KW_CPDR,	104,
	KW_LD,	105,
	KW_PUSH,	122,
	KW_POP,	130,
	KW_EX,	133,
	KW_ADD,	137,
	KW_SUB,	138,
	KW_ADC,	139,
	KW_SBC,	140,
	KW_AND,	141,
	KW_OR,	142,
	KW_XOR,	143,
	KW_CP,	144,
	KW_INC,	145,
	KW_DEC,	146,
	KW_DAA,	147,
	KW_CPL,	149,
	KW_NEG,	151,
	KW_CCF,	153,
	KW_SCF,	155,
	KW_NOP,	157,
	KW_HALT,	159,
	KW_DI,	161,
	KW_EI,	163,
	KW_IM,	165,
	KW_RLCA,	167,
	KW_RLA,	169,
	KW_RRCA,	171,
	KW_RRA,	173,
	KW_RLC,	175,
	KW_RL,	176,
	KW_RRC,	177,
	KW_RR,	178,
	KW_SLA,	179,
	KW_SRA,	180,
	KW_SRL,	181,
	KW_RLD,	182,
	KW_RRD,	184,
	KW_BIT,	186,
	KW_SET,	188,
	KW_RES,	190,
	KW_JP,	192,
	KW_JR,	203,
	KW_CALL,	213,
	KW_RET,	215,
	KW_DJNZ,	225,
	KW_RETI,	227,
	KW_RETN,	229,
	KW_RST,	231,
	KW_IN,	233,
	KW_OUT,	235,
	KW_DB,	237,
	KW_DW,	239,
	KW_DS,	241,
	KW_DEFB,	243,
	KW_DEFW,	245,
	KW_ORG,	247,
	KW_END,	249,
	KW_INCLUDE,	251,
	KW_TITLE,	253,
	KW_SUBTTL,	255,
	KW_LOOP,	257,
	KW_LEND,	259,
	KW_KEEP,	262,
	KW_KEEPOUT,	265,
	comment,	96,
	0,	-1,	/* 5 */
	label,	290,
	const,	291,
	str_const,	292,
	'+',	293,
	'-',	294,
	'[',	295,
	']',	296,
	'*',	297,
	'/',	298,
	0,	20100,	/* 6 */
	KW_EQU,	305,
	0,	20001,	/* 7 */
	0,	20102,	/* 8 */
	0,	20104,	/* 9 */
	0,	20105,	/* 10 */
	0,	20106,	/* 11 */
	0,	20107,	/* 12 */
	0,	20108,	/* 13 */
	0,	20109,	/* 14 */
	0,	20110,	/* 15 */
	0,	20111,	/* 16 */
	0,	-1,	/* 17 */
	KW_I,	314,
	KW_R,	316,
	'(',	324,
	KW_IX,	318,
	KW_IY,	319,
	KW_HL,	320,
	KW_SP,	323,
	KW_DE,	321,
	KW_BC,	322,
	KW_A,	307,
	KW_B,	308,
	KW_C,	309,
	KW_D,	310,
	KW_E,	311,
	KW_H,	312,
	KW_L,	313,
	0,	-1,	/* 18 */
	KW_AF,	338,
	KW_IX,	318,
	KW_IY,	319,
	KW_HL,	320,
	KW_SP,	323,
	KW_DE,	321,
	KW_BC,	322,
	0,	-1,	/* 19 */
	KW_AF,	342,
	YYLNK,	124,
	0,	-1,	/* 20 */
	KW_DE,	346,
	KW_AF,	348,
	'(',	350,
	0,	20112,	/* 21 */
	0,	20115,	/* 22 */
	0,	20113,	/* 23 */
	0,	20114,	/* 24 */
	0,	20116,	/* 25 */
	0,	20117,	/* 26 */
	0,	20118,	/* 27 */
	0,	20119,	/* 28 */
	0,	20120,	/* 29 */
	0,	20121,	/* 30 */
	0,	20103,	/* 31 */
	comment,	96,
	0,	20103,	/* 32 */
	comment,	96,
	0,	20103,	/* 33 */
	comment,	96,
	0,	20103,	/* 34 */
	comment,	96,
	0,	20103,	/* 35 */
	comment,	96,
	0,	20103,	/* 36 */
	comment,	96,
	0,	20103,	/* 37 */
	comment,	96,
	0,	20103,	/* 38 */
	comment,	96,
	0,	20103,	/* 39 */
	comment,	96,
	0,	-1,	/* 40 */
	YYLNK,	84,
	0,	20103,	/* 41 */
	comment,	96,
	0,	20103,	/* 42 */
	comment,	96,
	0,	20103,	/* 43 */
	comment,	96,
	0,	20103,	/* 44 */
	comment,	96,
	0,	20122,	/* 45 */
	0,	20123,	/* 46 */
	0,	20124,	/* 47 */
	0,	20125,	/* 48 */
	0,	20126,	/* 49 */
	0,	20127,	/* 50 */
	0,	20128,	/* 51 */
	0,	20103,	/* 52 */
	comment,	96,
	0,	20103,	/* 53 */
	comment,	96,
	0,	-1,	/* 54 */
	YYLNK,	84,
	0,	-1,	/* 55 */
	YYLNK,	84,
	0,	-1,	/* 56 */
	YYLNK,	84,
	0,	-1,	/* 57 */
	KW_C,	390,
	KW_NC,	391,
	KW_Z,	392,
	KW_NZ,	393,
	KW_P,	394,
	KW_M,	395,
	KW_PE,	396,
	KW_PO,	397,
	'(',	398,
	YYLNK,	84,
	0,	-1,	/* 58 */
	KW_C,	390,
	KW_NC,	391,
	KW_Z,	392,
	KW_NZ,	393,
	KW_P,	394,
	KW_M,	395,
	KW_PE,	396,
	KW_PO,	397,
	YYLNK,	84,
	0,	-1,	/* 59 */
	YYLNK,	204,
	0,	20103,	/* 60 */
	KW_C,	390,
	KW_NC,	391,
	KW_Z,	392,
	KW_NZ,	393,
	KW_P,	394,
	KW_M,	395,
	KW_PE,	396,
	KW_PO,	397,
	comment,	96,
	0,	-1,	/* 61 */
	YYLNK,	84,
	0,	20103,	/* 62 */
	comment,	96,
	0,	20103,	/* 63 */
	comment,	96,
	0,	-1,	/* 64 */
	YYLNK,	84,
	0,	-1,	/* 65 */
	KW_A,	428,
	0,	-1,	/* 66 */
	'(',	430,
	0,	-1,	/* 67 */
	YYLNK,	84,
	0,	-1,	/* 68 */
	YYLNK,	84,
	0,	-1,	/* 69 */
	YYLNK,	84,
	0,	-1,	/* 70 */
	YYLNK,	84,
	0,	-1,	/* 71 */
	YYLNK,	84,
	0,	-1,	/* 72 */
	YYLNK,	84,
	0,	20103,	/* 73 */
	comment,	96,
	0,	-1,	/* 74 */
	label,	450,
	0,	-1,	/* 75 */
	label,	452,
	0,	-1,	/* 76 */
	label,	454,
	0,	-1,	/* 77 */
	YYLNK,	109,
	0,	20103,	/* 78 */
	KW_A,	462,
	comment,	96,
	0,	-1,	/* 79 */
	KW_AF,	466,
	YYLNK,	124,
	0,	20103,	/* 80 */
	comment,	96,
	0,	-1,	/* 81 */
	'\n',	475,
	0,	20103,	/* 82 */
	comment,	96,
	0,	-1,	/* 83 */
	KW_A,	478,
	YYLNK,	124,
	0,	-1,	/* 84 */
	'(',	398,
	KW_A,	307,
	KW_B,	308,
	KW_C,	309,
	KW_D,	310,
	KW_E,	311,
	KW_H,	312,
	KW_L,	313,
	YYLNK,	84,
	0,	-1,	/* 85 */
	'(',	398,
	YYLNK,	109,
	0,	-1,	/* 86 */
	'(',	398,
	YYLNK,	115,
	0,	20169,	/* 87 */
	0,	20171,	/* 88 */
	0,	20172,	/* 89 */
	0,	20175,	/* 90 */
	0,	20176,	/* 91 */
	0,	20173,	/* 92 */
	0,	20174,	/* 93 */
	0,	20177,	/* 94 */
	0,	20178,	/* 95 */
	0,	20103,	/* 96 */
	comment,	96,
	0,	20166,	/* 97 */
	YYLNK,	84,
	0,	20168,	/* 98 */
	0,	20170,	/* 99 */
	0,	-1,	/* 100 */
	YYLNK,	84,
	0,	20136,	/* 101 */
	0,	20137,	/* 102 */
	0,	20138,	/* 103 */
	0,	20139,	/* 104 */
	0,	20140,	/* 105 */
	0,	20141,	/* 106 */
	0,	20142,	/* 107 */
	0,	-1,	/* 108 */
	',',	503,
	0,	-1,	/* 109 */
	',',	505,
	0,	20146,	/* 110 */
	0,	20147,	/* 111 */
	0,	20145,	/* 112 */
	0,	20149,	/* 113 */
	0,	20148,	/* 114 */
	0,	20144,	/* 115 */
	0,	-1,	/* 116 */
	KW_IX,	507,
	KW_IY,	511,
	KW_HL,	515,
	KW_DE,	321,
	KW_BC,	322,
	YYLNK,	84,
	0,	-1,	/* 117 */
	',',	521,
	0,	-1,	/* 118 */
	',',	526,
	0,	20143,	/* 119 */
	0,	-1,	/* 120 */
	',',	528,
	0,	20103,	/* 121 */
	comment,	96,
	0,	20103,	/* 122 */
	comment,	96,
	0,	20103,	/* 123 */
	comment,	96,
	0,	20103,	/* 124 */
	comment,	96,
	0,	-1,	/* 125 */
	',',	543,
	0,	-1,	/* 126 */
	',',	545,
	0,	-1,	/* 127 */
	KW_SP,	547,
	0,	-1,	/* 128 */
	'\n',	549,
	0,	-1,	/* 129 */
	'\n',	550,
	0,	-1,	/* 130 */
	'\n',	551,
	0,	-1,	/* 131 */
	'\n',	552,
	0,	-1,	/* 132 */
	'\n',	553,
	0,	-1,	/* 133 */
	'\n',	554,
	0,	-1,	/* 134 */
	'\n',	555,
	0,	-1,	/* 135 */
	'\n',	556,
	0,	-1,	/* 136 */
	'\n',	557,
	0,	20103,	/* 137 */
	comment,	96,
	0,	-1,	/* 138 */
	'\n',	560,
	0,	-1,	/* 139 */
	'\n',	561,
	0,	-1,	/* 140 */
	'\n',	562,
	0,	-1,	/* 141 */
	'\n',	563,
	0,	-1,	/* 142 */
	'\n',	564,
	0,	-1,	/* 143 */
	'\n',	565,
	0,	-1,	/* 144 */
	',',	566,
	0,	-1,	/* 145 */
	',',	568,
	0,	-1,	/* 146 */
	',',	570,
	0,	20150,	/* 147 */
	0,	20151,	/* 148 */
	0,	20152,	/* 149 */
	0,	20153,	/* 150 */
	0,	20154,	/* 151 */
	0,	20155,	/* 152 */
	0,	20156,	/* 153 */
	0,	20157,	/* 154 */
	0,	-1,	/* 155 */
	KW_IX,	507,
	KW_IY,	511,
	KW_HL,	515,
	0,	20103,	/* 156 */
	comment,	96,
	0,	20103,	/* 157 */
	comment,	96,
	0,	-1,	/* 158 */
	',',	576,
	0,	20103,	/* 159 */
	comment,	96,
	0,	-1,	/* 160 */
	',',	580,
	0,	20103,	/* 161 */
	comment,	96,
	0,	-1,	/* 162 */
	',',	584,
	0,	-1,	/* 163 */
	'\n',	586,
	0,	20103,	/* 164 */
	comment,	96,
	0,	20103,	/* 165 */
	comment,	96,
	0,	-1,	/* 166 */
	'\n',	591,
	0,	-1,	/* 167 */
	'\n',	592,
	0,	20103,	/* 168 */
	comment,	96,
	0,	-1,	/* 169 */
	',',	595,
	0,	-1,	/* 170 */
	KW_C,	597,
	YYLNK,	84,
	0,	20165,	/* 171 */
	0,	20103,	/* 172 */
	comment,	96,
	0,	20163,	/* 173 */
	',',	603,
	0,	20103,	/* 174 */
	comment,	96,
	0,	20103,	/* 175 */
	comment,	96,
	0,	20103,	/* 176 */
	comment,	96,
	0,	20103,	/* 177 */
	comment,	96,
	0,	20103,	/* 178 */
	comment,	96,
	0,	-1,	/* 179 */
	'\n',	615,
	0,	20103,	/* 180 */
	comment,	96,
	0,	20103,	/* 181 */
	comment,	96,
	0,	20103,	/* 182 */
	comment,	96,
	0,	20103,	/* 183 */
	',',	622,
	comment,	96,
	0,	20103,	/* 184 */
	',',	626,
	comment,	96,
	0,	20103,	/* 185 */
	comment,	96,
	0,	-1,	/* 186 */
	'\n',	632,
	0,	20159,	/* 187 */
	0,	20158,	/* 188 */
	0,	20103,	/* 189 */
	comment,	96,
	0,	20162,	/* 190 */
	0,	20160,	/* 191 */
	',',	635,
	0,	-1,	/* 192 */
	'\n',	637,
	0,	20099,	/* 193 */
	0,	-1,	/* 194 */
	'\n',	638,
	0,	-1,	/* 195 */
	',',	639,
	0,	-1,	/* 196 */
	',',	641,
	0,	20103,	/* 197 */
	comment,	96,
	0,	20103,	/* 198 */
	comment,	96,
	0,	20103,	/* 199 */
	comment,	96,
	0,	20103,	/* 200 */
	comment,	96,
	0,	20103,	/* 201 */
	comment,	96,
	0,	20103,	/* 202 */
	comment,	96,
	0,	20103,	/* 203 */
	comment,	96,
	0,	20103,	/* 204 */
	comment,	96,
	0,	-1,	/* 205 */
	'\n',	659,
	0,	20167,	/* 206 */
	0,	20103,	/* 207 */
	comment,	96,
	0,	-1,	/* 208 */
	YYLNK,	115,
	0,	-1,	/* 209 */
	YYLNK,	115,
	0,	-1,	/* 210 */
	')',	666,
	'+',	667,
	'-',	669,
	0,	-1,	/* 211 */
	')',	671,
	'+',	672,
	'-',	674,
	0,	-1,	/* 212 */
	')',	676,
	0,	-1,	/* 213 */
	')',	677,
	0,	-1,	/* 214 */
	')',	679,
	0,	-1,	/* 215 */
	KW_I,	681,
	KW_R,	683,
	'(',	685,
	YYLNK,	276,
	0,	-1,	/* 216 */
	YYLNK,	276,
	0,	-1,	/* 217 */
	'(',	697,
	KW_IX,	318,
	KW_IY,	319,
	KW_HL,	320,
	KW_SP,	323,
	YYLNK,	328,
	0,	-1,	/* 218 */
	'\n',	703,
	0,	-1,	/* 219 */
	'\n',	704,
	0,	-1,	/* 220 */
	'\n',	705,
	0,	-1,	/* 221 */
	'\n',	706,
	0,	-1,	/* 222 */
	KW_HL,	707,
	0,	-1,	/* 223 */
	KW_AF_DUSH,	709,
	0,	-1,	/* 224 */
	')',	711,
	0,	20038,	/* 225 */
	0,	20039,	/* 226 */
	0,	20040,	/* 227 */
	0,	20041,	/* 228 */
	0,	20042,	/* 229 */
	0,	20043,	/* 230 */
	0,	20044,	/* 231 */
	0,	20045,	/* 232 */
	0,	20046,	/* 233 */
	0,	-1,	/* 234 */
	'\n',	713,
	0,	20048,	/* 235 */
	0,	20049,	/* 236 */
	0,	20050,	/* 237 */
	0,	20051,	/* 238 */
	0,	20054,	/* 239 */
	0,	20055,	/* 240 */
	0,	-1,	/* 241 */
	YYLNK,	288,
	0,	-1,	/* 242 */
	YYLNK,	288,
	0,	-1,	/* 243 */
	YYLNK,	288,
	0,	-1,	/* 244 */
	'\n',	726,
	0,	-1,	/* 245 */
	'\n',	727,
	0,	-1,	/* 246 */
	YYLNK,	84,
	0,	-1,	/* 247 */
	'\n',	730,
	0,	-1,	/* 248 */
	YYLNK,	84,
	0,	-1,	/* 249 */
	'\n',	733,
	0,	-1,	/* 250 */
	YYLNK,	84,
	0,	20070,	/* 251 */
	0,	-1,	/* 252 */
	'\n',	736,
	0,	-1,	/* 253 */
	'\n',	737,
	0,	20072,	/* 254 */
	0,	20073,	/* 255 */
	0,	-1,	/* 256 */
	'\n',	738,
	0,	-1,	/* 257 */
	'(',	739,
	0,	-1,	/* 258 */
	')',	742,
	0,	-1,	/* 259 */
	')',	744,
	0,	-1,	/* 260 */
	'\n',	746,
	0,	-1,	/* 261 */
	YYLNK,	84,
	0,	-1,	/* 262 */
	'\n',	748,
	0,	-1,	/* 263 */
	'\n',	749,
	0,	-1,	/* 264 */
	'\n',	750,
	0,	-1,	/* 265 */
	'\n',	751,
	0,	-1,	/* 266 */
	'\n',	752,
	0,	20088,	/* 267 */
	0,	-1,	/* 268 */
	'\n',	753,
	0,	-1,	/* 269 */
	'\n',	754,
	0,	-1,	/* 270 */
	'\n',	755,
	0,	-1,	/* 271 */
	YYLNK,	84,
	0,	-1,	/* 272 */
	'\n',	758,
	0,	-1,	/* 273 */
	YYLNK,	84,
	0,	-1,	/* 274 */
	'\n',	761,
	0,	-1,	/* 275 */
	'\n',	762,
	0,	20093,	/* 276 */
	0,	-1,	/* 277 */
	'\n',	763,
	0,	-1,	/* 278 */
	YYLNK,	263,
	0,	20096,	/* 279 */
	0,	20027,	/* 280 */
	0,	-1,	/* 281 */
	YYLNK,	275,
	0,	-1,	/* 282 */
	YYLNK,	124,
	0,	-1,	/* 283 */
	'\n',	773,
	0,	-1,	/* 284 */
	'\n',	774,
	0,	-1,	/* 285 */
	'\n',	775,
	0,	-1,	/* 286 */
	'\n',	776,
	0,	-1,	/* 287 */
	'\n',	777,
	0,	-1,	/* 288 */
	'\n',	778,
	0,	-1,	/* 289 */
	'\n',	779,
	0,	-1,	/* 290 */
	'\n',	780,
	0,	20098,	/* 291 */
	0,	-1,	/* 292 */
	'\n',	781,
	0,	20103,	/* 293 */
	comment,	96,
	0,	20103,	/* 294 */
	comment,	96,
	0,	20133,	/* 295 */
	0,	-1,	/* 296 */
	YYLNK,	84,
	0,	-1,	/* 297 */
	YYLNK,	84,
	0,	20134,	/* 298 */
	0,	-1,	/* 299 */
	YYLNK,	84,
	0,	-1,	/* 300 */
	YYLNK,	84,
	0,	20135,	/* 301 */
	0,	-1,	/* 302 */
	',',	794,
	0,	-1,	/* 303 */
	',',	796,
	0,	20103,	/* 304 */
	comment,	96,
	0,	20103,	/* 305 */
	comment,	96,
	0,	-1,	/* 306 */
	YYLNK,	325,
	0,	20103,	/* 307 */
	comment,	96,
	0,	20103,	/* 308 */
	comment,	96,
	0,	20103,	/* 309 */
	comment,	96,
	0,	20103,	/* 310 */
	comment,	96,
	0,	20103,	/* 311 */
	comment,	96,
	0,	-1,	/* 312 */
	YYLNK,	84,
	0,	20103,	/* 313 */
	comment,	96,
	0,	20103,	/* 314 */
	comment,	96,
	0,	20022,	/* 315 */
	0,	20020,	/* 316 */
	0,	20023,	/* 317 */
	0,	20021,	/* 318 */
	0,	20103,	/* 319 */
	comment,	96,
	0,	20103,	/* 320 */
	comment,	96,
	0,	-1,	/* 321 */
	',',	826,
	0,	20047,	/* 322 */
	0,	20103,	/* 323 */
	comment,	96,
	0,	20103,	/* 324 */
	comment,	96,
	0,	20103,	/* 325 */
	comment,	96,
	0,	20103,	/* 326 */
	comment,	96,
	0,	20103,	/* 327 */
	comment,	96,
	0,	20103,	/* 328 */
	comment,	96,
	0,	20062,	/* 329 */
	0,	20066,	/* 330 */
	0,	20103,	/* 331 */
	comment,	96,
	0,	20064,	/* 332 */
	0,	20103,	/* 333 */
	comment,	96,
	0,	20068,	/* 334 */
	0,	20103,	/* 335 */
	comment,	96,
	0,	20071,	/* 336 */
	0,	20067,	/* 337 */
	0,	20074,	/* 338 */
	0,	-1,	/* 339 */
	KW_C,	846,
	YYLNK,	84,
	0,	-1,	/* 340 */
	',',	850,
	0,	-1,	/* 341 */
	',',	852,
	0,	20079,	/* 342 */
	0,	20164,	/* 343 */
	0,	20080,	/* 344 */
	0,	20083,	/* 345 */
	0,	20081,	/* 346 */
	0,	20082,	/* 347 */
	0,	20087,	/* 348 */
	0,	20084,	/* 349 */
	0,	20085,	/* 350 */
	0,	20086,	/* 351 */
	0,	20103,	/* 352 */
	comment,	96,
	0,	20089,	/* 353 */
	0,	20103,	/* 354 */
	comment,	96,
	0,	20090,	/* 355 */
	0,	20094,	/* 356 */
	0,	20095,	/* 357 */
	0,	20161,	/* 358 */
	0,	20103,	/* 359 */
	comment,	96,
	0,	20103,	/* 360 */
	comment,	96,
	0,	20103,	/* 361 */
	comment,	96,
	0,	20103,	/* 362 */
	comment,	96,
	0,	20031,	/* 363 */
	0,	20032,	/* 364 */
	0,	20033,	/* 365 */
	0,	20034,	/* 366 */
	0,	20035,	/* 367 */
	0,	20037,	/* 368 */
	0,	20052,	/* 369 */
	0,	20053,	/* 370 */
	0,	20097,	/* 371 */
	0,	-1,	/* 372 */
	'\n',	866,
	0,	-1,	/* 373 */
	'\n',	867,
	0,	-1,	/* 374 */
	')',	868,
	0,	-1,	/* 375 */
	')',	869,
	0,	-1,	/* 376 */
	')',	870,
	0,	-1,	/* 377 */
	')',	871,
	0,	-1,	/* 378 */
	YYLNK,	109,
	0,	-1,	/* 379 */
	YYLNK,	115,
	0,	-1,	/* 380 */
	'\n',	878,
	0,	-1,	/* 381 */
	'\n',	879,
	0,	-1,	/* 382 */
	')',	880,
	0,	-1,	/* 383 */
	')',	882,
	0,	-1,	/* 384 */
	'\n',	884,
	0,	-1,	/* 385 */
	'\n',	885,
	0,	-1,	/* 386 */
	'\n',	886,
	0,	-1,	/* 387 */
	'\n',	887,
	0,	-1,	/* 388 */
	'\n',	888,
	0,	-1,	/* 389 */
	')',	889,
	0,	-1,	/* 390 */
	'\n',	891,
	0,	-1,	/* 391 */
	'\n',	892,
	0,	-1,	/* 392 */
	'\n',	893,
	0,	-1,	/* 393 */
	'\n',	894,
	0,	-1,	/* 394 */
	YYLNK,	124,
	0,	-1,	/* 395 */
	'\n',	897,
	0,	-1,	/* 396 */
	'\n',	898,
	0,	-1,	/* 397 */
	'\n',	899,
	0,	-1,	/* 398 */
	'\n',	900,
	0,	-1,	/* 399 */
	'\n',	901,
	0,	-1,	/* 400 */
	'\n',	902,
	0,	-1,	/* 401 */
	'\n',	903,
	0,	-1,	/* 402 */
	'\n',	904,
	0,	-1,	/* 403 */
	'\n',	905,
	0,	-1,	/* 404 */
	')',	906,
	0,	-1,	/* 405 */
	')',	908,
	0,	-1,	/* 406 */
	KW_A,	910,
	0,	-1,	/* 407 */
	KW_A,	912,
	0,	-1,	/* 408 */
	'\n',	914,
	0,	-1,	/* 409 */
	'\n',	915,
	0,	-1,	/* 410 */
	'\n',	916,
	0,	-1,	/* 411 */
	'\n',	917,
	0,	-1,	/* 412 */
	'\n',	918,
	0,	-1,	/* 413 */
	'\n',	919,
	0,	20013,	/* 414 */
	0,	20015,	/* 415 */
	0,	20129,	/* 416 */
	0,	20131,	/* 417 */
	0,	20130,	/* 418 */
	0,	20132,	/* 419 */
	0,	20103,	/* 420 */
	comment,	96,
	0,	20103,	/* 421 */
	comment,	96,
	0,	20103,	/* 422 */
	comment,	96,
	0,	20012,	/* 423 */
	0,	20014,	/* 424 */
	0,	20103,	/* 425 */
	comment,	96,
	0,	20103,	/* 426 */
	comment,	96,
	0,	20003,	/* 427 */
	0,	20004,	/* 428 */
	0,	20005,	/* 429 */
	0,	20006,	/* 430 */
	0,	20007,	/* 431 */
	0,	20103,	/* 432 */
	comment,	96,
	0,	20016,	/* 433 */
	0,	20019,	/* 434 */
	0,	20024,	/* 435 */
	0,	20025,	/* 436 */
	0,	20103,	/* 437 */
	comment,	96,
	0,	20056,	/* 438 */
	0,	20057,	/* 439 */
	0,	20058,	/* 440 */
	0,	20059,	/* 441 */
	0,	20060,	/* 442 */
	0,	20061,	/* 443 */
	0,	20063,	/* 444 */
	0,	20065,	/* 445 */
	0,	20069,	/* 446 */
	0,	20103,	/* 447 */
	comment,	96,
	0,	20103,	/* 448 */
	comment,	96,
	0,	20103,	/* 449 */
	comment,	96,
	0,	20103,	/* 450 */
	comment,	96,
	0,	20091,	/* 451 */
	0,	20092,	/* 452 */
	0,	20028,	/* 453 */
	0,	20029,	/* 454 */
	0,	20030,	/* 455 */
	0,	20036,	/* 456 */
	0,	-1,	/* 457 */
	'\n',	942,
	0,	-1,	/* 458 */
	'\n',	943,
	0,	-1,	/* 459 */
	'\n',	944,
	0,	-1,	/* 460 */
	'\n',	945,
	0,	-1,	/* 461 */
	'\n',	946,
	0,	-1,	/* 462 */
	'\n',	947,
	0,	-1,	/* 463 */
	'\n',	948,
	0,	-1,	/* 464 */
	'\n',	949,
	0,	-1,	/* 465 */
	'\n',	950,
	0,	-1,	/* 466 */
	'\n',	951,
	0,	-1,	/* 467 */
	'\n',	952,
	0,	20011,	/* 468 */
	0,	20018,	/* 469 */
	0,	20010,	/* 470 */
	0,	20009,	/* 471 */
	0,	20008,	/* 472 */
	0,	20017,	/* 473 */
	0,	20026,	/* 474 */
	0,	20076,	/* 475 */
	0,	20075,	/* 476 */
	0,	20078,	/* 477 */
	0,	20077,	/* 478 */
	0,	0
};
typedef	struct {
	short	old;
	short	new;
} YYGOTO;
YYGOTO	yyg1[] = {
	0,	5,
	-1,	-1
};
YYGOTO	yyg2[] = {
	0,	8,
	5,	95,
	-1,	-1
};
YYGOTO	yyg3[] = {
	0,	9,
	5,	9,
	-1,	-1
};
YYGOTO	yyg4[] = {
	105,	331,
	257,	456,
	274,	482,
	284,	488,
	287,	494,
	503,	662,
	505,	664,
	521,	687,
	526,	693,
	566,	714,
	568,	718,
	570,	722,
	639,	765,
	794,	872,
	796,	876,
	-1,	-1
};
YYGOTO	yyg5[] = {
	9,	267,
	147,	352,
	149,	354,
	151,	356,
	153,	358,
	155,	360,
	157,	362,
	159,	364,
	161,	366,
	163,	368,
	167,	372,
	169,	374,
	171,	376,
	173,	378,
	182,	380,
	184,	382,
	215,	416,
	227,	422,
	229,	424,
	249,	448,
	259,	464,
	265,	473,
	269,	476,
	299,	498,
	338,	535,
	340,	537,
	342,	539,
	344,	541,
	370,	558,
	402,	572,
	404,	574,
	408,	578,
	412,	582,
	418,	587,
	420,	589,
	426,	593,
	434,	601,
	438,	605,
	440,	607,
	442,	609,
	444,	611,
	446,	613,
	450,	616,
	452,	618,
	454,	620,
	456,	624,
	459,	628,
	462,	630,
	468,	633,
	482,	643,
	484,	645,
	486,	647,
	488,	649,
	490,	651,
	492,	653,
	494,	655,
	496,	657,
	501,	660,
	662,	782,
	664,	784,
	681,	798,
	683,	800,
	687,	806,
	689,	808,
	691,	810,
	693,	812,
	695,	814,
	699,	818,
	701,	820,
	707,	822,
	709,	824,
	714,	828,
	716,	830,
	718,	832,
	720,	834,
	722,	836,
	724,	838,
	728,	840,
	731,	842,
	734,	844,
	756,	854,
	759,	856,
	765,	858,
	767,	860,
	769,	862,
	771,	864,
	872,	920,
	874,	922,
	876,	924,
	880,	926,
	882,	928,
	889,	930,
	895,	932,
	906,	934,
	908,	936,
	910,	938,
	912,	940,
	-1,	-1
};
YYGOTO	yyg6[] = {
	83,	299,
	165,	370,
	186,	384,
	188,	386,
	190,	388,
	192,	402,
	203,	408,
	213,	412,
	225,	420,
	231,	426,
	237,	433,
	239,	433,
	241,	440,
	243,	433,
	245,	433,
	247,	446,
	274,	484,
	305,	501,
	324,	517,
	430,	599,
	521,	689,
	526,	695,
	528,	699,
	576,	728,
	580,	731,
	584,	734,
	603,	747,
	622,	756,
	626,	759,
	639,	767,
	667,	786,
	669,	788,
	672,	790,
	674,	792,
	685,	802,
	697,	816,
	739,	848,
	-1,	-1
};
YYGOTO	yyg7[] = {
	105,	333,
	192,	404,
	274,	486,
	284,	490,
	287,	496,
	521,	691,
	566,	716,
	568,	720,
	570,	724,
	639,	769,
	-1,	-1
};
YYGOTO	yyg8[] = {
	105,	335,
	122,	335,
	130,	335,
	257,	335,
	262,	335,
	271,	335,
	284,	335,
	324,	519,
	528,	335,
	635,	335,
	641,	335,
	685,	804,
	794,	335,
	826,	335,
	-1,	-1
};
YYGOTO	yyg9[] = {
	105,	336,
	122,	340,
	130,	344,
	257,	459,
	262,	467,
	271,	480,
	284,	492,
	528,	701,
	635,	467,
	641,	771,
	794,	874,
	826,	895,
	-1,	-1
};
YYGOTO	yyg10[] = {
	9,	269,
	-1,	-1
};
YYGOTO	yyg11[] = {
	9,	271,
	-1,	-1
};
YYGOTO	yyg12[] = {
	9,	274,
	-1,	-1
};
YYGOTO	yyg13[] = {
	9,	284,
	-1,	-1
};
YYGOTO	yyg14[] = {
	9,	287,
	-1,	-1
};
YYGOTO	yyg15[] = {
	192,	406,
	203,	410,
	213,	414,
	215,	418,
	-1,	-1
};
YYGOTO	yyg16[] = {
	237,	434,
	239,	438,
	243,	442,
	245,	444,
	-1,	-1
};
YYGOTO	yyg17[] = {
	262,	468,
	-1,	-1
};
YYGOTO	yyg18[] = {
	262,	470,
	635,	764,
	-1,	-1
};
YYGOTO	yyg19[] = {
	262,	471,
	-1,	-1
};
YYGOTO	yyg20[] = {
	237,	436,
	239,	436,
	243,	436,
	245,	436,
	-1,	-1
};
YYGOTO	yyg21[] = {
	83,	301,
	165,	301,
	186,	301,
	188,	301,
	190,	301,
	192,	301,
	203,	301,
	213,	301,
	225,	301,
	231,	301,
	237,	301,
	239,	301,
	241,	301,
	243,	301,
	245,	301,
	247,	301,
	274,	301,
	305,	301,
	324,	301,
	430,	301,
	521,	301,
	526,	301,
	528,	301,
	576,	301,
	580,	301,
	584,	301,
	603,	301,
	622,	301,
	626,	301,
	639,	301,
	667,	301,
	669,	301,
	672,	301,
	674,	301,
	685,	301,
	697,	301,
	739,	301,
	-1,	-1
};
YYGOTO	yyg22[] = {
	83,	303,
	165,	303,
	186,	303,
	188,	303,
	190,	303,
	192,	303,
	203,	303,
	213,	303,
	225,	303,
	231,	303,
	237,	303,
	239,	303,
	241,	303,
	243,	303,
	245,	303,
	247,	303,
	274,	303,
	301,	500,
	305,	303,
	324,	303,
	430,	303,
	521,	303,
	526,	303,
	528,	303,
	576,	303,
	580,	303,
	584,	303,
	603,	303,
	622,	303,
	626,	303,
	639,	303,
	667,	303,
	669,	303,
	672,	303,
	674,	303,
	685,	303,
	697,	303,
	739,	303,
	-1,	-1
};
YYGOTO	yyg23[] = {
	83,	304,
	165,	304,
	186,	304,
	188,	304,
	190,	304,
	192,	304,
	203,	304,
	213,	304,
	225,	304,
	231,	304,
	237,	304,
	239,	304,
	241,	304,
	243,	304,
	245,	304,
	247,	304,
	274,	304,
	301,	304,
	305,	304,
	324,	304,
	430,	304,
	521,	304,
	526,	304,
	528,	304,
	576,	304,
	580,	304,
	584,	304,
	603,	304,
	622,	304,
	626,	304,
	639,	304,
	667,	304,
	669,	304,
	672,	304,
	674,	304,
	685,	304,
	697,	304,
	739,	304,
	-1,	-1
};
YYGOTO	*yyg[] = {
	0,
	yyg1,
	yyg1,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg2,
	yyg3,
	yyg3,
	yyg5,
	yyg5,
	yyg10,
	yyg10,
	yyg10,
	yyg10,
	yyg10,
	yyg10,
	yyg10,
	yyg10,
	yyg11,
	yyg11,
	yyg11,
	yyg12,
	yyg12,
	yyg12,
	yyg12,
	yyg12,
	yyg13,
	yyg13,
	yyg14,
	yyg14,
	yyg14,
	yyg14,
	yyg14,
	yyg14,
	yyg14,
	yyg7,
	yyg7,
	yyg7,
	yyg7,
	yyg7,
	yyg7,
	yyg7,
	yyg4,
	yyg4,
	yyg4,
	yyg4,
	yyg4,
	yyg4,
	yyg4,
	yyg9,
	yyg9,
	yyg9,
	yyg9,
	yyg9,
	yyg8,
	yyg8,
	yyg15,
	yyg15,
	yyg15,
	yyg15,
	yyg15,
	yyg15,
	yyg15,
	yyg15,
	yyg18,
	yyg18,
	yyg17,
	yyg19,
	yyg19,
	yyg16,
	yyg20,
	yyg20,
	yyg6,
	yyg21,
	yyg21,
	yyg22,
	yyg22,
	yyg22,
	yyg22,
	yyg22,
	yyg22,
	yyg23,
	yyg23,
	yyg23,
	yyg23,
};
short	yylen[] = {
	1,
	2,
	1,
	7,
	7,
	7,
	7,
	7,
	9,
	9,
	9,
	9,
	7,
	7,
	7,
	7,
	7,
	9,
	9,
	7,
	5,
	5,
	5,
	5,
	7,
	7,
	9,
	4,
	7,
	7,
	7,
	5,
	5,
	5,
	5,
	5,
	7,
	5,
	4,
	4,
	4,
	4,
	4,
	4,
	4,
	4,
	4,
	5,
	4,
	4,
	4,
	4,
	5,
	5,
	4,
	4,
	7,
	7,
	7,
	7,
	7,
	7,
	5,
	7,
	5,
	7,
	5,
	5,
	5,
	7,
	4,
	5,
	4,
	4,
	5,
	9,
	9,
	9,
	9,
	5,
	5,
	5,
	5,
	5,
	5,
	5,
	5,
	5,
	4,
	5,
	5,
	7,
	7,
	4,
	5,
	5,
	4,
	6,
	5,
	3,
	2,
	0,
	1,
	0,
	1,
	1,
	1,
	1,
	1,
	1,
	1,
	1,
	1,
	1,
	1,
	1,
	1,
	1,
	1,
	1,
	1,
	1,
	1,
	1,
	1,
	1,
	1,
	1,
	1,
	5,
	5,
	5,
	5,
	3,
	3,
	3,
	1,
	1,
	1,
	1,
	1,
	1,
	1,
	1,
	1,
	1,
	1,
	1,
	1,
	1,
	1,
	1,
	1,
	1,
	1,
	1,
	1,
	1,
	1,
	1,
	1,
	3,
	1,
	1,
	3,
	1,
	1,
	2,
	1,
	1,
	1,
	1,
	1,
	1,
	1,
	1,
	1,
	1,
	1,
};
#include <yaccpar.c>



char * reg_name( NODE_p node )
{
	switch( node->number ){
	case KW_A :
		return( "AL" );
	case KW_B :
		return( "CH" );
	case KW_C :
		return( "CL" );
	case KW_D :
		return( "DH" );
	case KW_E :
		return( "DL" );
	case KW_H :
		return( "BH" );
	case KW_L :
		return( "BL" );

	case KW_HL :
		return( "BX" );
	case KW_IX :
		return( "SI" );
	case KW_IY :
		return( "DI" );
	case KW_BC :
		return( "CX" );
	case KW_DE :
		return( "DX" );
	case KW_SP :
		return( "SP" );

	default :
		return( "?REG?" );
	}
}

char * cond_name( NODE_p cond )
{
	switch( cond->number ){
	case KW_C :
		return( "C" );
	case KW_NC :
		return( "NC" );
	case KW_Z :
		return( "Z" );
	case KW_NZ :
		return( "NZ" );
	case KW_P :
		return( "P" );
	case KW_M :
		return( "M" );
	case KW_PE :
		yyerror("ERROR : JP PE not suppoerted");
		return("?COND?");
	case KW_PO :
		yyerror("ERROR : JP PO not suppoerted");
		return("?COND?");
	default :
		yyerror("ERROR : unknown condition caused");
		return("?COND?");
	}
}

char * mnm_name( NODE_p node )
{
	switch( node->number ){

	case KW_SBC :
		return( "SBB" );
	case KW_CP :
		return( "CMP" );

	case KW_RLC :
		return( "ROL" );
	case KW_RL :
		return( "RCL" );
	case KW_RRC :
		return( "ROR" );
	case KW_RR :
		return( "RCR" );
	case KW_SLA :
		return( "SAL" );
	case KW_SRA :
		return( "SAR" );
	case KW_SRL :
		return( "SHR" );

	default :
		return( node->value );
	}
}


/*数値定数ｔｏｋｅｎを読み込む(yylexの機能ルーチン)*/
/*Ｈ形式１６進数と１０進定数を扱うが、識別は完全ではない*/
/*（末尾にＨのつかない１６進定数を許してしまう）*/
void get_const(  NODE_p node )
{
	int ch;

	while(1){
		ch = getchar();
		if( ! isxdigit( ch ) ) break;
		charcat( node->value, ch );
	}
	if( toupper( ch ) == 'H' ){
		charcat( node->value, ch );
	} else {
		ungetchar( ch );
	}
}



/*％２進定数ｔｏｋｅｎを読み込む(yylexの機能ルーチン)*/
/*（％形式からＢ形式への変換を含む）*/
void get_bin_const( NODE_p node )
{
	int ch;

	node->value[0] = '0';
	node->value[1] = '\0';
	while(1){
		ch = getchar();
		if( ch == '$' ) continue;
		if( (ch != '0')&&( ch != '1') ) break;
		charcat( node->value, ch );
	}
	charcat( node->value, 'B' );
	ungetchar( ch );
}


/*＄１６進定数ｔｏｋｅｎを読み込む(yylexの機能ルーチン)*/
/*（＄形式からＨ形式への変換を含む）*/
void get_hex_const( NODE_p node )
{
	int ch;

	node->value[0] = '0';
	node->value[1] = '\0';
	while(1){
		ch = getchar();
		if( ! isxdigit( ch ) ) break;
		charcat( node->value, ch );
	}
	charcat( node->value, 'H' );
	ungetchar( ch );
}


/*文字列ｔｏｋｅｎを読み込む(yylexの機能ルーチン)*/
void get_str_const( NODE_p node, int term_char )
{
	int ch;

	while(1){
		ch = getchar();
		charcat( node->value, ch );
		if( ch == term_char ) break;
	}
}

/*コメントｔｏｋｅｎを読み込む(yylexの機能ルーチン)*/
void get_comment( NODE_p node )
{
	int ch;

	while(1){
		ch = getchar();
		if( ch == '\n' ) break;
		charcat( node->value, ch );
	}
	ungetchar( ch );
}



/*ラベルまたは予約語ｔｏｋｅｎを読み込む(yylexの機能ルーチン)*/
int get_names( NODE_p node )
{
	int ch;
	int id;
	int n;

	*(node->value) = toupper( *(node->value) );

	/*まず、その名前をバッファに読み込む*/
	while(1){
		ch = getchar();
		if( isalpha( ch ) ){
			charcat( node->value, toupper( ch ) );
		} else if( isdigit( ch ) ){
			charcat( node->value, ch );
		} else if( ch == '_' ){
			charcat( node->value, ch );
		} else if( ch == '.' ){
			charcat( node->value, ch );
		} else if( ch == '?' ){
			charcat( node->value, ch );
		} else if( ch == '@' ){
			charcat( node->value, ch );
		} else if( ch == '\'' ){
			charcat( node->value, ch );
		} else break;
	}
	ungetchar( ch );

	/*その名前を検索する*/
	id = sch_name( node->value );		/*ハッシュサーチ！*/
	if( id == -1 ){			/*予約語テーブルから発見できなかった*/
		return( label );	/*予約語でなければラベル扱い*/
	}
	n = id_to_number[id];
	node->number = n;		/*構造体にも予約語tokenコードを収めておく*/
	return( n );			/*予約語tokenコードを返す*/
}


int yylex( void )
{
	int c;
	c = getchar();

	/*すべてのｔｏｋｅｎに一律にＮＯＤＥを割り当てる*/
	yylval = make_node();

	/*とにかく、その１文字は９９パーセントｔｏｋｅｎのｖａｌｕｅ*/
	/*の１文字目になるので、セットする*/
	yylval->value[0] = c;
	yylval->value[1] = '\0';

	/*０－９なら、数値定数の処理*/
	if( isdigit( c ) ) {
		get_const( yylval );
		return( const );
	/*Ａ－Ｚなら、ラベルまたは予約語の処理*/
	} else if( isalpha( c ) ){
		return( get_names( yylval ) );
	} else switch( c ) {
		case '%' :
			get_bin_const( yylval );
			return( const );
		case '$' :
			get_hex_const( yylval );
			return( const );
		case '\"' :
			get_str_const( yylval, c );
			return( str_const );
		case '\'' :
			get_str_const( yylval, c );
			return( str_const );
		case ';' :
			get_comment( yylval );
			return( comment );
		case ' ' :
		case '\t' :
			return( yylex() );
	}

	return( c );
}


/*　予約語初期化で使用される構造体　*/
typedef struct {
	char * name;
	int code_number;
} TOKEN_TABLE;


/*予約語、初期化テーブル*/
TOKEN_TABLE token_table[] = {
	{ "EQU",KW_EQU },

	{ "LD",KW_LD },
	{ "PUSH",KW_PUSH },
	{ "POP",KW_POP },
	{ "EX",KW_EX },

	{ "ADD", KW_ADD },
	{ "SUB", KW_SUB },
	{ "ADC", KW_ADC },
	{ "SBC", KW_SBC },
	{ "AND", KW_AND },
	{ "OR", KW_OR },
	{ "XOR", KW_XOR },
	{ "CP", KW_CP },
	{ "INC", KW_INC },
	{ "DEC", KW_DEC },

	{ "LDI",KW_LDI },
	{ "LDIR",KW_LDIR },
	{ "LDD",KW_LDD },
	{ "LDDR",KW_LDDR },
	{ "CPI",KW_CPI },
	{ "CPIR",KW_CPIR },
	{ "CPD",KW_CPD },
	{ "CPDR",KW_CPDR },

	{ "DAA", KW_DAA },
	{ "CPL", KW_CPL },
	{ "NEG", KW_NEG },
	{ "CCF", KW_CCF },
	{ "SCF", KW_SCF },
	{ "NOP", KW_NOP },
	{ "HALT", KW_HALT },
	{ "DI", KW_DI },
	{ "EI", KW_EI },
	{ "IM", KW_IM },

	{ "RLCA", KW_RLCA },
	{ "RLA", KW_RLA },
	{ "RRCA", KW_RRCA },
	{ "RRA", KW_RRA },
	{ "RLC", KW_RLC },
	{ "RL", KW_RL },
	{ "RRC", KW_RRC },
	{ "RR", KW_RR },
	{ "SLA", KW_SLA },
	{ "SRA", KW_SRA },
	{ "SRL", KW_SRL },
	{ "RLD", KW_RLD },
	{ "RRD", KW_RRD },

	{ "BIT", KW_BIT },
	{ "SET", KW_SET },
	{ "RES", KW_RES },

	{ "JP",KW_JP },
	{ "JR", KW_JR },
	{ "CALL", KW_CALL },
	{ "RET", KW_RET },
	{ "DJNZ", KW_DJNZ },
	{ "RETI", KW_RETI },
	{ "RETN", KW_RETN },
	{ "RST", KW_RST },
	{ "IN", KW_IN },
	{ "INI", KW_INI },
	{ "INIR", KW_INIR },
	{ "IND", KW_IND },
	{ "INDR", KW_INDR },
	{ "OUT", KW_OUT },
	{ "OUTI", KW_OUTI },
	{ "OTIR", KW_OTIR },
	{ "OUTD", KW_OUTD },
	{ "OTDR", KW_OTDR },

	{ "DB", KW_DB },
	{ "DW", KW_DW },
	{ "DS", KW_DS },
	{ "DEFB", KW_DEFB },
	{ "DEFW", KW_DEFW },
	{ "DEFS", KW_DEFS },
	{ "ORG", KW_ORG },
	{ "END", KW_END },
	{ "INCLUDE", KW_INCLUDE },
	{ "TITLE", KW_TITLE },
	{ "SUBTTL", KW_SUBTTL },

	{ "LOOP", KW_LOOP },
	{ "LEND", KW_LEND },
	{ "KEEP", KW_KEEP },
	{ "KEEPOUT", KW_KEEPOUT },

	{ "A",KW_A },
	{ "B",KW_B },
	{ "C",KW_C },
	{ "D",KW_D },
	{ "E",KW_E },
	{ "H",KW_H },
	{ "L",KW_L },

	{ "I",KW_I },
	{ "R",KW_R },

	{ "AF",KW_AF },
	{ "AF'",KW_AF_DUSH },
	{ "BC",KW_BC },
	{ "DE",KW_DE },
	{ "SP",KW_SP },
	{ "HL",KW_HL },
	{ "IX",KW_IX },
	{ "IY",KW_IY },

	{ "NC",KW_NC },
	{ "Z",KW_Z },
	{ "NZ",KW_NZ },
	{ "P",KW_P },
	{ "M",KW_M },
	{ "PE",KW_PE },
	{ "PO",KW_PO },


	{ NULL, 0 }
};


/*予約語の初期化*/
/*予約語テーブルから、ハッシュサーチテーブルに登録する*/
void token_pre_def( void )
{
	TOKEN_TABLE * tp;
	int id;

	sch_init();
	tp = token_table;
	while( tp->name != NULL ){
		id = add_name( tp->name );
		id_to_number[id] = tp->code_number;
		tp++;
	}
}


void yyerror(char *s)
{
	fputs(s, stderr);
}


void	main()
{
	token_pre_def();	/*予約語の初期化*/

	printf("\t\tinclude\taza86.inc\n");
	yyparse();
}
