#pragma once
#include "scs_sdk_min.h"

namespace aa::log {

// Recebe o logger do jogo (aparece no game.log.txt e no console do jogo).
void init(scs_log_t gameLogger);
void shutdown();

void info(const char *fmt, ...);
void warn(const char *fmt, ...);
void error(const char *fmt, ...);

} // namespace aa::log
