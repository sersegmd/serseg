#include "serseg.hpp"
#include <sstream>
#include <string>
#include <utility>
#include <cstring>
namespace serseg {
#ifdef _WIN32
static constexpr bool windows = true;
#else
static constexpr bool windows = false;
#endif
class Generator {
 std::ostringstream code,data;
 int label=0;
 std::vector<std::pair<std::string,std::string>> loops;
 std::string return_label;
 std::string fresh(){return ".Lserseg"+std::to_string(label++);}
 std::string loc(int slot){return "-"+std::to_string(8*(slot+1))+"(%rbp)";}
 std::string call(const std::string& name){return name+(windows?"":"@PLT");}
 void frame(int slots){
  code<<"  pushq %rbp\n  movq %rsp, %rbp\n  andq $-16, %rsp\n";
  int bytes=(slots*8+15)/16*16+(windows?32:0);
  if(bytes)code<<"  subq $"<<bytes<<", %rsp\n";
 }
 void literal(const std::string& s,const std::string& name){data<<"  .balign 8\n"<<name<<":\n  .quad -1\n  .quad "<<s.size()<<"\n  .byte ";for(unsigned char c:s)data<<(unsigned)c<<",";data<<"0\n";}
 // Preserve expression temporaries, maintain ABI alignment and reserve Win64 shadow space.
 void ccall(const std::string& name){code<<"  movq %rsp, %r11\n  andq $-16, %rsp\n  subq $"<<(windows?48:16)<<", %rsp\n  movq %r11, "<<(windows?32:0)<<"(%rsp)\n  call "<<call(name)<<"\n  movq "<<(windows?32:0)<<"(%rsp), %rsp\n";}
 void arg1(const char* reg){code<<"  movq "<<reg<<", "<<(windows?"%rcx":"%rdi")<<"\n";}
 bool managed(Type type){return type==Type::Text||is_array(type);}
 const char* retain_fn(Type type){return type==Type::Text?"serseg_text_retain":"serseg_array_retain";}
 const char* release_fn(Type type){return type==Type::Text?"serseg_text_release":"serseg_array_release";}
 void release_slot(int slot,Type type){code<<"  movq "<<loc(slot)<<", %rax\n";arg1("%rax");ccall(release_fn(type));}
 void collect_slots(const Program& p,std::vector<std::pair<int,Type>>& slots){for(const auto& ptr:p){const auto& s=*ptr;if(s.tag==Stmt::Tag::Task)continue;if(s.tag==Stmt::Tag::Let&&managed(s.value_type))slots.push_back({s.slot,s.value_type});if(s.tag==Stmt::Tag::Each){slots.push_back({s.array_slot,Type::IntArray});if(managed(s.value_type))slots.push_back({s.slot,s.value_type});}collect_slots(s.body,slots);collect_slots(s.otherwise,slots);}}
 void init_slots(const std::vector<std::pair<int,Type>>& slots){for(auto slot:slots)code<<"  movq $0, "<<loc(slot.first)<<"\n";}
 void release_slots(const std::vector<std::pair<int,Type>>& slots){for(auto slot:slots)release_slot(slot.first,slot.second);}
 void ex(const Expr& e){
  switch(e.tag){
   case Expr::Tag::Int:code<<"  movabsq $"<<std::stoll(e.value)<<", %rax\n";break;
   case Expr::Tag::Float:{double value=std::stod(e.value);std::uint64_t bits;std::memcpy(&bits,&value,sizeof(bits));code<<"  movabsq $"<<bits<<", %rax\n";break;}
   case Expr::Tag::Bool:code<<"  movq $"<<(e.value=="true"?1:0)<<", %rax\n";break;
   case Expr::Tag::Text:{auto l=fresh();literal(e.value,l);code<<"  leaq "<<l<<"(%rip), %rax\n";break;}
   case Expr::Tag::Var:code<<"  movq "<<loc(e.slot)<<", %rax\n";if(managed(e.type)){arg1("%rax");ccall(retain_fn(e.type));}break;
   case Expr::Tag::Array:{
    code<<"  movq $"<<e.args.size()<<", "<<(windows?"%rcx":"%rdi")<<"\n  movq $"<<(e.type==Type::TextArray?1:0)<<", "<<(windows?"%rdx":"%rsi")<<"\n";ccall("serseg_array_new");code<<"  pushq %rax\n";
    for(size_t i=0;i<e.args.size();i++){ex(*e.args[i]);if(windows)code<<"  movq %rax, %r8\n  movq $"<<i<<", %rdx\n  movq (%rsp), %rcx\n";else code<<"  movq %rax, %rdx\n  movq $"<<i<<", %rsi\n  movq (%rsp), %rdi\n";ccall("serseg_array_set_borrowed");}
    code<<"  popq %rax\n";break;
   }
   case Expr::Tag::Index:{ex(*e.left);code<<"  pushq %rax\n";ex(*e.right);if(windows)code<<"  movq %rax, %rdx\n  popq %rcx\n";else code<<"  movq %rax, %rsi\n  popq %rdi\n";ccall("serseg_array_get");break;}
   case Expr::Tag::Length:{ex(*e.left);arg1("%rax");ccall(e.left->type==Type::Text?"serseg_text_length":"serseg_array_length");break;}
   case Expr::Tag::Call:{
    if(e.value=="input"){if(e.args.empty())ccall("serseg_input");else {ex(*e.args[0]);arg1("%rax");ccall("serseg_input_prompt");}break;}
    if(e.value=="len"||e.value=="to_int"||e.value=="str"||e.value=="to_float"||e.value=="is_int"||e.value=="is_float"||e.value=="utf8_len"||e.value=="trim"){
     ex(*e.args[0]);arg1("%rax");
     if(e.value=="len")ccall("serseg_text_length");
     else if(e.value=="to_int")ccall("serseg_text_to_int");
     else if(e.value=="to_float")ccall("serseg_text_to_float_bits");
     else if(e.value=="is_int")ccall("serseg_text_is_int");
     else if(e.value=="is_float")ccall("serseg_text_is_float");
     else if(e.value=="utf8_len")ccall("serseg_text_utf8_length");
     else if(e.value=="trim")ccall("serseg_text_trim");
     else ccall(e.args[0]->type==Type::Int?"serseg_str_int":e.args[0]->type==Type::Float?"serseg_str_float_bits":"serseg_str_bool");
     break;
    }
    if(e.value=="contains"||e.value=="starts_with"||e.value=="ends_with"||e.value=="slice"||e.value=="utf8_slice"){
     ex(*e.args[0]);code<<"  pushq %rax\n";ex(*e.args[1]);
     if(e.value=="slice"||e.value=="utf8_slice"){
      code<<"  pushq %rax\n";ex(*e.args[2]);
      if(windows)code<<"  movq %rax, %r8\n  popq %rdx\n  popq %rcx\n";else code<<"  movq %rax, %rdx\n  popq %rsi\n  popq %rdi\n";
      ccall(e.value=="slice"?"serseg_text_slice":"serseg_text_utf8_slice");
     }else{
      if(windows)code<<"  movq %rax, %rdx\n  popq %rcx\n";else code<<"  movq %rax, %rsi\n  popq %rdi\n";
      ccall(e.value=="contains"?"serseg_text_contains":e.value=="starts_with"?"serseg_text_starts_with":"serseg_text_ends_with");
     }
     break;
    }
    for(auto& arg:e.args){ex(*arg);code<<"  pushq %rax\n";}code<<"  call .Lserseg_task_"<<e.value<<"\n";if(!e.args.empty())code<<"  addq $"<<(e.args.size()*8)<<", %rsp\n";break;}
   case Expr::Tag::Unary:ex(*e.left);if(e.value=="-"&&e.type==Type::Float){code<<"  movabsq $-9223372036854775808, %rcx\n  xorq %rcx, %rax\n";break;}code<<(e.value=="-"?"  negq %rax\n":e.value=="~"?"  notq %rax\n":"  xorq $1, %rax\n");break;
   case Expr::Tag::Binary:{
    auto op=e.value;
    if(op=="&&"||op=="||"){auto end=fresh();ex(*e.left);code<<"  testq %rax, %rax\n  "<<(op=="&&"?"jz ":"jnz ")<<end<<"\n";ex(*e.right);code<<end<<":\n";break;}
    ex(*e.left);code<<"  pushq %rax\n";ex(*e.right);code<<"  movq %rax, %rcx\n  popq %rax\n";
    if(e.left->type==Type::Text){
     if(windows)code<<"  movq %rcx, %rdx\n  movq %rax, %rcx\n";else code<<"  movq %rax, %rdi\n  movq %rcx, %rsi\n";
     if(op=="+")ccall("serseg_text_concat");else {ccall("serseg_text_equal");if(op=="!=")code<<"  xorq $1, %rax\n";}break;
    }
    if(e.left->type==Type::Float){
     code<<"  movq %rax, %xmm0\n  movq %rcx, %xmm1\n";
     if(op=="+"||op=="-"||op=="*"||op=="/"){code<<"  "<<(op=="+"?"addsd":op=="-"?"subsd":op=="*"?"mulsd":"divsd")<<" %xmm1, %xmm0\n  movq %xmm0, %rax\n";}
     else {code<<"  ucomisd %xmm1, %xmm0\n";
      if(op=="=="||op=="<"||op=="<="){code<<"  "<<(op=="=="?"sete":op=="<"?"setb":"setbe")<<" %al\n  setnp %dl\n  andb %dl, %al\n";}
      else if(op=="!=")code<<"  setne %al\n  setp %dl\n  orb %dl, %al\n";
      else code<<"  "<<(op==">"?"seta":"setae")<<" %al\n";
      code<<"  movzbq %al, %rax\n";
     }
     break;
    }
    if(op=="+")code<<"  addq %rcx, %rax\n";
    else if(op=="-")code<<"  subq %rcx, %rax\n";
    else if(op=="*")code<<"  imulq %rcx, %rax\n";
    else if(op=="&")code<<"  andq %rcx, %rax\n";
    else if(op=="|")code<<"  orq %rcx, %rax\n";
    else if(op=="^")code<<"  xorq %rcx, %rax\n";
    else if(op=="<<"||op==">>"){code<<"  cmpq $63, %rcx\n  ja .Lserseg_shift_error\n  "<<(op=="<<"?"salq":"sarq")<<" %cl, %rax\n";}
    else if(op=="/"||op=="%"){auto ok=fresh();code<<"  testq %rcx, %rcx\n  jz .Lserseg_div_error\n  movabsq $-9223372036854775808, %rdx\n  cmpq %rdx, %rax\n  jne "<<ok<<"\n  cmpq $-1, %rcx\n  je .Lserseg_div_error\n"<<ok<<":\n  cqto\n  idivq %rcx\n";if(op=="%")code<<"  movq %rdx, %rax\n";}
    else{std::string inst=op=="=="?"sete":op=="!="?"setne":op=="<"?"setl":op=="<="?"setle":op==">"?"setg":"setge";code<<"  cmpq %rcx, %rax\n  "<<inst<<" %al\n  movzbq %al, %rax\n";}break;
   }
  }
 }
 void block(const Program& p){
  for(auto& ptr:p){auto& s=*ptr;switch(s.tag){
   case Stmt::Tag::Let:case Stmt::Tag::Assign:ex(*s.expr);if(managed(s.value_type)){code<<"  pushq %rax\n";release_slot(s.slot,s.value_type);code<<"  popq %rax\n";}code<<"  movq %rax, "<<loc(s.slot)<<"\n";break;
   case Stmt::Tag::IndexAssign:{code<<"  movq "<<loc(s.slot)<<", %rax\n";arg1("%rax");ccall("serseg_array_retain");code<<"  pushq %rax\n";ex(*s.expr);code<<"  pushq %rax\n";ex(*s.end);if(windows)code<<"  movq %rax, %r8\n  popq %rdx\n  popq %rcx\n";else code<<"  movq %rax, %rdx\n  popq %rsi\n  popq %rdi\n";ccall("serseg_array_set");break;}
   case Stmt::Tag::Push:{code<<"  movq "<<loc(s.slot)<<", %rax\n";arg1("%rax");ccall("serseg_array_retain");code<<"  pushq %rax\n";ex(*s.expr);if(windows)code<<"  movq %rax, %rdx\n  popq %rcx\n";else code<<"  movq %rax, %rsi\n  popq %rdi\n";ccall("serseg_array_push");break;}
   case Stmt::Tag::Show:{
    ex(*s.expr);
    const char* arg=windows?"%rcx":"%rdi";
    if(s.expr->type==Type::Int){if(windows)code<<"  movq %rax, %rdx\n  leaq .Lserseg_int_fmt(%rip), %rcx\n";else code<<"  movq %rax, %rsi\n  leaq .Lserseg_int_fmt(%rip), %rdi\n  xorl %eax, %eax\n";code<<"  call "<<call("printf")<<"\n";}
    else if(s.expr->type==Type::Float){if(windows)code<<"  movq %rax, %rdx\n  movq %rax, %xmm1\n  leaq .Lserseg_float_fmt(%rip), %rcx\n";else code<<"  movq %rax, %xmm0\n  leaq .Lserseg_float_fmt(%rip), %rdi\n  movl $1, %eax\n";code<<"  call "<<call("printf")<<"\n";}
    else if(s.expr->type==Type::Text){code<<"  pushq %rax\n  leaq 16(%rax), "<<arg<<"\n";ccall("puts");code<<"  popq %rax\n";arg1("%rax");ccall("serseg_text_release");}
    else{auto f=fresh(),end=fresh();code<<"  testq %rax, %rax\n  jz "<<f<<"\n  leaq .Lserseg_true(%rip), "<<arg<<"\n  jmp "<<end<<"\n"<<f<<":\n  leaq .Lserseg_false(%rip), "<<arg<<"\n"<<end<<":\n  call "<<call("puts")<<"\n";}break;
   }
   case Stmt::Tag::Assert:ex(*s.expr);code<<"  testq %rax, %rax\n  jz .Lserseg_assert_error\n";break;
   case Stmt::Tag::When:{auto alt=fresh(),end=fresh();ex(*s.expr);code<<"  testq %rax, %rax\n  jz "<<alt<<"\n";block(s.body);code<<"  jmp "<<end<<"\n"<<alt<<":\n";block(s.otherwise);code<<end<<":\n";break;}
   case Stmt::Tag::While:{auto start=fresh(),end=fresh();code<<start<<":\n";ex(*s.expr);code<<"  testq %rax, %rax\n  jz "<<end<<"\n";loops.push_back({end,start});block(s.body);loops.pop_back();code<<"  jmp "<<start<<"\n"<<end<<":\n";break;}
   case Stmt::Tag::Repeat:{auto start=fresh(),step=fresh(),end=fresh();ex(*s.expr);code<<"  movq %rax, "<<loc(s.slot)<<"\n";ex(*s.end);code<<"  movq %rax, "<<loc(s.slot+1)<<"\n"<<start<<":\n  movq "<<loc(s.slot)<<", %rax\n  cmpq "<<loc(s.slot+1)<<", %rax\n  jge "<<end<<"\n";loops.push_back({end,step});block(s.body);loops.pop_back();code<<step<<":\n  addq $1, "<<loc(s.slot)<<"\n  jmp "<<start<<"\n"<<end<<":\n";break;}
   case Stmt::Tag::Each:{
    auto start=fresh(),step=fresh(),end=fresh();
    ex(*s.expr);code<<"  pushq %rax\n";release_slot(s.array_slot,Type::IntArray);code<<"  popq %rax\n  movq %rax, "<<loc(s.array_slot)<<"\n";
    arg1("%rax");ccall("serseg_array_retain");arg1("%rax");ccall("serseg_array_length");
    code<<"  movq %rax, "<<loc(s.limit_slot)<<"\n  movq $0, "<<loc(s.index_slot)<<"\n"<<start<<":\n  movq "<<loc(s.index_slot)<<", %rax\n  cmpq "<<loc(s.limit_slot)<<", %rax\n  jge "<<end<<"\n";
    if(windows)code<<"  movq %rax, %rdx\n  movq "<<loc(s.array_slot)<<", %rcx\n";else code<<"  movq %rax, %rsi\n  movq "<<loc(s.array_slot)<<", %rdi\n";
    ccall("serseg_array_get_borrowed");if(managed(s.value_type)){code<<"  pushq %rax\n";release_slot(s.slot,s.value_type);code<<"  popq %rax\n";}code<<"  movq %rax, "<<loc(s.slot)<<"\n";
    loops.push_back({end,step});block(s.body);loops.pop_back();
    code<<step<<":\n  addq $1, "<<loc(s.index_slot)<<"\n  jmp "<<start<<"\n"<<end<<":\n";
    release_slot(s.array_slot,Type::IntArray);code<<"  movq $0, "<<loc(s.array_slot)<<"\n";break;
   }
   case Stmt::Tag::Give:ex(*s.expr);code<<"  jmp "<<return_label<<"\n";break;
   case Stmt::Tag::Break:code<<"  jmp "<<loops.back().first<<"\n";break;
   case Stmt::Tag::Continue:code<<"  jmp "<<loops.back().second<<"\n";break;
   case Stmt::Tag::Task:break;
   case Stmt::Tag::Use:break;
  }}
 }
public:
 std::string run(const Program& p,int slots){
  std::vector<std::pair<int,Type>> main_text_slots;collect_slots(p,main_text_slots);
  code<<".text\n.globl main\n";if(!windows)code<<".type main,@function\n";code<<"main:\n";frame(slots);if(windows){code<<"  call __main\n";ccall("serseg_windows_console_init");}init_slots(main_text_slots);block(p);release_slots(main_text_slots);code<<"  xorl %eax, %eax\n  leave\n  ret\n";if(!windows)code<<".size main, .-main\n";
  for(const auto& ptr:p)if(ptr->tag==Stmt::Tag::Task){const auto& f=*ptr;return_label=fresh();std::vector<std::pair<int,Type>> text_slots;collect_slots(f.body,text_slots);for(const auto& param:f.params)if(param.annotation=="text")text_slots.push_back({param.slot,Type::Text});else if(param.annotation=="int_array"||param.annotation=="float_array"||param.annotation=="bool_array"||param.annotation=="text_array")text_slots.push_back({param.slot,Type::IntArray});code<<".Lserseg_task_"<<f.name<<":\n";frame(f.frame_slots);init_slots(text_slots);for(size_t i=0;i<f.params.size();i++)code<<"  movq "<<(16+8*(f.params.size()-1-i))<<"(%rbp), %rax\n  movq %rax, "<<loc(f.params[i].slot)<<"\n";block(f.body);code<<return_label<<":\n  pushq %rax\n";release_slots(text_slots);code<<"  popq %rax\n  leave\n  ret\n";}
  // Both failure paths can be entered with expression temporaries on the stack.
  code<<".Lserseg_assert_error:\n  andq $-16, %rsp\n";
  if(windows)code<<"  subq $32, %rsp\n  movl $2, %ecx\n  leaq .Lserseg_assert_msg(%rip), %rdx\n  movl $.Lserseg_assert_len, %r8d\n  call _write\n  movl $1, %ecx\n  call exit\n";
  else code<<"  movq $2, %rdi\n  leaq .Lserseg_assert_msg(%rip), %rsi\n  movq $.Lserseg_assert_len, %rdx\n  call write@PLT\n  movl $1, %edi\n  call exit@PLT\n";
  code<<"  ud2\n";
  code<<".Lserseg_shift_error:\n  andq $-16, %rsp\n";
  if(windows)code<<"  subq $32, %rsp\n  movl $2, %ecx\n  leaq .Lserseg_shift_msg(%rip), %rdx\n  movl $.Lserseg_shift_len, %r8d\n  call _write\n  movl $1, %ecx\n  call exit\n";
  else code<<"  movq $2, %rdi\n  leaq .Lserseg_shift_msg(%rip), %rsi\n  movq $.Lserseg_shift_len, %rdx\n  call write@PLT\n  movl $1, %edi\n  call exit@PLT\n";
  code<<"  ud2\n";
  code<<".Lserseg_div_error:\n  andq $-16, %rsp\n";
  if(windows)code<<"  subq $32, %rsp\n  movl $2, %ecx\n  leaq .Lserseg_div_msg(%rip), %rdx\n  movl $.Lserseg_div_len, %r8d\n  call _write\n  movl $1, %ecx\n  call exit\n";
  else code<<"  movq $2, %rdi\n  leaq .Lserseg_div_msg(%rip), %rsi\n  movq $.Lserseg_div_len, %rdx\n  call write@PLT\n  movl $1, %edi\n  call exit@PLT\n";
  code<<"  ud2\n"<<(windows?".section .rdata,\"dr\"\n":".section .rodata\n")<<".Lserseg_int_fmt: .asciz \""<<(windows?"%lld":"%ld")<<"\\n\"\n.Lserseg_float_fmt: .asciz \"%.17g\\n\"\n.Lserseg_true: .asciz \"true\"\n.Lserseg_false: .asciz \"false\"\n.Lserseg_div_msg: .ascii \"SERSEG runtime error: invalid integer division\\n\"\n.set .Lserseg_div_len, . - .Lserseg_div_msg\n"<<".Lserseg_assert_msg: .ascii \"SERSEG runtime error: assertion failed\\n\"\n.set .Lserseg_assert_len, . - .Lserseg_assert_msg\n"<<".Lserseg_shift_msg: .ascii \"SERSEG runtime error: invalid shift count\\n\"\n.set .Lserseg_shift_len, . - .Lserseg_shift_msg\n"<<data.str();
  if(!windows)code<<".section .note.GNU-stack,\"\",@progbits\n";
  return code.str();
 }
};
std::string generate(const Program& p,int slots){return Generator().run(p,slots);}
}
