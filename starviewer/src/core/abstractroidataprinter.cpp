#include "abstractroidataprinter.h"

#include "roidata.h"
#include "q2dviewer.h"
#include "volume.h"
#include "standarduptakevaluemeasurehandler.h"

namespace udg {

AbstractROIDataPrinter::AbstractROIDataPrinter(const QMap<int, ROIData> &roiDataMap, const QString &areaString)
{
    m_roiDataMap = roiDataMap;
    m_areaString = areaString;
}

AbstractROIDataPrinter::~AbstractROIDataPrinter()
{
}

QString AbstractROIDataPrinter::getStandardizedUptakeValueMeasureString(ROIData &roiData, Image *petImage) const
{
    QString suvMeasurement;
    
    if (roiData.getModality() == "PT")
    {
        StandardUptakeValueMeasureHandler suvHandler;
        suvHandler.setImage(petImage);
        if (suvHandler.canComputePreferredFormula())
        {
            double maximum = suvHandler.computePreferredFormula(roiData.getMaximum());
            double mean = suvHandler.computePreferredFormula(roiData.getMean());

            QString units = suvHandler.getComputedFormulaUnits();
            suvMeasurement = QObject::tr("SUV (%1)").arg(suvHandler.getComputedFormulaLabel());
            suvMeasurement += QObject::tr("\nMax: ") + getFormattedValueString(maximum, units);
            suvMeasurement += QObject::tr("\nMean: ") + getFormattedValueString(mean, units);
        }
        else
        {
            suvMeasurement = QObject::tr("SUV (%1) - N/A").arg(suvHandler.getPreferredFormulaLabel());
        }
    }

    return suvMeasurement;
}

QString AbstractROIDataPrinter::getFormattedValueString(double value, const QString &units) const
{
    QString valueString;

    valueString = QString("%1").arg(value, 0, 'f', 2);
    if (!units.isEmpty())
    {
        valueString += " " + units;
    }
    return valueString;
}

Image* AbstractROIDataPrinter::getCurrentImage(Q2DViewer *viewer, int inputIndex) const
{
    if (!viewer)
    {
        return 0;
    }
    
    Image *image = viewer->getCurrentDisplayedImageOnInput(inputIndex);
    if (!image)
    {
        image = viewer->getInput(inputIndex)->getImage(0);
    }

    return image;
}

} // End namespace udg