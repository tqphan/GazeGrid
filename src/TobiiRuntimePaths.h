#pragma once
#include <QDirIterator>
#include <QFileInfo>
#include <QStringList>

// Search only the supplied vendor installation trees, never the working directory
// or PATH. Do not follow directory junctions/symlinks outside those trees.
inline QStringList tobiiRuntimePaths(const QStringList &installationRoots){
 QStringList paths;
 for(const auto &root:installationRoots){
  const QString canonicalRoot=QFileInfo(root).canonicalFilePath();
  if(canonicalRoot.isEmpty())continue;
  QDirIterator it(root,{"tobii_stream_engine.dll"},QDir::Files,QDirIterator::Subdirectories);
  while(it.hasNext()){
   const QString path=QFileInfo(it.next()).canonicalFilePath();
   if(path.startsWith(canonicalRoot+"/",Qt::CaseInsensitive)&&!paths.contains(path))paths<<path;
  }
 }
 paths.sort(Qt::CaseInsensitive);
 return paths;
}
