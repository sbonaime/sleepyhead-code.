/********************************************************************
 gTitle (Graph Title) Implementation
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*********************************************************************/

#include "gTitle.h"

gTitle::gTitle(const QString & _title,QColor color,QFont font)
:gLayer(MC_UNKNOWN),m_title(_title),m_color(color),m_font(font)
{
}
gTitle::~gTitle()
{
}
void gTitle::Plot(gGraphWindow & w,float scrx,float scry)
{
    if (!m_visible) return;
    scrx=scrx;

    float width,height;
    GetTextExtent(m_title,width,height);
    int xp=(height/2)+15;
    //if (m_alignment==wxALIGN_RIGHT) xp=scrx-4-height;
    DrawText(m_title,xp,scry-(w.GetBottomMargin()+((scry-w.GetBottomMargin())/2.0)),90.0,m_color,&m_font);

    //DrawText(w,m_title,150,-40,45.0); //20+xp,scry-(w.GetBottomMargin()+((scry-w.GetBottomMargin())/2.0)+(height/2)),90.0,m_color,&m_font);

    //w.renderText(xp,scry-(w.GetBottomMargin()+((scry-w.GetBottomMargin())/2.0)+(height/2)),0.35,m_title,m_font);

}

