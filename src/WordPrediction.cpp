#include "WordPrediction.h"
#include <algorithm>
namespace {
bool letter(QChar ch){return (ch>='a'&&ch<='z')||(ch>='A'&&ch<='Z');}
bool wordCharacter(QChar ch){return letter(ch)||ch=='\'';}
}
QString WordPrediction::prefix(const QString &text){
 int start=text.size();while(start>0&&wordCharacter(text[start-1]))--start;
 auto result=text.mid(start);
 // Do not complete a word embedded in a number or unsupported alphabet.
 if(start>0&&text[start-1].isLetterOrNumber())return {};
 if(result.isEmpty()||!letter(result.front()))return {};
 return result;
}
QString WordPrediction::completedWord(const QString &text){
 int end=text.size();while(end>0&&!text[end-1].isLetterOrNumber()&&text[end-1]!='\''){
  if(text[end-1].isSpace())return {};
  --end;
 }
 auto word=prefix(text.left(end));
 while(word.endsWith('\''))word.chop(1);
 return word.size()<64?word.toLower():QString();
}
QStringList WordPrediction::suggest(const QString &text,const QStringList &dictionary,const QMap<QString,int> &frequencies,int limit){
 const auto current=prefix(text);if(current.isEmpty()||limit<=0)return {};
 QStringList matches;
 for(const auto &word:dictionary){
  if(!word.startsWith(current,Qt::CaseInsensitive)||word.size()<=current.size())continue;
  if(!std::all_of(word.begin(),word.end(),wordCharacter))continue;
  QString suffix=word.mid(current.size()).toLower();
  // Preserve what was already typed and continue deliberate ALL CAPS prefixes.
  if(current.size()>1&&current==current.toUpper())suffix=suffix.toUpper();
  matches<<current+suffix;
 }
 matches.removeDuplicates();
 std::sort(matches.begin(),matches.end(),[&](const QString &a,const QString &b){
  const int fa=frequencies.value(a.toLower()),fb=frequencies.value(b.toLower());
  if(fa!=fb)return fa>fb;
  if(a.size()!=b.size())return a.size()<b.size();
  return a.compare(b,Qt::CaseInsensitive)<0;
 });
 return matches.mid(0,limit);
}
