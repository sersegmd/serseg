#pragma once
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
#include <utility>
#include <filesystem>

namespace serseg {
struct Pos { int line, col; std::string file; Pos(int l=1,int c=1,std::string f={}):line(l),col(c),file(std::move(f)){} };
struct Error: std::runtime_error { Pos pos; std::string code; Error(Pos p, std::string c, std::string m):std::runtime_error(std::move(m)),pos(p),code(std::move(c)){} };
enum class Kind { End, Newline, Id, Int, Float, String, LBrace, RBrace, LParen, RParen, LBracket, RBracket, Dot, Colon, Comma, Arrow, Plus, Minus, Star, Slash, Percent, Eq, EqEq, NotEq, Lt, Le, Gt, Ge, Bang, And, Or, BitAnd, BitOr, BitXor, BitNot, ShiftLeft, ShiftRight };
struct Token { Kind kind; std::string text; Pos pos; };
std::vector<Token> lex(const std::string& source);
enum class Type { Int, Float, Bool, Text, IntArray, FloatArray, BoolArray, TextArray, Invalid };
inline bool is_array(Type t){return t==Type::IntArray||t==Type::FloatArray||t==Type::BoolArray||t==Type::TextArray;}
inline Type array_element(Type t){switch(t){case Type::IntArray:return Type::Int;case Type::FloatArray:return Type::Float;case Type::BoolArray:return Type::Bool;case Type::TextArray:return Type::Text;default:return Type::Invalid;}}
inline Type array_of(Type t){switch(t){case Type::Int:return Type::IntArray;case Type::Float:return Type::FloatArray;case Type::Bool:return Type::BoolArray;case Type::Text:return Type::TextArray;default:return Type::Invalid;}}
struct Expr { enum class Tag { Int, Float, Bool, Text, Var, Unary, Binary, Call, Array, Index, Length } tag; Pos pos; std::string value; std::unique_ptr<Expr> left, right; Type type=Type::Invalid; int slot=-1; std::vector<std::unique_ptr<Expr>> args; };
struct Param { std::string name, annotation; Pos pos; int slot=-1; };
struct Stmt { enum class Tag { Let, Assign, IndexAssign, Push, Show, When, Repeat, Each, While, Task, Give, Break, Continue, Assert, Use } tag; Pos pos; std::string name, annotation; bool mut=false, constant=false; Type value_type=Type::Invalid; int slot=-1, frame_slots=0, array_slot=-1, index_slot=-1, limit_slot=-1; std::vector<Param> params; std::unique_ptr<Expr> expr, end; std::vector<std::unique_ptr<Stmt>> body, otherwise; };
using Program=std::vector<std::unique_ptr<Stmt>>;
Program parse(const std::vector<Token>& tokens);
Program load_program(const std::filesystem::path& path, const std::filesystem::path& stdlib = {});
int analyze(Program& program);
void optimize_constants(Program& program);
std::string generate(const Program& program, int slots);
std::string type_name(Type t);
}
