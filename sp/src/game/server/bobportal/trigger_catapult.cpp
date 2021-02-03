//=========  ============//
//
// Purpose: Replica of Portal 2's entity by the same name.
//
//=============================================================================

#include "cbase.h"
#include "triggers.h"
#include "movevars_shared.h"

ConVar sv_debug_catapults( "sv_debug_catapults", "0", FCVAR_CHEAT | FCVAR_REPLICATED, "Display some debug information for catapults." );

class CTriggerCatapult : public CTriggerMultiple {
    public:
        DECLARE_CLASS(CTriggerCatapult, CTriggerMultiple);
        DECLARE_DATADESC();

        CTriggerCatapult();

        void Spawn();
        void Touch(CBaseEntity *pOther);
        
        Vector CalcJumpLaunchVelocity(const Vector &startPos, const Vector &endPos, float flGravity, float *pminHeight, float maxHorzVelocity, Vector *pvecApex );

    protected:
        string_t m_jumpTarget;

        float m_PlayerLaunchSpeed;
        float m_PhysicsLaunchSpeed;
        
        Vector m_vLaunchDirection;
        QAngle m_LaunchDirection;
};

LINK_ENTITY_TO_CLASS(trigger_catapult, CTriggerCatapult)

BEGIN_DATADESC( CTriggerCatapult )

    DEFINE_KEYFIELD(m_jumpTarget, FIELD_STRING, "launchTarget"),
    DEFINE_KEYFIELD(m_PlayerLaunchSpeed, FIELD_FLOAT, "playerSpeed"),
    DEFINE_KEYFIELD(m_PhysicsLaunchSpeed, FIELD_FLOAT, "physicsSpeed"),

    DEFINE_KEYFIELD(m_vLaunchDirection, FIELD_VECTOR, "launchDirection")

END_DATADESC()

CTriggerCatapult::CTriggerCatapult(){}

void CTriggerCatapult::Spawn() {
    BaseClass::Spawn();

    SetSolid(SOLID_VPHYSICS);
    SetModel( STRING( GetModelName() ) );

    m_LaunchDirection = QAngle(m_vLaunchDirection.x, m_vLaunchDirection.y, m_vLaunchDirection.z);

    if (m_jumpTarget == NULL_STRING)
        Warning("prop_catapult at %.0f %.0f %.0f without a target!\n", GetAbsOrigin().x, GetAbsOrigin().y, GetAbsOrigin().z);

    AddSolidFlags(FSOLID_NOT_SOLID | FSOLID_TRIGGER);

    SetTouch(&CTriggerCatapult::Touch);

    VPhysicsInitShadow( false, false );
}

void CTriggerCatapult::Touch(CBaseEntity *pOther) {

	if ( !pOther->IsSolid() || (pOther->GetMoveType() == MOVETYPE_PUSH || pOther->GetMoveType() == MOVETYPE_NONE ) )
		return;

	if (!PassesTriggerFilters(pOther))
		return;

	if (pOther->GetMoveParent())
		return;


    Msg("checking wee\n");
    if (!PassesTriggerFilters(pOther)) {
        Msg("can't wee\n");
        return;
    }

    Msg("go wee, my friend %s\n", pOther->GetClassname());


    if (m_jumpTarget == NULL_STRING) {
        Error("prop_catapult at %.0f %.0f %.0f has no target! Cannot launch!\n", GetAbsOrigin().x, GetAbsOrigin().y, GetAbsOrigin().z);
        return;
    }

    // Get the entitiy
    CBaseEntity *pJumpTarget = NULL;
    pJumpTarget = gEntList.FindEntityByName(pJumpTarget, m_jumpTarget);

    if (pJumpTarget == NULL) {
        Error("prop_catapult at %.0f %.0f %.0f couldn't find its' target. Can't launch!\n", GetAbsOrigin().x, GetAbsOrigin().y, GetAbsOrigin().z);
        return; // Didn't find anyone!
    }

    /** Copied parts from the antlion, bodged the rest **/
	
	float minJumpHeight = GetAbsOrigin().z - pJumpTarget->GetAbsOrigin().z; // It must be ATLEAST high enough for our target
    float maxHorzVel;
    if (pOther->IsPlayer())
    	maxHorzVel = m_PlayerLaunchSpeed;
    else
        maxHorzVel = m_PhysicsLaunchSpeed;

    float m_flFlyTime = 1.0f;

    Vector vecApex;
	Vector rawJumpVel = CalcJumpLaunchVelocity(GetAbsOrigin(), pJumpTarget->GetAbsOrigin(), GetCurrentGravity(), &minJumpHeight, maxHorzVel, &vecApex );

	if ( sv_debug_catapults.GetInt() == 1 )
	{
		NDebugOverlay::Line( GetAbsOrigin(), vecApex, 255, 0, 255, 0, 5 );
		NDebugOverlay::Line( GetAbsOrigin(), pJumpTarget->GetAbsOrigin(), 0, 255, 0, 0, 5 );
		NDebugOverlay::Line( GetAbsOrigin(), rawJumpVel, 255, 255, 0, 0, 5 );

        // Draw the path we SHOULD take
		NDebugOverlay::Line( GetAbsOrigin(), vecApex, 0, 255, 255, 0, 5 );
		NDebugOverlay::Line( vecApex, pJumpTarget->GetAbsOrigin(), 0, 255, 255, 0, 5 );
	}

    Vector velGoZoomZoom = rawJumpVel;

    pOther->SetGroundEntity(NULL);
    pOther->SetAbsVelocity(rawJumpVel);
}

/** Copied from CAI_MoveProbe **/
Vector CTriggerCatapult::CalcJumpLaunchVelocity(const Vector &startPos, const Vector &endPos, float flGravity, float *pminHeight, float maxHorzVelocity, Vector *pvecApex )
{
	// Get the height I have to jump to get to the target
	float	stepHeight = endPos.z - startPos.z;

	// get horizontal distance to target
	Vector targetDir2D	= endPos - startPos;
	targetDir2D.z = 0;
	float distance = VectorNormalize(targetDir2D);

	Assert( maxHorzVelocity > 0 );

	// get minimum times and heights to meet ideal horz velocity
	float minHorzTime = distance / maxHorzVelocity;
	float minHorzHeight = 0.5 * flGravity * (minHorzTime * 0.5) * (minHorzTime * 0.5);

	// jump height must be enough to hang in the air
	*pminHeight = MAX( *pminHeight, minHorzHeight );
	// jump height must be enough to cover the step up
	*pminHeight = MAX( *pminHeight, stepHeight );

	// time from start to apex
	float t0 = sqrt( ( 2.0 * *pminHeight) / flGravity );
	// time from apex to end
	float t1 = sqrt( ( 2.0 * fabs( *pminHeight - stepHeight) ) / flGravity );

	float velHorz = distance / (t0 + t1);

	Vector jumpVel = targetDir2D * velHorz;

	jumpVel.z = (float)sqrt(2.0f * flGravity * (*pminHeight));

	if (pvecApex)
	{
		*pvecApex = startPos + targetDir2D * velHorz * t0 + Vector( 0, 0, *pminHeight );
	}

	// -----------------------------------------------------------
	// Make the horizontal jump vector and add vertical component
	// -----------------------------------------------------------

	return jumpVel;
}
