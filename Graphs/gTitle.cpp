/*
 gTitle (Graph Title) Implementation
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*/

#include "gTitle.h"

gTitle::gTitle(const QString & _title,QColor color,QFont font)
:gLayer(EmptyChannel),m_title(_title),m_color(color),m_font(font)
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
    int j=scry/2-w.GetTopMargin();
    DrawText(w,m_title,xp,j,90.0,m_color,&m_font);
}

