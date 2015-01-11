#include "triggers.h"

idTriggerDef::idTriggerDef(void)
{
	Lock = false;
	Used = false;
	TriggerDistance = 100.0f;
	ReleaseDistance = 125.0f;
}

idTriggerDef::~idTriggerDef(void)
{
}
