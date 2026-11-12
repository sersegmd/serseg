#include "serseg.hpp"
#include <utility>
namespace serseg {
class Parser {
 const std::vector<Token>& t; size_t i=0; int expression_depth=0, expression_nodes=0, block_depth=0, when_chain_depth=0;
 struct DepthGuard { int& depth; explicit DepthGuard(int& d):depth(d){++depth;} ~DepthGuard(){--depth;} };
 void node(Pos p){if(++expression_nodes>256)throw Error(p,"P007","Expression is too complex (maximum 256 nodes)");}
 const Token& cur()const{return t[i];} bool is(Kind k)const{return cur().kind==k;} bool word(const char* s)const{return is(Kind::Id)&&cur().text==s;} Token take(){return t[i++];}
 bool eat(Kind k){if(is(k)){take();return true;}return false;} bool eatword(const char*s){if(word(s)){take();return true;}return false;}
 Token expect(Kind k,const std::string& msg){if(!is(k))throw Error(cur().pos,"P001",msg);return take();} void newlines(){while(eat(Kind::Newline)){} }
 std::unique_ptr<Expr> expr(int min=0){if(expression_depth==0)expression_nodes=0;if(expression_depth>=256)throw Error(cur().pos,"P007","Expression nesting exceeds 256");DepthGuard guard(expression_depth);std::unique_ptr<Expr> a;Token x=take();node(x.pos);
  if(x.kind==Kind::Int||x.kind==Kind::Float||x.kind==Kind::String||x.kind==Kind::Id){a=std::make_unique<Expr>();a->pos=x.pos;a->value=x.text;a->tag=x.kind==Kind::Int?Expr::Tag::Int:x.kind==Kind::Float?Expr::Tag::Float:x.kind==Kind::String?Expr::Tag::Text:(x.text=="true"||x.text=="false")?Expr::Tag::Bool:Expr::Tag::Var;}
  else if(x.kind==Kind::Minus||x.kind==Kind::Bang||x.kind==Kind::BitNot){a=std::make_unique<Expr>();a->tag=Expr::Tag::Unary;a->pos=x.pos;a->value=x.kind==Kind::Minus?"-":x.kind==Kind::Bang?"!":"~";a->left=expr(11);}
  else if(x.kind==Kind::LBracket){a=std::make_unique<Expr>();a->tag=Expr::Tag::Array;a->pos=x.pos;if(!is(Kind::RBracket)){do {a->args.push_back(expr());} while(eat(Kind::Comma));}expect(Kind::RBracket,"Expected ']' after array elements");}
  else if(x.kind==Kind::LParen){a=expr();expect(Kind::RParen,"Expected ')' after expression");}
  else throw Error(x.pos,"P002","Expected an expression");
  if(a->tag==Expr::Tag::Var && eat(Kind::LParen)){a->tag=Expr::Tag::Call;if(!is(Kind::RParen)){do {a->args.push_back(expr());} while(eat(Kind::Comma));}expect(Kind::RParen,"Expected ')' after arguments");}
  while(is(Kind::LBracket)||is(Kind::Dot)){
   Token postfix=take();node(postfix.pos);auto b=std::make_unique<Expr>();b->pos=postfix.pos;b->left=std::move(a);
   if(postfix.kind==Kind::LBracket){b->tag=Expr::Tag::Index;b->right=expr();expect(Kind::RBracket,"Expected ']' after index");}
   else {auto member=expect(Kind::Id,"Expected member after '.'");
    if(b->left->tag==Expr::Tag::Var&&eat(Kind::LParen)){
     b->tag=Expr::Tag::Call;b->value=b->left->value+"."+member.text;b->left.reset();
     if(!is(Kind::RParen)){do {b->args.push_back(expr());} while(eat(Kind::Comma));}
     expect(Kind::RParen,"Expected ')' after arguments");
    }else {b->tag=Expr::Tag::Length;if(member.text!="length")throw Error(member.pos,"P009","Expected '.length' or a module function call");}
   }
   a=std::move(b);
  }
  while(true){int prec=0;std::string op;switch(cur().kind){case Kind::Or:prec=1;op="||";break;case Kind::And:prec=2;op="&&";break;case Kind::BitOr:prec=3;op="|";break;case Kind::BitXor:prec=4;op="^";break;case Kind::BitAnd:prec=5;op="&";break;case Kind::EqEq:prec=6;op="==";break;case Kind::NotEq:prec=6;op="!=";break;case Kind::Lt:prec=7;op="<";break;case Kind::Le:prec=7;op="<=";break;case Kind::Gt:prec=7;op=">";break;case Kind::Ge:prec=7;op=">=";break;case Kind::ShiftLeft:prec=8;op="<<";break;case Kind::ShiftRight:prec=8;op=">>";break;case Kind::Plus:prec=9;op="+";break;case Kind::Minus:prec=9;op="-";break;case Kind::Star:prec=10;op="*";break;case Kind::Slash:prec=10;op="/";break;case Kind::Percent:prec=10;op="%";break;default:break;}if(prec<min||prec==0)break;Token o=take();node(o.pos);auto b=std::make_unique<Expr>();b->tag=Expr::Tag::Binary;b->pos=o.pos;b->value=op;b->left=std::move(a);b->right=expr(prec+1);a=std::move(b);}
  return a;
 }
 Program block(){if(block_depth>=128)throw Error(cur().pos,"P008","Block nesting exceeds 128");DepthGuard guard(block_depth);expect(Kind::LBrace,"Expected '{' to start block");newlines();Program b;while(!is(Kind::RBrace)){if(is(Kind::End))throw Error(cur().pos,"P003","Expected '}' before end of file");b.push_back(stmt());separator();}take();return b;}
 void separator(){if(is(Kind::Newline)){newlines();return;}if(is(Kind::RBrace)||is(Kind::End))return;throw Error(cur().pos,"P004","Expected end of line");}
 std::unique_ptr<Stmt> stmt(){auto s=std::make_unique<Stmt>();s->pos=cur().pos;
  if(word("let")||word("var")||word("const")){bool is_const=eatword("const");bool is_var=!is_const&&eatword("var");if(!is_const&&!is_var)eatword("let");s->tag=Stmt::Tag::Let;s->constant=is_const;s->mut=is_var||(!is_const&&eatword("mut"));s->name=expect(Kind::Id,"Expected a variable name").text;if(eat(Kind::Colon))s->annotation=expect(Kind::Id,"Expected a type name").text;expect(Kind::Eq,"Expected '=' after declaration");s->expr=expr();}
  else if(eatword("use")){s->tag=Stmt::Tag::Use;s->name=expect(Kind::Id,"Expected module name after use").text;}
  else if(eatword("push")){s->tag=Stmt::Tag::Push;s->name=expect(Kind::Id,"Expected array variable after push").text;expect(Kind::Comma,"Expected comma after array variable");s->expr=expr();}
  else if(eatword("show")){s->tag=Stmt::Tag::Show;s->expr=expr();}
  else if(eatword("assert")){s->tag=Stmt::Tag::Assert;s->expr=expr();}
  else if(eatword("task")){s->tag=Stmt::Tag::Task;s->name=expect(Kind::Id,"Expected function name").text;expect(Kind::LParen,"Expected '(' after function name");if(!is(Kind::RParen)){do {Param p;p.pos=cur().pos;p.name=expect(Kind::Id,"Expected parameter name").text;expect(Kind::Colon,"Expected ':' after parameter name");p.annotation=expect(Kind::Id,"Expected parameter type").text;s->params.push_back(std::move(p));}while(eat(Kind::Comma));}expect(Kind::RParen,"Expected ')' after parameters");expect(Kind::Arrow,"Expected '->' and return type");s->annotation=expect(Kind::Id,"Expected return type").text;s->body=block();}
  else if(eatword("give")){s->tag=Stmt::Tag::Give;s->expr=expr();}
  else if(eatword("break")){s->tag=Stmt::Tag::Break;}
  else if(eatword("continue")){s->tag=Stmt::Tag::Continue;}
  else if(eatword("when")){s->tag=Stmt::Tag::When;s->expr=expr();s->body=block();size_t saved=i;newlines();if(eatword("otherwise")){if(word("when")){if(when_chain_depth>=128)throw Error(cur().pos,"P008","Conditional chain exceeds 128 branches");DepthGuard guard(when_chain_depth);s->otherwise.push_back(stmt());}else s->otherwise=block();}else i=saved;}
  else if(eatword("each")){s->tag=Stmt::Tag::Each;s->name=expect(Kind::Id,"Expected element name after each").text;if(!eatword("in"))throw Error(cur().pos,"P005","Expected 'in' after element name");s->expr=expr();s->body=block();}
  else if(eatword("while")){s->tag=Stmt::Tag::While;s->expr=expr();s->body=block();}
  else if(eatword("repeat")){s->tag=Stmt::Tag::Repeat;s->name=expect(Kind::Id,"Expected loop variable").text;if(!eatword("from"))throw Error(cur().pos,"P005","Expected 'from'");s->expr=expr();if(!eatword("to"))throw Error(cur().pos,"P005","Expected 'to'");s->end=expr();s->body=block();}
  else if(is(Kind::Id)&&i+1<t.size()&&t[i+1].kind==Kind::LBracket){s->tag=Stmt::Tag::IndexAssign;s->name=take().text;take();s->expr=expr();expect(Kind::RBracket,"Expected ']' after index");expect(Kind::Eq,"Expected '=' after indexed target");s->end=expr();}
  else if(is(Kind::Id)&&i+1<t.size()&&t[i+1].kind==Kind::Eq){s->tag=Stmt::Tag::Assign;s->name=take().text;take();s->expr=expr();}
  else { throw Error(cur().pos,"P006","Expected a statement"); }
  return s;}
public:explicit Parser(const std::vector<Token>& t):t(t){} Program run(){Program p;newlines();while(!is(Kind::End)){p.push_back(stmt());separator();}return p;}
};
Program parse(const std::vector<Token>& t){return Parser(t).run();}
}
