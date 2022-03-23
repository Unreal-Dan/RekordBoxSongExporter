#include <inttypes.h>
#include "DjEngineInterface.h";
#include "LastTrackStorage.h"
#include "RowDataTrack.h"
#include "UIPlayer.h"
#include "SigScan.h"
#include "Config.h"
#include "Hook.h"
#include "Log.h"

// sig for djengine::djengineIF::getinstance()
#define DJENG_GET_INST_SIG "40 53 48 83 EC 30 48 C7 44 24 20 FE FF FF FF 48 8B 05 90 90 90 90 48 85 C0 75 6B 48 8D 1D"

// some type used in DjPlayerUnit functions
typedef bool EnableOrNot;

// some classes to satisfy pointers in the DjPlayerUnit vtable
class ISyncMaster { };
class ISyncSlave { };
class IKeySyncMaster { };
class IKeySyncSlave { };
class IAudioProcessUnit { };
class IControlClient { };

// The Fractuin<uint16_t> is commonly used in args for stuff like 1/4 beat loop
template <typename t>
struct Fraction {
    t bot;
    t top;
};

// Rebuilt class and vtable of DjMixerUnit, commented out parts are argument types that haven't been reversed yet
class DjMixerUnit
{
public:
    virtual ~DjMixerUnit() = 0;
    virtual uintptr_t enableTrim(bool) = 0;
    virtual uintptr_t setTrimPosition(uint32_t deck, float trim) = 0;
    virtual uintptr_t adjustGain(float) = 0;
    virtual uintptr_t setSamplerGain(float) = 0;
    virtual uintptr_t setEqPosition() = 0; //EMixerChannelLine_t, EEq_Iso_Band_t, float) = 0;
    virtual uintptr_t selectEqIso() = 0; //EEq_Type_t) = 0;
    virtual uintptr_t setChannelFx() = 0; //AbstractFxUnit *, EMixerChannelLine_t, int) = 0;
    virtual uintptr_t setMasterFx() = 0; //AbstractFxUnit *, int) = 0;
    virtual uintptr_t setAuxFx() = 0; //AbstractFxUnit *, int) = 0;
    virtual uintptr_t setMergeSamplerFx() = 0; //AbstractFxUnit *, int) = 0;
    virtual uintptr_t setChannelFaderPosition(uint32_t, float) = 0;
    virtual uintptr_t selectChannelFaderCurve() = 0; //EChannelFaderCurve_t) = 0;
    virtual uintptr_t setCrossFaderPosition(float) = 0;
    virtual uintptr_t selectCrossFaderCurve() = 0; //ECrossFaderCurve_t, int) = 0;
    virtual uintptr_t selectCrossFaderAssign() = 0; //EMixerChannelLine_t, ECrossFaderAssign) = 0;
    virtual uintptr_t setChannelCue(int, bool) = 0;
    virtual uintptr_t setMasterCue(bool) = 0;
    virtual uintptr_t setSamplerCue(bool) = 0;
    virtual uintptr_t setHeadPhoneMixingBalance(float) = 0;
    virtual uintptr_t setHeadPhoneVolume(float) = 0;
    virtual uintptr_t selectHeadPhoneCueMode() = 0; //EHeadPhoneCueMode_t) = 0;
    virtual uintptr_t setMasterVolume(float) = 0;
    virtual uintptr_t setOutputMode(bool) = 0;
    virtual uintptr_t enableMasterVolume(bool) = 0;
    virtual uintptr_t enableMasterLevelMeter(bool) = 0;
    virtual uintptr_t setMicOnOff(bool) = 0;
    virtual uintptr_t setMicLevelKnob(int, float) = 0;
    virtual uintptr_t setMicEqPosition(int, float) = 0;
    virtual uintptr_t setMicFx() = 0; //AbstractFxUnit *, int) = 0;
    virtual uintptr_t setMicFeedbackEliminatorOnOff(bool) = 0;
    virtual uintptr_t setMicFeedbackEliminatorType() = 0; //EMicFeedbackEliminatorType_t) = 0;
    virtual uintptr_t connectMicToBoothOut(bool) = 0;
    virtual uintptr_t connectMicToRecOut(bool) = 0;
    virtual uintptr_t enableBoothOutStereo(bool) = 0;
    virtual uintptr_t enableMasterOutStereo(bool) = 0;
    virtual uintptr_t setRecOutDelayTimeMs(float) = 0;
    virtual uintptr_t enableMicLoCutFilter(bool) = 0;
    virtual uintptr_t enableTalkover(bool) = 0;
    virtual uintptr_t enableAdvTalkover(bool) = 0;
    virtual uintptr_t setTalkOverLevel(int) = 0;
    virtual uintptr_t setNumMics(int) = 0;
    virtual uintptr_t enableMasterLimitter(bool) = 0;
    virtual uintptr_t enableBoothLimitter(bool) = 0;
    virtual uintptr_t enableMicLimitterForMaster(bool) = 0;
    virtual uintptr_t enableMicLimitterForBooth(bool) = 0;
    virtual uintptr_t setMute(int, bool) = 0;
    virtual uintptr_t setOutputMute(bool) = 0;
    virtual uintptr_t switchSamplerRouting(bool) = 0;
    virtual uintptr_t setMergeFxSamplerVolumePosition(float) = 0;
    virtual uintptr_t adjustMergeFxSampleSoundGain(float) = 0;
    virtual uintptr_t adjustMergeFxAutoGain(float) = 0;
};

// Rebuilt class and vtable of DjMixerUnit, commented out parts are argument types that haven't been reversed yet
class DjPlayerUnit 
{
public:
    // 0
    virtual ~DjPlayerUnit() = 0;
    virtual uintptr_t setRenderableProcessor(bool) = 0;
    virtual uintptr_t load(IControlClient *) = 0; //, DjPlayerAudioSourceInfo *, DjPlayerLoadInfo *) = 0;
    virtual uintptr_t unload(IControlClient *) = 0;
    virtual uintptr_t instantDouble(IControlClient *) = 0; //, IDjPlayerControl const *) = 0;
    virtual uintptr_t sequenceLoad(IControlClient *) = 0; //, DjPlayerAudioSourceInfo *, DjPlayerLoadInfo *, ISyncMaster const *) = 0;
    virtual uintptr_t getGridEditListener(void) = 0;
    virtual uintptr_t setAutoCueLevel(int) = 0;
    virtual uintptr_t enableAutoCue(IControlClient *, EnableOrNot) = 0;
    virtual uintptr_t enableQuantize(EnableOrNot) = 0;
    // 10
    virtual uintptr_t setQuantizeMode(bool, bool) = 0;
    virtual uintptr_t setQuantizeEnabled() = 0; //DjPlayerDef::QuantizeType, bool, bool) = 0;
    virtual uintptr_t setQuantizeUnit() = 0; //DjPlayerDef::QuantizeType, Fraction<uint16_t>, bool) = 0;
    virtual uintptr_t setQuantizeUnit2() = 0; //Fraction<uint16_t>, bool) = 0;
    virtual uintptr_t enableSlip(EnableOrNot) = 0;
    virtual uintptr_t enableMasterTempo(EnableOrNot) = 0;
    virtual uintptr_t enableVinyl(EnableOrNot) = 0;
    virtual uintptr_t enableHotCueSampler(EnableOrNot) = 0;
    virtual uintptr_t enableAutoBeatLoopStartMode(EnableOrNot) = 0;
    virtual uintptr_t enableActiveLoop(IControlClient *, EnableOrNot) = 0;
    // 20
    virtual uintptr_t adjustVinylSpeed(float, float) = 0;
    virtual uintptr_t adjustVinylSpeedByDefaultTable(int, int) = 0;
    virtual uintptr_t setAsSyncSlave(EnableOrNot) = 0;
    virtual uintptr_t setBeatSyncPolicy() = 0; //DjPlayerDef::BeatSyncPolicy) = 0;
    virtual uintptr_t enableMultiBpmSync(EnableOrNot) = 0;
    virtual uintptr_t syncPlayPositionMsAt(IControlClient *, int) = 0; //, IDjPlayerControl const *, int) = 0;
    virtual operator ISyncMaster *(void) = 0;
    virtual operator ISyncSlave *(void) = 0;
    virtual uintptr_t play(IControlClient *) = 0;
    virtual uintptr_t pause(IControlClient *) = 0;
    // 30
    virtual uintptr_t scratch(IControlClient *, bool, float, bool, bool) = 0;
    virtual uintptr_t stopScratch(IControlClient *, bool, bool) = 0;
    virtual uintptr_t setBendDiff(IControlClient *, float) = 0;
    virtual uintptr_t setBendSlope(IControlClient *, float, bool) = 0;
    virtual uintptr_t setBaseTempoRatio(IControlClient *, float) = 0;
    virtual uintptr_t changePlayDirection(IControlClient *, bool direction, bool keepplaying) = 0;
    virtual uintptr_t movePlayPositionMs(IControlClient *, int, bool, bool) = 0;
    virtual uintptr_t movePlayPositionRatio(IControlClient *, float, bool) = 0;
    virtual uintptr_t shiftPlayKeySemitone(IControlClient *, int) = 0;
    virtual uintptr_t resetPlayKey(IControlClient *) = 0;
    // 40
    virtual uintptr_t setOrientedKeyCode(IControlClient *) = 0; //, IKeyInfo::KeyCode) = 0;
    virtual operator IKeySyncMaster *(void) = 0;
    virtual operator IKeySyncSlave *(void) = 0;
    virtual uintptr_t startScan(IControlClient *, bool) = 0; //, ScanParam const *) = 0;
    virtual uintptr_t stopScan(IControlClient *) = 0;
    virtual uintptr_t beatSearch(IControlClient *, bool) = 0; //, Fraction<uint16_t>, bool, bool) = 0;
    virtual uintptr_t msSearch(IControlClient *, bool) = 0;
    virtual uintptr_t backToCurrentCue(IControlClient *) = 0;
    virtual uintptr_t startCueSampler(IControlClient *, bool) = 0;
    virtual uintptr_t setCurrentCue(IControlClient *, bool) = 0;
    // 50
    virtual uintptr_t setLoopOutPoint(IControlClient *, bool, bool, bool) = 0;
    virtual uintptr_t setPointsAsCurrentLoop(IControlClient *, int, int, bool, Fraction<uint16_t>) = 0;
    virtual uintptr_t enableCurrentLoop(IControlClient *) = 0;
    virtual uintptr_t exitLoop(IControlClient *, bool) = 0;
    virtual uintptr_t loopCut(IControlClient *, bool) = 0;
    virtual uintptr_t loopDouble(IControlClient *, bool) = 0;
    virtual uintptr_t loopMove(IControlClient *, int, int) = 0;
    virtual uintptr_t registValidAutoLoopBeats(IControlClient *) = 0; //, juce::Array<Fraction<uint16_t>, juce::DummyCriticalSection, 0> const &) = 0;
    virtual uintptr_t autoBeatLoop(IControlClient *, EnableOrNot) = 0; //, Fraction<uint16_t>, bool, bool) = 0;
    virtual uintptr_t instantSlipLoop(IControlClient *, Fraction<uint16_t> frac, bool) = 0;
    // 60
    virtual uintptr_t exitInstantSlipLoop(IControlClient *, bool) = 0;
    virtual uintptr_t jumpToLoopInPoint(IControlClient *, bool) = 0;
    virtual uintptr_t setPointsAsActiveLoop(IControlClient *) = 0; //, CuePointInfoList const &) = 0;
    virtual uintptr_t getQuantizedPosMs(double) = 0;
    virtual uintptr_t adjustLoopPoint(IControlClient *, bool, int) = 0;
    virtual uintptr_t adjustLoopLength(IControlClient *, double) = 0;
    virtual uintptr_t launchHotCue(IControlClient *, int, bool, int, bool) = 0;
    virtual uintptr_t stopHotCue(IControlClient *, int) = 0;
    virtual uintptr_t recHotCue(IControlClient *, int) = 0;
    virtual uintptr_t recPointsAsHotCue(IControlClient *, int, int, int) = 0; //, Fraction<uint16_t>) = 0;
    // 70
    virtual uintptr_t deleteHotCue(IControlClient *, int) = 0;
    virtual uintptr_t getSlicer(void) = 0;
    virtual uintptr_t getDVS(void) = 0;
    virtual uintptr_t getActiveCensor(void) = 0;
    virtual uintptr_t mutePlayer(bool, IControlClient *) = 0;
    virtual uintptr_t mutePlayerIfNoOneControls(bool, IControlClient *) = 0;
    virtual uintptr_t getAutoScratchControl(void) = 0;
    virtual uintptr_t getAutoMuteControl(void) = 0;
    virtual uintptr_t refDjPlayerState(void) = 0;
    virtual uintptr_t addDjPlayerStatListener() = 0; //IDjPlayerStatListener *) = 0;
    // 80
    virtual uintptr_t removeDjPlayerStatListener() = 0; //IDjPlayerStatListener *) = 0;
    virtual uintptr_t setPlayHistoryRegistry() = 0; //IPlayHistoryRegistry *) = 0;
    virtual operator IAudioProcessUnit *(void) = 0;
    virtual uintptr_t setAudioAnalysisManager() = 0; //IAudioAnalysisManager *) = 0;
    virtual uintptr_t setPreparedDataManager() = 0; //IPreparedDataManager *) = 0;
    virtual uintptr_t getPreparedDataListener(void) = 0;
    virtual uintptr_t getAutoGainControl(void) = 0;
    virtual uintptr_t getControllerPerformance(void) = 0;
    virtual uintptr_t setSyncManagerToModule() = 0; //SyncManager *) = 0;
    // =======================================
    // DjPlayerStreamingEventSender
    virtual uintptr_t onLodedTrackChanged(IControlClient *) = 0; //, IDjPlayerState const &) = 0;
    // 90
    virtual uintptr_t onUnloadedTrackChanged(IControlClient *) = 0; //, IDjPlayerState const &) = 0;
    virtual uintptr_t onLoadedInstantDouble(IControlClient *) = 0; //, IDjPlayerState const &) = 0;
    virtual uintptr_t onLoadedSequence(IControlClient *) = 0; //, IDjPlayerState const &) = 0;
    virtual uintptr_t onBeatGridChanged(IControlClient *) = 0; //, IDjPlayerState const &) = 0;
    virtual uintptr_t onAnalysisDataUpdated(IControlClient *) = 0; //, IDjPlayerState const &) = 0;
    virtual uintptr_t onPlayModeChanged(IControlClient *) = 0; //, IDjPlayerState const &) = 0;
    virtual uintptr_t onScanModeChanged(IControlClient *) = 0; //, IDjPlayerState const &) = 0;
    virtual uintptr_t onSyncStateChanged(IControlClient *) = 0; //, IDjPlayerState const &) = 0;
    virtual uintptr_t onBeatSyncPolicyChanged(IControlClient *) = 0; //, IDjPlayerState const &) = 0;
    virtual uintptr_t onKeySyncStateChanged(IControlClient *) = 0; //, IDjPlayerState const &) = 0;
    // 100
    virtual uintptr_t onPlayDirectionChanged(IControlClient *) = 0; //, IDjPlayerState const &) = 0;
    virtual uintptr_t onPlayPositionEdgeReached(IControlClient *) = 0; //, IDjPlayerState const &) = 0;
    virtual uintptr_t onPlayPositionJumped(IControlClient *) = 0; //, IDjPlayerState const &) = 0;
    virtual uintptr_t onPlayKeyChanged(IControlClient *) = 0; //, IDjPlayerState const &) = 0;
    virtual uintptr_t onOrientedKeyCodeChanged(IControlClient *) = 0; //, IDjPlayerState const &) = 0;
    virtual uintptr_t onDiffSemitoneOnTempoRatioChanged(IControlClient *) = 0; //, IDjPlayerState const &) = 0;
    virtual uintptr_t onMasterTempoEnabled(IControlClient *) = 0; //, IDjPlayerState const &) = 0;
    virtual uintptr_t onAutoCueEnabled(IControlClient *) = 0; //, IDjPlayerState const &) = 0;
    virtual uintptr_t onQuantizeSettingChanged(IControlClient *) = 0; //, IDjPlayerState const &) = 0;
    virtual uintptr_t onVinylEnabled(IControlClient *) = 0; //, IDjPlayerState const &) = 0;
    // 110
    virtual uintptr_t onLoopStateChanged(IControlClient *) = 0; //, IDjPlayerState const &) = 0;
    virtual uintptr_t onCurrentCueChanged(IControlClient *) = 0; //, IDjPlayerState const &) = 0;
    virtual uintptr_t onHotCueChanged(IControlClient *) = 0; //, IDjPlayerState const &, int) = 0;
    virtual uintptr_t onHotCueLaunched(IControlClient *) = 0; //, IDjPlayerState const &, int, int) = 0;
    virtual uintptr_t onActiveLoopChanged(IControlClient *) = 0; //, IDjPlayerState const &) = 0;
    virtual uintptr_t onMemoryCueChanged(IControlClient *) = 0; //, IDjPlayerState const &) = 0;
    virtual uintptr_t onMemoryCueCalled(IControlClient *) = 0; //, IDjPlayerState const &) = 0;
    virtual uintptr_t onSlipModeChanged(IControlClient *) = 0; //, IDjPlayerState const &) = 0;
    virtual uintptr_t onSlipBgStatChanged(IControlClient *) = 0; //, IDjPlayerState const &) = 0;
    virtual uintptr_t timerCallback(void) = 0;
};

// DjUnitAudioGraph has the 4 DjPlayerUnits
class DjUnitAudioGraph 
{
public:
    uint32_t getNumPlayer() {
        return *(uint32_t *)((uintptr_t)this + 0x10);
    }
    DjPlayerUnit *getPlayerControl(uint32_t deck) {
        if (deck > 3) {
            return NULL;
        }
        DjPlayerUnit **playerControllers = *(DjPlayerUnit ***)this;
        if (!playerControllers) {
            return NULL;
        }
        return playerControllers[deck];
    }
};

// The main DjSystem class that contains the MixerUnit and the DjUnitAudioGraph
class RekordBoxDjSystem
{
public:
    DjUnitAudioGraph *getDjUnitAudioGraph() {
        // 0x3F0 to DjUnitAudioGraph (no dereference)
        return (DjUnitAudioGraph *)((uintptr_t)this + 0x3F0);
    }
    DjMixerUnit *getMixer() {
        // 0x408 to something idk (no dereference)
        // then inside is a pointer to an array, we want index 0
        // then the mixer unit offset e0 from that... idk
        return (DjMixerUnit *)((**(uintptr_t **)((uintptr_t)this + 0x408)) + 0xE0);
    }
};

// DjEngineInterface is the main interface, it has control clients and the RekordboxDjSystem
// The control clients need to be passed to some functions like ShiftKey
// and the RekordboxDjSystem has other components inside it
class DjEngineIF {
public:
    IControlClient *getControlClient(uint32_t deck) {
        if (deck > 3) {
            return NULL;
        }
        return ((IControlClient **)((uintptr_t)this + 0xF8))[deck];
    }
    RekordBoxDjSystem *getDjSystem() {
        // *0xF0 to RekordboxDjSystem 
        return *(RekordBoxDjSystem **)((uintptr_t)this + 0xF0);
    }
};

class RekordboxMultiFxUnit
{
public:
    virtual ~RekordboxMultiFxUnit() = 0; // 0
    virtual uintptr_t AbstractFxUnit_addListener(void * /* IDjEffectStateListener * */) = 0;
    virtual uintptr_t AbstractFxUnit_removeListener(void * /* IDjEffectStateListener * */) = 0;
    virtual uintptr_t IDjEffectControl_getNumFxChannels(void) = 0;
    virtual uintptr_t IDjEffectControl_enableFxToChannel(int) = 0;
    virtual uintptr_t IDjEffectControl_disableFxToChannel(int) = 0;
    virtual uintptr_t AbstractFxUnit_getNumOfLayer(int) = 0;
    virtual uintptr_t AbstractFxUnit_getNumOfEffect(int, int) = 0;
    virtual uintptr_t AbstractFxUnit_getEffectName(int, int, int) = 0;
    virtual uintptr_t AbstractFxUnit_selectEffect(int, int, int) = 0;
    virtual uintptr_t AbstractFxUnit_getCurrentFxId(int, int) = 0;  // 10
    virtual uintptr_t AbstractFxUnit_startEffect(int, int) = 0;
    virtual uintptr_t AbstractFxUnit_stopEffect(int, int) = 0;
    virtual uintptr_t AbstractFxUnit_isEffectOn(int, int) = 0;
    virtual uintptr_t AbstractFxUnit_getNumOfParameter(int, int) = 0;
    virtual uintptr_t AbstractFxUnit_setParameter(float, int, int, int) = 0;
    virtual uintptr_t AbstractFxUnit_getParameter(int, int, int) = 0;
    virtual uintptr_t AbstractFxUnit_getParameterRange(int, int, int) = 0;
    virtual uintptr_t AbstractFxUnit_getParameterNumSteps(int, int, int) = 0;
    virtual uintptr_t AbstractFxUnit_getParameterDefaultValue(int, int, int) = 0;
    virtual uintptr_t AbstractFxUnit_getParameterName(int, int, int) = 0; // 20
    virtual uintptr_t AbstractFxUnit_getParameterText(int, int, int) = 0;
    virtual uintptr_t AbstractFxUnit_setBeatPerMitunes(int, int) = 0;
    virtual uintptr_t AbstractFxUnit_setEffectTimeMs(int, int, int) = 0;
    virtual uintptr_t AbstractFxUnit_getEffectTimeMs(int, int) = 0;
    virtual uintptr_t AbstractFxUnit_setEffectBeat(int /*EffectBeat*/, int, int) = 0;
    virtual uintptr_t AbstractFxUnit_getEffectBeat(int, int) = 0;
    virtual uintptr_t IDjEffectControl_setEffectMixRatio(int, float, int) = 0;
    virtual uintptr_t IDjEffectControl_setEffectPercent(int, float, int) = 0;
    virtual uintptr_t AbstractFxUnit_beforeEffectProcess(int /*EffectPosition_id*/, int /*AudioBuffer*/ &) = 0;
    virtual uintptr_t AbstractFxUnit_processBlock(int /*EffectPosition_id*/, int /*AudioBuffer*/ &) = 0; // 30
    virtual uintptr_t AbstractFxUnit_afterEffectProcess(int /*EffectPosition_id*/, int /*AudioBuffer*/ &) = 0;
    virtual uintptr_t AbstractFxUnit_onSamplingRateChanged(void) = 0;
    virtual uintptr_t AbstractFxUnit_onBlockSizeChanged(void) = 0;
    virtual uintptr_t AbstractFxUnit_onFxChanged(void /*EffectLayerBase*/ const *) = 0;
    virtual uintptr_t AbstractFxUnit_onFxStateChanged(void /*EffectLayerBase*/ const *) = 0;
    virtual uintptr_t AbstractFxUnit_addMultiChannelFxStatListener(void /*IDjMultiChannelFxStateListener*/ *) = 0;
    virtual uintptr_t AbstractFxUnit_removeMultiChannelFxStatListener(void /*IDjMultiChannelFxStateListener*/ *) = 0;
    //virtual uintptr_t AbstractFxUnit_deleteEffect(int) = 0;
    //virtual uintptr_t AbstractFxUnit_deleteEffect(int, int) = 0;
    virtual uintptr_t AbstractFxUnit_clearEffectBuffer(int, int) = 0; // 40
    virtual uintptr_t RekordboxMultiFxUnit_setEffectPercent(int, float) = 0;
};

// typedef of the djengineIF::getInstance
typedef DjEngineIF *(*get_instance_fn_t)(void);
static get_instance_fn_t get_instance = NULL;

// the global djengine singleton instance
static DjEngineIF *djengine = NULL;

// The list of control clients inside the DjEngine
static IControlClient *control_clients[4] = {0};

// The djsystem inside the djengine
static RekordBoxDjSystem *djsystem = NULL;

// the audiograph and djmixerunit inside the djsystem
static DjUnitAudioGraph *audio_graph = NULL;
static DjMixerUnit *mixer = NULL;

// The list of players inside the DjUnitAudioGraph
static DjPlayerUnit *players[4] = {0};

static void *wait_keypress(void *arg);

uintptr_t __fastcall sliploop_callback(hook_arg_t hook_arg, func_args *args)
{
    info("sliploop%u(%p, %p, %p, %p)", (uint32_t)hook_arg, args->arg1, args->arg2, args->arg3, args->arg4);
    return 0;
}

extern uintptr_t cfxunit;

bool init_djengine_interface()
{
    // First lookup the DjEngineIF::GetInstance function
    get_instance = (get_instance_fn_t)sig_scan(DJENG_GET_INST_SIG);
    if (!get_instance) {
        error("Failed to locate djengineIF::getInstance");
        return false;
    }
    success("Found DjEngineIF::GetInstance: %p", get_instance);
    // then call it and get an instance of the djengineIF
    djengine = get_instance();
    if (!djengine) {
        error("Failed to get pointer to djeingine");
        return false;
    }
    success("DjEngine: %p", djengine);
    // grab the djsystem pointer from the dj engine
    djsystem = djengine->getDjSystem();
    if (!djsystem) {
        error("Failed to get pointer to djsystem");
        return false;
    }
    success("DjSystem: %p", djsystem);
    // now fetch the DjUnitAudioGraph object from inside the djsystem
    audio_graph = djsystem->getDjUnitAudioGraph();
    if (!audio_graph) {
        error("Failed to get audio graph");
        return false;
    }
    success("AudioGraph: %p", audio_graph);
    // as well as the mixer object
    mixer = djsystem->getMixer();
    if (!mixer) {
        error("Failed to get mixer");
        return false;
    }
    success("DjMixer: %p", mixer);
    // Then fetch the four DjPlayerUnits from the audio graph, and the four control clients from the DjEngine
    for (int i = 0; i < 4; ++i) {
        players[i] = audio_graph->getPlayerControl(i);
        if (!players[i]) {
            error("Failed to get player unit for deck %u", i);
            return false;
        }
        control_clients[i] = djengine->getControlClient(i);
        if (!control_clients[i]) {
            error("Failed to get control client for deck %u", i);
            return false;
        }
        success("Player %u: %p Control client: %p", i, players[i], control_clients[i]);
    }
    //CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)wait_keypress, NULL, 0, NULL);
    info("Successfully initialized djengine interface");

    // [*] RekordboxDjMultiFxLayer::creatEffectBase(000002978DD9D580, 0000000000000007, 0000000000000000, 00007FF6483618D0)
    // [*] MultiBeatFxFlanger::MultiBeatFxFlanger(00000297B1F785C0, 0000000000000000, 000000000000028B, 0000000000000012)

    //hook_sig("RekordboxDjMultiFxLayer::creatEffectBase", 
    //    "48 8B C4 55 41 54 41 55 41 56 41 57 48 8B EC 48 83 EC 40 48 C7 45 E0 FE FF FF FF 48 89 58 10 48 89 70 18 48 89 78 20 0F 29 70 C8");

    //hook_sig("MultiBeatFxFlanger::MultiBeatFxFlanger",
    //    "48 89 4C 24 08 57 48 83 EC 40 48 C7 44 24 30 FE FF FF FF 48 89 5C 24 58 48 8B F9 E8 ?? ?? ?? ?? 90 48 8D 05 50 08 FA 01 48 89 07");

    /*
    [*] MultiBeatFxFlanger::MultiBeatFxFlanger(0000023F2E6B5780, 0000000000000000, 000000000000028B, 0000000000000012)
    [*] MultiBeatFxFlanger::MultiBeatFxFlanger(0000023F2E6B4650, 0000000000000000, 000000000000028B, 0000000000000012)
    [*] MultiBeatFxFlanger::MultiBeatFxFlanger(0000023F2E6B4650, 0000000000000000, 000000000000028B, 0000000000000012)
    [*] MultiBeatFxFlanger::MultiBeatFxFlanger(0000023F2E6B4650, 0000000000000000, 000000000000028B, 0000000000000012)
    */
#if 0
    hook_sig("EfxProc", "48 89 5C 24 20 55 41 56 41 57 48 83 EC 60 0F 29 74 24 50");
    hook_sig("ProcessFXCore", "40 53 57 41 55 41 57 48 83 EC 48 48 8B 59 18 4C 8B E9 48 89 6C 24 78 48 89 B4 24 80 00 00 00");
    hook_sig("DjColorFxUnit", "48 89 4C 24 08 53 48 83 EC 30 48 C7 44 24 20 FE FF FF FF 48 8B D9 C7 44 24 58 00 00 00 00 45 85 C9 74 1D 48 8D");
#endif
    //hook_sig("AbstractFxUnit::StartEffect", "4C 8B 41 48 48 8B 41 50 49 2B C0 48 C1 F8 03 3B D0 7C 06 B8 03 00 00 00 C3 48 63 C2 49 8B 0C C0 33 C0 C6 41 38 01 C3");
    //hook_sig("EffectBase::turnOn", "40 53 48 83 EC 20 48 8B 01 48 8B D9 FF 90 E8 00 00 00 48 8B 03");

    // abstract fx unit signature
    //hook_sig("AbstractFxUnit", "\x48\x8B\xC4\x44\x89\x48\x20\x48\x89\x48\x08\x53\x55\x56\x57\x41\x56\x48\x83\xEC\x40\x48\xC7\x40\xC8\xFE\xFF\xFF\xFF\x49\x63\xF8");

// effect list: 0x78
    uintptr_t multifx_container = NULL;

#if 0
    static uintptr_t __fastcall hook_callback(hook_arg_t hook_arg, func_args * args)
    {
        info("%s(%p, %p, %p, %p)", (const char *)hook_arg, args->arg1, args->arg2, args->arg3, args->arg4);
        if (!multifx_container) {
            multifx_container = args->arg1;
        }
        return 0;
    }
#endif

    return true;
}

static uintptr_t __fastcall hook_callback(hook_arg_t hook_arg, func_args *args)
{
    info("%d(%p, %p, %p, %p)", (uint32_t)hook_arg, args->arg1, args->arg2, args->arg3, args->arg4);
    return 0;
}

extern uintptr_t multifx_container;
static void *wait_keypress(void *arg)
{
    int key = 0;
    while (1) {
        if (GetAsyncKeyState(VK_UP) & 1) {
            info("Add 1 key to deck %u", get_master());
            if (key < 12) {
                key++;
            }
            adjust_deck_key(get_master(), key);
        }
        if (GetAsyncKeyState(VK_DOWN) & 1) {
            info("Subtract 1 key from deck %u", get_master());
            if (key > -12) {
                key--;
            }
            adjust_deck_key(get_master(), key);
        }
        if (GetAsyncKeyState(VK_LEFT) & 1) {
            info("Sliploop deck %u", get_master());
            slip_loop(get_master());
        }
        if (GetAsyncKeyState(VK_RIGHT) & 1) {
            info("Stop sliploop deck %u", get_master());
            stop_slip_loop(get_master());
        }
        if (GetAsyncKeyState(VK_SPACE) & 1) {

            RekordboxMultiFxUnit *unit = (RekordboxMultiFxUnit *)cfxunit;
            //unit->RekordboxMultiFxUnit_setEffectPercent(0, 100.0);
            //unit->AbstractFxUnit_selectEffect(0, 0, 0);
            info("Started effect on 0");

            continue;

            info("Flanging...");
            typedef uintptr_t(*flang_fn_t)(void *, int, int);
            // MultiBeatFxFlanger::MultiBeatFxFlanger constructor
            static flang_fn_t flang_fn = (flang_fn_t)sig_scan("48 89 4C 24 08 57 48 83 EC 40 48 C7 44 24 30 FE FF FF FF 48 89 5C 24 58 48 8B F9 E8 ?? ?? ?? ?? 90 48 8D 05 50 08 FA 01 48 89 07");
            void *flang_obj = calloc(1, 0x368);
            if (!flang_obj) {
                return 0;
            }
            flang_fn(flang_obj, 0, 0x28B);
            // yeah idk what this is
            *(uintptr_t *)((uintptr_t)flang_obj + 240) = 1;
            *(uintptr_t *)((uintptr_t)flang_obj + 244) = 0x41F00000;
            void **vtable = *(void ***)flang_obj;
            typedef void (*set_param_fn_t)(void *, int, float);
            set_param_fn_t set_param = (set_param_fn_t)vtable[14];
            set_param(flang_obj, 1, 1);
            typedef void (*turn_on_fn_t)(void *, float);
            turn_on_fn_t turn_on = (turn_on_fn_t)vtable[7];
            //turn_on(flang_obj, 1);

            // EffectBase__setParameter
            success("Flanged");
        }
        if (GetAsyncKeyState(VK_BACK) & 1) {
            //info("fader set to 0 on %u", get_master());
            //set_master_volume(0);
        }

        Sleep(100);
    }
    return NULL;
}

// shift key on a deck up from -12 to +12
bool adjust_deck_key(uint32_t deck, int key_offset)
{
    if (deck > 3 || key_offset > 12 || key_offset < -12) {
        error("Invalid deck or invalid key: deck=%u key=%d", deck, key_offset);
        return false;
    }
    players[deck]->shiftPlayKeySemitone(control_clients[deck], key_offset);
    //set_key(instance, deck, key_offset);
    return true;
}

// lookup original bpm of song on deck
static uint32_t get_track_bpm_of_deck(uint32_t deck)
{
    // lookup a rowdata object
    row_data *rowdata = lookup_row_data(deck);
    if (!rowdata) {
        return 0;
    }
    // grab the bpm out
    uint32_t bpm = rowdata->getBpm();
    // cleanup the rowdata object we got from rekordbox
    destroy_row_data(rowdata);
    return bpm;
}

// add some bpm amount to a deck
bool adjust_deck_bpm(uint32_t deck, int tempo_offset)
{
    if (deck > 3) {
        error("Invalid deck: %u", deck);
        return false;
    }
    djplayer_uiplayer *player = lookup_player(deck);
    if (!player) {
        error("player %u is NULL", deck);
        return false;
    }
    // get the original track bpm ex: 15000 (150)
    int orig_bpm = (int)get_track_bpm_of_deck(deck);
    // get the real current bpm of deck ex: 15500 (155)
    int real_bpm = (int)player->getDeckBPM();
    // get the intended new bpm, ex: 16000 (160)
    int new_bpm = real_bpm + (tempo_offset * 100);
    // the diff is how far the new value is from the original ex: 1500 (+15)
    int bpm_diff = new_bpm - orig_bpm;
    // the precent diff is the percent of the original that is different ex: 15/150 (+10%)
    float percent_diff = ((float)bpm_diff / (float)orig_bpm) * (float)100.0;
    // call the rekordbox functions to set tempo via percent offset
    players[deck]->setBaseTempoRatio(control_clients[deck], percent_diff);
    return true;
}

// play a deck
bool play_deck(uint32_t deck)
{
    if (deck > 3) {
        error("Invalid deck: %u", deck);
        return false;
    }
    players[deck]->play(control_clients[deck]);
    return true;
}

// pause a deck
bool pause_deck(uint32_t deck)
{
    if (deck > 3) {
        error("Invalid deck: %u", deck);
        return false;
    }
    players[deck]->pause(control_clients[deck]);
    return true;
}

// start scratching a deck
bool start_scratch(uint32_t deck, float magnitude)
{
    if (deck > 3) {
        error("Invalid deck: %u", deck);
        return false;
    }
    players[deck]->scratch(control_clients[deck], false, magnitude, false, false);
    return true;
}

// stop scratching a deck
bool stop_scratch(uint32_t deck)
{
    if (deck > 3) {
        error("Invalid deck: %u", deck);
        return false;
    }
    players[deck]->stopScratch(control_clients[deck], false, false);
    return true;
}

// Mute/unmute a deck
bool mute_deck(uint32_t deck, bool mute)
{
    if (deck > 3) {
        error("Invalid deck: %u", deck);
        return false;
    }
    players[deck]->mutePlayer(mute, control_clients[deck]);
    return true;
}

// go back to current cue point and pause
bool back_to_current_cue(uint32_t deck)
{
    if (deck > 3) {
        error("Invalid deck: %u", deck);
        return false;
    }
    players[deck]->backToCurrentCue(control_clients[deck]);
    return true;
}

// XXX: broken
bool slip_loop(uint32_t deck)
{
    if (deck > 3) {
        error("Invalid deck: %u", deck);
        return false;
    }
    Fraction<uint16_t> frac;
    // 1/4
    frac.top = 1;
    frac.bot = 4;
    players[deck]->instantSlipLoop(control_clients[deck], frac, false);
    Sleep(1000);
    players[deck]->exitInstantSlipLoop(control_clients[deck], false);
    //players[deck]->adjustLoopPoint(control_clients[deck], false, 1);
    //players[deck]->adjustLoopLength(control_clients[deck], 100);
    //players[deck]->autoBeatLoop(control_clients[deck], 2);
    //players[deck]->instantSlipLoop(control_clients[deck], frac, false);
    //players[deck]->jumpToLoopInPoint(control_clients[deck], false);
    return true;
}

// XXX: broken
bool stop_slip_loop(uint32_t deck)
{
    if (deck > 3) {
        error("Invalid deck: %u", deck);
        return false;
    }
    //players[deck]->exitInstantSlipLoop(control_clients[deck], false);
    players[deck]->changePlayDirection(control_clients[deck], true, true);
    return true;
}

// set play to true=forward, false=backwards. If backwards set temporary=true for a temporary change
bool set_play_direction(uint32_t deck, bool forward, bool temporary)
{
    if (deck > 3) {
        error("Invalid deck: %u", deck);
        return false;
    }
    players[deck]->changePlayDirection(control_clients[deck], forward, temporary);
    return true;
}

// set a channel fader to a volume amount
bool set_channel_fader(uint32_t deck, float amount)
{
    if (deck > 3) {
        error("Invalid deck: %u", deck);
        return false;
    }
    if (!mixer) {
        error("Mixer is NULL");
        return false;
    }
    mixer->setChannelFaderPosition(deck, amount);
    return true;
}

// set the master output volume
bool set_master_volume(float amount)
{
    if (!mixer) {
        error("Mixer is NULL");
        return false;
    }
    mixer->setMasterVolume(amount);
    return true;
}

// turn the mic on or off
bool set_mic_onoff(bool on)
{
    if (!mixer) {
        error("Mixer is NULL");
        return false;
    }
    mixer->setMicOnOff(on);
    return true;
}
