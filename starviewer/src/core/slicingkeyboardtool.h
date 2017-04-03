/*************************************************************************************
  Copyright (C) 2014 Laboratori de Gràfics i Imatge, Universitat de Girona &
  Institut de Diagnòstic per la Imatge.
  Girona 2014. All rights reserved.
  http://starviewer.udg.edu

  This file is part of the Starviewer (Medical Imaging Software) open source project.
  It is subject to the license terms in the LICENSE file found in the top-level
  directory of this distribution and at http://starviewer.udg.edu/license. No part of
  the Starviewer (Medical Imaging Software) open source project, including this file,
  may be copied, modified, propagated, or distributed except according to the
  terms contained in the LICENSE file.
 *************************************************************************************/

#ifndef UDGSLICINGKEYBOARDTOOL_H
#define UDGSLICINGKEYBOARDTOOL_H

#include "slicingtool.h"

namespace udg {

class SlicingKeyboardTool : public SlicingTool {
Q_OBJECT
public:
    explicit SlicingKeyboardTool(QViewer *viewer, QObject *parent = 0);
    virtual ~SlicingKeyboardTool();
    
    virtual void handleEvent(unsigned long eventID) override;
    
public slots:
    virtual void reassignAxis() override;

private:
    void onHomePress();
    void onEndPress();
    void onUpPress();
    void onDownPress();
    void onLeftPress();
    void onRightPress();
    void onPlusPress();
    void onMinusPress();
    
    void incrementLocationWithVolumesLoop(int shift);
    
    static constexpr auto MAIN_AXIS = 0;
    static constexpr auto SECONDARY_AXIS = 1;
    
};

}

#endif
