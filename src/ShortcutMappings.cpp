#include "ShortcutMappings.h"
#include "ActionRunner.h"
#include "ActionCatalog.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QSet>
QJsonObject ShortcutMappings::parse(const QByteArray &json,QString &error){
 error.clear();QJsonParseError parseError;auto doc=QJsonDocument::fromJson(json,&parseError);auto profile=doc.object();
 if(parseError.error!=QJsonParseError::NoError||profile["version"].toDouble()!=1||profile["platform"]!="linux"||!profile["overrides"].isArray()){
  error="Mappings require version 1, platform linux, and an overrides array";return {};
 }
 QSet<QString> seen;
 for(const auto &entry:profile["overrides"].toArray()){
  auto rule=entry.toObject();const auto match=rule["match"].toString();
  if(match.isEmpty()||seen.contains(match)||!rule["commands"].isArray()||rule["commands"].toArray().isEmpty()||!rule["description"].isString()||rule["description"].toString().trimmed().isEmpty()){
   error="Each mapping needs a unique match, a description, and nonempty commands";return {};
  }
  const auto commands=rule["commands"].toArray().toVariantList();auto commandError=ActionRunner::validate(commands);
  if(!commandError.isEmpty()){error="Invalid mapped commands: "+commandError;return {};}
  for(const auto &command:commands){auto c=command.toMap();if(c["tag"]=="Action"&&!ActionCatalog::supported(c["text"].toString().trimmed())){error="Mapping contains an unsupported action";return {};}}
  seen.insert(match);
 }
 return profile;
}
ShortcutMappings::Result ShortcutMappings::resolve(const QVariantList &commands,const QJsonObject &profile,bool literal){
 const auto signature=ActionRunner::signature(commands);
 if(literal)return {commands,"Literal keys: "+signature,true};
 for(const auto &entry:profile["overrides"].toArray()){
  auto rule=entry.toObject();if(rule["match"].toString()!=signature)continue;
  const auto effective=rule["commands"].toArray().toVariantList();
  return {effective,rule["description"].toString()+" — original: "+signature+" — effective: "+ActionRunner::signature(effective),true};
 }
 bool meta=false,other=false;
 for(const auto &entry:commands){auto command=entry.toMap();auto tag=command["tag"].toString(),key=KeyNames::canonical(command["text"].toString());
  if(tag=="KeyDown"&&(key=="LWIN"||key=="RWIN"))meta=true;
  else if(tag=="Text"||tag=="KeyDown")other=true;
 }
 if(meta&&other)return {{},"Unmapped Windows shortcut: "+signature+". Add an override in Mappings or enable Literal keys.",false};
 return {commands,"Linux: original physical key sequence (no override)",true};
}
