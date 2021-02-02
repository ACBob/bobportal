//=========  ============//
//
// Purpose: Prop-based button base.
//
//=============================================================================


#ifndef PROPBUTTON_H
#ifdef _WIN32
#pragma once
#endif // _WIN32

#include "cbase.h"
#include "props.h"


class CPropButton : public CDynamicProp {

    public:
        DECLARE_CLASS(CPropButton, CDynamicProp);
        DECLARE_DATADESC();

        CPropButton() {};
        bool CreateVPhysics() {
            // For whatever reason, No matter what I do, no matter what I write, no matter how I write it,
            // The bone followers ALWAYS end up non-solid. Because of this, we MUST overwrite the vphysics creation code.
            // So that *we* can make a solid(!!) collection of bone followers, allowing collision.
            // Thanks, source!

            // we don't allow disabling them anyway, so just create them...
            CreateBoneFollowers();

            if (m_BoneFollowerManager.GetNumBoneFollowers()) {
                // We can skip the whole setting them as non-solid, because that's what's creating the issue in the first place!

                AddSolidFlags(FSOLID_NOT_SOLID); // *we* aren't solid, our bone followers should be though.
        		AddSolidFlags( FSOLID_CUSTOMRAYTEST | FSOLID_CUSTOMBOXTEST ); // Use our bone followers for collision.
            }
            else {
                VPhysicsInitStatic();
            }

            return true;
        };

        void InputPress( inputdata_t& inputdata );

        void Press();
        void UnPress();
        void Reset();
        
        void CheckSequence(int id);
        
        void Spawn();

        void ButtonUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
        
        int ObjectCaps();

        void ReachedEndOfSequence(void);

    protected:
        int idleSequence;
        int downSequence;
        int upSequence;
    
        int m_SkinA;

        bool m_bStay;
        bool m_bCantCancel;
        bool m_bPressed;
        
        string_t m_DownSound;
        string_t m_UpSound;
        string_t m_ModelName;

        float m_flWaitTime;

        COutputEvent m_OnPressed;
        COutputEvent m_OnUnPressed;
        COutputEvent m_OnReset;
};


#endif // PROPBUTTON_H
