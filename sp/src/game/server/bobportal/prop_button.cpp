//=========  ============//
//
// Purpose: Replica of Portal 2's entity by the same name.
//
//=============================================================================

#include "prop_button.h"

#define BUTTON_MODEL "models/props/switch001.mdl"
#define BUTTON_DOWN_SOUND "Portal.button_down"
#define BUTTON_UP_SOUND "Portal.button_up"

/* this is the base button, allowing us to make other prop-based buttons that aren't confined to a portal button... It's feature creep, essentially. */

BEGIN_DATADESC(CPropButton)
    
    // Key Fields
    DEFINE_KEYFIELD(m_ModelName, FIELD_STRING, "model"),
    DEFINE_KEYFIELD(m_nSkin, FIELD_INTEGER, "skin"),
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
    Precache();
    BaseClass::Spawn();
    SetModel( STRING(m_ModelName) );

    idleSequence = LookupSequence("idle");
    downSequence = LookupSequence("down");
    upSequence = LookupSequence("up");
    
    CheckSequence(idleSequence);
    CheckSequence(downSequence);
    CheckSequence(upSequence);

    SetUse(&CPortalPropButton::Use);

    CreateVPhysics();
    PropSetAnim("idle");
}

void CPortalPropButton::Press() {
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
    if (GetSequence() == downSequence && !m_bStay) { // If we've just pushed down, AND we aren't a toggle button
		SetNextThink( gpGlobals->curtime + m_flWaitTime );
		SetThink( &CPortalPropButton::UnPress );
    }

    if (GetSequence() == upSequence) {
        Reset();
    }
}
