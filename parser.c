#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

typedef struct {const VetorTokens *v; int pos, profundidade; ListaErrosSintaticos *erros;} Parser;
void vetor_tokens_inicializar(VetorTokens*v){memset(v,0,sizeof(*v));}
void vetor_tokens_adicionar(VetorTokens*v,Token t){if(v->total==v->capacidade){v->capacidade=v->capacidade?2*v->capacidade:128;v->itens=realloc(v->itens,(size_t)v->capacidade*sizeof(Token));if(!v->itens){fprintf(stderr,"Memoria insuficiente\n");exit(3);}}v->itens[v->total++]=t;}
void vetor_tokens_liberar(VetorTokens*v){for(int i=0;i<v->total;i++)free(v->itens[i].lexema);free(v->itens);memset(v,0,sizeof(*v));}
void erros_sintaticos_liberar(ListaErrosSintaticos*e){free(e->itens);memset(e,0,sizeof(*e));}
static const Token *at(Parser*p,int n){int i=p->pos+n; if(i<0)i=0;if(i>=p->v->total)i=p->v->total-1;return &p->v->itens[i];}
static const char *lex(Parser*p,int n){return at(p,n)->lexema;}
static int is(Parser*p,const char*s){return strcmp(lex(p,0),s)==0;}
static int eof(Parser*p){return at(p,0)->tipo==TOK_FIM_ARQUIVO;}
static void advance(Parser*p){if(!eof(p))p->pos++;}
static int match(Parser*p,const char*s){if(is(p,s)){advance(p);return 1;}return 0;}
static void error(Parser*p,const char*fmt,...){const Token*t=at(p,0);if(p->erros->total>=100)return;ListaErrosSintaticos*e=p->erros;if(e->total==e->capacidade){e->capacidade=e->capacidade?e->capacidade*2:16;e->itens=realloc(e->itens,(size_t)e->capacidade*sizeof(ErroSintatico));if(!e->itens)exit(3);}ErroSintatico*x=&e->itens[e->total++];x->linha=t->linha;x->coluna=t->coluna;va_list ap;va_start(ap,fmt);vsnprintf(x->mensagem,sizeof(x->mensagem),fmt,ap);va_end(ap);}
static int expect(Parser*p,const char*s){if(match(p,s))return 1;error(p,"Esperado '%s', encontrado '%s'",s,lex(p,0));return 0;}
static int identifier(Parser*p){return at(p,0)->tipo==TOK_IDENTIFICADOR || is(p,"var") || is(p,"record");}
static int expect_id(Parser*p){if(identifier(p)){advance(p);return 1;}error(p,"Identificador esperado, encontrado '%s'",lex(p,0));return 0;}
static int type_start(Parser*p){const char *s=lex(p,0);return identifier(p)||!strcmp(s,"int")||!strcmp(s,"long")||!strcmp(s,"byte")||!strcmp(s,"short")||!strcmp(s,"boolean")||!strcmp(s,"char")||!strcmp(s,"double")||!strcmp(s,"float")||!strcmp(s,"void");}
static int primitive(Parser*p){const char*s=lex(p,0);return !strcmp(s,"int")||!strcmp(s,"long")||!strcmp(s,"byte")||!strcmp(s,"short")||!strcmp(s,"boolean")||!strcmp(s,"char")||!strcmp(s,"double")||!strcmp(s,"float")||!strcmp(s,"void");}
static int modifier(Parser*p){const char*s=lex(p,0);return !strcmp(s,"public")||!strcmp(s,"private")||!strcmp(s,"protected")||!strcmp(s,"static")||!strcmp(s,"final")||!strcmp(s,"abstract")||!strcmp(s,"native")||!strcmp(s,"synchronized")||!strcmp(s,"strictfp")||!strcmp(s,"transient")||!strcmp(s,"volatile")||!strcmp(s,"default");}
static void expr(Parser*p,int minprec);
static void statement(Parser*p);
static void type(Parser*p);
static void annotation(Parser*p){expect(p,"@");expect_id(p);while(match(p,"."))expect_id(p);if(match(p,"(")){if(!is(p,")")){expr(p,1);while(match(p,","))expr(p,1);}expect(p,")");}}
static void modifiers(Parser*p){while(modifier(p)||is(p,"@")){if(is(p,"@"))annotation(p);else advance(p);}}
static void qualified(Parser*p){expect_id(p);while(match(p,"."))expect_id(p);}

static void generic(Parser*p){if(!match(p,"<"))return;int depth=1;while(!eof(p)&&depth){if(is(p,"<")){depth++;advance(p);}else if(is(p,">")){depth--;advance(p);}else if(is(p,">>")||is(p,">>>")){depth-=is(p,">>")?2:3;advance(p);}else advance(p);}if(depth!=0)error(p,"Lista de tipos genericos nao fechada corretamente");}
static void type(Parser*p){if(!type_start(p)){error(p,"Tipo esperado, encontrado '%s'",lex(p,0));return;}int prim=primitive(p);advance(p);if(!prim){while(match(p,"."))expect_id(p);generic(p);}while(match(p,"["))expect(p,"]");}
static int precedence(const char*s){if(!strcmp(s,"=")||!strcmp(s,"+=")||!strcmp(s,"-=")||!strcmp(s,"*=")||!strcmp(s,"/=")||!strcmp(s,"%=")||!strcmp(s,"&=")||!strcmp(s,"|=")||!strcmp(s,"^=")||!strcmp(s,"<<=")||!strcmp(s,">>=")||!strcmp(s,">>>="))return 1;
if(!strcmp(s,"?"))return 2;
if(!strcmp(s,"||"))return 3;
if(!strcmp(s,"&&"))return 4;
if(!strcmp(s,"|"))return 5;
if(!strcmp(s,"^"))return 6;
if(!strcmp(s,"&"))return 7;
if(!strcmp(s,"==")||!strcmp(s,"!="))return 8;
if(!strcmp(s,"<")||!strcmp(s,">")||!strcmp(s,"<=")||!strcmp(s,">=")||!strcmp(s,"instanceof"))return 9;
if(!strcmp(s,"<<")||!strcmp(s,">>")||!strcmp(s,">>>"))return 10;
if(!strcmp(s,"+")||!strcmp(s,"-"))return 11;
if(!strcmp(s,"*")||!strcmp(s,"/")||!strcmp(s,"%"))return 12;
return 0;}
static int literal(Parser*p){TipoToken t=at(p,0)->tipo;return t==TOK_LITERAL_INTEIRO||t==TOK_LITERAL_DECIMAL||t==TOK_LITERAL_TEXTO||t==TOK_LITERAL_CARACTERE||t==TOK_LITERAL_BOOLEANO||t==TOK_LITERAL_NULO;}
static void initializer(Parser*p){if(match(p,"{")){if(!is(p,"}")){initializer(p);while(match(p,",")){if(is(p,"}"))break;initializer(p);}}expect(p,"}");}else expr(p,1);}
static void arguments(Parser*p){expect(p,"(");if(!is(p,")")){expr(p,1);while(match(p,","))expr(p,1);}expect(p,")");}
static void primary(Parser*p){if(p->profundidade++>160){error(p,"Limite de aninhamento sintatico excedido");p->profundidade--;return;}
if(literal(p)||identifier(p)||is(p,"this")||is(p,"super")){advance(p);}
else if(match(p,"new")){if(type_start(p)){int prim=primitive(p);advance(p);if(!prim){while(match(p,"."))expect_id(p);generic(p);}}else error(p,"Tipo esperado apos new");if(is(p,"(")){arguments(p);if(is(p,"{")){int d=0;do{if(is(p,"{"))d++;else if(is(p,"}"))d--;advance(p);}while(!eof(p)&&d);}}else if(match(p,"[")){if(!is(p,"]"))expr(p,1);expect(p,"]");while(match(p,"[")){if(!is(p,"]"))expr(p,1);expect(p,"]");}if(is(p,"{"))initializer(p);}else error(p,"Esperado construtor ou indice apos 'new'");}
else if(match(p,"(")){
int save=p->pos;int ok=1;while(!eof(p)&&!is(p,")")&&p->pos-save<24){if(!identifier(p)&&!type_start(p)&&!is(p,",")&&!is(p,"[")&&!is(p,"]"))ok=0;advance(p);}if(!is(p,")")||strcmp(lex(p,1),"->"))ok=0;p->pos=save;if(ok){while(!is(p,")")&&!eof(p))advance(p);expect(p,")");expect(p,"->");if(is(p,"{"))statement(p);else expr(p,1);}else{expr(p,1);expect(p,")");}}
else if(match(p,"{")){if(!is(p,"}")){initializer(p);while(match(p,","))initializer(p);}expect(p,"}");}
else{error(p,"Expressao esperada, encontrado '%s'",lex(p,0));if(!eof(p))advance(p);}
while(!eof(p)){
if(is(p,"(")){arguments(p);}else if(match(p,".")){if(is(p,"class")){advance(p);}else expect_id(p);}else if(match(p,"[")){expr(p,1);expect(p,"]");}else if(match(p,"::")){if(is(p,"new"))advance(p);else expect_id(p);}else if(match(p,"++")||match(p,"--")){}else if(match(p,"->")){if(is(p,"{"))statement(p);else expr(p,1);}else break;}
p->profundidade--;}
static void unary(Parser*p){if(is(p,"!")||is(p,"~")||is(p,"+")||is(p,"-")||is(p,"++")||is(p,"--")){advance(p);unary(p);}else primary(p);}
static void expr(Parser*p,int minprec){int before=p->pos;unary(p);while(!eof(p)){int prec=precedence(lex(p,0));if(prec<minprec)break;char op[16];snprintf(op,sizeof(op),"%s",lex(p,0));advance(p);if(!strcmp(op,"?")){expr(p,1);expect(p,":");expr(p,2);}else if(!strcmp(op,"instanceof")){type(p);}else{int right=(prec==1);expr(p,prec+(right?0:1));}if(p->pos==before)break;}}
static void var_tail(Parser*p){while(match(p,"["))expect(p,"]");if(match(p,"="))initializer(p);while(match(p,",")){expect_id(p);while(match(p,"["))expect(p,"]");if(match(p,"="))initializer(p);}}
static int declaration_lookahead(Parser*p){int i=p->pos, limit=0;if(modifier(p))return 1;if(!type_start(p))return 0;
if(primitive(p)||identifier(p)){advance(p);while(match(p,".")) {if(!identifier(p))break;advance(p);}if(is(p,"<")){int d=0;do{if(is(p,"<"))d++;if(is(p,">"))d--;if(is(p,">>"))d-=2;if(is(p,">>>"))d-=3;advance(p);}while(!eof(p)&&d>0&&++limit<80);}while(match(p,"[")){if(!match(p,"]"))break;}}
int ok=identifier(p);p->pos=i;return ok;}
static void var_declaration(Parser*p,int terminator){type(p);expect_id(p);var_tail(p);if(terminator)expect(p,";");}
static void block(Parser*p){expect(p,"{");int n=0;while(!eof(p)&&!is(p,"}")&&n++<100000){int before=p->pos;statement(p);if(p->pos==before){error(p,"Nao foi possivel avancar na instrucao");advance(p);}}expect(p,"}");}
static void statement(Parser*p){if(eof(p))return;if(is(p,"{")){block(p);return;}if(match(p,";"))return;
if(match(p,"if")){expect(p,"(");expr(p,1);expect(p,")");statement(p);if(match(p,"else"))statement(p);return;}
if(match(p,"while")||match(p,"switch")||match(p,"synchronized")){expect(p,"(");expr(p,1);expect(p,")");statement(p);return;}
if(match(p,"for")){expect(p,"(");if(!is(p,";")){if(declaration_lookahead(p))var_declaration(p,0);else{expr(p,1);while(match(p,","))expr(p,1);}}if(match(p,":")){expr(p,1);expect(p,")");statement(p);return;}expect(p,";");if(!is(p,";"))expr(p,1);expect(p,";");if(!is(p,")")){expr(p,1);while(match(p,","))expr(p,1);}expect(p,")");statement(p);return;}
if(match(p,"do")){statement(p);expect(p,"while");expect(p,"(");expr(p,1);expect(p,")");expect(p,";");return;}
if(match(p,"return")||match(p,"throw")){if(!is(p,";"))expr(p,1);expect(p,";");return;}
if(match(p,"break")||match(p,"continue")){if(identifier(p))advance(p);expect(p,";");return;}
if(match(p,"try")){if(match(p,"(")){while(!eof(p)&&!is(p,")"))advance(p);expect(p,")");}block(p);while(match(p,"catch")){expect(p,"(");type(p);expect_id(p);expect(p,")");block(p);}if(match(p,"finally"))block(p);return;}
if(match(p,"assert")){expr(p,1);if(match(p,":"))expr(p,1);expect(p,";");return;}
if(declaration_lookahead(p)){modifiers(p);var_declaration(p,1);return;}
if(identifier(p)&&!strcmp(lex(p,1),":")){advance(p);advance(p);statement(p);return;}
expr(p,1);expect(p,";");}
static void params(Parser*p){expect(p,"(");if(!is(p,")")){do{modifiers(p);type(p);match(p,"...");expect_id(p);while(match(p,"["))expect(p,"]");}while(match(p,","));}expect(p,")");}
static void member(Parser*p,const char*class_name){modifiers(p);if(is(p,"}"))return;if(match(p,";"))return;
if(is(p,"class")||is(p,"interface")||is(p,"enum")){advance(p);expect_id(p);while(!eof(p)&&!is(p,"{"))advance(p);if(is(p,"{"))block(p);return;}
if(is(p,"{")){block(p);return;}
if(identifier(p)&&!strcmp(lex(p,0),class_name)&&!strcmp(lex(p,1),"(")){advance(p);params(p);if(is(p,"{"))block(p);else expect(p,";");return;}
if(!type_start(p)){error(p,"Tipo ou membro esperado, encontrado '%s'",lex(p,0));advance(p);return;}
type(p);if(!expect_id(p)){while(!eof(p)&&!is(p,";")&&!is(p,"}"))advance(p);match(p,";");return;}
if(is(p,"(")){params(p);if(match(p,"throws")){qualified(p);while(match(p,","))qualified(p);}if(is(p,"{"))block(p);else expect(p,";");}
else{var_tail(p);expect(p,";");}}
static void class_decl(Parser*p){modifiers(p);int kind=0;if(match(p,"class"))kind=1;else if(match(p,"interface"))kind=2;else if(match(p,"enum"))kind=3;else{error(p,"Declaracao de classe, interface ou enum esperada");advance(p);return;}
char name[256]="";if(identifier(p)){snprintf(name,sizeof(name),"%s",lex(p,0));advance(p);}else expect_id(p);generic(p);
if(match(p,"extends")){type(p);while(match(p,","))type(p);}if(match(p,"implements")){type(p);while(match(p,","))type(p);}if(!expect(p,"{"))return;
int count=0;while(!eof(p)&&!is(p,"}")&&count++<100000){int old=p->pos;if(kind==3&&identifier(p)&&(!strcmp(lex(p,1),",")||!strcmp(lex(p,1),";")||!strcmp(lex(p,1),"}"))){advance(p);if(!match(p,","))match(p,";");continue;}member(p,name);if(p->pos==old)advance(p);}expect(p,"}");}
void analisar_sintaxe(const VetorTokens*tokens,ListaErrosSintaticos*erros){memset(erros,0,sizeof(*erros));if(tokens->total==0)return;Parser p={tokens,0,0,erros};
if(match(&p,"package")){qualified(&p);expect(&p,";");}
while(match(&p,"import")){match(&p,"static");expect_id(&p);while(match(&p,".")){if(match(&p,"*"))break;expect_id(&p);}expect(&p,";");}
while(!eof(&p)){int before=p.pos;if(match(&p,";"))continue;class_decl(&p);if(p.pos==before)advance(&p);}
}
