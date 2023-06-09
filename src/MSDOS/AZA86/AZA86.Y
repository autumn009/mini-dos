
%{
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


%}

%token label
%token const
%token str_const
%token comment

%token	KW_A
%token	KW_B
%token	KW_C
%token	KW_D
%token	KW_E
%token	KW_H
%token	KW_L

%token	KW_I
%token	KW_R

%token	KW_IX
%token	KW_IY
%token	KW_HL
%token	KW_DE
%token	KW_BC
%token	KW_SP
%token	KW_AF
%token	KW_AF_DUSH

%token	KW_NC
%token	KW_Z
%token	KW_NZ
%token	KW_P
%token	KW_M
%token	KW_PE
%token	KW_PO

%token	KW_EQU

%token	KW_LDI
%token	KW_LDIR
%token	KW_LDD
%token	KW_LDDR
%token	KW_CPI
%token	KW_CPIR
%token	KW_CPD
%token	KW_CPDR

%token	KW_LD
%token	KW_PUSH
%token	KW_POP
%token	KW_EX


%token	KW_ADD
%token	KW_SUB
%token	KW_ADC
%token	KW_SBC
%token	KW_AND
%token	KW_OR
%token	KW_XOR
%token	KW_CP
%token	KW_INC
%token	KW_DEC

%token	KW_DAA
%token	KW_CPL
%token	KW_NEG
%token	KW_CCF
%token	KW_SCF
%token	KW_NOP
%token	KW_HALT
%token	KW_DI
%token	KW_EI
%token	KW_IM

%token	KW_RLCA
%token	KW_RLA
%token	KW_RRCA
%token	KW_RRA
%token	KW_RLC
%token	KW_RL
%token	KW_RRC
%token	KW_RR
%token	KW_SLA
%token	KW_SRA
%token	KW_SRL
%token	KW_RLD
%token	KW_RRD
%token	KW_BIT
%token	KW_SET
%token	KW_RES

%token	KW_JP
%token	KW_JR
%token	KW_CALL
%token	KW_RET
%token	KW_DJNZ
%token	KW_RETI
%token	KW_RETN
%token	KW_RST
%token	KW_IN
%token	KW_INI
%token	KW_INIR
%token	KW_IND
%token	KW_INDR
%token	KW_OUT
%token	KW_OUTI
%token	KW_OTIR
%token	KW_OUTD
%token	KW_OTDR

%token	KW_DB
%token	KW_DW
%token	KW_DS
%token	KW_DEFB
%token	KW_DEFW
%token	KW_DEFS
%token	KW_ORG
%token	KW_END
%token	KW_INCLUDE
%token	KW_TITLE
%token	KW_SUBTTL

%token	KW_LOOP
%token	KW_LEND
%token	KW_KEEP
%token	KW_KEEPOUT

%%

program	:	program line
	|	line
	;


line
/* LD reg,reg */
	: l_def KW_LD reg ',' reg c_def '\n'	{
		strcpy( mnem, "MOV" );
		sprintf( operand, "%s,%s", reg_name($3), reg_name($5) );
		line_putout_with_release_nodes();
	}

/* LD reg,n */
	| l_def KW_LD reg ',' expr c_def '\n'	{
		strcpy( mnem, "MOV" );
		sprintf( operand, "%s,offset %s", reg_name($3), $5 );
		line_putout_with_release_nodes();
	}

/* LD reg,(IX+n) */
	| l_def KW_LD reg ',' idx c_def '\n'	{
		strcpy( mnem, "MOV" );
		sprintf( operand, "%s,byte ptr %s", reg_name($3), $5->value);
		line_putout_with_release_nodes();
	}

/* LD (IX+n),reg */
	| l_def KW_LD idx ',' reg c_def '\n'	{
		strcpy( mnem, "MOV" );
		sprintf( operand, "byte ptr %s,%s", $3->value, reg_name($5) );
		line_putout_with_release_nodes();
	}

/* LD (IX+n),n */
	| l_def KW_LD idx ',' expr c_def '\n'	{
		strcpy( mnem, "MOV" );
		sprintf( operand, "byte ptr %s,offset %s", $3->value, $5->value);
		line_putout_with_release_nodes();
	}

/* LD A,(DE/BC)*/
	| l_def KW_LD reg ',' '(' bc_or_de ')' c_def '\n'	{
		strcpy( mnem, "XCHG" );
		sprintf( operand, "BX,%s",reg_name( $6 ) );
		line_putout();
		strcpy( mnem, "MOV" );
		sprintf( operand, "%s,byte ptr [BX]",reg_name( $3 ) );
		line_putout();
		strcpy( mnem, "XCHG" );
		sprintf( operand, "BX,%s",reg_name( $6 ) );
		line_putout_with_release_nodes();
	}

/* LD A,(expr)*/
	| l_def KW_LD reg ',' '(' expr ')' c_def '\n'	{
		strcpy( mnem, "MOV" );
		sprintf( operand, "%s,byte ptr [%s]", reg_name($3), $6->value);
		line_putout_with_release_nodes();
	}

/* LD (DE/BC),A*/
	| l_def KW_LD '(' bc_or_de ')' ',' reg c_def '\n'	{
		strcpy( mnem, "XCHG" );
		sprintf( operand, "BX,%s",reg_name( $4 ) );
		line_putout();
		strcpy( mnem, "MOV" );
		sprintf( operand, "byte ptr [BX],%s",reg_name( $7 ) );
		line_putout();
		strcpy( mnem, "XCHG" );
		sprintf( operand, "BX,%s",reg_name( $4 ) );
		line_putout_with_release_nodes();
	}

/* LD (expr),A*/
	| l_def KW_LD '(' expr ')' ',' reg c_def '\n'	{
		strcpy( mnem, "MOV" );
		sprintf( operand, "byte ptr [%s],%s", $4->value, reg_name($7) );
		line_putout_with_release_nodes();
	}

/* LD A,I */
	| l_def KW_LD reg ',' KW_I c_def '\n'	{
		unconvertable( "LD A,I","Ｉレジスタは変換できません" );
	}

/* LD I,A */
	| l_def KW_LD KW_I ',' reg c_def '\n'	{
		unconvertable( "LD I,A","Ｉレジスタは変換できません" );
	}

/* LD A,R */
	| l_def KW_LD reg ',' KW_R c_def '\n'	{
		unconvertable( "LD A,R","Ｒレジスタは変換できません" );
	}

/* LD R,A */
	| l_def KW_LD KW_R ',' reg c_def '\n'	{
		unconvertable( "LD R,A","Ｒレジスタは変換できません" );
	}

/* LD dd,expr */
	| l_def KW_LD dd ',' expr c_def '\n'	{
		strcpy( mnem, "MOV" );
		sprintf( operand, "%s,offset %s", reg_name($3), $5 );
		line_putout_with_release_nodes();
	}

/* LD dd,(expr) */
	| l_def KW_LD dd ',' '(' expr ')' c_def '\n'	{
		strcpy( mnem, "MOV" );
		sprintf( operand, "%s,word ptr [%s]", reg_name($3), $6 );
		line_putout_with_release_nodes();
	}

/* LD (expr),dd */
	| l_def KW_LD '(' expr ')' ',' dd c_def '\n'	{
		strcpy( mnem, "MOV" );
		sprintf( operand, "word ptr [%s],%s", $4, reg_name($7) );
		line_putout_with_release_nodes();
	}

/* LD SP,dd */
	| l_def KW_LD dd ',' dd c_def '\n'	{
		strcpy( mnem, "MOV" );
		sprintf( operand, "%s,%s", reg_name($3), reg_name($5) );
		line_putout_with_release_nodes();
	}

/* PUSH dd */
	| l_def KW_PUSH dd c_def '\n'	{
		strcpy( mnem, "PUSH" );
		strcpy( operand, reg_name($3) );
		line_putout_with_release_nodes();
	}

/* POP dd */
	| l_def KW_POP dd c_def '\n'	{
		strcpy( mnem, "POP" );
		strcpy( operand, reg_name($3) );
		line_putout_with_release_nodes();
	}

/* PUSH AF */
	| l_def KW_PUSH KW_AF c_def '\n'	{
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

/* POP AF */
	| l_def KW_POP KW_AF c_def '\n'	{
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

/* EX DE,HL */
	| l_def KW_EX KW_DE ',' KW_HL c_def '\n'	{
		strcpy( mnem, "XCHG" );
		sprintf( operand, "%s,%s", reg_name($3), reg_name($5) );
		line_putout_with_release_nodes();
	}

/* EX AF,AF' */
	| l_def KW_EX KW_AF ',' KW_AF_DUSH c_def '\n'	{
		unconvertable( "EX AF,AF\'","裏レジスタは存在しません" );
	}

/* EX (SP),dd */
	| l_def KW_EX '(' KW_SP ')' ',' dd c_def '\n'	{
		strcpy( mnem, "XCHG" );
		sprintf( operand, "word ptr [%s],%s", reg_name($4), reg_name($7) );
		line_putout_with_release_nodes();
	}

/* LDI,LDIR,...,CPI,... */
	| l_def strings c_def '\n'	{
		strcpy( mnem, $2->value );
		line_putout_with_release_nodes();
	}

/* ADD,SBC,... */
	| l_def calc_with_src KW_A ',' reg c_def '\n'	{
		strcpy( mnem, $2->value );
		sprintf( operand, "AL,%s", reg_name($5) );
		line_putout_with_release_nodes();
	}

/* ADD,SBC,... */
	| l_def calc_with_src KW_A ',' expr c_def '\n'	{
		strcpy( mnem, mnm_name($2) );
		sprintf( operand, "AL,offset %s", $5->value );
		line_putout_with_release_nodes();
	}

/* ADD,SBC,... */
	| l_def calc_with_src KW_A ',' idx c_def '\n'	{
		strcpy( mnem, mnm_name($2) );
		sprintf( operand, "AL,byte ptr %s", $5->value );
		line_putout_with_release_nodes();
	}

/* AND,OR,... */
	| l_def calcs reg c_def '\n'	{
		strcpy( mnem, mnm_name($2) );
		sprintf( operand, "AL,%s", reg_name($3) );
		line_putout_with_release_nodes();
	}

/* AND,OR,... */
	| l_def calcs expr c_def '\n'	{
		strcpy( mnem, mnm_name($2) );
		sprintf( operand, "AL,offset %s", $3->value );
		line_putout_with_release_nodes();
	}

/* AND,OR,... */
	| l_def calcs idx c_def '\n'	{
		strcpy( mnem, mnm_name($2) );
		sprintf( operand, "AL,byte ptr %s", $3->value );
		line_putout_with_release_nodes();
	}

/* INC/DEC reg */
	| l_def inc_dec reg c_def '\n'	{
		strcpy( mnem, mnm_name($2) );
		sprintf( operand, "%s", reg_name($3) );
		line_putout_with_release_nodes();
	}

/* INC/DEC idx */
	| l_def inc_dec idx c_def '\n'	{
		strcpy( mnem, mnm_name($2) );
		sprintf( operand, "byte ptr %s", $3->value );
		line_putout_with_release_nodes();
	}

/* ADD/ADC/SBC dd,dd */
	| l_def calc_with_src dd ',' dd c_def '\n'	{
		strcpy( mnem, mnm_name($2) );
		sprintf( operand, "%s,%s", reg_name($3), reg_name($5) );
		line_putout_with_release_nodes();
	}

/* INC/DEC dd */
	| l_def inc_dec dd c_def '\n'	{
		strcpy( mnem, mnm_name($2) );
		sprintf( operand, "%s", reg_name($3) );
		line_putout_with_release_nodes();
	}

/* DAA */
	| l_def KW_DAA c_def '\n'	{
		unconvertable( "DAA","not supported" );
	}

/* CPL */
	| l_def KW_CPL c_def '\n'	{
		strcpy( mnem, "NOT" );
		strcpy( operand, "AL" );
		line_putout_with_release_nodes();
	}

/* NEG */
	| l_def KW_NEG c_def '\n'	{
		strcpy( mnem, "NEG" );
		strcpy( operand, "AL" );
		line_putout_with_release_nodes();
	}

/* CCF */
	| l_def KW_CCF c_def '\n'	{
		strcpy( mnem, "CMC" );
		line_putout_with_release_nodes();
	}

/* SCF */
	| l_def KW_SCF c_def '\n'	{
		strcpy( mnem, "STC" );
		line_putout_with_release_nodes();
	}

/* NOP */
	| l_def KW_NOP c_def '\n'	{
		strcpy( mnem, "NOP" );
		line_putout_with_release_nodes();
	}

/* HALT */
	| l_def KW_HALT c_def '\n'	{
		unconvertable( "HALT","not supported" );
	}

/* DI */
	| l_def KW_DI c_def '\n'	{
		strcpy( mnem, "CLI" );
		line_putout_with_release_nodes();
	}

/* EI */
	| l_def KW_EI c_def '\n'	{
		strcpy( mnem, "STI" );
		line_putout_with_release_nodes();
	}

/* IM expr */
	| l_def KW_IM expr c_def '\n'	{
		unconvertable( "IM mode","対応する割り込みモードの概念無し" );
	}

/* RLCA */
	| l_def KW_RLCA c_def '\n'	{
		strcpy( mnem, "ROL" );
		strcpy( operand, "AL,1" );
		line_putout_with_release_nodes();
	}

/* RLA */
	| l_def KW_RLA c_def '\n'	{
		strcpy( mnem, "RCL" );
		strcpy( operand, "AL,1" );
		line_putout_with_release_nodes();
	}

/* RRCA */
	| l_def KW_RRCA c_def '\n'	{
		strcpy( mnem, "ROR" );
		strcpy( operand, "AL,1" );
		line_putout_with_release_nodes();
	}

/* RRA */
	| l_def KW_RRA c_def '\n'	{
		strcpy( mnem, "RCR" );
		strcpy( operand, "AL,1" );
		line_putout_with_release_nodes();
	}

/* rotate & shifts reg */
	| l_def rot_sft reg c_def '\n'	{
		strcpy( mnem, mnm_name($2) );
		sprintf( operand, "%s,1", reg_name($3) );
		line_putout_with_release_nodes();
	}

/* rotate & shifts idx */
	| l_def rot_sft idx c_def '\n'	{
		strcpy( mnem, mnm_name($2) );
		sprintf( operand, "byte ptr %s,1", $3->value );
		line_putout_with_release_nodes();
	}

/* RLD */
	| l_def KW_RLD c_def '\n'	{
		unconvertable( "RLD","対応する命令無し" );
	}

/* RRD */
	| l_def KW_RRD c_def '\n'	{
		unconvertable( "RRD","対応する命令無し" );
	}


/* BIT n,reg */
	| l_def KW_BIT expr ',' reg c_def '\n'	{
		strcpy( mnem, "TEST" );
		sprintf( operand, "%s,1 SHL (%s)", reg_name($5), $3->value );
		line_putout_with_release_nodes();
	}

/* BIT n,idx */
	| l_def KW_BIT expr ',' idx c_def '\n'	{
		strcpy( mnem, "TEST" );
		sprintf( operand, "byte ptr %s,1 SHL (%s)", $5->value, $3->value );
		line_putout_with_release_nodes();
	}

/* SET n,reg */
	| l_def KW_SET expr ',' reg c_def '\n'	{
		strcpy( mnem, "PUSHF" );
		line_putout();
		strcpy( mnem, "OR" );
		sprintf( operand, "%s,1 SHL (%s)", reg_name($5), $3->value );
		line_putout();
		strcpy( mnem, "POPF" );
		line_putout_with_release_nodes();
	}

/* SET n,idx */
	| l_def KW_SET expr ',' idx c_def '\n'	{
		strcpy( mnem, "PUSHF" );
		line_putout();
		strcpy( mnem, "OR" );
		sprintf( operand, "byte ptr %s,1 SHL (%s)", $5->value, $3->value );
		line_putout();
		strcpy( mnem, "POPF" );
		line_putout_with_release_nodes();
	}

/* RES n,reg */
	| l_def KW_RES expr ',' reg c_def '\n'	{
		strcpy( mnem, "PUSHF" );
		line_putout();
		strcpy( mnem, "AND" );
		sprintf( operand, "%s,NOT(1 SHL (%s))", reg_name($5), $3->value );
		line_putout();
		strcpy( mnem, "POPF" );
		line_putout_with_release_nodes();
	}

/* RES n,idx */
	| l_def KW_RES expr ',' idx c_def '\n'	{
		strcpy( mnem, "PUSHF" );
		line_putout();
		strcpy( mnem, "AND" );
		sprintf( operand, "byte ptr %s,NOT(1 SHL (%s))", $5->value, $3->value );
		line_putout();
		strcpy( mnem, "POPF" );
		line_putout_with_release_nodes();
	}


	| l_def KW_JP expr c_def '\n'	{
		strcpy(mnem,"JMP");
		strcpy(operand, $3->value );
		line_putout_with_release_nodes();
	}

	| l_def KW_JP cond ',' expr c_def '\n'	{
		sprintf( mnem, "LJ%s", cond_name( $3 ) );
		strcpy( operand, $5->value );
		line_putout_with_release_nodes();
	}

	| l_def KW_JR expr c_def '\n'	{
#ifdef ALL_LONG_JUMP
		strcpy(mnem,"JMP");
		strcpy(operand, $3->value );
		line_putout_with_release_nodes();
#else
		strcpy(mnem,"JMP");
		sprintf(operand, "short %s",$3->value );
		line_putout_with_release_nodes();
#endif
	}


	| l_def KW_JR cond ',' expr c_def '\n'	{
#ifdef ALL_LONG_JUMP
		sprintf( mnem, "LJ%s", cond_name( $3 ) );
		strcpy( operand, $5->value );
		line_putout_with_release_nodes();
#else
		sprintf( mnem, "J%s", cond_name( $3 ) );
		strcpy( operand, $5->value );
		line_putout_with_release_nodes();
#endif
	}

/* JP (HL/IX/IY) */
	| l_def KW_JP idx c_def '\n'	{
		strcpy(mnem,"JMP");
		strcpy(operand, $3->value );
		line_putout_with_release_nodes();
	}

/* DJNZ:本当は、この展開ではまずいのだが、なんとかなるだろう */
	| l_def KW_DJNZ expr c_def '\n'	{
		strcpy(mnem,"DEC");
		strcpy(operand, "CH" );
		line_putout();
		strcpy(mnem,"JNZ");
		strcpy(operand, $3->value );
		line_putout_with_release_nodes();
	}

/* CALL expr */
	| l_def KW_CALL expr c_def '\n'	{
		strcpy(mnem,"CALL");
		strcpy(operand, $3->value );
		line_putout_with_release_nodes();
	}

/* CALL cond,expr */
	| l_def KW_CALL cond ',' expr c_def '\n'	{
		sprintf( mnem, "CALL%s", cond_name( $3 ) );
		strcpy( operand, $5->value );
		line_putout_with_release_nodes();
	}

/* RET */
	| l_def KW_RET c_def '\n'	{
		strcpy(mnem,"RET");
		line_putout_with_release_nodes();
	}

/* RET cond */
	| l_def KW_RET cond c_def '\n'	{
		sprintf( mnem, "RET%s", cond_name( $3 ) );
		line_putout_with_release_nodes();
	}

/* RETI */
	| l_def KW_RETI c_def '\n'	{
		unconvertable( "RETI","１対１で対応する命令無し" );
	}

/* RETN */
	| l_def KW_RETN c_def '\n'	{
		unconvertable( "RETN","対応する命令無し" );
	}

/* RST expr */
	| l_def KW_RST expr c_def '\n'	{
		unconvertable( (sprintf(buf, "RST %s", $3->value),buf),"対応する命令無し" );
	}


/* IN A,(expr) */
	| l_def KW_IN KW_A ',' '(' expr ')' c_def '\n'	{
		unconvertable( (sprintf(buf, "IN A,(%s)", $6->value),buf),"変換しません" );
	}

/* IN A,(C) */
	| l_def KW_IN KW_A ',' '(' KW_C ')' c_def '\n'	{
		unconvertable( "IN A,(C)", "変換しません" );
	}


/* OUT (expr),A */
	| l_def KW_OUT '(' expr ')' ',' KW_A c_def '\n'	{
		unconvertable( (sprintf(buf, "OUT (%s),A", $4->value),buf),"変換しません" );
	}

/* OUT (C),A */
	| l_def KW_OUT '(' KW_C ')' ',' KW_A c_def '\n'	{
		unconvertable( "OUT (C),A", "変換しません" );
	}


/* db or dw */
	| l_def KW_DB e_list c_def '\n'	{
		strcpy( mnem,"DB");
		strcpy( operand, $3->value );
		line_putout_with_release_nodes();
	}

	| l_def KW_DW e_list c_def '\n'	{
		strcpy( mnem,"DW");
		strcpy( operand, $3->value );
		line_putout_with_release_nodes();
	}

	| l_def KW_DEFB e_list c_def '\n'	{
		strcpy( mnem,"DB");
		strcpy( operand, $3->value );
		line_putout_with_release_nodes();
	}
	| l_def KW_DEFW e_list c_def '\n'	{
		strcpy( mnem,"DW");
		strcpy( operand, $3->value );
		line_putout_with_release_nodes();
	}


/* ds */
	| l_def KW_DS expr c_def '\n'	{
		strcpy( mnem,"DB");
		strcpy( operand, $3->value );
		strcat( operand, " DUP(?)" );
		line_putout_with_release_nodes();
	}

/* include */
	| l_def KW_INCLUDE label c_def '\n'	{
		strcpy( mnem,"INCLUDE");
		strcpy( operand, $3->value );
		line_putout_with_release_nodes();
	}

/* title */
	| l_def KW_TITLE label c_def '\n'	{
		unconvertable( "TITLE","TITLE疑似命令は変換できません" );
	}

/* subttl */
	| l_def KW_SUBTTL label c_def '\n'	{
		unconvertable( "SUBTTL","SUBTTL疑似命令は変換できません" );
	}

/* org */
	| l_def KW_ORG expr c_def '\n'	{
		unconvertable( "ORG","ORG疑似命令は変換できません" );
	}

/* end */
	| l_def KW_END c_def '\n'	{
		strcpy( mnem,"END");
		line_putout_with_release_nodes();
	}

/* LOOP reg */
	| l_def KW_LOOP reg c_def '\n'	{
		strcpy( mnem,"ALOOP");
		sprintf( operand, "%s",reg_name($3) );
		line_putout_with_release_nodes();
	}

/* LOOP dd */
	| l_def KW_LOOP dd c_def '\n'	{
		strcpy( mnem,"ALOOP");
		sprintf( operand, "%s",reg_name($3) );
		line_putout_with_release_nodes();
	}

/* LOOP reg,expr */
	| l_def KW_LOOP reg ',' expr c_def '\n'	{
		strcpy( mnem,"ALOOP");
		sprintf( operand, "%s,%s",reg_name($3),$5->value );
		line_putout_with_release_nodes();
	}

/* LOOP dd,expr */
	| l_def KW_LOOP dd ',' expr c_def '\n'	{
		strcpy( mnem,"ALOOP");
		sprintf( operand, "%s,%s",reg_name($3),$5->value );
		line_putout_with_release_nodes();
	}

/* LEND */
	| l_def KW_LEND c_def '\n'	{
		strcpy( mnem,"LEND");
		line_putout_with_release_nodes();
	}

/* LEND A */
	| l_def KW_LEND KW_A c_def '\n'	{
		strcpy( mnem,"LEND");
		line_putout_with_release_nodes();
	}

/* KEEP */
	| l_def KW_KEEP keep_list c_def '\n'	{
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

/* KEEPOUT */
	| l_def KW_KEEPOUT c_def '\n'	{
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



/* EQU */
	| label ':' KW_EQU expr c_def '\n'	{
		equ_putout( $1, $4 );
	}

	| label     KW_EQU expr c_def '\n'	{
		equ_putout( $1, $3 );
	}

/*　命令本体の無いとき　*/
	| l_def c_def '\n'	{
		strcpy( mnem,"");
		strcpy( operand, "" );
		line_putout_with_release_nodes();
	}

	;


l_def	:	label	':'	{ strcpy( lbl, $1->value ); }
	|			{ strcpy( lbl, "" ); }
	;


c_def	:	comment		{ strcpy( comm, $1->value ); }
	|			{ strcpy( comm, "" ); }
	;


strings	:	KW_LDI | KW_LDIR | KW_LDD | KW_LDDR
	|	KW_CPI | KW_CPIR | KW_CPD | KW_CPDR
	;


calc_with_src :	KW_ADD | KW_ADC | KW_SBC ;
calcs	:	KW_SUB
	|	KW_AND | KW_OR | KW_XOR | KW_CP
	;


inc_dec	:	KW_INC | KW_DEC ;


rot_sft	:	KW_RLC | KW_RL | KW_RRC | KW_RR | KW_SLA | KW_SRA | KW_SRL ;


/* index addressing(with reference by ptr) */
idx	:	'(' KW_IX '+' expr ')'	{
						$$ = make_node();
						sprintf($$->value, "[SI+%s]",$4 );
					}
	|	'(' KW_IY '+' expr ')'	{
						$$ = make_node();
						sprintf($$->value,"[DI+%s]",$4 );
					}
	|	'(' KW_IX '-' expr ')'	{
						$$ = make_node();
						sprintf($$->value,"[SI-%s]",$4 );
					}
	|	'(' KW_IY '-' expr ')'	{
						$$ = make_node();
						sprintf($$->value,"[DI-%s]",$4 );
					}
	|	'(' KW_IX ')'		{ $$ = make_const_node("[SI]"); }
	|	'(' KW_IY ')'		{ $$ = make_const_node("[DI]"); }
	|	'(' KW_HL ')'		{ $$ = make_const_node("[BX]"); }
	;

reg	:	KW_A | KW_B | KW_C | KW_D | KW_E | KW_H | KW_L ;

dd	:	bc_or_de | KW_SP | KW_HL | KW_IX | KW_IY ;

bc_or_de	:	KW_BC | KW_DE ;

cond	:	KW_C | KW_NC | KW_Z | KW_NZ | KW_P | KW_M | KW_PE | KW_PO ;

dd_af	:	dd | KW_AF ;


keep_list	: keep_list0	{
				number_of_klist = kptr - klist_buf;
			}
		;

keep_list0	: keep_list0 ',' dd_af	{
				*kptr++ = $3;
			}
		| dd_af	{
				kptr = klist_buf;
				*kptr++ = $1;
			}
		;


/*任意個の式の並び（ＤＢ／ＤＷ用）*/
e_list	: e_list0	{
				$$ = make_node();
				strcpy( $$->value, elist_buf );
			}
	;

e_list0	: e_list0 ',' expr	{
				strcat( elist_buf, "," );
				strcat( elist_buf, $3->value );
			}
	| expr		{
				elist_buf[0] = '\0';
				strcat( elist_buf, $1->value );
			}
	;


/*式の文法解析をする必要はないから、こんなものでいいんでないかな？*/
expr	:	expr0		{
					/*printf("\nanswer=[%s]\n", expr_buf );*/
					$$ = make_node();
					strcpy( $$->value, expr_buf );
				}
	;

expr0	:	expr0 e_item	{
					/*printf("[1:%s]",$2->value);*/
					strcat( expr_buf, $2->value );
				}
	|	e_item		{
					/*printf("[2:%s]",$1->value);*/
					expr_buf[0] = '\0';
					strcat( expr_buf, $1->value );
				}
	;

e_item	:	label
	|	twoop
	|	const
	|	str_const
	/*|	')'*/		/*丸括弧はindirectの括弧とconfrictしてしまう*/
	/*|	'('*/		/*ので、使用できない*/
	|	'['
	|	']'
	;

twoop	:	'+'
	|	'-'
	|	'*'
	|	'/'
	;


%%


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
