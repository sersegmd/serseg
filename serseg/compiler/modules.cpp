#include "serseg.hpp"
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
namespace serseg {
namespace fs = std::filesystem;
class ModuleLoader {
 enum class State { Visiting, Done };
 std::unordered_map<std::string,State> states;
 std::unordered_map<std::string,std::unordered_set<std::string>> exports;
 using Imports=std::unordered_map<std::string,std::unordered_set<std::string>>;
 void resolve_expr(Expr& e,const Imports& imports){
  if(e.left)resolve_expr(*e.left,imports);
  if(e.right)resolve_expr(*e.right,imports);
  for(auto& a:e.args)resolve_expr(*a,imports);
  if(e.tag==Expr::Tag::Call){auto dot=e.value.find('.');if(dot!=std::string::npos){auto module=e.value.substr(0,dot),name=e.value.substr(dot+1);auto found=imports.find(module);if(found==imports.end())throw Error(e.pos,"M105","Module '"+module+"' is not imported in this file");if(!found->second.count(name))throw Error(e.pos,"M106","Module '"+module+"' does not export task '"+name+"'");e.value=name;}}
 }
 void resolve_block(Program& program,const Imports& imports){for(auto& s:program){if(s->expr)resolve_expr(*s->expr,imports);if(s->end)resolve_expr(*s->end,imports);resolve_block(s->body,imports);resolve_block(s->otherwise,imports);}}

 fs::path stdlib;
 Program result;
 void load(const fs::path& path, Pos origin, bool root, unsigned depth) {
  if(depth>128)throw Error(origin,"M104","Module dependency depth exceeds 128");
  std::error_code ec;auto canonical=fs::canonical(path,ec);
  if(ec)throw Error(origin,"M101","Cannot find module file '"+path.u8string()+"'");
  auto key=canonical.generic_u8string();auto found=states.find(key);
  if(found!=states.end()){if(found->second==State::Visiting)throw Error(origin,"M102","Circular module dependency involving '"+canonical.u8string()+"'");return;}
  if(states.size()>=1024)throw Error(origin,"M104","Project exceeds 1024 source files");
  states.emplace(key,State::Visiting);
  std::ifstream stream(canonical,std::ios::binary);if(!stream)throw Error(origin,"M101","Cannot open module file '"+canonical.u8string()+"'");
  std::ostringstream contents;contents<<stream.rdbuf();if(stream.bad())throw Error(origin,"M101","Cannot read module file '"+canonical.u8string()+"'");
  std::vector<Token> tokens;
  try{tokens=lex(contents.str());}catch(Error& e){e.pos.file=canonical.u8string();throw;}
  for(auto& token:tokens)token.pos.file=canonical.u8string();
  auto program=parse(tokens);
  for(auto& stmt:program)if(stmt->tag==Stmt::Tag::Task)exports[key].insert(stmt->name);
  Imports imports;
  for(auto& stmt:program)if(stmt->tag==Stmt::Tag::Use){auto target=canonical.parent_path()/(stmt->name+".ser");if(!fs::exists(target)&&!stdlib.empty())target=stdlib/(stmt->name+".ser");load(target,stmt->pos,false,depth+1);auto resolved=fs::canonical(target).generic_u8string();imports[stmt->name]=exports[resolved];}
  resolve_block(program,imports);
  for(auto& stmt:program){if(stmt->tag==Stmt::Tag::Use)continue;if(!root&&stmt->tag!=Stmt::Tag::Task)throw Error(stmt->pos,"M103","Imported modules may contain only tasks and imports at top level");result.push_back(std::move(stmt));}
  states[key]=State::Done;
 }
public:
 explicit ModuleLoader(fs::path dir):stdlib(std::move(dir)){}
 Program run(const fs::path& path){load(path,Pos{1,1,path.u8string()},true,0);return std::move(result);}
};
Program load_program(const fs::path& path,const fs::path& stdlib){return ModuleLoader(stdlib).run(path);}
}
