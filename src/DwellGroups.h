#pragma once
#include <QStringList>
inline const QStringList &dwellGroupNames(){
 static const QStringList names{"Alphabet","Numbers","Punctuations","Modifiers","Functions","Predictions","Conversations","Navigation","Editing","Mouse","Gamepad","Others"};
 return names;
}
