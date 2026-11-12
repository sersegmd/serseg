#include "serseg.hpp"
#include <cstring>
#include <cstdint>
namespace serseg {
namespace {
std::int64_t signed_bits(std::uint64_t value){std::int64_t out;static_assert(sizeof(out)==sizeof(value),"int64 size");std::memcpy(&out,&value,sizeof(out));return out;}
void fold_expr(Expr& e){
 if(e.left)fold_expr(*e.left);
 if(e.right)fold_expr(*e.right);
 for(auto& arg:e.args)fold_expr(*arg);
 auto replace_int=[&](std::uint64_t value){e.tag=Expr::Tag::Int;e.value=std::to_string(signed_bits(value));e.type=Type::Int;e.left.reset();e.right.reset();};
 auto replace_bool=[&](bool value){e.tag=Expr::Tag::Bool;e.value=value?"true":"false";e.type=Type::Bool;e.left.reset();e.right.reset();};
 if(e.tag==Expr::Tag::Unary&&e.left){if(e.left->tag==Expr::Tag::Int){auto v=static_cast<std::uint64_t>(std::stoll(e.left->value));if(e.value=="-")replace_int(0-v);else if(e.value=="~")replace_int(~v);}else if(e.left->tag==Expr::Tag::Bool&&e.value=="!")replace_bool(e.left->value!="true");return;}
 if(e.tag!=Expr::Tag::Binary||!e.left||!e.right)return;
 if(e.left->tag==Expr::Tag::Int&&e.right->tag==Expr::Tag::Int){auto a=std::stoll(e.left->value),b=std::stoll(e.right->value);auto ua=static_cast<std::uint64_t>(a),ub=static_cast<std::uint64_t>(b);auto op=e.value;
  if(op=="+")replace_int(ua+ub);else if(op=="-")replace_int(ua-ub);else if(op=="*")replace_int(ua*ub);else if(op=="&")replace_int(ua&ub);else if(op=="|")replace_int(ua|ub);else if(op=="^")replace_int(ua^ub);else if(op=="==")replace_bool(a==b);else if(op=="!=")replace_bool(a!=b);else if(op=="<")replace_bool(a<b);else if(op=="<=")replace_bool(a<=b);else if(op==">")replace_bool(a>b);else if(op==">=")replace_bool(a>=b);
 }else if(e.left->tag==Expr::Tag::Bool&&e.right->tag==Expr::Tag::Bool){bool a=e.left->value=="true",b=e.right->value=="true";auto op=e.value;if(op=="&&")replace_bool(a&&b);else if(op=="||")replace_bool(a||b);else if(op=="==")replace_bool(a==b);else if(op=="!=")replace_bool(a!=b);}
}
void fold_block(Program& program){for(auto& ptr:program){auto& s=*ptr;if(s.expr)fold_expr(*s.expr);if(s.end)fold_expr(*s.end);fold_block(s.body);fold_block(s.otherwise);}}
}
void optimize_constants(Program& program){fold_block(program);}
}
