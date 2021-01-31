//=========  ============//
//
// Purpose: Replica of Portal 2's entity by the same name.
//
//=============================================================================

#include "cbase.h"
#include "doors.h"
#include "ndebugoverlay.h"
#include "spark.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "tier1/strtools.h"
#include "buttons.h"
#include "eventqueue.h"
#include "props.h"

class CPropButton : public CBaseProp {

    public:
        DECLARE_CLASS(CPropButton, CBaseProp);

        CPropButton();

        void Activate();
        void Spawn(void);
        void Precache();

        void ButtonUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
        void ButtonReturn();
        void ButtonActivate();

        void SetPropAnim( const char *szAnim );

        bool CreateVPhysics();

        int ObjectCaps();


        void InputLock( inputdata_t& input_data );
        void InputUnlock( inputdata_t& input_data );
        void InputPress( inputdata_t& input_data );
        void InputPressIn( inputdata_t& input_data );
        void InputPressOut( inputdata_t& input_data );

	    DECLARE_DATADESC();

        COutputEvent m_OnDamaged;
        COutputEvent m_OnPressed;
        COutputEvent m_OnUseLocked;
        COutputEvent m_OnIn;
        COutputEvent m_OnOut;

    protected:
        string_t m_ModelName;
        int m_iSkin;

        float m_flHoldTime; // How long we hold the button in a 'pushed' state

        bool m_bDisabled;
        bool m_bLocked;

        string_t m_UpSound;
        string_t m_DownSound;
};

LINK_ENTITY_TO_CLASS(prop_button, CPropButton);

BEGIN_DATADESC( CPropButton )
	
    DEFINE_KEYFIELD( m_ModelName, FIELD_STRING, "modelname" ),
    DEFINE_KEYFIELD( m_iSkin, FIELD_INTEGER, "skin" ),
    DEFINE_KEYFIELD( m_flHoldTime, FIELD_FLOAT, "delay" ),
    
    DEFINE_KEYFIELD( m_UpSound, FIELD_FLOAT, "downSound" ),
    DEFINE_KEYFIELD( m_DownSound, FIELD_FLOAT, "upSound" ),

	// Function Pointers
	DEFINE_FUNCTION( ButtonUse ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID, "Lock", InputLock ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Unlock", InputUnlock ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Press", InputPress ),
	DEFINE_INPUTFUNC( FIELD_VOID, "PressIn", InputPressIn ),
	DEFINE_INPUTFUNC( FIELD_VOID, "PressOut", InputPressOut ),

	// Outputs
	DEFINE_OUTPUT( m_OnDamaged, "OnDamaged" ),
	DEFINE_OUTPUT( m_OnPressed, "OnPressed" ),
	DEFINE_OUTPUT( m_OnUseLocked, "OnUseLocked" ),
	DEFINE_OUTPUT( m_OnIn, "OnIn" ),
	DEFINE_OUTPUT( m_OnOut, "OnOut" ),

END_DATADESC()

CPropButton::CPropButton(void) {}

// Stuff
void CPropButton::Activate() {
}

void CPropButton::Spawn() {
	char *szModel = (char *)STRING( GetModelName() );
	if (!szModel || !*szModel)
	{
		Warning( "prop_button at %.0f %.0f %0.f missing modelname, using default\n", GetAbsOrigin().x, GetAbsOrigin().y, GetAbsOrigin().z );
        szModel = "models/props/switch001.mdl";
       
        SetModelName( AllocPooledString(szModel) );
	}

    Precache();
    SetModel( szModel );

    SetSolid( SOLID_VPHYSICS );

    SetUse(&CPropButton::ButtonUse);

    BaseClass::Spawn();

    m_bDisabled = false;
    m_bLocked = false;

    CreateVPhysics();
}

bool CPropButton::CreateVPhysics()
{
	VPhysicsInitShadow( false, false );
	return true;
}

void CPropButton::Precache() {
    BaseClass::Precache();

    PrecacheModel( STRING( GetModelName() ) );
    PrecacheScriptSound( STRING(m_DownSound) );
    PrecacheScriptSound( STRING(m_UpSound) );
}

// Functions
void CPropButton::ButtonUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
    if (m_bDisabled || m_bLocked)
        return; // Don't do anything for disabled OR locked buttons
    
    m_bLocked = true; // Lock the button
    ButtonActivate();

    m_OnPressed.FireOutput(pActivator, this);

	SetNextThink( gpGlobals->curtime + m_flHoldTime );
	SetThink( &CPropButton::ButtonReturn );
    
}

void CPropButton::ButtonReturn( void )
{
    m_bLocked = false; //Unlock the button
    
	if ( m_UpSound != NULL_STRING )
	{
		CPASAttenuationFilter filter( this );

		EmitSound_t ep;
		ep.m_nChannel = CHAN_VOICE;
		ep.m_pSoundName = (char*)STRING(m_UpSound);
		ep.m_flVolume = 1;
		ep.m_SoundLevel = SNDLVL_NORM;

		EmitSound( filter, entindex(), ep );
	}

    SetPropAnim("down");
}

int	CPropButton::ObjectCaps(void)
{
	return((BaseClass::ObjectCaps() & ~FCAP_ACROSS_TRANSITION) | FCAP_IMPULSE_USE | FCAP_USE_IN_RADIUS );
}

void CPropButton::ButtonActivate( void )
{
	if ( m_DownSound != NULL_STRING )
	{
		CPASAttenuationFilter filter( this );

		EmitSound_t ep;
		ep.m_nChannel = CHAN_VOICE;
		ep.m_pSoundName = (char*)STRING(m_DownSound);
		ep.m_flVolume = 1;
		ep.m_SoundLevel = SNDLVL_NORM;

		EmitSound( filter, entindex(), ep );
	}

    SetPropAnim("up");
}

void CPropButton::SetPropAnim( const char *szAnim )
{
	if ( !szAnim )
		return;

	int nSequence = LookupSequence( szAnim );

	// Set to the desired anim, or default anim if the desired is not present
	if ( nSequence > ACTIVITY_NOT_AVAILABLE )
	{
		BaseClass::SetSequence( nSequence );
	}
	else
	{
		// Not available try to get default anim
		Warning( "Dynamic prop %s: no sequence named:%s\n", GetDebugName(), szAnim );
		SetSequence( 0 );
	}
}

// Inputs
void CPropButton::InputLock( inputdata_t& input_data ) {
    m_bLocked = true;
}
void CPropButton::InputUnlock( inputdata_t& input_data ) {
    m_bLocked = false;
}
void CPropButton::InputPress( inputdata_t& input_data ) {
}
void CPropButton::InputPressIn( inputdata_t& input_data ) {
}
void CPropButton::InputPressOut( inputdata_t& input_data ) {
}
