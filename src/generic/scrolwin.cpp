/////////////////////////////////////////////////////////////////////////////
// Name:        generic/scrolwin.cpp
// Purpose:     wxScrolledWindow implementation
// Author:      Julian Smart
// Modified by: Vadim Zeitlin on 31.08.00: wxScrollHelper allows to implement
//              scrolling in any class, not just wxScrolledWindow
// Created:     01/02/97
// RCS-ID:      $Id$
// Copyright:   (c) wxWindows team
// Licence:     wxWindows license
/////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#ifdef __GNUG__
    #pragma implementation "scrolwin.h"
#endif

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#include "wx/utils.h"
#include "wx/dcclient.h"

#include "wx/scrolwin.h"
#include "wx/panel.h"

#ifdef __WXMSW__
    #include "windows.h"
#endif

#ifdef __WXMOTIF__
// For wxRETAINED implementation
#ifdef __VMS__ //VMS's Xm.h is not (yet) compatible with C++
               //This code switches off the compiler warnings
# pragma message disable nosimpint
#endif
#include <Xm/Xm.h>
#ifdef __VMS__
# pragma message enable nosimpint
#endif
#endif

// ----------------------------------------------------------------------------
// wxScrollHelperEvtHandler: intercept the events from the window and forward
// them to wxScrollHelper
// ----------------------------------------------------------------------------

class wxScrollHelperEvtHandler : public wxEvtHandler
{
public:
    wxScrollHelperEvtHandler(wxScrollHelper *scrollHelper)
    {
        m_scrollHelper = scrollHelper;
    }

    virtual bool ProcessEvent(wxEvent& event)
    {
        if ( wxEvtHandler::ProcessEvent(event) )
            return TRUE;

        switch ( event.GetEventType() )
        {
            case wxEVT_SCROLLWIN_TOP:
            case wxEVT_SCROLLWIN_BOTTOM:
            case wxEVT_SCROLLWIN_LINEUP:
            case wxEVT_SCROLLWIN_LINEDOWN:
            case wxEVT_SCROLLWIN_PAGEUP:
            case wxEVT_SCROLLWIN_PAGEDOWN:
            case wxEVT_SCROLLWIN_THUMBTRACK:
            case wxEVT_SCROLLWIN_THUMBRELEASE:
                m_scrollHelper->HandleOnScroll((wxScrollWinEvent&)event);
                break;

            case wxEVT_PAINT:
                m_scrollHelper->HandleOnPaint((wxPaintEvent&)event);
                break;

            case wxEVT_SIZE:
                m_scrollHelper->HandleOnSize((wxSizeEvent&)event);
                break;

            case wxEVT_CHAR:
                m_scrollHelper->HandleOnChar((wxKeyEvent&)event);
                break;

            default:
                return FALSE;
        }

        return TRUE;
    }

private:
    wxScrollHelper *m_scrollHelper;
};

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wxScrollHelper construction
// ----------------------------------------------------------------------------

wxScrollHelper::wxScrollHelper(wxWindow *win)
{
    m_xScrollPixelsPerLine =
    m_yScrollPixelsPerLine =
    m_xScrollPosition =
    m_yScrollPosition =
    m_xScrollLines =
    m_yScrollLines =
    m_xScrollLinesPerPage =
    m_yScrollLinesPerPage = 0;

    m_xScrollingEnabled =
    m_yScrollingEnabled = TRUE;

    m_scaleX =
    m_scaleY = 1.0;

    m_win =
    m_targetWindow = (wxWindow *)NULL;

    if ( win )
        SetWindow(win);
}

void wxScrollHelper::SetWindow(wxWindow *win)
{
    wxCHECK_RET( win, _T("wxScrollHelper needs a window to scroll") );

    m_targetWindow = m_win = win;

    // install the event handler which will intercept the events we're
    // interested in
    m_win->PushEventHandler(new wxScrollHelperEvtHandler(this));
}

wxScrollHelper::~wxScrollHelper()
{
    if ( m_targetWindow )
        m_targetWindow->PopEventHandler(TRUE /* do delete it */);
}

// ----------------------------------------------------------------------------
// setting scrolling parameters
// ----------------------------------------------------------------------------

void wxScrollHelper::SetScrollbars(int pixelsPerUnitX,
                                   int pixelsPerUnitY,
                                   int noUnitsX,
                                   int noUnitsY,
                                   int xPos,
                                   int yPos,
                                   bool noRefresh)
{
    int xpos, ypos;

    CalcUnscrolledPosition(xPos, yPos, &xpos, &ypos);
    bool do_refresh =
    (
      (noUnitsX != 0 && m_xScrollLines == 0) ||
      (noUnitsX < m_xScrollLines && xpos > pixelsPerUnitX*noUnitsX) ||

      (noUnitsY != 0 && m_yScrollLines == 0) ||
      (noUnitsY < m_yScrollLines && ypos > pixelsPerUnitY*noUnitsY) ||
      (xPos != m_xScrollPosition) ||
      (yPos != m_yScrollPosition)
    );

    m_xScrollPixelsPerLine = pixelsPerUnitX;
    m_yScrollPixelsPerLine = pixelsPerUnitY;
    m_xScrollPosition = xPos;
    m_yScrollPosition = yPos;
    m_xScrollLines = noUnitsX;
    m_yScrollLines = noUnitsY;

#ifdef __WXMOTIF__
    // Sorry, some Motif-specific code to implement a backing pixmap
    // for the wxRETAINED style. Implementing a backing store can't
    // be entirely generic because it relies on the wxWindowDC implementation
    // to duplicate X drawing calls for the backing pixmap.

    if ( m_targetWindow->GetWindowStyle() & wxRETAINED )
    {
        Display* dpy = XtDisplay((Widget)m_targetWindow->GetMainWidget());

        int totalPixelWidth = m_xScrollLines * m_xScrollPixelsPerLine;
        int totalPixelHeight = m_yScrollLines * m_yScrollPixelsPerLine;
        if (m_targetWindow->m_backingPixmap &&
           !((m_targetWindow->m_pixmapWidth == totalPixelWidth) &&
             (m_targetWindow->m_pixmapHeight == totalPixelHeight)))
        {
            XFreePixmap (dpy, (Pixmap) m_targetWindow->m_backingPixmap);
            m_targetWindow->m_backingPixmap = (WXPixmap) 0;
        }

        if (!m_targetWindow->m_backingPixmap &&
           (noUnitsX != 0) && (noUnitsY != 0))
        {
            int depth = wxDisplayDepth();
            m_pixmapWidth = totalPixelWidth;
            m_pixmapHeight = totalPixelHeight;
            m_backingPixmap = (WXPixmap) XCreatePixmap (dpy, RootWindow (dpy, DefaultScreen (dpy)),
            m_pixmapWidth, m_pixmapHeight, depth);
        }

    }
#endif // Motif

    AdjustScrollbars();

    if (do_refresh && !noRefresh)
        m_targetWindow->Refresh();

#ifdef __WXMAC__
    m_targetWindow->MacUpdateImmediately() ;
#endif
}

// ----------------------------------------------------------------------------
// target window handling
// ----------------------------------------------------------------------------

void wxScrollHelper::SetTargetWindow( wxWindow *target )
{
    wxASSERT_MSG( target, wxT("target window must not be NULL") );
    m_targetWindow = target;
}

wxWindow *wxScrollHelper::GetTargetWindow() const
{
    return m_targetWindow;
}

// ----------------------------------------------------------------------------
// scrolling implementation itself
// ----------------------------------------------------------------------------

void wxScrollHelper::HandleOnScroll(wxScrollWinEvent& event)
{
    int orient = event.GetOrientation();

    int nScrollInc = CalcScrollInc(event);
    if (nScrollInc == 0) return;

    if (orient == wxHORIZONTAL)
    {
        int newPos = m_xScrollPosition + nScrollInc;
        m_win->SetScrollPos(wxHORIZONTAL, newPos, FALSE );
    }
    else
    {
        int newPos = m_yScrollPosition + nScrollInc;
        m_win->SetScrollPos(wxVERTICAL, newPos, FALSE );
    }

    if (orient == wxHORIZONTAL)
    {
        m_xScrollPosition += nScrollInc;
    }
    else
    {
        m_yScrollPosition += nScrollInc;
    }

    if (orient == wxHORIZONTAL)
    {
       if (m_xScrollingEnabled)
            m_targetWindow->ScrollWindow(-m_xScrollPixelsPerLine * nScrollInc, 0, (const wxRect *) NULL);
       else
            m_targetWindow->Refresh();
    }
    else
    {
        if (m_yScrollingEnabled)
            m_targetWindow->ScrollWindow(0, -m_yScrollPixelsPerLine * nScrollInc, (const wxRect *) NULL);
        else
            m_targetWindow->Refresh();
    }
#ifdef __WXMAC__
    m_targetWindow->MacUpdateImmediately() ;
#endif
}

int wxScrollHelper::CalcScrollInc(wxScrollWinEvent& event)
{
    int pos = event.GetPosition();
    int orient = event.GetOrientation();

    int nScrollInc = 0;
    switch (event.GetEventType())
    {
        case wxEVT_SCROLLWIN_TOP:
        {
            if (orient == wxHORIZONTAL)
                nScrollInc = - m_xScrollPosition;
            else
                nScrollInc = - m_yScrollPosition;
            break;
        }
        case wxEVT_SCROLLWIN_BOTTOM:
        {
            if (orient == wxHORIZONTAL)
                nScrollInc = m_xScrollLines - m_xScrollPosition;
            else
                nScrollInc = m_yScrollLines - m_yScrollPosition;
            break;
        }
        case wxEVT_SCROLLWIN_LINEUP:
        {
            nScrollInc = -1;
            break;
        }
        case wxEVT_SCROLLWIN_LINEDOWN:
        {
            nScrollInc = 1;
            break;
        }
        case wxEVT_SCROLLWIN_PAGEUP:
        {
            if (orient == wxHORIZONTAL)
                nScrollInc = -GetScrollPageSize(wxHORIZONTAL);
            else
                nScrollInc = -GetScrollPageSize(wxVERTICAL);
            break;
        }
        case wxEVT_SCROLLWIN_PAGEDOWN:
        {
            if (orient == wxHORIZONTAL)
                nScrollInc = GetScrollPageSize(wxHORIZONTAL);
            else
                nScrollInc = GetScrollPageSize(wxVERTICAL);
            break;
        }
        case wxEVT_SCROLLWIN_THUMBTRACK:
        case wxEVT_SCROLLWIN_THUMBRELEASE:
        {
            if (orient == wxHORIZONTAL)
                nScrollInc = pos - m_xScrollPosition;
            else
                nScrollInc = pos - m_yScrollPosition;
            break;
        }
        default:
        {
            break;
        }
    }

    if (orient == wxHORIZONTAL)
    {
        if (m_xScrollPixelsPerLine > 0)
        {
            int w, h;
            m_targetWindow->GetClientSize(&w, &h);

            int nMaxWidth = m_xScrollLines*m_xScrollPixelsPerLine;
            int noPositions = (int) ( ((nMaxWidth - w)/(double)m_xScrollPixelsPerLine) + 0.5 );
            if (noPositions < 0)
                noPositions = 0;

            if ( (m_xScrollPosition + nScrollInc) < 0 )
                nScrollInc = -m_xScrollPosition; // As -ve as we can go
            else if ( (m_xScrollPosition + nScrollInc) > noPositions )
                nScrollInc = noPositions - m_xScrollPosition; // As +ve as we can go
        }
        else
            m_targetWindow->Refresh();
    }
    else
    {
        if (m_yScrollPixelsPerLine > 0)
        {
            int w, h;
            m_targetWindow->GetClientSize(&w, &h);

            int nMaxHeight = m_yScrollLines*m_yScrollPixelsPerLine;
            int noPositions = (int) ( ((nMaxHeight - h)/(double)m_yScrollPixelsPerLine) + 0.5 );
            if (noPositions < 0)
                noPositions = 0;

            if ( (m_yScrollPosition + nScrollInc) < 0 )
                nScrollInc = -m_yScrollPosition; // As -ve as we can go
            else if ( (m_yScrollPosition + nScrollInc) > noPositions )
                nScrollInc = noPositions - m_yScrollPosition; // As +ve as we can go
        }
        else
            m_targetWindow->Refresh();
    }

    return nScrollInc;
}

// Adjust the scrollbars - new version.
void wxScrollHelper::AdjustScrollbars()
{
    int w, h;
    m_targetWindow->GetClientSize(&w, &h);

    int oldXScroll = m_xScrollPosition;
    int oldYScroll = m_yScrollPosition;

    if (m_xScrollLines > 0)
    {
        // Calculate page size i.e. number of scroll units you get on the
        // current client window
        int noPagePositions = (int) ( (w/(double)m_xScrollPixelsPerLine) + 0.5 );
        if (noPagePositions < 1) noPagePositions = 1;
        if ( noPagePositions > m_xScrollLines )
            noPagePositions = m_xScrollLines;

        // Correct position if greater than extent of canvas minus
        // the visible portion of it or if below zero
        m_xScrollPosition = wxMin( m_xScrollLines-noPagePositions, m_xScrollPosition);
        m_xScrollPosition = wxMax( 0, m_xScrollPosition );

        m_win->SetScrollbar(wxHORIZONTAL, m_xScrollPosition, noPagePositions, m_xScrollLines);
        // The amount by which we scroll when paging
        SetScrollPageSize(wxHORIZONTAL, noPagePositions);
    }
    else
    {
        m_xScrollPosition = 0;
        m_win->SetScrollbar (wxHORIZONTAL, 0, 0, 0, FALSE);
    }

    if (m_yScrollLines > 0)
    {
        // Calculate page size i.e. number of scroll units you get on the
        // current client window
        int noPagePositions = (int) ( (h/(double)m_yScrollPixelsPerLine) + 0.5 );
        if (noPagePositions < 1) noPagePositions = 1;
        if ( noPagePositions > m_yScrollLines )
            noPagePositions = m_yScrollLines;

        // Correct position if greater than extent of canvas minus
        // the visible portion of it or if below zero
        m_yScrollPosition = wxMin( m_yScrollLines-noPagePositions, m_yScrollPosition );
        m_yScrollPosition = wxMax( 0, m_yScrollPosition );

        m_win->SetScrollbar(wxVERTICAL, m_yScrollPosition, noPagePositions, m_yScrollLines);
        // The amount by which we scroll when paging
        SetScrollPageSize(wxVERTICAL, noPagePositions);
    }
    else
    {
        m_yScrollPosition = 0;
        m_win->SetScrollbar (wxVERTICAL, 0, 0, 0, FALSE);
    }

    if (oldXScroll != m_xScrollPosition)
    {
       if (m_xScrollingEnabled)
            m_targetWindow->ScrollWindow( m_xScrollPixelsPerLine * (oldXScroll-m_xScrollPosition), 0, (const wxRect *) NULL );
       else
            m_targetWindow->Refresh();
    }

    if (oldYScroll != m_yScrollPosition)
    {
        if (m_yScrollingEnabled)
            m_targetWindow->ScrollWindow( 0, m_yScrollPixelsPerLine * (oldYScroll-m_yScrollPosition), (const wxRect *) NULL );
        else
            m_targetWindow->Refresh();
    }
}

void wxScrollHelper::DoPrepareDC(wxDC& dc)
{
    dc.SetDeviceOrigin( -m_xScrollPosition * m_xScrollPixelsPerLine,
                        -m_yScrollPosition * m_yScrollPixelsPerLine );
    dc.SetUserScale( m_scaleX, m_scaleY );
}

void wxScrollHelper::GetScrollPixelsPerUnit (int *x_unit, int *y_unit) const
{
    if ( x_unit )
        *x_unit = m_xScrollPixelsPerLine;
    if ( y_unit )
        *y_unit = m_yScrollPixelsPerLine;
}

int wxScrollHelper::GetScrollPageSize(int orient) const
{
    if ( orient == wxHORIZONTAL )
        return m_xScrollLinesPerPage;
    else
        return m_yScrollLinesPerPage;
}

void wxScrollHelper::SetScrollPageSize(int orient, int pageSize)
{
    if ( orient == wxHORIZONTAL )
        m_xScrollLinesPerPage = pageSize;
    else
        m_yScrollLinesPerPage = pageSize;
}

/*
 * Scroll to given position (scroll position, not pixel position)
 */
void wxScrollHelper::Scroll( int x_pos, int y_pos )
{
    if (!m_targetWindow)
        return;

    if (((x_pos == -1) || (x_pos == m_xScrollPosition)) &&
        ((y_pos == -1) || (y_pos == m_yScrollPosition))) return;

    int w, h;
    m_targetWindow->GetClientSize(&w, &h);

    if ((x_pos != -1) && (m_xScrollPixelsPerLine))
    {
        int old_x = m_xScrollPosition;
        m_xScrollPosition = x_pos;

        // Calculate page size i.e. number of scroll units you get on the
        // current client window
        int noPagePositions = (int) ( (w/(double)m_xScrollPixelsPerLine) + 0.5 );
        if (noPagePositions < 1) noPagePositions = 1;

        // Correct position if greater than extent of canvas minus
        // the visible portion of it or if below zero
        m_xScrollPosition = wxMin( m_xScrollLines-noPagePositions, m_xScrollPosition );
        m_xScrollPosition = wxMax( 0, m_xScrollPosition );

        if (old_x != m_xScrollPosition) {
            m_targetWindow->SetScrollPos( wxHORIZONTAL, m_xScrollPosition, TRUE );
            m_targetWindow->ScrollWindow( (old_x-m_xScrollPosition)*m_xScrollPixelsPerLine, 0 );
        }
    }
    if ((y_pos != -1) && (m_yScrollPixelsPerLine))
    {
        int old_y = m_yScrollPosition;
        m_yScrollPosition = y_pos;

        // Calculate page size i.e. number of scroll units you get on the
        // current client window
        int noPagePositions = (int) ( (h/(double)m_yScrollPixelsPerLine) + 0.5 );
        if (noPagePositions < 1) noPagePositions = 1;

        // Correct position if greater than extent of canvas minus
        // the visible portion of it or if below zero
        m_yScrollPosition = wxMin( m_yScrollLines-noPagePositions, m_yScrollPosition );
        m_yScrollPosition = wxMax( 0, m_yScrollPosition );

        if (old_y != m_yScrollPosition) {
            m_targetWindow->SetScrollPos( wxVERTICAL, m_yScrollPosition, TRUE );
            m_targetWindow->ScrollWindow( 0, (old_y-m_yScrollPosition)*m_yScrollPixelsPerLine );
        }
    }

#ifdef __WXMAC__
    m_targetWindow->MacUpdateImmediately();
#endif
}

void wxScrollHelper::EnableScrolling (bool x_scroll, bool y_scroll)
{
    m_xScrollingEnabled = x_scroll;
    m_yScrollingEnabled = y_scroll;
}

void wxScrollHelper::GetVirtualSize (int *x, int *y) const
{
    if ( x )
        *x = m_xScrollPixelsPerLine * m_xScrollLines;
    if ( y )
        *y = m_yScrollPixelsPerLine * m_yScrollLines;
}

// Where the current view starts from
void wxScrollHelper::GetViewStart (int *x, int *y) const
{
    if ( x )
        *x = m_xScrollPosition;
    if ( y )
        *y = m_yScrollPosition;
}

void wxScrollHelper::CalcScrolledPosition(int x, int y, int *xx, int *yy) const
{
    if ( xx )
        *xx = x - m_xScrollPosition * m_xScrollPixelsPerLine;
    if ( yy )
        *yy = y - m_yScrollPosition * m_yScrollPixelsPerLine;
}

void wxScrollHelper::CalcUnscrolledPosition(int x, int y, int *xx, int *yy) const
{
    if ( xx )
        *xx = x + m_xScrollPosition * m_xScrollPixelsPerLine;
    if ( yy )
        *yy = y + m_yScrollPosition * m_yScrollPixelsPerLine;
}

// ----------------------------------------------------------------------------
// event handlers
// ----------------------------------------------------------------------------

// Default OnSize resets scrollbars, if any
void wxScrollHelper::HandleOnSize(wxSizeEvent& WXUNUSED(event))
{
#if wxUSE_CONSTRAINTS
    if ( m_targetWindow->GetAutoLayout() )
        m_targetWindow->Layout();
#endif

    AdjustScrollbars();
}

// This calls OnDraw, having adjusted the origin according to the current
// scroll position
void wxScrollHelper::HandleOnPaint(wxPaintEvent& WXUNUSED(event))
{
    wxPaintDC dc(m_targetWindow);
    DoPrepareDC(dc);

    OnDraw(dc);
}

// kbd handling: notice that we use OnChar() and not OnKeyDown() for
// compatibility here - if we used OnKeyDown(), the programs which process
// arrows themselves in their OnChar() would never get the message and like
// this they always have the priority
void wxScrollHelper::HandleOnChar(wxKeyEvent& event)
{
    int stx, sty,       // view origin
        szx, szy,       // view size (total)
        clix, cliy;     // view size (on screen)

    GetViewStart(&stx, &sty);
    m_win->GetClientSize(&clix, &cliy);
    GetVirtualSize(&szx, &szy);

    if( m_xScrollPixelsPerLine )
    {
        clix /= m_xScrollPixelsPerLine;
        szx /= m_xScrollPixelsPerLine;
    }
    else
    {
        clix = 0;
        szx = -1;
    }
    if( m_yScrollPixelsPerLine )
    {
        cliy /= m_yScrollPixelsPerLine;
        szy /= m_yScrollPixelsPerLine;
    }
    else
    {
        cliy = 0;
        szy = -1;
    }

    int dsty;
    switch ( event.KeyCode() )
    {
        case WXK_PAGEUP:
        case WXK_PRIOR:
            dsty = sty - (5 * cliy / 6);
            Scroll(-1, (dsty == -1) ? 0 : dsty);
            break;

        case WXK_PAGEDOWN:
        case WXK_NEXT:
            Scroll(-1, sty + (5 * cliy / 6));
            break;

        case WXK_HOME:
            Scroll(0, event.ControlDown() ? 0 : -1);
            break;

        case WXK_END:
            Scroll(szx - clix, event.ControlDown() ? szy - cliy : -1);
            break;

        case WXK_UP:
            Scroll(-1, sty - 1);
            break;

        case WXK_DOWN:
            Scroll(-1, sty + 1);
            break;

        case WXK_LEFT:
            Scroll(stx - 1, -1);
            break;

        case WXK_RIGHT:
            Scroll(stx + 1, -1);
            break;

        default:
            // not for us
            event.Skip();
    }
}

// ----------------------------------------------------------------------------
// wxScrolledWindow implementation
// ----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(wxScrolledWindow, wxPanel)

bool wxScrolledWindow::Create(wxWindow *parent,
                              wxWindowID id,
                              const wxPoint& pos,
                              const wxSize& size,
                              long style,
                              const wxString& name)
{
    m_targetWindow = this;

    bool ok = wxPanel::Create(parent, id, pos, size, style, name);

#ifdef __WXMSW__
    // we need to process arrows ourselves for scrolling
    m_lDlgCode |= DLGC_WANTARROWS;
#endif // __WXMSW__

    return ok;
}

wxScrolledWindow::~wxScrolledWindow()
{
}

#if WXWIN_COMPATIBILITY
void wxScrolledWindow::GetScrollUnitsPerPage (int *x_page, int *y_page) const
{
      *x_page = GetScrollPageSize(wxHORIZONTAL);
      *y_page = GetScrollPageSize(wxVERTICAL);
}

void wxScrolledWindow::CalcUnscrolledPosition(int x, int y, float *xx, float *yy) const
{
    if ( xx )
        *xx = (float)(x + m_xScrollPosition * m_xScrollPixelsPerLine);
    if ( yy )
        *yy = (float)(y + m_yScrollPosition * m_yScrollPixelsPerLine);
}
#endif // WXWIN_COMPATIBILITY

