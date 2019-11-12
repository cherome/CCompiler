#include<ctype.h>
#include<stdarg.h>
#include<stdbool.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>


// 抽象構文木のノードの種類
typedef enum {
	ND_ADD,	// +
	ND_SUB,	// -
	ND_MUL, // *
	ND_DIV, // /
	ND_NUM, // number
} NodeKind;

typedef struct  Node Node;

// 抽象構文木のノードの型
struct  Node {
	NodeKind kind;	// ノードの型
	Node *lhs;		// 左辺
	Node *rhs;		// 右辺
	int val;		// kindがND_NUMの場合number
};

Node *new_node(NodeKind kind, Node *lhs, Node *rhs) {
	Node *node = calloc(1, sizeof(Node));
	node->kind = kind;
	node->lhs = lhs;
	node->rhs = rhs;
	return node;
}

Node *new_node_num(int val) {
	Node *node = calloc(1, sizeof(Node));
	node->kind = ND_NUM;
	node->val = val;
	return node;
}

Node *expr();
Node *mul();
Node *primary();
bool consume(char op);
void expect(char op);
int expect_number();

Node *expr() {
	Node *node = mul();

	for(;;) {
		if(consume('+'))
			node = new_node(ND_ADD, node, mul());
		else if(consume('-'))
			node = new_node(ND_SUB, node, mul());
		else
			return node;
	}
}

Node *mul() {
	Node *node = primary();
	
	for(;;) {
		if(consume('*'))
			node = new_node(ND_MUL, node, primary());
		else if(consume('/'))
			node = new_node(ND_DIV, node, primary());
		else
			return node;
	}
}

Node *primary() {
	// 次のトークンが"("の場合、"(" expr ")" となるはず。
	if(consume('(')) {
		Node *node = expr();
		expect(')');
		return node;
	}

	// それ以外は数値
	return new_node_num(expect_number());
}

void gen(Node *node) {
	if(node->kind == ND_NUM) {
		printf("\tpush %d\n", node->val);
		return;
	}

	gen(node->lhs);
	gen(node->rhs);

	printf("\tpop rdi\n");
	printf("\tpop rax\n");

	switch (node->kind)	{
	case ND_ADD:
		printf("\tadd rax, rdi\n");
		break;
	case ND_SUB:
		printf("\tsub rax, rdi\n");
		break;
	case ND_MUL:
		printf("\timul rax, rdi\n");
		break;
	case ND_DIV:
		printf("\tcqo\n");
		printf("\tidiv rdi\n");
		break;
	}

	printf("\tpush rax\n");
}

typedef enum {
	TK_RESERVED,	// 記号
	TK_NUM,			// 整数トークン
	TK_EOF			// 入力終わり
} TokenKind;

typedef struct Token Token;

// トークン型
struct Token {
	TokenKind kind;	// トークンの型
	Token *next;	// 次のトークン
	int val;		// kindがTK_NUMの場合、数値
	char *str;		// トークン文字列
};

// 現在見ているトークン
Token *token;
// 入力プログラム
char *user_input;

// エラー箇所報告関数
void error_at(char *loc, char *fmt, ...) {
	va_list ap;
	va_start(ap,fmt);

	int pos = loc - user_input;
	fprintf(stderr, "%s\n", user_input);
	fprintf(stderr, "%*s", pos, "");
	fprintf(stderr, "^ ");
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	exit(1);
}

// エラー用関数
void error(char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	exit(1);
}

// 次のトークンが記号のときはトークンを１つ読み進めて真を返す。
// それ以外の場合は偽を返す。
bool consume(char op) {
	if(token->kind != TK_RESERVED || token->str[0] != op)
		return false;
	token = token->next;
	return true;
}

// 次のトークンが期待している記号のときには、トークンを進める。
// それ以外の場合にはエラーとする。
void expect(char op) {
	if(token->kind != TK_RESERVED || token->str[0] != op)
		error_at(token->str, "'%c'ではありません。");
	token = token->next;
}

// 次のトークンが数値の場合、トークンを進めてその数値を返す。
// それ以外の場合にはエラーとする。
int expect_number() {
	if(token->kind != TK_NUM)
		error_at(token->str, "数値ではありません");
	int val = token->val;
	token = token->next;
	return val;
}

bool at_eof() {
	return token->kind == TK_EOF;
}

// 新しいトークンを作成してcurにつなげる
Token *new_token(TokenKind kind, Token *cur, char *str)
{
	Token *tok = calloc(1,sizeof(Token));
	tok->kind = kind;
	tok->str = str;
	cur->next = tok;
	return tok;
}

// 入力文字列user_inputをトークン化して返却
Token *tokenize() {
	char *p = user_input;
	Token head;
	head.next = NULL;
	Token *cur = &head;

	while(*p) {
		// 空白のスキップ
		if(isspace(*p)){
			p++;
			continue;
		}

		// 演算子系の判定
		if(strchr("+-*/()", *p)) {
			cur = new_token(TK_RESERVED, cur, p++);
			continue;
		}

		if(isdigit(*p)) {
			cur = new_token(TK_NUM, cur, p);
			cur->val = strtol(p, &p, 10);
			continue;
		}

		error_at(token->str, "トークナイズできません。");
	}

	new_token(TK_EOF, cur, p);
	return head.next;
}

int main(int argc, char **argv) {
	if(argc != 2) {
		fprintf(stderr,"引数の個数が正しくありません\n");
		return 1;
	}
	
	// トークナイズしてパースする。
	user_input = argv[1];
	token = tokenize();
	Node *node = expr();

	// おまじない
	printf(".intel_syntax noprefix\n");
	printf(".global main\n");
	printf("main:\n");

	gen(node);

	// スタックトップに式全体の結果が残っているので
	// raxにロードして、返り値とする。
	printf("\tpop rax\n");
	printf("\tret\n");
	return 0;
}
