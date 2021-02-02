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
            BaseClass::CreateVPhysics(); // We're essentially a coked-up prop_dynamic, just use their vphysics creation....
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
