#include "Game/LiveActor/ModelObj.hpp"
#include "Game/LiveActor/Nerve.hpp"
#include "Game/MapObj/PowerStar.hpp"
#include "Game/MapObj/PowerStarAppearPoint.hpp"
#include "Game/MapObj/PowerStarHolder.hpp"
#include "Game/NameObj/NameObjArchiveListCollector.hpp"
#include "math_types.hpp"

const GXColor lightColor[5] = {
    {0x96, 0x96, 0x32, 0 },
    {0x32, 0x32, 0x96, 0 },
    {0x32, 0x96, 0x32, 0 },
    {0x96, 0x32, 0x32, 0 },
    {0x64, 0x64, 0x64, 0 },
};

namespace {
    void setupColor(LiveActor *pActor, bool useFrame, int frame) {
        MR::startBtp(pActor, "PowerStar");
        MR::setBtpFrameAndStop(pActor, useFrame ? 0.0f : frame);
        MR::startBva(pActor, "PowerStar");
        MR::setBvaFrameAndStop(pActor, useFrame ? 1.0f : 0.0f);
    }

    void setupColorGrandStar(LiveActor *pActor, bool dontStartRecover) {
        if (dontStartRecover) {
            MR::showMaterial(pActor, "GrandStarEmpty");
            MR::hideMaterial(pActor, "FooMat");
        }
        else {
            MR::showMaterial(pActor, "FooMat");
            MR::hideMaterial(pActor, "GrandStarEmpty");
            MR::startBrk(pActor, "Recover");
            MR::setBrkFrameEndAndStop(pActor);
        }
    }

    void requestPointLight(const LiveActor *pActor, int a2) {
        TVec3f jointPos;

        MR::copyJointPos(pActor, "PowerStar", &jointPos);
        MR::requestPointLight(pActor, TVec3f(jointPos), lightColor[a2], 1.0f, -1);
    }
};

namespace NrvPowerStar {
    NEW_NERVE(PowerStarNrvWaitStartAppear, PowerStar, WaitStartAppear);
    NEW_NERVE(PowerStarNrvAppearDemoRise, PowerStar, AppearDemoRise);
    NEW_NERVE(PowerStarNrvAppearDemoMove, PowerStar, AppearDemoMove);
    NEW_NERVE(PowerStarNrvAppearDemoKoopa, PowerStar, AppearDemoKoopa);
    NEW_NERVE(PowerStarNrvWait, PowerStar, Wait);
    NEW_NERVE(PowerStarNrvWeak, PowerStar, Weak);
    NEW_NERVE(PowerStarNrvWeakNoRotate, PowerStar, WeakNoRotate);
    NEW_NERVE(PowerStarNrvWeakToWait, PowerStar, WeakToWait);
    NEW_NERVE(PowerStarNrvStageClearDemo, PowerStar, StageClearDemo);
};

PowerStar::PowerStar(const char *pName) :
    LiveActor(pName),
    mPowerStarId(-1),
    mIsInDemo(false),
    mAppearPosition(gZeroVec),
    _A0(gZeroVec),
    _AC(gZeroVec),
    mPowerStarModelObj(NULL),
    _11C(0),
    mColorFrame(0),
    mIsGrandStar(false),
    _125(false),
    _126(false),
    _127(false),
    mCameraInfo(NULL),
    mCameraActor(NULL),
    mLuigiNPC(NULL),
    _164(false)
{
    _B8.identity();
    _E8.identity();
    _134.identity();
}

void PowerStar::init(const JMapInfoIter &rIter) {
    mIsGrandStar = MR::isEqualObjectName(rIter, "GrandStar");

    initMapToolInfo(rIter);
    initModel();

    if (mIsGrandStar) {
        MR::connectToSceneIndirectMapObj(this);
        initSensorGrandStar();
    }
    else {
        MR::connectToSceneNoSilhouettedMapObj(this);
        initHitSensor(1);
        MR::addHitSensor(this, "body", 0x67, 8, 75.0f, TVec3f(0.0f, 0.0f, 0.0f));
    }

    initEffectKeeper(0, NULL, false);

    if (MR::isGalaxyGhostCometAppearInCurrentStage()) {
        MR::invalidateClipping(this);
    }
    else {
        MR::setClippingTypeSphere(this, 5000.0f);
    }

    initShadow(rIter);
    MR::initActorCamera(this, rIter, &mCameraInfo);
    ModelObj* power_star_mdl = mPowerStarModelObj;
    ActorCameraInfo* cam_info = mCameraInfo;
    MR::initAnimCamera(power_star_mdl, cam_info, mIsGrandStar ? "GrandStarGet" : "PowerStarGet");

    if (mIsGrandStar) {
        if (MR::isStageKoopaVs()) {
            bool isStageV3 = MR::isStageKoopaVs3();
            power_star_mdl = mPowerStarModelObj;
            cam_info = mCameraInfo;
            MR::initAnimCamera(power_star_mdl, cam_info, isStageV3 ? "DemoKoopaGrandStarVs3" : "DemoKoopaGrandStar");

            if (MR::isStageKoopaVs3()) {
                MR::initAnimCamera(mPowerStarModelObj, mCameraInfo, cStageClearAnimNameKoopaVs3);
            }
        }
    }

    if (MR::tryRegisterDemoCast(this, rIter)) {
        MR::registerDemoActionNerve(this, &NrvPowerStar::PowerStarNrvWeakNoRotate::sInstance, "ミニ太陽消失");
        MR::registerDemoActionNerve(this, &NrvPowerStar::PowerStarNrvWeakToWait::sInstance, "グランドスター復活");
        MR::registerDemoSimpleCastAll(this);
    }

    initSound(4, false);
    initNerve(&NrvPowerStar::PowerStarNrvWait::sInstance);

    if (mPowerStarId == -1) {
        if (mIsInDemo) {
            makeActorDead();
        }
        else {
            makeActorAppeared();
        }
    }
    else {
        makeActorAppeared();
    }
}

void PowerStar::initAfterPlacement() {

}

void PowerStar::appear() {
    LiveActor::appear();
    MR::invalidateClipping(this);
    requestAppearOrWait();
}

void PowerStar::makeActorAppeared() {
    _AC.set<f32>(mPosition);
    initPosture();
    LiveActor::makeActorAppeared();
}

void PowerStar::requestAppear() {
    makeActorAppeared();
    MR::invalidateClipping(this);
    MR::hideModel(this);
    MR::invalidateHitSensors(this);
    setNerve(&NrvPowerStar::PowerStarNrvWaitStartAppear::sInstance);

    if (MR::isStageKoopaVs()) {
        MR::requestStartDemoMarioPuppetableWithoutCinemaFrame(this, cAppearDemoName, &NrvPowerStar::PowerStarNrvAppearDemoKoopa::sInstance, NULL);
    }
    else {
        MR::requestStartDemoWithoutCinemaFrame(this, cAppearDemoName, &NrvPowerStar::PowerStarNrvAppearDemoRise::sInstance, NULL);
    }
}

void PowerStar::setDemoAppearPos(const TVec3f &rVec) {
    bool isGroupExist = MR::getGroupFromArray(this);

    if (isGroupExist) {
        mCameraActor = getNearestAppearPoint(rVec);
        mAppearPosition.set<f32>(mCameraActor->mPosition);
    }

    mPosition.set<f32>(rVec);
}

bool PowerStar::isEndAppearDemo() const {
    return isNerve(&NrvPowerStar::PowerStarNrvWait::sInstance) || isNerve(&NrvPowerStar::PowerStarNrvStageClearDemo::sInstance);
}

void PowerStar::offAppearDemo() {
    mIsInDemo = false;
}

void PowerStar::setupColor(LiveActor *pActor, const NameObj *a2, s32 a3) {
    bool hasPowerStar;
    s32 val = 0;
    hasPowerStar = MR::hasPowerStarInCurrentStageWithDeclarer(a2->mName, a3);

    if (MR::isPowerStarGreenInCurrentStageWithDeclarer(a2->mName, a3)) {
        val = 2;
    }
    else if (MR::isPowerStarRedInCurrentStageWithDeclarer(a2->mName, a3)) {
        val = 3;
    }

    if (MR::isGrandStarInCurrentStageWithDeclarer(a2->mName, a3)) {
        ::setupColorGrandStar(pActor, hasPowerStar);
    }
    else {
        ::setupColor(pActor, hasPowerStar, val);
    }
}

void PowerStar::setupColorAtResultSequence(LiveActor *pPowerStar, bool a2) {
    bool hasStarAtRes;
    s32 val = 0;
    hasStarAtRes = MR::hasPowerStarAtResultSequence();

    if (MR::isPowerStarGreenAtResultSequence()) {
        val = 2;
    }
    else if (MR::isPowerStarRedAtResultSequence()) {
        val = 3;
    }

    if (a2) {
        ::setupColorGrandStar(pPowerStar, hasStarAtRes);
    }
    else {
        ::setupColor(pPowerStar, hasStarAtRes, val);
    }
}

void PowerStar::requestPointLight(const LiveActor *pActor, const NameObj *a2, s32 a3) {
    s32 colorIdx = 0;

    if (MR::hasPowerStarInCurrentStageWithDeclarer(a2->mName, a3)) {
        colorIdx = 4;
    }
    else if (MR::isPowerStarGreenInCurrentStageWithDeclarer(a2->mName, a3)) {
        colorIdx = 2;
    }
    else if (MR::isPowerStarRedInCurrentStageWithDeclarer(a2->mName, a3)) {
        colorIdx = 3;
    }

    ::requestPointLight(pActor, colorIdx);
}

void PowerStar::requestPointLightAtResultSequence(const LiveActor *pActor) {
    s32 val = 0;

    if (MR::hasPowerStarAtResultSequence()) {
        val = 4;
    }
    else if (MR::isPowerStarGreenAtResultSequence()) {
        val = 2;
    }
    else if (MR::isPowerStarRedAtResultSequence()) {
        val = 3;
    }

    ::requestPointLight(pActor, val);
}

f32 PowerStar::getPowerStarWaitRotateSpeed() {
    return 3.0f;
}

s32 PowerStar::getBtpFrameCurrentStage(s32 a1) {
    if (MR::isPowerStarGreenInCurrentStage(a1)) {
        return 2;
    }

    if (MR::isPowerStarRedInCurrentStage(a1)) {
        return 3;
    }

    return 0;
}

bool PowerStar::isCurrentStageKoopaVs3() {
    return MR::isStageKoopaVs3();
}

void PowerStar::initShadowPowerStar(LiveActor *pActor, bool a2) {
    MR::initShadowFromCSV(pActor, "Shadow");
    MR::invalidateShadowAll(pActor);
    MR::validateShadowGroup(pActor, a2 ? "円柱" : "通常");
}

void PowerStar::makeArchiveList(NameObjArchiveListCollector *pCollector, const JMapInfoIter &rIter) {
    if (MR::isPowerStarGetDemoWithLuigiCurrentGalaxy()) {
        pCollector->addArchive("LuigiNPC");
    }
}

void PowerStar::control() {
    if (!isNerve(&NrvPowerStar::PowerStarNrvWaitStartAppear::sInstance)) {
        TVec3f jointPos;

        if (isNerve(&NrvPowerStar::PowerStarNrvStageClearDemo::sInstance)) {
            MR::copyJointPos(mPowerStarModelObj, "PowerStar", &jointPos);
        }
        else {
            MR::copyJointPos(this, "PowerStar", &jointPos);
        }

        bool cond = (isNerve(&NrvPowerStar::PowerStarNrvAppearDemoKoopa::sInstance) && MR::isStageKoopaVs3()) ? true : false;
        s32 val = cond ? 0x78 : -1;

        MR::requestPointLight(this, TVec3f(jointPos), lightColor[mColorFrame], 1.0f, val);
    }
}

#ifdef NON_MATCHING
void PowerStar::calcAndSetBaseMtx() {
    f32 derp = 0.017453292f * mRotation.y;
    TVec3f stack_14(0.0f, 1.0f, 0.0f);
    TPos3f pos;
    pos.zeroTrans();
    TVec3f stack_8;
    stack_8.set(stack_14);
    PSVECMag(stack_8);
    PSVECNormalize(stack_8, stack_8);
    f32 _sin = sin(derp);
    f32 _cos = cos(derp);
    pos.mMtx[0][0] = _cos + ((1.0f - _cos) * (stack_8.x * stack_8.x));
    pos.mMtx[1][1] = _cos + ((1.0f - _cos) * (stack_8.y * stack_8.y));
    pos.mMtx[2][2] = _cos + ((1.0f - _cos) * (stack_8.z * stack_8.z));
    pos.mMtx[0][1] = (stack_8.y * ((1.0f - _cos) * stack_8.x)) - (_sin * stack_8.z);
    pos.mMtx[0][2] = (stack_8.z * ((1.0f - _cos) * stack_8.x)) - (_sin * stack_8.y);
    pos.mMtx[1][0] = (stack_8.y * ((1.0f - _cos) * stack_8.x)) - (_sin * stack_8.z);
    pos.mMtx[2][0] = (stack_8.z * ((1.0f - _cos) * stack_8.x)) - (_sin * stack_8.y);
    pos.mMtx[1][2] = (stack_8.z * ((1.0f - _cos) * stack_8.y)) - (_sin * stack_8.x);
    pos.mMtx[2][1] = (stack_8.z * ((1.0f - _cos) * stack_8.y)) - (_sin * stack_8.x);
    TPos3f mtx;
    mtx.concat(_B8, pos);
    mtx.setTrans(mPosition);
    MR::setBaseTRMtx(this, mtx);
}
#endif

bool PowerStar::receiveMsgPlayerAttack(u32 msg,  HitSensor *a2, HitSensor *a3) {
    if (MR::isMsgStarPieceReflect(msg)) {
        return true;
    }

    return MR::isMsgJetTurtleAttack(msg);
}

bool PowerStar::receiveOtherMsg(u32 msg, HitSensor *pSender, HitSensor *pReceiver) {
    switch(msg) {
    case 0x98:
        return true;
    case 0x92:
        if (isNerve(&NrvPowerStar::PowerStarNrvWait::sInstance)) {
            MR::startSystemSE("SE_SY_STAR_GET", -1, -1);
            MR::stopSoundPlayer("SE_PV_BURN_RUN", 0);
            MR::stopSoundPlayer("SE_PV_NEEDLE_DAMAGE_RUN", 0);
            MR::makeMtxTR((MtxPtr)&_E8, *MR::getPlayerPos(), *MR::getPlayerRotate());
            setNerve(&NrvPowerStar::PowerStarNrvStageClearDemo::sInstance);
            return true;
        }
        break;
    case 0xA1:
        MR::setPlayerBaseMtx((MtxPtr)&_E8);
        return true;
    }

    return false;
}

void PowerStar::initMapToolInfo(const JMapInfoIter &rIter) {
    MR::initDefaultPos(this, rIter);
    MR::getJMapInfoArg0NoInit(rIter, &mPowerStarId);
    MR::getJMapInfoArg1NoInit(rIter, &_125);
    MR::getJMapInfoArg2NoInit(rIter, &_126);
    MR::getJMapInfoArg4NoInit(rIter, &_127);
    mAppearPosition.set<f32>(mPosition);
    _A0.set<f32>(mPosition);
    MR::registerPowerStar(this, mPowerStarId);
    MR::joinToGroupArray(this, rIter, "パワースター出現ポイントグループ", 0x10);

    if (mPowerStarId > 0) {
        if (!MR::hasPowerStarInCurrentStage(mPowerStarId)) {
            u32 frame;

            if (mIsGrandStar) {
                frame = 0;
            }
            else {
                frame = getBtpFrameCurrentStage(mPowerStarId);
            }

            mColorFrame = frame;
        }
        else {
            mColorFrame = 4;
        }
    }
}

void PowerStar::initModel() {
    initModelManagerWithAnm(isGrandStar() ? "GrandStar" : "PowerStar", 0, false);

    if (mIsGrandStar) {
        ::setupColorGrandStar(this, mColorFrame_v == 4);
    }
    else {
        ::setupColor(this, mColorFrame_v == 4, mColorFrame_v);
    }

    if (mIsGrandStar) {
        MtxPtr pMtx = (MtxPtr)&_E8;
        const char* pName = "グランドスターデモモデル";

        mPowerStarModelObj = MR::createModelObjIndirectMapObj(pName, mIsGrandStar ? "GrandStar" : "PowerStar", pMtx);

        ::setupColorGrandStar(mPowerStarModelObj, mColorFrame_v == 4);
    }
    else {
        MtxPtr pMtx = (MtxPtr)&_E8;
        const char* pName = "パワースターデモモデル";

        mPowerStarModelObj = MR::createModelObjNoSilhouettedMapObj(pName, mIsGrandStar ? "GrandStar" : "PowerStar", pMtx);

        ::setupColor(mPowerStarModelObj, mColorFrame_v == 4, mColorFrame_v);
    }

    MR::invalidateClipping(mPowerStarModelObj);
    mPowerStarModelObj->makeActorDead();

    if (MR::isPowerStarGetDemoWithLuigiCurrentGalaxy()) {
        mLuigiNPC = MR::createModelObjNpc("ルイージデモモデル", "LuigiNPC", (MtxPtr)&_E8);

        MR::initLightCtrl(mLuigiNPC);
        MR::invalidateClipping(mLuigiNPC);
        mLuigiNPC->makeActorDead();
    }

    MR::tryCreateMirrorActor(this, isGrandStar() ? "GrandStar" : "PowerStar");
}

void PowerStar::initSensorGrandStar() {
    initHitSensor(6);
    TVec3f body1Offs = TVec3f(0.0f, 100.0f, 0.0f);
    const char* body1Name = "body1";
    MR::addHitSensorMtx(this, body1Name, 0x67, 8, 60.0f, getBaseMtx(), body1Offs);
    TVec3f body2Offs = TVec3f(-95.0f, 35.0f, 0.0f);
    const char* body2Name = "body2";
    MR::addHitSensorMtx(this, body2Name, 0x67, 8, 60.0f, getBaseMtx(), body2Offs);
    TVec3f body3Offs = TVec3f(-55.0f, -75.0f, 0.0f);
    const char* body3Name = "body3";
    MR::addHitSensorMtx(this, body3Name, 0x67, 8, 60.0f, getBaseMtx(), body3Offs);
    TVec3f body4Offs = TVec3f(55.0f, -75.0f, 0.0f);
    const char* body4Name = "body4";
    MR::addHitSensorMtx(this, body4Name, 0x67, 8, 60.0f, getBaseMtx(), body4Offs);
    TVec3f body5Offs = TVec3f(95.0f, 35.0f, 0.0f);
    const char* body5Name = "body5";
    MR::addHitSensorMtx(this, body5Name, 0x67, 8, 60.0f, getBaseMtx(), body5Offs);
    TVec3f body6Offs = TVec3f(0.0f, 0.0f, 0.0f);
    const char* body6Name = "body6";
    MR::addHitSensorMtx(this, body6Name, 0x67, 8, 80.0f, getBaseMtx(), body6Offs);
}

void PowerStar::initShadow(const JMapInfoIter &rIter) {
    if (!mIsGrandStar) {
        bool arg3 = false;
        MR::getJMapInfoArg3NoInit(rIter, &arg3);
        bool thing = arg3;
        MR::initShadowFromCSV(this, "Shadow");
        MR::invalidateShadowAll(this);
        MR::validateShadowGroup(this, thing ? "円柱" : "通常");
    }
    else {
        MR::initShadowFromCSV(this, "Shadow");
    }
}

/*
void PowerStar::initPosture() {
    MR::calcGravity(this, mAppearPosition);
    TMtx34f rotate;
    MR::makeMtxRotate((MtxPtr)&rotate, _A0);

    if (!_126) {
        TVec3f stack_14;
        f32 z = rotate.mMtx[2][2];
        f32 y = rotate.mMtx[1][2];
        f32 x = rotate.mMtx[0][2];
        stack_14.set(x, y, z);

        TVec3f negGravity;
        negGravity.negateInline_2(mGravity);

        if (!MR::isSameDirection(negGravity, stack_14, 0.0099999998f)) {
            MR::makeMtxUpFront(&_B8, negGravity, stack_14);
        }
        else {
            MR::makeMtxUpNoSupport(&_B8, negGravity);
        }
    }
    else {
        JMath::gekko_ps_copy12(&_B8, &rotate);
    }

    _B8.zeroTrans();
    mRotation.y = 0.0f;
}
*/

void PowerStar::endAppearDemo() {
    if (!_11C) {
        MR::endDemo(this, cAppearDemoName);
    }

    MR::moveVolumeStageBGM(1.0f, 60);
    MR::moveVolumeSubBGM(1.0f, 60);
    setNerve(&NrvPowerStar::PowerStarNrvWait::sInstance);
}

PowerStarAppearPoint* PowerStar::getNearestAppearPoint(const TVec3f &rPos) const {
    LiveActorGroup* pGroup = MR::getGroupFromArray(this);
    f32 minDist = 3.4028235e38f;
    PowerStarAppearPoint* pNearestAppearPoint = NULL;

    for (int i = 0; i < pGroup->mObjectCount; i++) {
        LiveActor* pActor = pGroup->getActor(i);

        if (pActor != this) {
            f32 dist = PSVECDistance(&rPos, &pActor->mPosition);

            if (dist < minDist) {
                pNearestAppearPoint = static_cast<PowerStarAppearPoint*>(pActor);
                minDist = dist;
            }
        }
    }

    return pNearestAppearPoint;
}

LiveActor* PowerStar::getAppearCameraActor() {
    if (mCameraActor == NULL) {
        return this;
    }

    return mCameraActor;
}

ActorCameraInfo* PowerStar::getAppearCameraInfo() const {
    if (mCameraActor == NULL) {
        return mCameraInfo;
    }

    return mCameraActor->mCameraInfo;
}

void PowerStar::requestAppearOrWait() {
    if (MR::isDemoCast(this, 0)) {
        setNerve(&NrvPowerStar::PowerStarNrvWeak::sInstance);
    }
    else if (mIsInDemo) {
        if (_11C) {
            MR::invalidateClipping(this);
            MR::requestMovementOn(this);

            if (MR::isStageKoopaVs()) {
                setNerve(&NrvPowerStar::PowerStarNrvAppearDemoKoopa::sInstance);
            }
            else {
                setNerve(&NrvPowerStar::PowerStarNrvAppearDemoRise::sInstance);
            }
        }
        else {
            requestAppear();
        }
    }
    else {
        setNerve(&NrvPowerStar::PowerStarNrvWait::sInstance);
    }
}

void PowerStar::calcAppearDemoRiseTrans(TVec3f *pOutTrans, f32 a2) const {
    if (_125) {
        MR::calcGravityVector(this, _AC, pOutTrans, 0, 0);
        pOutTrans->x *= -a2;
        pOutTrans->y = (pOutTrans->y * -a2);
        pOutTrans->z = (pOutTrans->z * -a2);
    }
    else {
        TVec3f vec;
        vec.y = a2;
        vec.x = 0.0f;
        vec.z = 0.0f;
        _B8.mult(vec, *pOutTrans);
    }

    pOutTrans->add(_AC);
}

#ifdef NON_MATCHING
void PowerStar::processWait(f32 val) {
    f32 derp = (mRotation.y + val) - 0.0;
    f32 rot_y = fmod((360.0f + derp), 360.0f);
    mRotation.y = rot_y + 0.0;

    if (mIsGrandStar) {
        if (MR::changeShowModelFlagSyncNearClipping(this, 250.0f)) {
            MR::onDrawEffect(this);
        }
        else {
            MR::offDrawEffect(this);
        }
    }

    if (MR::isGalaxyGhostCometAppearInCurrentStage()) {
        if (MR::isJudgedToClipFrustum100m(mPosition, 5000.0f)) {
            MR::hideModelAndOnCalcAnimIfShown(this);
            MR::forceDeleteEffect(this, "Light");
        }
        else {
            MR::showModelIfHidden(this);
            MR::emitEffect(this, "Light");
        }
    }

    if (!_164) {
        if (MR::isInWater(this, TVec3f(0.0f, 0.0f, 0.0f))) {
            MR::startLevelSound(this, "SE_OJ_LV_POW_STAR_EXIST_W", -1, -1, -1);
        }
        else {
            MR::startLevelSound(this, "SE_OJ_LV_POW_STAR_EXIST", -1, -1, -1);
        }
    }
}
#endif

void PowerStar::exeWaitStartAppear() {
    
}

#ifdef NON_MATCHING
void PowerStar::exeAppearDemoRise() {
    if (MR::isFirstStep(this)) {
        MR::showModelIfHidden(this);
        MR::moveVolumeStageBGM(0.0f, 5);
        MR::moveVolumeSubBGM(0.0f, 5);

        CameraTargetArg target_arg(this);
        MR::startActorCameraTargetOther(getAppearCameraActor(), getAppearCameraInfo(), target_arg, 0);
        mPosition.set(_AC);
        MR::emitEffect(this, "Light");
    }

    TVec3f stack_8(0.0f, 0.0f, 0.0f);
    if (MR::isInWater(this, stack_8)) {
        MR::startLevelSound(this, "SE_OJ_LV_POW_STAR_EXIST_W", -1, -1, -1);
    }
    else {
        MR::startLevelSound(this, "SE_OJ_LV_POW_STAR_EXIST", -1, -1, -1);
    }

    f32 easeOut = MR::calcNerveEaseOutValue(this, 0x3C, 0.0f, 300.0f);
    calcAppearDemoRiseTrans(&mPosition, easeOut);
    mRotation.y = 0.0f + fmod((360.0f + ((10.0f + mRotation.y) - 0.0f)), 360.0f);

    if (MR::isStep(this, 0x50)) {
        setNerve(&NrvPowerStar::PowerStarNrvAppearDemoMove::sInstance);
    }
}
#endif

// PowerStar::exeAppearDemoMove()
// PowerStar::exeAppearDemoKoopa()

void PowerStar::exeWait() {
    if (MR::isFirstStep(this)) {
        if (MR::isGalaxyGhostCometAppearInCurrentStage()) {
            MR::validateClipping(this);
        }

        MR::validateHitSensors(this);
        MR::emitEffect(this, "Light");
    }

    processWait(mIsGrandStar ? 2.0f : 3.0f);
}

void PowerStar::exeWeak() {
    if (MR::isFirstStep(this)) {
        MR::invalidateHitSensors(this);
        MR::validateClipping(this);

        if (mColorFrame != 4) {
            MR::startBrkAndSetFrameAndStop(this, "Recover", 0.0f);
        }
    }

    _164 = true;

    processWait(0.3f);
}

void PowerStar::exeWeakNoRotate() {
    if (MR::isFirstStep(this)) {
        mRotation.y = 45.0f;
    }

    _164 = true;

    processWait(0.0f);
}

void PowerStar::exeWeakToWait() {
    if (MR::isFirstStep(this)) {
        if (mColorFrame != 4) {
            MR::setBrkRate(this, 1.0f);
        }
    }

    _164 = false;

    processWait(MR::calcNerveEaseInValue(this, 30, 0.3f, mIsGrandStar ? 2.0f : 3.0f));
    MR::setNerveAtStep(this, &NrvPowerStar::PowerStarNrvWait::sInstance, 30);
}

// PowerStar::exeStageClearDemo

PowerStar::~PowerStar() {
    
}
