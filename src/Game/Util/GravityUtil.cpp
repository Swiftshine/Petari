#include "Game/Gravity.hpp"
#include "Game/Util/GravityUtil.hpp"
#include "Game/Util/JMapInfo.hpp"
#include "Game/Scene/SceneObjHolder.hpp"
#include "Game/LiveActor/LiveActor.hpp"
#include <cstring>

namespace {
	void getJMapInfoArgPlus(const JMapInfoIter &rIter, const char *pFieldName, f32 *pDest) {

	}

	void getJMapInfoArgPlus(const JMapInfoIter &rIter, const char *pFieldName, s32 *pDest) {
		// Get row and column of data
		s32 row = rIter.mIndex;
		const JMapInfo* pInfo = rIter.mInfo;
		s32 column = pInfo->searchItemInfo(pFieldName);

		// Try to read value
		s32 result;
		bool read = column < 0 ? false : pInfo->getValueFast(row, column, &result);

		// Set result if applicable
		if (read && result >= 0.0f) {
			*pDest = result;
		}
	}

	bool calcGravityVectorOrZero(const NameObj *pActor, const TVec3f &rPosition, u32 gravityType, TVec3f * pDest, GravityInfo *pInfo, u32 host) {
		if (!host) {
			host = (u32)pActor;
		}

		PlanetGravityManager* pPlanetGravityManager
            = MR::getSceneObj<PlanetGravityManager*>(SceneObj_PlanetGravityManager);

		return pPlanetGravityManager->calcTotalGravityVector(pDest, pInfo, rPosition, gravityType, host);
	}
}

namespace MR {
	void registerGravity(PlanetGravity *pGravity) {
		MR::getSceneObj<PlanetGravityManager*>(SceneObj_PlanetGravityManager)->registerGravity(pGravity);
	}

	bool calcGravityVector(const LiveActor *pActor, TVec3f *pDest, GravityInfo *pInfo, u32 host) {
		u32 typeFlags = GRAVITY_TYPE_NORMAL;
		return ::calcGravityVectorOrZero(pActor, pActor->mPosition, typeFlags, pDest, pInfo, host);
	}

	bool calcGravityVector(const NameObj *pActor, const TVec3f &rPosition, TVec3f *pDest, GravityInfo *pInfo, u32 host) {
		u32 typeFlags = GRAVITY_TYPE_NORMAL;
		return ::calcGravityVectorOrZero(pActor, rPosition, typeFlags, pDest, pInfo, host);
	}

	bool calcDropShadowVector(const LiveActor *pActor, TVec3f *pDest, GravityInfo *pInfo, u32 host) {
		u32 typeFlags = GRAVITY_TYPE_SHADOW;
		return ::calcGravityVectorOrZero(pActor, pActor->mPosition, typeFlags, pDest, pInfo, host);
	}

	bool calcDropShadowVector(const NameObj *pActor, const TVec3f &rPosition, TVec3f *pDest, GravityInfo *pInfo, u32 host) {
		u32 typeFlags = GRAVITY_TYPE_SHADOW;
		return ::calcGravityVectorOrZero(pActor, rPosition, typeFlags, pDest, pInfo, host);
	}

	bool calcGravityAndDropShadowVector(const LiveActor *pActor, TVec3f *pDest, GravityInfo *pInfo, u32 host) {
		u32 typeFlags = GRAVITY_TYPE_NORMAL | GRAVITY_TYPE_SHADOW;
		return ::calcGravityVectorOrZero(pActor, pActor->mPosition, typeFlags, pDest, pInfo, host);
	}

	bool calcGravityAndMagnetVector(const NameObj *pActor, const TVec3f &rPosition, TVec3f *pDest, GravityInfo *pInfo, u32 host) {
		u32 typeFlags = GRAVITY_TYPE_NORMAL | GRAVITY_TYPE_MAGNET;
		return ::calcGravityVectorOrZero(pActor, rPosition, typeFlags, pDest, pInfo, host);
	}

	bool calcGravityVectorOrZero(const LiveActor *pActor, TVec3f *pDest, GravityInfo *pInfo, u32 host) {
		u32 typeFlags = GRAVITY_TYPE_NORMAL;
		return ::calcGravityVectorOrZero(pActor, pActor->mPosition, typeFlags, pDest, pInfo, host);
	}

	bool calcGravityVectorOrZero(const NameObj *pActor, const TVec3f &rPosition, TVec3f *pDest, GravityInfo *pInfo, u32 host) {
		u32 typeFlags = GRAVITY_TYPE_NORMAL;
		return ::calcGravityVectorOrZero(pActor, rPosition, typeFlags, pDest, pInfo, host);
	}

	bool calcDropShadowVectorOrZero(const NameObj *pActor, const TVec3f &rPosition, TVec3f *pDest, GravityInfo *pInfo, u32 host) {
		u32 typeFlags = GRAVITY_TYPE_SHADOW;
		return ::calcGravityVectorOrZero(pActor, rPosition, typeFlags, pDest, pInfo, host);
	}

	bool calcGravityAndDropShadowVectorOrZero(const LiveActor *pActor, TVec3f *pDest, GravityInfo *pInfo, u32 host) {
		u32 typeFlags = GRAVITY_TYPE_NORMAL | GRAVITY_TYPE_SHADOW;
		return ::calcGravityVectorOrZero(pActor, pActor->mPosition, typeFlags, pDest, pInfo, host);
	}

	bool calcAttractMarioLauncherOrZero(const LiveActor *pActor, TVec3f *pDest, GravityInfo *pInfo, u32 host) {
		u32 typeFlags = GRAVITY_TYPE_MARIO_LAUNCHER;
		return ::calcGravityVectorOrZero(pActor, pActor->mPosition, typeFlags, pDest, pInfo, host);
	}

	bool isZeroGravity(const LiveActor *pActor) {
		TVec3f dummyGravity;
		return ::calcGravityVectorOrZero(pActor, pActor->mPosition, GRAVITY_TYPE_NORMAL, &dummyGravity, nullptr, 0) == false;
	}

	bool isLightGravity(const GravityInfo &rInfo) {
		PlanetGravity* pGravity = rInfo.mGravityInstance;
		return !pGravity ? false : pGravity->mGravityPower == GRAVITY_POWER_LIGHT;
	}

	void settingGravityParamFromJMap(PlanetGravity *pGravity, const JMapInfoIter &rIter) {
		// Get range, distant and priority
		f32 argRange = pGravity->mRange;
		::getJMapInfoArgPlus(rIter, "Range", &argRange);
		f32 distant = pGravity->mDistant;
		pGravity->mRange = argRange;
		f32 argDistant = distant;
		::getJMapInfoArgPlus(rIter, "Distant", &argDistant);
		s32 priority = pGravity->mPriority;
		pGravity->mDistant = argDistant;
		s32 argPriority = priority;
		::getJMapInfoArgPlus(rIter, "Priority", &argPriority);
		pGravity->setPriority(argPriority);

		// Get gravity ID
		s32 gravityId = pGravity->mGravityId;
		::getJMapInfoArgPlus(rIter, "Gravity_id", &gravityId);
		pGravity->mGravityId = gravityId;

		// Get gravity type and power
		getJMapInfoGravityType(rIter, pGravity);
		getJMapInfoGravityPower(rIter, pGravity);

		// Check if gravity is inverse
		s32 inverse = pGravity->mIsInverse != false;
		::getJMapInfoArgPlus(rIter, "Inverse", &inverse);
		pGravity->mIsInverse = inverse;
	}

	void getJMapInfoGravityType(const JMapInfoIter &rIter, PlanetGravity *pGravity) {
		const char *pType = nullptr;

		if (rIter.getValue("Gravity_type", &pType)) {
			if (!strcmp(pType, "Normal")) {
				pGravity->mGravityType = GRAVITY_TYPE_NORMAL;
			}
			else if (!strcmp(pType, "Shadow")) {
				pGravity->mGravityType = GRAVITY_TYPE_SHADOW;
			}
			else if (!strcmp(pType, "Magnet")) {
				pGravity->mGravityType = GRAVITY_TYPE_MAGNET;
			}
		}
	}

	void getJMapInfoGravityPower(const JMapInfoIter &rIter, PlanetGravity *pGravity) {
		const char *pPower = nullptr;

		if (rIter.getValue("Power", &pPower)) {
			if (!strcmp(pPower, "Light")) {
				pGravity->mGravityPower = GRAVITY_POWER_LIGHT;
			}
			else if (!strcmp(pPower, "Normal")) {
				pGravity->mGravityPower = GRAVITY_POWER_NORMAL;
			}
			else if (!strcmp(pPower, "Heavy")) {
				pGravity->mGravityPower = GRAVITY_POWER_HEAVY;
			}
		}
	}
}
