#include"9cc.h"

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

	codegen(node);

	// スタックトップに式全体の結果が残っているので
	// raxにロードして、返り値とする。
	printf("\tpop rax\n");
	printf("\tret\n");
	return 0;
}