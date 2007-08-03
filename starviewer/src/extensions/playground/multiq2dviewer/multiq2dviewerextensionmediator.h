/***************************************************************************
 *   Copyright (C) 2005-2007 by Grup de Gràfics de Girona                  *
 *   http://iiia.udg.es/GGG/index.html?langu=uk                            *
 *                                                                         *
 *   Universitat de Girona                                                 *
 ***************************************************************************/
#ifndef UDGMULTIQ2DVIEWEREXTENSIONMEDIATOR_H
#define UDGMULTIQ2DVIEWEREXTENSIONMEDIATOR_H
#include "extensionmediator.h"

#include "installextension.h"
#include "multiq2dviewerextension.h"

namespace udg{

/**
    @author Grup de Gràfics de Girona  ( GGG ) <vismed@ima.udg.es>
*/
class MultiQ2DViewerExtensionMediator: public ExtensionMediator
{
public:
    MultiQ2DViewerExtensionMediator(QObject *parent = 0);

    ~MultiQ2DViewerExtensionMediator();

    virtual bool initializeExtension(QWidget* extension, const ExtensionContext &extensionContext, ExtensionHandler* extensionHandler);

    virtual DisplayableID getExtensionID() const;
};

static InstallExtension<MultiQ2DViewerExtension, MultiQ2DViewerExtensionMediator> registerMultiQ2DViewerExtension;


} //udg namespace

#endif
