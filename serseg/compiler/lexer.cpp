#include "serseg.hpp"
#include <cctype>
namespace serseg {
std::vector<Token> lex(const std::string& s) {
 std::vector<Token> out; size_t i=0; int line=1,col=1;
 if(s.size()>=3 && static_cast<unsigned char>(s[0])==0xef && static_cast<unsigned char>(s[1])==0xbb && static_cast<unsigned char>(s[2])==0xbf)i=3;
 auto advance=[&](){char c=s[i++]; if(c=='\n'){line++;col=1;} else col++; return c;};
 auto add=[&](Kind k,std::string v,Pos p){out.push_back({k,std::move(v),p});};
 while(i<s.size()) { char c=s[i]; Pos p{line,col};
  if(c==' '||c=='\t'||c=='\r'){advance();continue;}
  if(c=='\n'){advance();add(Kind::Newline,"",p);continue;}
  if(c=='#'){while(i<s.size()&&s[i]!='\n')advance();continue;}
  if(std::isalpha((unsigned char)c)||c=='_'){std::string v; while(i<s.size()&&(std::isalnum((unsigned char)s[i])||s[i]=='_'))v+=advance();add(Kind::Id,v,p);continue;}
  if(std::isdigit((unsigned char)c)){std::string v;bool floating=false;while(i<s.size()&&std::isdigit((unsigned char)s[i]))v+=advance();if(i<s.size()&&s[i]=='.'){floating=true;v+=advance();if(i==s.size()||!std::isdigit((unsigned char)s[i]))throw Error(p,"L005","Expected digit after decimal point");while(i<s.size()&&std::isdigit((unsigned char)s[i]))v+=advance();}if(i<s.size()&&(s[i]=='e'||s[i]=='E')){floating=true;v+=advance();if(i<s.size()&&(s[i]=='+'||s[i]=='-'))v+=advance();if(i==s.size()||!std::isdigit((unsigned char)s[i]))throw Error(p,"L005","Expected exponent digits");while(i<s.size()&&std::isdigit((unsigned char)s[i]))v+=advance();}add(floating?Kind::Float:Kind::Int,v,p);continue;}
  if(c=='"'){advance();std::string v;bool closed=false;while(i<s.size()){char a=advance();if(a=='"'){closed=true;break;}if(a=='\n')throw Error(p,"L002","Unterminated string literal");if(a=='\\'){if(i==s.size())break;char e=advance();if(e=='n')v+='\n';else if(e=='t')v+='\t';else if(e=='r')v+='\r';else if(e=='"')v+='"';else if(e=='\\')v+='\\';else throw Error({line,col-1},"L003","Unknown string escape");}else {if(a=='\0')throw Error(p,"L004","NUL is not allowed in text literals");v+=a;}}if(!closed)throw Error(p,"L002","Unterminated string literal");add(Kind::String,v,p);continue;}
  advance(); auto two=[&](char next,Kind yes,Kind no){if(i<s.size()&&s[i]==next){advance();add(yes,"",p);}else add(no,"",p);};
  switch(c){case '{':add(Kind::LBrace,"",p);break;case '}':add(Kind::RBrace,"",p);break;case '(':add(Kind::LParen,"",p);break;case ')':add(Kind::RParen,"",p);break;case '[':add(Kind::LBracket,"",p);break;case ']':add(Kind::RBracket,"",p);break;case '.':add(Kind::Dot,"",p);break;case ':':add(Kind::Colon,"",p);break;case ',':add(Kind::Comma,"",p);break;case '+':add(Kind::Plus,"",p);break;case '-':two('>',Kind::Arrow,Kind::Minus);break;case '*':add(Kind::Star,"",p);break;case '/':add(Kind::Slash,"",p);break;case '%':add(Kind::Percent,"",p);break;case '=':two('=',Kind::EqEq,Kind::Eq);break;case '!':two('=',Kind::NotEq,Kind::Bang);break;case '<':if(i<s.size()&&s[i]=='<'){advance();add(Kind::ShiftLeft,"",p);}else two('=',Kind::Le,Kind::Lt);break;case '>':if(i<s.size()&&s[i]=='>'){advance();add(Kind::ShiftRight,"",p);}else two('=',Kind::Ge,Kind::Gt);break;case '&':two('&',Kind::And,Kind::BitAnd);break;case '|':two('|',Kind::Or,Kind::BitOr);break;case '^':add(Kind::BitXor,"",p);break;case '~':add(Kind::BitNot,"",p);break;default:throw Error(p,"L001","Unexpected character");}
 }
 out.push_back({Kind::End,"",{line,col}});return out;
}
}
