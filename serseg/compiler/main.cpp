#include "serseg.hpp"
#include "runtime.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <cerrno>
#include <cctype>
#include <cstdlib>
#include <algorithm>
#ifdef _WIN32
#include "windows_process.hpp"
#include <shellapi.h>
#include <random>
#else
#include <sys/wait.h>
#include <unistd.h>
#endif
namespace fs=std::filesystem;
#ifndef _WIN32
static fs::path serseg_path(const std::string& value){return fs::path(value);}
#endif
static int execute(const std::vector<std::string>& args, const fs::path& directory = {}){
#ifdef _WIN32
 return serseg_windows_execute(args,directory);
#else
 if(!directory.empty())throw std::runtime_error("Unsupported process working directory");
 std::vector<const char*> a;for(auto& value:args)a.push_back(value.c_str());a.push_back(nullptr);
 pid_t p=fork();if(p<0)throw std::runtime_error("Could not start process");if(p==0){execvp(a[0],const_cast<char* const*>(a.data()));perror("exec");_exit(127);}int status;while(waitpid(p,&status,0)<0){if(errno!=EINTR)throw std::runtime_error("Could not wait for process");}if(WIFEXITED(status))return WEXITSTATUS(status);if(WIFSIGNALED(status))return 128+WTERMSIG(status);return 1;
#endif
}
static std::string read_file(const fs::path& p){std::ifstream f(p,std::ios::binary);if(!f)throw std::runtime_error("Cannot open source file: "+p.u8string());std::ostringstream b;b<<f.rdbuf();if(f.bad())throw std::runtime_error("Cannot read source file: "+p.u8string());return b.str();}
static void diagnostic(const serseg::Error& e,const fs::path& path,const std::string& source){fs::path actual=e.pos.file.empty()?path:serseg_path(e.pos.file);std::string text=source;if(actual!=path){try{text=read_file(actual);}catch(...){text.clear();}}std::cerr<<"Error["<<e.code<<"]: "<<e.what()<<"\n  --> "<<actual.u8string()<<":"<<e.pos.line<<":"<<e.pos.col<<"\n";std::istringstream input(text);std::string line;for(int i=1;i<=e.pos.line&&std::getline(input,line);i++){if(i==e.pos.line){if(i==1&&line.compare(0,3,"\xef\xbb\xbf")==0)line.erase(0,3);std::cerr<<i<<" | "<<line<<"\n  | "<<std::string(e.pos.col>0?e.pos.col-1:0,' ')<<"^\n";}}}
struct Temp { fs::path path;Temp(){
#ifdef _WIN32
 std::vector<wchar_t> buffer(32768);DWORD len=GetTempPathW(static_cast<DWORD>(buffer.size()),buffer.data());if(!len||len>=buffer.size())throw std::runtime_error("Cannot find Windows temporary directory");fs::path base(buffer.data());
 std::random_device random;for(int i=0;i<32;i++){auto name="serseg-"+std::to_string(random())+"-"+std::to_string(random())+"-"+std::to_string(random());auto candidate=base/name;std::error_code ec;if(fs::create_directory(candidate,ec)){path=candidate;return;}}throw std::runtime_error("Cannot create temporary directory");
#else
 std::string t=(fs::temp_directory_path()/"serseg-XXXXXX").string();std::vector<char>b(t.begin(),t.end());b.push_back(0);char* p=mkdtemp(b.data());if(!p)throw std::runtime_error("Cannot create temporary directory");path=p;
#endif
 }~Temp(){std::error_code ec;fs::remove_all(path,ec);}};
static void create_project(const std::string& name){
 if(name.empty()||name.size()>64||!(std::isalpha(static_cast<unsigned char>(name[0]))||name[0]=='_'))throw std::runtime_error("Project name must begin with an ASCII letter or underscore");
 for(unsigned char c:name)if(!(c<128&&(std::isalnum(c)||c=='_'||c=='-')))throw std::runtime_error("Project name may contain only ASCII letters, digits, underscores, and hyphens");
 std::error_code ec;if(!fs::create_directory(name,ec))throw std::runtime_error("Cannot create project directory; it may already exist: "+name+(ec?" ("+ec.message()+")":""));
 auto write=[](const fs::path& p,const std::string& content){std::ofstream f(p,std::ios::binary);if(!f)throw std::runtime_error("Cannot create "+p.u8string());f<<content;if(!f)throw std::runtime_error("Cannot write "+p.u8string());};
 auto dir=fs::path(name);if(!fs::create_directory(dir/"src"))throw std::runtime_error("Cannot create project source directory");
 write(dir/"src"/"main.ser","show \"Hello from SERSEG!\"\n");
 write(dir/"serseg.toml","# Metadata only: this compiler does not read this file yet.\n[package]\nname = \""+name+"\"\nversion = \"0.1.0\"\ndescription = \"A SERSEG application\"\n\n[dependencies]\n");
 write(dir/"README.md","# "+name+"\n\nRun with `serseg run`. This compiler does not yet read serseg.toml.\n");
 std::cout<<"Created project "<<name<<" (serseg.toml is not read by this compiler yet)\n";
}
static fs::path find_executable(const std::string& name){
 auto usable=[](const fs::path& p){std::error_code ec;if(!fs::is_regular_file(p,ec))return false;
#ifdef _WIN32
 return true;
#else
 return access(p.c_str(),X_OK)==0;
#endif
 };
 if(name.find('/')!=std::string::npos || name.find('\\')!=std::string::npos){fs::path p=serseg_path(name);return usable(p)?fs::absolute(p):fs::path{};}
#ifdef _WIN32
 DWORD size=GetEnvironmentVariableW(L"PATH",nullptr,0);if(!size)return {};
 std::wstring value(size,L'\0');DWORD got=GetEnvironmentVariableW(L"PATH",&value[0],size);if(!got||got>=size)return {};value.resize(got);
 std::wstringstream paths(value);std::wstring part;auto wide_name=serseg_widen(name);
 while(std::getline(paths,part,L';')){if(part.size()>=2&&part.front()==L'"'&&part.back()==L'"')part=part.substr(1,part.size()-2);fs::path dir=part.empty()?fs::path(L"."):fs::path(part);for(const auto& suffix:{L".exe",L".cmd",L".bat",L""}){auto p=dir/(wide_name+suffix);if(usable(p))return fs::absolute(p);}}return {};
#else
 const char* value=std::getenv("PATH");if(!value)return {};
 std::stringstream paths(value);std::string part;while(std::getline(paths,part,':')){fs::path dir=part.empty()?fs::path("."):fs::path(part);auto p=dir/name;if(usable(p))return fs::absolute(p);}return {};
#endif
}
static int doctor(const char* self){
 std::cout<<"SERSEG toolchain diagnostics\n";auto compiler=find_executable(self);auto gcc=find_executable("gcc");auto global=find_executable("serseg");auto code=find_executable("code");
 if(compiler.empty())std::cout<<"[warning] Cannot locate compiler executable.\n";else std::cout<<"[ok] Compiler: "<<compiler.u8string()<<"\n";
 if(gcc.empty())std::cout<<"[missing] GCC: install a compatible GCC toolchain and add it to PATH."
#ifdef _WIN32
 <<" On Windows, install MSYS2 UCRT64 GCC and add its bin directory to PATH."
#endif
 <<"\n";else std::cout<<"[ok] GCC found: "<<gcc.u8string()<<" (linker integrity not checked)\n";
 if(global.empty())std::cout<<"[warning] serseg is not available on PATH; installation is not configured.\n";else std::cout<<"[ok] serseg on PATH: "<<global.u8string()<<"\n";
 if(code.empty())std::cout<<"[info] VS Code command not found on PATH; extension status not checked.\n";else std::cout<<"[info] VS Code command found; extension status not checked.\n";
 std::error_code ec;auto resolved=compiler.empty()?fs::path{}:fs::canonical(compiler,ec);if(ec)resolved=compiler;
 auto math=resolved.empty()?fs::path{}:resolved.parent_path().parent_path()/"stdlib"/"math.ser";
 if(!math.empty() && fs::is_regular_file(math,ec))std::cout<<"[ok] Math standard library module: "<<math.u8string()<<"\n";
 else std::cout<<"[missing] Math standard library module not found alongside compiler.\n";
 std::cout<<"[missing] Other standard library modules and general-purpose managed objects are unavailable.\n";
 std::cout<<"[info] Generated programs use the platform C runtime; dependency portability is not checked.\n";
 return 1;
}
static int run_tests(const char* self){
 auto compiler=find_executable(self);if(compiler.empty())throw std::runtime_error("Cannot locate compiler for test execution");
 fs::path dir="tests";if(!fs::is_directory(dir))throw std::runtime_error("No tests directory found in the current directory");
 std::vector<fs::path> paths;for(const auto& entry:fs::recursive_directory_iterator(dir))if(entry.is_regular_file()&&entry.path().extension()==".ser")paths.push_back(entry.path());std::sort(paths.begin(),paths.end());
 if(paths.empty())throw std::runtime_error("No .ser test files found in tests/");
 int failed=0;for(const auto& p:paths){std::cout<<"TEST "<<p.u8string()<<"\n"<<std::flush;int status=execute({compiler.u8string(),"run",p.u8string()});if(status==0)std::cout<<"PASS "<<p.u8string()<<"\n";else{std::cout<<"FAIL "<<p.u8string()<<" (exit "<<status<<")\n";failed++;}}
 std::cout<<(paths.size()-failed)<<" passed; "<<failed<<" failed\n";return failed?1:0;
}
static void help(){std::cout<<"SERSEG compiler (x86-64 native)\n\nUsage:\n  serseg --version\n  serseg help\n  serseg doctor\n  serseg new NAME\n  serseg check FILE.ser\n  serseg build FILE.ser [-o OUTPUT] [-O0|-O1]\n  serseg run [FILE.ser] [-O0|-O1]\n  serseg test\n\nSee README.md for implemented features and limitations.\n";}
static int serseg_main(int argc,char**argv){try{if(argc<2){help();return 2;}std::string cmd=argv[1];if(cmd=="--version"||cmd=="version"){if(argc!=2)throw std::runtime_error("version takes no arguments");std::cout<<"SERSEG 0.2.0\nCompiler: SERSEG 0.2.0\nHost target: "
#ifdef _WIN32
 <<"x86_64-windows-gnu\n";
#else
 <<"x86_64-linux-gnu\n";
#endif
return 0;}if(cmd=="help"||cmd=="--help"){help();return 0;}if(cmd=="test"){if(argc!=2)throw std::runtime_error("test takes no arguments");return run_tests(argv[0]);}if(cmd=="doctor"){if(argc!=2)throw std::runtime_error("doctor takes no arguments");return doctor(argv[0]);}if(cmd=="new"){if(argc!=3)throw std::runtime_error("Usage: serseg new NAME");create_project(argv[2]);return 0;}if(cmd!="check"&&cmd!="build"&&cmd!="run")throw std::runtime_error("Unknown or unavailable command '"+cmd+"'; use 'serseg help'");fs::path source_path,output;int optimization=0;bool have_opt=false;
for(int i=2;i<argc;i++){std::string arg=argv[i];if(arg=="-O0"||arg=="-O1"){if(cmd=="check"||have_opt)throw std::runtime_error("Optimization option not valid here");optimization=arg=="-O1"?1:0;have_opt=true;}else if(arg=="-o"){if(cmd!="build"||!output.empty()||i+1>=argc)throw std::runtime_error("-o requires one build output path");output=serseg_path(argv[++i]);if(output.empty())throw std::runtime_error("Empty output path");}else if(!arg.empty()&&arg[0]=='-')throw std::runtime_error("Unknown option '"+arg+"'");else if(source_path.empty())source_path=serseg_path(arg);else throw std::runtime_error("Unexpected argument '"+arg+"'");}
if(source_path.empty()){if(cmd=="run")source_path="src/main.ser";else throw std::runtime_error("Missing .ser source file");}if(source_path.extension()!=".ser")throw std::runtime_error("Source files must have the .ser extension");
if(cmd=="build"&&output.empty()){output=source_path.filename();
#ifdef _WIN32
output.replace_extension(".exe");
#else
output.replace_extension();
#endif
}
#ifdef _WIN32
if(cmd=="build" && !output.empty() && output.extension().empty())output+=L".exe";
#endif
auto source=read_file(source_path);serseg::Program program;int slots;try{auto self=find_executable(argv[0]);std::error_code canonical_error;auto resolved=self.empty()?fs::path{}:fs::canonical(self,canonical_error);if(canonical_error)resolved=self;auto stdlib=resolved.empty()?fs::path{}:resolved.parent_path().parent_path()/"stdlib";program=serseg::load_program(source_path,stdlib);slots=serseg::analyze(program);}catch(const serseg::Error& e){diagnostic(e,source_path,source);return 1;}if(optimization==1)serseg::optimize_constants(program);if(cmd=="check"){std::cout<<"Check passed: "<<source_path.u8string()<<"\n";return 0;}
 Temp temp;fs::path assembly=temp.path/"program.s",runtime=temp.path/"runtime.c";
#ifdef _WIN32
 fs::path binary=temp.path/"program.exe";
#else
 fs::path binary=temp.path/"program";
#endif
 if(cmd=="build"&&fs::exists(output)){throw std::runtime_error("Output already exists; refusing to overwrite: "+output.u8string());}
 {std::ofstream f(assembly,std::ios::binary);f<<serseg::generate(program,slots);if(!f)throw std::runtime_error("Could not write temporary assembly");}{std::ofstream f(runtime,std::ios::binary);f<<serseg_runtime;if(!f)throw std::runtime_error("Could not write temporary runtime source");}
#ifdef _WIN32
 int status=execute({"gcc",optimization==1?"-O1":"-O0","program.s","runtime.c","-Wl,--stack,8388608","-o","program.exe"},temp.path);
#else
 int status=execute({"gcc",optimization==1?"-O1":"-O0",assembly.u8string(),runtime.u8string(),"-o",binary.u8string()});
#endif
if(status!=0){std::cerr<<"Native assembler/linker failed (exit "<<status<<")\n";return status;}if(cmd=="build"){std::error_code ec;if(!fs::copy_file(binary,output,fs::copy_options::none,ec))throw std::runtime_error("Cannot create output (existing files are not overwritten): "+ec.message());std::cout<<"Built "<<output.u8string()<<"\n";return 0;}return execute({fs::absolute(binary).u8string()});}catch(const std::exception& e){std::cerr<<"Error: "<<e.what()<<"\n";return 1;}}

int main(int argc,char** argv){
#ifdef _WIN32
 (void)argc;(void)argv;
 int count=0;wchar_t** values=CommandLineToArgvW(GetCommandLineW(),&count);
 if(!values){std::cerr<<"Error: cannot read Windows command line\n";return 1;}
 try{
  std::vector<std::string> utf8;utf8.reserve(static_cast<size_t>(count));
  for(int i=0;i<count;i++)utf8.push_back(serseg_narrow(values[i]));
  LocalFree(values);values=nullptr;
  std::vector<char*> args;args.reserve(utf8.size());for(auto& value:utf8)args.push_back(value.data());
  return serseg_main(count,args.data());
 }catch(const std::exception& e){if(values)LocalFree(values);std::cerr<<"Error: "<<e.what()<<"\n";return 1;}
#else
 return serseg_main(argc,argv);
#endif
}
