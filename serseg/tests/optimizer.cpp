#include "../compiler/serseg.hpp"
#include <iostream>
#include <stdexcept>
#include <string>
static void require(bool b,const std::string& why){if(!b)throw std::runtime_error(why);}
int main(){
 try{
  auto p=serseg::parse(serseg::lex("show (2 + 3) * 4\nshow 1 / 0\n"));
  int slots=serseg::analyze(p);
  auto before=serseg::generate(p,slots);
  serseg::optimize_constants(p);
  auto after=serseg::generate(p,slots);
  require(before.find("imulq %rcx, %rax")!=std::string::npos,"expected multiplication before folding");
  require(after.find("imulq %rcx, %rax")==std::string::npos,"constant multiplication was not folded");
  require(before.find("addq %rcx, %rax")!=std::string::npos,"expected addition before folding");
  require(after.find("addq %rcx, %rax")==std::string::npos,"constant addition was not folded");
  require(after.find("idivq %rcx")!=std::string::npos,"division trap must remain");
  std::cout<<"PASS constant folding removes arithmetic and preserves division\n";
  return 0;
 }catch(const std::exception& e){std::cerr<<"FAIL optimizer: "<<e.what()<<"\n";return 1;}
}
