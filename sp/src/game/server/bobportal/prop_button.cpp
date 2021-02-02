//=========  ============//
//
// Purpose: Replica of Portal 2's entity by the same name.
//
//=============================================================================

#include "prop_button.h"

#define BUTTON_MODEL "models/props/switch001.mdl"
#define BUTTON_FLOOR_MODEL "models/props/button_joined.mdl"
#define BUTTON_DOWN_SOUND "Portal.button_down"
#define BUTTON_UP_SOUND "Portal.button_up"

/* this is the base button, allowing us to make other prop-based buttons that aren't confined to a portal button... It's feature creep, essentially. */

BEGIN_DATADESC(CPropButton)
    
    // Key Fields
    DEFINE_KEYFIELD(m_ModelName, FIELD_STRING, "model"),
    DEFINE_KEYFIELD(m_SkinA, FIELD_INTEGER, "skin"),
    DEFINE_KEYFIELD(m_DownSound, FIELD_STRING, "downSound"),
    DEFINE_KEYFIELD(m_UpSound, FIELD_STRING, "upSound"),
    DEFINE_KEYFIELD(m_flWaitTime, FIELD_FLOAT, "delay"),
    DEFINE_KEYFIELD(m_bCantCancel, FIELD_BOOLEAN, "preventfastreset"),

    // Functions
    DEFINE_USEFUNC(ButtonUse),
    DEFINE_INPUTFUNC(FIELD_VOID, "Press", InputPress),
    
    // Outputs
    DEFINE_OUTPUT(m_OnPressed, "OnPressed"),
    DEFINE_OUTPUT(m_OnReset, "OnButtonReset")

END_DATADESC()

void CPropButton::CheckSequence(int id) {
    bool sequenceStatus = PrefetchSequence(id);
    sequenceStatus;
    Assert(id != ACT_INVALID && sequenceStatus);
}

int CPropButton::ObjectCaps() {
    return BaseClass::ObjectCaps() | FCAP_IMPULSE_USE;
}

void CPropButton::InputPress(inputdata_t& inputdata) {
    Press();
}

void CPropButton::Press() {
    m_bPressed = true;
    m_OnPressed.FireOutput(this, this);
}
void CPropButton::UnPress() {
    m_bPressed = false;
    m_OnUnPressed.FireOutput(this, this);

    if (m_flWaitTime > 0)
        SetThink( NULL );
}
void CPropButton::Reset() {
    m_OnReset.FireOutput(this, this);

    // Also reset our animation
    PropSetAnim("idle");
    m_iszDefaultAnim = MAKE_STRING( "idle" );
}

void CPropButton::ButtonUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value) {
    Press();
}


void CPropButton::Spawn() {
    BaseClass::Spawn();

    SetSolid(SOLID_VPHYSICS);
    SetUse(&CPropButton::Use);

    CreateVPhysics();
    SetPlaybackRate(1.0f);

    m_bStay = (m_flWaitTime <= -1);
    if (m_flWaitTime < 0.0f)
        m_flWaitTime = 0.0f;
}

void CPropButton::ReachedEndOfSequence(void) {
    if (GetSequence() == upSequence) {
		SetNextThink( gpGlobals->curtime + m_flWaitTime );
		SetThink( &CPropButton::UnPress );
    }

    if (GetSequence() == downSequence) {
        Reset();
    }
}

/* Ok now here's the portal button */

class CPortalPropButton : public CPropButton {

    public:
        DECLARE_CLASS(CPortalPropButton, CPropButton);

        CPortalPropButton();
        
        void Spawn();
        void Precache();

        void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);

        void Press();
        void UnPress();
        void ReachedEndOfSequence(void);
};

LINK_ENTITY_TO_CLASS(prop_button, CPortalPropButton);

CPortalPropButton::CPortalPropButton(){}

void CPortalPropButton::Precache() {

    if (m_ModelName == NULL_STRING)
        m_ModelName = MAKE_STRING( BUTTON_MODEL );
    PrecacheModel( STRING( m_ModelName ) );

    if (m_DownSound == NULL_STRING)
        m_DownSound = MAKE_STRING( BUTTON_DOWN_SOUND );
    PrecacheScriptSound( STRING( m_DownSound ) );

    if (m_UpSound == NULL_STRING)
        m_UpSound = MAKE_STRING( BUTTON_UP_SOUND );
    PrecacheScriptSound( STRING( m_UpSound ) );

    BaseClass::Precache();
}

void CPortalPropButton::Spawn() {
	SetSolid(SOLID_VPHYSICS);
    Precache();
    SetModel( STRING(m_ModelName) );
    BaseClass::Spawn();

    idleSequence = LookupSequence("idle");
    downSequence = LookupSequence("down");
    upSequence = LookupSequence("up");
    
    CheckSequence(idleSequence);
    CheckSequence(downSequence);
    CheckSequence(upSequence);

    SetUse(&CPortalPropButton::Use);

	CreateVPhysics();

    PropSetAnim("idle");

    m_nSkin = m_SkinA;

    m_bPressed = false;
}

void CPortalPropButton::Press() {
    if (m_bPressed) // If we're pressed, don't allow it
        return;

    BaseClass::Press();

    PropSetAnim("down");
    EmitSound( STRING( m_DownSound ) );
}

void CPortalPropButton::UnPress() {
    if (!m_bPressed)
        return; // If we're not even pressed, don't allow it

    BaseClass::UnPress();

    PropSetAnim("up");
    EmitSound( STRING( m_UpSound ) );
}

void CPortalPropButton::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value) {
    if (!m_bPressed)    
        Press();
    else if (!m_bCantCancel) { // We're pressed AND we can cancel it
        UnPress(); // Cancel the press
    }
}

void CPortalPropButton::ReachedEndOfSequence() {
    // HACK: since we're called pretty frequently any-hoo...
    m_nSkin = m_bPressed ? m_SkinA + 1 : m_SkinA;

    if (GetSequence() == downSequence && !m_bStay) { // If we've just pushed down, AND we aren't a toggle button
		SetNextThink( gpGlobals->curtime + m_flWaitTime );
		SetThink( &CPortalPropButton::UnPress );
    }

    if (GetSequence() == upSequence) {
        Reset();
    }
}

/*============================================================================//
//
// Purpose: The floor button
//
//============================================================================*/

class CPropPortalFloorButton : public CPropButton {
    public:
        DECLARE_CLASS(CPropPortalFloorButton, CPropButton);

        CPropPortalFloorButton();

        void Spawn();
        void Precache();
        void TryTouch(CBaseEntity *pOther);

        void Press();
        void UnPress();

        void TouchThink();

    protected:
        int m_TouchCount;

        int idleDownSequence;

        Vector m_vButtonPartBound; // The bounding box around the 'button' part

};

LINK_ENTITY_TO_CLASS(prop_floor_button, CPropPortalFloorButton);

CPropPortalFloorButton::CPropPortalFloorButton(){}

void CPropPortalFloorButton::Spawn() {
	SetSolid(SOLID_VPHYSICS);
    Precache();

    SetModel( STRING(m_ModelName) );
    PropSetAnim("idle");
    m_iszDefaultAnim = MAKE_STRING("idle");

    BaseClass::Spawn();

    SetUse(NULL);

    idleSequence = LookupSequence("idle");
    downSequence = LookupSequence("down");
    upSequence = LookupSequence("up");
    idleDownSequence = LookupSequence("idledown");
    
    CheckSequence(idleSequence);
    CheckSequence(downSequence);
    CheckSequence(upSequence);
    CheckSequence(idleDownSequence);

    // TODO: What do the numbers mean??!!
    //       The numbers, mason! What do they mean?!
    //       ..Stop writing code in the early hours of the morning!
    m_vButtonPartBound = Vector(48, 48, 40);

    SetThink(&CPropPortalFloorButton::TouchThink);
    SetNextThink(gpGlobals->curtime + TICK_INTERVAL);

    m_bPressed = false;

    CreateVPhysics();
}

void CPropPortalFloorButton::Precache() {

    if (m_ModelName == NULL_STRING)
        m_ModelName = MAKE_STRING( BUTTON_FLOOR_MODEL );
    PrecacheModel( STRING( m_ModelName ) );

    if (m_DownSound == NULL_STRING)
        m_DownSound = MAKE_STRING( BUTTON_DOWN_SOUND );
    PrecacheScriptSound( STRING( m_DownSound ) );

    if (m_UpSound == NULL_STRING)
        m_UpSound = MAKE_STRING( BUTTON_UP_SOUND );
    PrecacheScriptSound( STRING( m_UpSound ) );

    BaseClass::Precache();
}

void CPropPortalFloorButton::TouchThink(void) {

    m_TouchCount = 0;
    CBaseEntity *pObject = NULL;

    // TODO: right now it only responds to players and prop_physics-es-ess...
    while ( (pObject = gEntList.FindEntityByClassname(pObject, "prop_physics")) != NULL ) {
        TryTouch(pObject);
    }
    while ( (pObject = gEntList.FindEntityByClassname(pObject, "player")) != NULL ) {
        TryTouch(pObject);
    }

    if (m_TouchCount == 0 && m_bPressed) {
        UnPress();
    }
    else if (m_TouchCount > 0 && !m_bPressed) {
        Press();
    }

    m_nSkin = m_bPressed ? m_SkinA + 1 : m_SkinA;

    SetThink(&CPropPortalFloorButton::TouchThink);
    SetNextThink(gpGlobals->curtime + TICK_INTERVAL);
}

void CPropPortalFloorButton::TryTouch(CBaseEntity *pOther) {
    Vector ourPos = GetAbsOrigin();
    Vector theirPos = pOther->GetAbsOrigin();
    Vector posDifference = ourPos - theirPos;

    // Test to see if the entity is within our working bounds
    if (posDifference.x > m_vButtonPartBound.x || posDifference.x < -m_vButtonPartBound.x)
        return;
    if (posDifference.y > m_vButtonPartBound.y || posDifference.y < -m_vButtonPartBound.y)
        return;
    if (posDifference.x > m_vButtonPartBound.z || posDifference.z < -m_vButtonPartBound.z)
        return;

    m_TouchCount++;
}

void CPropPortalFloorButton::Press() {
    BaseClass::Press();

    PropSetAnim("down");
    m_iszDefaultAnim = MAKE_STRING("idledown");
    EmitSound( STRING( m_DownSound ) );
}
void CPropPortalFloorButton::UnPress() {
    BaseClass::UnPress();
    Reset();

    PropSetAnim("up");
    m_iszDefaultAnim = MAKE_STRING("idle");
    EmitSound( STRING( m_UpSound ) );
}