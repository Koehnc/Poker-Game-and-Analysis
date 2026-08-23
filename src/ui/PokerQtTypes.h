#pragma once
#include "game/VisibleInformation.h"
#include "core/Card.h"
#include <QMetaType>
#include <vector>

Q_DECLARE_METATYPE(poker::Action)
Q_DECLARE_METATYPE(poker::GameStateView)
Q_DECLARE_METATYPE(poker::HandStep)
Q_DECLARE_METATYPE(std::vector<poker::Card>)
