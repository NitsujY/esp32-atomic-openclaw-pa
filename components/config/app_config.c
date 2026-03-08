#include "config/app_config.h"

const char *app_config_default_mode_name(void)
{
#if CONFIG_OPENCLAW_DEFAULT_MODE_WORK
    return "work";
#else
    return "kid";
#endif
}
