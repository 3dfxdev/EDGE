#pragma once

#include "splines.h"

struct idTriggerDef;

typedef void (*TriggerAction)(idTriggerDef *t);

struct idTriggerDef
{
public:
	idTriggerDef(void);
	~idTriggerDef(void);

	virtual bool triggerCondition(idVec3_t p)
	{
		if (p)
		{
			idVec3_t dist = TriggerPosition - p;
			return (dist.Length() <= TriggerDistance);
		}
		return false;
	}

	virtual bool releaseCondition(idVec3_t p) {
		if (p) {
			idVec3_t dist = TriggerPosition - p;
			return (dist.Length() > ReleaseDistance);
		}
		return false;
	}

	void doAction(idVec3_t target, TriggerAction action) {
		if (Used) {
			if (!Lock) {
				if (triggerCondition(target)) {
					Lock = true;
					action(this);
				}
			} else {
				if (releaseCondition(target)) Lock = false;
			}
		}
	}

	bool Lock, Used;
	idVec3_t TriggerPosition;
	float TriggerDistance;
	float ReleaseDistance;

	TriggerAction action;
};
