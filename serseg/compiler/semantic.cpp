#include "serseg.hpp"
#include <unordered_set>
#include <cmath>
namespace serseg {
std::string type_name(Type t){switch(t){case Type::Int:return "int";case Type::Float:return "float";case Type::Bool:return "bool";case Type::Text:return "text";case Type::IntArray:return "int_array";case Type::FloatArray:return "float_array";case Type::BoolArray:return "bool_array";case Type::TextArray:return "text_array";default:return "invalid";}}
class Analyzer {
 struct Symbol {Type type;bool mut;int slot;};
 struct Signature {std::vector<Type> params;Type result;};
 std::vector<std::unordered_map<std::string,Symbol>> scopes{{}};
 std::unordered_map<std::string,Signature> functions;
 int next=0,loop_depth=0;Type return_type=Type::Invalid;
 Type annotation(const std::string& name,Pos p){if(name=="int")return Type::Int;if(name=="float")return Type::Float;if(name=="bool")return Type::Bool;if(name=="text")return Type::Text;if(name=="int_array")return Type::IntArray;if(name=="float_array")return Type::FloatArray;if(name=="bool_array")return Type::BoolArray;if(name=="text_array")return Type::TextArray;throw Error(p,"E109","Unknown type '"+name+"'");}
 bool reserved(const std::string& n){static const std::unordered_set<std::string> words={"let","var","mut","const","show","when","otherwise","repeat","from","to","true","false","task","give","type","use","break","continue","while","assert","push","each","in","input","len","str","to_int","to_float","is_int","is_float","utf8_len","utf8_slice","contains","starts_with","ends_with","trim","slice"};return words.count(n);}
 Symbol lookup(const std::string& name,Pos p){for(auto i=scopes.rbegin();i!=scopes.rend();++i){auto f=i->find(name);if(f!=i->end())return f->second;}throw Error(p,"E102","Undefined variable '"+name+"'. Declare it before use.");}
 void declare(const std::string& n,Pos p,Type t,bool m,int& slot){if(reserved(n))throw Error(p,"E103","Reserved word cannot be a variable name: '"+n+"'");if(scopes.back().count(n))throw Error(p,"E104","Duplicate declaration '"+n+"'");if(next>=400)throw Error(p,"E117","Too many local slots in this function or program");slot=next++;scopes.back()[n]={t,m,slot};}
 Type expected_expr(Expr& e,Type expected){if(e.tag==Expr::Tag::Array&&e.args.empty()&&is_array(expected))e.type=expected;return expression(e);}
 Type expression(Expr& e){switch(e.tag){
 case Expr::Tag::Int:{try{size_t end;std::stoll(e.value,&end);if(end!=e.value.size())throw std::out_of_range("integer");}catch(...){throw Error(e.pos,"E105","Integer literal exceeds signed 64-bit range");}return e.type=Type::Int;}
 case Expr::Tag::Float:{try{size_t end;double value=std::stod(e.value,&end);if(end!=e.value.size()||!std::isfinite(value))throw std::out_of_range("float");}catch(...){throw Error(e.pos,"E120","Float literal is out of range");}return e.type=Type::Float;}
 case Expr::Tag::Bool:return e.type=Type::Bool;
 case Expr::Tag::Text:return e.type=Type::Text;
 case Expr::Tag::Array:{if(e.args.empty())return e.type=is_array(e.type)?e.type:Type::IntArray;Type element=expression(*e.args[0]);Type result=array_of(element);if(result==Type::Invalid)throw Error(e.pos,"E122","Arrays require int, float, bool or text elements");for(size_t i=1;i<e.args.size();++i)if(expression(*e.args[i])!=element)throw Error(e.args[i]->pos,"E122","Array elements must have matching types");return e.type=result;}
 case Expr::Tag::Index:{auto t=expression(*e.left);if(!is_array(t))throw Error(e.pos,"E123","Indexing requires an array");if(expression(*e.right)!=Type::Int)throw Error(e.pos,"E123","Array index must be int");return e.type=array_element(t);}
 case Expr::Tag::Length:{auto t=expression(*e.left);if(!is_array(t)&&t!=Type::Text)throw Error(e.pos,"E123",".length requires text or an array");return e.type=Type::Int;}
 case Expr::Tag::Var:{auto s=lookup(e.value,e.pos);e.slot=s.slot;return e.type=s.type;}
 case Expr::Tag::Call:{
  if(e.value=="input"){
   if(e.args.size()>1)throw Error(e.pos,"E111","input expects zero arguments or one text prompt");
   if(!e.args.empty()&&expression(*e.args[0])!=Type::Text)throw Error(e.args[0]->pos,"E111","input expects a text prompt");
   return e.type=Type::Text;
  }
  if(e.value=="len"||e.value=="str"||e.value=="to_int"||e.value=="to_float"||e.value=="is_int"||e.value=="is_float"||e.value=="utf8_len"||e.value=="trim"){
   if(e.args.size()!=1)throw Error(e.pos,"E111","Function '"+e.value+"' expects exactly one argument");
   auto t=expression(*e.args[0]);
   if(e.value=="str") {if(t!=Type::Int&&t!=Type::Float&&t!=Type::Bool)throw Error(e.args[0]->pos,"E112","str expects int, float or bool");return e.type=Type::Text;}
   if(t!=Type::Text)throw Error(e.args[0]->pos,"E112",e.value+" expects text");
   return e.type=e.value=="to_float"?Type::Float:e.value=="trim"?Type::Text:(e.value=="is_int"||e.value=="is_float")?Type::Bool:Type::Int;
  }
  if(e.value=="contains"||e.value=="starts_with"||e.value=="ends_with"||e.value=="slice"||e.value=="utf8_slice"){
   size_t count=(e.value=="slice"||e.value=="utf8_slice")?3:2;
   if(e.args.size()!=count)throw Error(e.pos,"E111","Function '"+e.value+"' expects "+std::to_string(count)+" arguments");
   for(size_t i=0;i<count;i++){
    Type expected=((e.value=="slice"||e.value=="utf8_slice")&&i>0)?Type::Int:Type::Text;
    if(expression(*e.args[i])!=expected)throw Error(e.args[i]->pos,"E112","Argument "+std::to_string(i+1)+" of '"+e.value+"' must be "+type_name(expected));
   }
   return e.type=(e.value=="slice"||e.value=="utf8_slice")?Type::Text:Type::Bool;
  }
  auto f=functions.find(e.value);if(f==functions.end())throw Error(e.pos,"E110","Undefined function '"+e.value+"'");if(e.args.size()!=f->second.params.size())throw Error(e.pos,"E111","Function '"+e.value+"' expects "+std::to_string(f->second.params.size())+" arguments, got "+std::to_string(e.args.size()));for(size_t i=0;i<e.args.size();i++){auto actual=expected_expr(*e.args[i],f->second.params[i]);if(actual!=f->second.params[i])throw Error(e.args[i]->pos,"E112","Argument "+std::to_string(i+1)+" must be "+type_name(f->second.params[i]));}return e.type=f->second.result;}
 case Expr::Tag::Unary:{auto a=expression(*e.left);if(e.value=="-"&&(a==Type::Int||a==Type::Float))return e.type=a;Type want=e.value=="!"?Type::Bool:Type::Int;if(a!=want)throw Error(e.pos,"E106","Unary operator requires "+type_name(want));return e.type=want;}
 case Expr::Tag::Binary:{auto a=expression(*e.left),b=expression(*e.right);auto op=e.value;if(op=="&&"||op=="||"){if(a!=Type::Bool||b!=Type::Bool)throw Error(e.pos,"E106","Logical operators require bool operands");return e.type=Type::Bool;}if(op=="=="||op=="!="){if(a!=b||is_array(a))throw Error(e.pos,"E106","Equality requires matching int, float, bool or text operands");return e.type=Type::Bool;}if(op=="+"&&a==Type::Text&&b==Type::Text)return e.type=Type::Text;if(a==Type::Float&&b==Type::Float&&(op=="+"||op=="-"||op=="*"||op=="/"||op=="<"||op=="<="||op==">"||op==">="))return e.type=(op=="<"||op=="<="||op==">"||op==">=")?Type::Bool:Type::Float;if(a!=Type::Int||b!=Type::Int)throw Error(e.pos,"E106","Operator '"+op+"' requires matching supported numeric operands");return e.type=(op=="<"||op=="<="||op==">"||op==">=")?Type::Bool:Type::Int;}}
 throw Error(e.pos,"E999","Invalid expression");}
 bool constant_expr(const Expr& e){switch(e.tag){case Expr::Tag::Int:case Expr::Tag::Float:case Expr::Tag::Bool:case Expr::Tag::Text:return true;case Expr::Tag::Unary:return constant_expr(*e.left);case Expr::Tag::Binary:return e.value!="/"&&e.value!="%"&&e.value!="<<"&&e.value!=">>"&&constant_expr(*e.left)&&constant_expr(*e.right);default:return false;}}
 bool returns(const Program& p){for(const auto& item:p){const auto& s=*item;if(s.tag==Stmt::Tag::Give)return true;if(s.tag==Stmt::Tag::When&&!s.otherwise.empty()&&returns(s.body)&&returns(s.otherwise))return true;}return false;}
 void statements(Program& p){for(auto& ptr:p){auto& s=*ptr;switch(s.tag){
 case Stmt::Tag::Let:{if(s.constant&&!constant_expr(*s.expr))throw Error(s.pos,"E119","const requires a literal-only expression without division");Type declared=s.annotation.empty()?Type::Invalid:annotation(s.annotation,s.pos);Type t=expected_expr(*s.expr,declared);if(!s.annotation.empty()&&declared!=t)throw Error(s.pos,"E107","Declared type '"+s.annotation+"' does not match "+type_name(t));s.value_type=t;declare(s.name,s.pos,t,s.mut,s.slot);break;}
 case Stmt::Tag::Assign:{auto v=lookup(s.name,s.pos);if(!v.mut)throw Error(s.pos,"E108","Cannot assign to immutable variable '"+s.name+"'; declare it with 'var' or 'let mut'");if(expected_expr(*s.expr,v.type)!=v.type)throw Error(s.pos,"E107","Assignment type does not match "+type_name(v.type));s.slot=v.slot;s.value_type=v.type;break;}
 case Stmt::Tag::IndexAssign:{auto v=lookup(s.name,s.pos);if(!v.mut)throw Error(s.pos,"E108","Cannot modify an array through immutable variable '"+s.name+"'");if(!is_array(v.type))throw Error(s.pos,"E123","Indexed assignment requires an array");if(expression(*s.expr)!=Type::Int||expression(*s.end)!=array_element(v.type))throw Error(s.pos,"E123","Array index or value type is invalid");s.slot=v.slot;s.value_type=v.type;break;}
 case Stmt::Tag::Push:{auto v=lookup(s.name,s.pos);if(!v.mut)throw Error(s.pos,"E108","Cannot modify an array through immutable variable '"+s.name+"'");if(!is_array(v.type))throw Error(s.pos,"E123","push requires an array");if(expression(*s.expr)!=array_element(v.type))throw Error(s.pos,"E123","push value must match the array element type");s.slot=v.slot;s.value_type=v.type;break;}
 case Stmt::Tag::Show:if(is_array(expression(*s.expr)))throw Error(s.pos,"E106","Cannot show an array directly; use an index");break;
 case Stmt::Tag::Assert:if(expression(*s.expr)!=Type::Bool)throw Error(s.pos,"E107","assert requires a bool expression");break;
 case Stmt::Tag::When:if(expression(*s.expr)!=Type::Bool)throw Error(s.pos,"E107","when requires a bool condition");scopes.emplace_back();statements(s.body);scopes.pop_back();scopes.emplace_back();statements(s.otherwise);scopes.pop_back();break;
 case Stmt::Tag::While:if(expression(*s.expr)!=Type::Bool)throw Error(s.pos,"E107","while requires a bool condition");scopes.emplace_back();loop_depth++;statements(s.body);loop_depth--;scopes.pop_back();break;
 case Stmt::Tag::Repeat:if(expression(*s.expr)!=Type::Int||expression(*s.end)!=Type::Int)throw Error(s.pos,"E107","repeat bounds must be int");scopes.emplace_back();declare(s.name,s.pos,Type::Int,false,s.slot);if(next>=400)throw Error(s.pos,"E117","Too many local slots in this function or program");next++;loop_depth++;statements(s.body);loop_depth--;scopes.pop_back();break;
 case Stmt::Tag::Each:{Type array=expression(*s.expr);if(!is_array(array))throw Error(s.pos,"E123","each requires an array");s.value_type=array_element(array);scopes.emplace_back();declare(s.name,s.pos,s.value_type,false,s.slot);if(next>397)throw Error(s.pos,"E117","Too many local slots in this function or program");s.array_slot=next++;s.index_slot=next++;s.limit_slot=next++;loop_depth++;statements(s.body);loop_depth--;scopes.pop_back();break;}
 case Stmt::Tag::Break:case Stmt::Tag::Continue:if(!loop_depth)throw Error(s.pos,"E113","break and continue require an enclosing loop");break;
 case Stmt::Tag::Give:if(return_type==Type::Invalid)throw Error(s.pos,"E114","give is only allowed inside a task");if(expected_expr(*s.expr,return_type)!=return_type)throw Error(s.pos,"E107","Return value must be "+type_name(return_type));break;
 case Stmt::Tag::Task:throw Error(s.pos,"E115","Tasks must be declared at top level");
 case Stmt::Tag::Use:throw Error(s.pos,"E121","Imports must be at the top level");
 }}}
public:int run(Program& p){
 functions.emplace("input",Signature{{},Type::Text});
 for(auto& ptr:p)if(ptr->tag==Stmt::Tag::Task){auto& s=*ptr;if(reserved(s.name))throw Error(s.pos,"E103","Reserved word cannot name a task");if(functions.count(s.name))throw Error(s.pos,"E104","Duplicate task '"+s.name+"'");if(s.params.size()>128)throw Error(s.pos,"E118","A task may have at most 128 parameters");Signature sig;for(auto& arg:s.params)sig.params.push_back(annotation(arg.annotation,arg.pos));sig.result=annotation(s.annotation,s.pos);functions.emplace(s.name,std::move(sig));}
 // Main statements use their own scope. Tasks cannot capture main locals.
 for(auto& ptr:p)if(ptr->tag!=Stmt::Tag::Task){Program one;one.push_back(std::move(ptr));statements(one);ptr=std::move(one[0]);}
 int main_slots=next;
 for(auto& ptr:p)if(ptr->tag==Stmt::Tag::Task){auto& s=*ptr;scopes.clear();scopes.emplace_back();next=0;return_type=functions.at(s.name).result;for(auto& arg:s.params)declare(arg.name,arg.pos,annotation(arg.annotation,arg.pos),false,arg.slot);statements(s.body);if(!returns(s.body))throw Error(s.pos,"E116","Not every path in task '"+s.name+"' gives a value");s.frame_slots=next;}
 return main_slots;
 }};
int analyze(Program& p){return Analyzer().run(p);}
}
