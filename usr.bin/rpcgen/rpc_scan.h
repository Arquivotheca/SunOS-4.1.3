/* @(#)rpc_scan.h 1.1 92/07/30 (C) 1987 SMI */

/*
 * rpc_scan.h, Definitions for the RPCL scanner 
 * Copyright (C) 1987, Sun Microsystems, Inc. 
 */

/*
 * kinds of tokens 
 */
enum tok_kind {
	TOK_IDENT,
	TOK_CHARCONST,
	TOK_STRCONST,
	TOK_LPAREN,
	TOK_RPAREN,
	TOK_LBRACE,
	TOK_RBRACE,
	TOK_LBRACKET,
	TOK_RBRACKET,
	TOK_LANGLE,
	TOK_RANGLE,
	TOK_STAR,
	TOK_COMMA,
	TOK_EQUAL,
	TOK_COLON,
	TOK_SEMICOLON,
	TOK_CONST,
	TOK_STRUCT,
	TOK_UNION,
	TOK_SWITCH,
	TOK_CASE,
	TOK_DEFAULT,
	TOK_ENUM,
	TOK_TYPEDEF,
	TOK_INT,
	TOK_SHORT,
	TOK_LONG,
	TOK_UNSIGNED,
	TOK_FLOAT,
	TOK_DOUBLE,
	TOK_OPAQUE,
	TOK_CHAR,
	TOK_STRING,
	TOK_BOOL,
	TOK_VOID,
	TOK_PROGRAM,
	TOK_VERSION,
	TOK_EOF
};
typedef enum tok_kind tok_kind;

/*
 * a token 
 */
struct token {
	tok_kind kind;
	char *str;
};
typedef struct token token;


/*
 * routine interface 
 */
void scan();
void scan2();
void scan3();
void scan_num();
void peek();
int peekscan();
void get_token();
