/********************************************************************
 gFlagsLine Header
 Copyright (c)2011 Mark Watkins <jedimark@users.sourceforge.net>
 License: GPL
*********************************************************************/

#ifndef GFLAGSLINE_H
#define GFLAGSLINE_H

#include "graphlayer.h"

class gFlagsLine:public gLayer
{
    public:
        gFlagsLine(gPointData *d=NULL,QColor col=QColor("black"),QString _label="",int _line_num=0,int _total_lines=0);
        virtual ~gFlagsLine();

        virtual void Plot(gGraphWindow & w,float scrx,float scry);

    protected:
        QString label;
        int line_num,total_lines;
};


#endif // GFLAGSLINE_H
