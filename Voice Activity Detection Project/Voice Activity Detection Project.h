
// Voice Activity Detection Project.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'pch.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols


// CVoiceActivityDetectionProjectApp:
// See Voice Activity Detection Project.cpp for the implementation of this class
//

class CVoiceActivityDetectionProjectApp : public CWinApp
{
public:
	CVoiceActivityDetectionProjectApp();

// Overrides
public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
};

extern CVoiceActivityDetectionProjectApp theApp;
