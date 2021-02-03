//=========  ============//
//
// Purpose: Replica of Portal 2's entity by the same name.
//
//=============================================================================

#include "cbase.h"
#include "triggers.h"
#include "movevars_shared.h"

ConVar sv_debug_catapults( "sv_debug_catapults", "0", FCVAR_CHEAT | FCVAR_REPLICATED, "Display some debug information for catapults." );

class CTriggerCatapult : public CBaseVPhysicsTrigger {
    public:
        DECLARE_CLASS(CTriggerCatapult, CBaseVPhysicsTrigger);
        DECLARE_DATADESC();

        CTriggerCatapult();

        void Spawn();
        void Touch(CBaseEntity *pOther);
        
        Vector CalcJumpLaunchVelocity(const Vector &startPos, const Vector &endPos, float flGravity, float *pminHeight, float maxHorzVelocity, Vector *pvecApex );

        Vector GenerateVelocity(Vector vecOther, Vector vecTarget, bool isPlayer);

        bool ThereIsReasonToNot(CBaseEntity *pOther);

    protected:
        string_t m_jumpTarget;

        float m_PlayerLaunchSpeed;
        float m_PhysicsLaunchSpeed;

        COutputEvent m_OutputOnCatapulted;
        
        Vector m_vLaunchDirection;
        QAngle m_LaunchDirection;

        bool m_bLaunchByAngles;
        bool m_bApplyRandomRotation;
};

LINK_ENTITY_TO_CLASS(trigger_catapult, CTriggerCatapult)

BEGIN_DATADESC( CTriggerCatapult )

    DEFINE_KEYFIELD(m_jumpTarget, FIELD_STRING, "launchTarget"),
    DEFINE_KEYFIELD(m_PlayerLaunchSpeed, FIELD_FLOAT, "playerSpeed"),
    DEFINE_KEYFIELD(m_PhysicsLaunchSpeed, FIELD_FLOAT, "physicsSpeed"),
    DEFINE_KEYFIELD(m_bApplyRandomRotation, FIELD_BOOLEAN, "applyAngularImpulse"),

    DEFINE_KEYFIELD(m_vLaunchDirection, FIELD_VECTOR, "launchDirection"),

    DEFINE_OUTPUT(m_OutputOnCatapulted, "OnCatapulted")

END_DATADESC()

CTriggerCatapult::CTriggerCatapult(){}

void CTriggerCatapult::Spawn() {
    BaseClass::Spawn();

    SetSolid(SOLID_VPHYSICS);
    SetModel( STRING( GetModelName() ) );

    m_bLaunchByAngles = (m_jumpTarget == NULL_STRING);

    m_LaunchDirection = QAngle(m_vLaunchDirection.x, m_vLaunchDirection.y, m_vLaunchDirection.z);

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
    

    // These IFs are confusing and could probably be written better, I'm just happy it's working.
    // First we check if we actually have a target set. if we don't, we choose to launch by angles and skip everything else.
    
    CBaseEntity *pJumpTarget = NULL;
    // Then, if we are willing to launch by target, we look for the target.
    if (m_jumpTarget != NULL_STRING) {
        // Get the entitiy
        pJumpTarget = gEntList.FindEntityByName(pJumpTarget, m_jumpTarget);

        // If we can't find it, we warn the stupid mapper and then choose to launch by angles anyway.
        if (pJumpTarget == NULL) {
            Warning("prop_catapult at %.0f %.0f %.0f has a target set, but we can't find a target by that name (%s).\nUsing angle launch instead...", GetAbsOrigin().x, GetAbsOrigin().y, GetAbsOrigin().z, m_jumpTarget);
            m_bLaunchByAngles = true;
        }
    }
    else {
        m_bLaunchByAngles = true;
    }

    Vector velGoZoomZoom;

    // If we don't want to launch by angles, we do the cool launch
    if (!m_bLaunchByAngles) {
        velGoZoomZoom = GenerateVelocity(pOther->GetAbsOrigin(), pJumpTarget->GetAbsOrigin(), pOther->IsPlayer());
        if (sv_debug_catapults.GetInt())
            NDebugOverlay::Line( GetAbsOrigin(), pJumpTarget->GetAbsOrigin(), 0, 255, 0, 0, 5 ); // Green
    }
    // Else, we just launch by the angle.
    else {
        Vector LaunchDir;
	    VectorRotate( m_vLaunchDirection, EntityToWorldTransform(), LaunchDir );
        velGoZoomZoom = LaunchDir * (pOther->IsPlayer() ? m_PlayerLaunchSpeed : m_PhysicsLaunchSpeed);
    }

    if (sv_debug_catapults.GetInt())
        NDebugOverlay::Line( GetAbsOrigin(), velGoZoomZoom, 255, 255, 0, 0, 5 ); // Yellow


    pOther->SetGroundEntity(NULL); // Essential for allowing the stuff to yEET


    if (pOther->IsPlayer())
        pOther->SetAbsVelocity(velGoZoomZoom);
    else if (pOther->GetMoveType() == MOVETYPE_VPHYSICS && pOther->VPhysicsGetObject()) {
        AngularImpulse velRotZoomZoom = Vector();
        if (m_bApplyRandomRotation) {
            float maxRandom = m_PhysicsLaunchSpeed / 10; // 450 = 45deg, 900 = 90 deg, etc.

            velRotZoomZoom = RandomVector(-maxRandom, maxRandom);
        }

        pOther->VPhysicsGetObject()->SetVelocity(&velGoZoomZoom, &velRotZoomZoom);
    }
    // We have launched the thing!
    m_OutputOnCatapulted.FireOutput(this, this);
}

Vector CTriggerCatapult::GenerateVelocity(Vector vecOther, Vector vecTarget, bool isPlayer) { // TODO: this doesn't take into account vphysics drag, which is quite strong
    /** Copied parts from the antlion, bodged the rest **/

    float minJumpHeight;
    if (vecOther.z > vecTarget.z)
        minJumpHeight = GetAbsOrigin().z - vecTarget.z; // It must be ATLEAST high enough for our target
    else if (vecOther.z < vecTarget.z)
        minJumpHeight = GetAbsOrigin().z + vecTarget.z; // It must be ATLEAST high enough for our target

    float maxHorzVel;
    if (isPlayer)
    	maxHorzVel = m_PlayerLaunchSpeed;
    else
        maxHorzVel = m_PhysicsLaunchSpeed;

    Vector vecApex;
	Vector rawJumpVel = CalcJumpLaunchVelocity(GetAbsOrigin(), vecTarget, GetCurrentGravity(), &minJumpHeight, maxHorzVel, &vecApex );

	if ( sv_debug_catapults.GetInt())
	{
		NDebugOverlay::Line( GetAbsOrigin(), vecApex, 255, 0, 255, 0, 5 ); // Magenta

        // Draw the path we SHOULD take
		NDebugOverlay::Line( GetAbsOrigin(), vecApex, 255, 0, 0, 0, 5 ); // Red
		NDebugOverlay::Line( vecApex, vecTarget, 0, 255, 0, 0, 5 ); // Red
	}

    return rawJumpVel;
}

/** Copied from CAI_MoveProbe, TODO: Is there a better way of accessing this function? **/
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
