/*
 graph spacer Header
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*/

#ifndef GSPACER_H
#define GSPACER_H

#include "gGraphView.h"


/*! \class gSpacer
    \brief A dummy graph spacer layer object
   */
class gSpacer:public Layer
{
    public:
        gSpacer(int space=20); // orientation?
        virtual void paint(gGraph & g,int left,int top, int width, int height) {
            Q_UNUSED(g)
            Q_UNUSED(left)
            Q_UNUSED(top)
            Q_UNUSED(width)
            Q_UNUSED(height)
        }
        int space() { return m_space; }

protected:
        int m_space;
};

#endif // GSPACER_H
