#include "config.h"
#ifdef MINGW_VER
__declspec(dllexport)
#endif
int output_setup(kafka_t* k, config* fk_conf);
#ifdef MINGW_VER
__declspec(dllexport)
#endif
int output_send(kafka_t* k, char* buf, size_t len);
#ifdef MINGW_VER
__declspec(dllexport)
#endif
int output_clean(kafka_t* k);
#ifdef MINGW_VER
__declspec(dllexport)
#endif
int output_update();
#include <output.h>
