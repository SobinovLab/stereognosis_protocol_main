#include "CStaticColor.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//#define RED        RGB(127,  0,  0)
//#define GREEN      RGB(  0,127,  0)
//#define BLUE       RGB(  0,  0,127)
//#define LIGHTRED   RGB(255,  0,  0)
//#define LIGHTGREEN RGB(  0,255,  0)
//#define LIGHTBLUE  RGB(  0,  0,255)
#define BLACK      RGB(  0,  0,  0)
//#define WHITE      RGB(255,255,255)
//#define GRAY       RGB(192,192,192)

/////////////////////////////////////////////////////////////////////////////
// CStaticColor

CStaticColor::CStaticColor()
{
	m_crBkColor = ::GetSysColor(COLOR_3DFACE); // Initializing the Background Color to the system face color.
	m_crTextColor = BLACK; // Initializing the text to Black
	m_brBkgnd.CreateSolidBrush(m_crBkColor); // Create the Brush Color for the Background.
}

CStaticColor::~CStaticColor()
{
}

BEGIN_MESSAGE_MAP(CStaticColor, CStatic)
	//{{AFX_MSG_MAP(CStaticColor)
	ON_WM_CTLCOLOR_REFLECT()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CStaticColor message handlers

HBRUSH CStaticColor::CtlColor(CDC* pDC, UINT nCtlColor)
{
	HBRUSH hbr;
	hbr = (HBRUSH)m_brBkgnd; // Passing a Handle to the Brush
	pDC->SetBkColor(m_crBkColor); // Setting the Color of the Text Background to the one passed by the Dialog
	pDC->SetTextColor(m_crTextColor); // Setting the Text Color to the one Passed by the Dialog

	if (nCtlColor)       // To get rid of compiler warning
		nCtlColor += 0;

	return hbr;

}

void CStaticColor::SetBkColor(COLORREF crColor)
{
	m_crBkColor = crColor; // Passing the value passed by the dialog to the member varaible for Backgound Color
	m_brBkgnd.DeleteObject(); // Deleting any Previous Brush Colors if any existed.
	m_brBkgnd.CreateSolidBrush(crColor); // Creating the Brush Color For the Static Text Background
	RedrawWindow();
}

void CStaticColor::SetTextColor(COLORREF crColor)
{
	m_crTextColor = crColor; // Passing the value passed by the dialog to the member varaible for Text Color
	RedrawWindow();
}