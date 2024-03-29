#include"9cc.h"

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
bool consume(char *op) {
	if(token->kind != TK_RESERVED || 
		strlen(op) != token->len ||
		memcmp(token->str, op, token->len))
		return false;
	token = token->next;
	return true;
}

// 次のトークンが期待している記号のときには、トークンを進める。
// それ以外の場合にはエラーとする。
void expect(char *op) {
	if(token->kind != TK_RESERVED || 
		strlen(op) != token->len ||
		memcmp(token->str, op, token->len))
		error_at(token->str, "\"%s\"ではありません。", op);
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
Token *new_token(TokenKind kind, Token *cur, char *str, int len)
{
	Token *tok = calloc(1,sizeof(Token));
	tok->kind = kind;
	tok->str = str;
	tok->len = len;
	cur->next = tok;
	return tok;
}

// pがqで始まる文字列か判定
bool startswith(char *p, char *q) {
	return memcmp(p, q, strlen(q)) == 0;
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
		// 2文字
		if(startswith(p, "==") ||
			startswith(p, "!=") ||
			startswith(p, "<=") ||
			startswith(p, ">=")) {
				cur = new_token(TK_RESERVED, cur, p, 2);
				p += 2;
				continue;
		}

		// 1文字
		if(strchr("+-*/()<>", *p)) {
			cur = new_token(TK_RESERVED, cur, p++, 1);
			continue;
		}

		if(isdigit(*p)) {
			cur = new_token(TK_NUM, cur, p, 0);
			char *q = p;
			cur->val = strtol(p, &p, 10);
			cur->len = p - q;
			continue;
		}

		error_at(token->str, "トークナイズできません。");
	}

	new_token(TK_EOF, cur, p, 0);
	return head.next;
}