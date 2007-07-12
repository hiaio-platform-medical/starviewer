/***************************************************************************
 *   Copyright (C) 2005 by Grup de Gràfics de Girona                       *
 *   http://iiia.udg.es/GGG/index.html?langu=uk                            *
 *                                                                         *
 *   Universitat de Girona                                                 *
 ***************************************************************************/
#ifndef UDGINPUT_CPP
#define UDGINPUT_CPP

#include "input.h"
#include "volumesourceinformation.h"
#include "logging.h"
//ITK
#include <itkMetaDataDictionary.h>
#include <itkMetaDataObject.h>
// QT
#include <QStringList>
#include <QFileInfo>
#include <QDir>
// VTK
#include <vtkMath.h> // pel cross

namespace udg {

Input::Input()
{
    m_reader = ReaderType::New();
    m_seriesReader = SeriesReaderType::New();
    m_volumeData = new Volume;

    m_gdcmIO = ImageIOType::New();
    m_namesGenerator = NamesGeneratorType::New();

   /* typedef itk::QtSignalAdaptor SignalAdaptorType;
    SignalAdaptorType m_progressSignalAdaptor;
   */
     itk::QtSignalAdaptor *m_progressSignalAdaptor = new itk::QtSignalAdaptor;
//   Connect the adaptor as an observer of a Filter's event
    m_seriesReader->AddObserver( itk::ProgressEvent(),  m_progressSignalAdaptor->GetCommand() );
//
//  Connect the adaptor's Signal to the Qt Widget Slot
   connect( m_progressSignalAdaptor, SIGNAL( Signal() ), this, SLOT( slotProgress() ) );
}

Input::~Input()
{
    m_seriesReader->Delete();
    m_reader->Delete();
    m_gdcmIO->Delete();
    delete m_volumeData;
}

int Input::openFile( QString fileName )
{
    ProgressCommand::Pointer observer = ProgressCommand::New();
    m_reader->AddObserver( itk::ProgressEvent(), observer );

    int errorCode = NoError;

    m_reader->SetFileName( qPrintable(fileName) );
    emit progress(0);
    try
    {
        m_reader->Update();
    }
    catch ( itk::ExceptionObject & e )
    {
        ERROR_LOG( QString("Excepció llegint els arxius del directori [%1]\nDescripció: [%2]")
                .arg( QFileInfo( fileName ).dir().path() )
                .arg( e.GetDescription() )
                );
        // llegim el missatge d'error per esbrinar de quin error es tracta
        QString errorMessage( e.GetDescription() );
        if( errorMessage.contains("Size mismatch") )
        {
            errorCode = SizeMismatch;
        }
        emit progress( -1 ); // això podria indicar excepció
    }
    if ( errorCode == NoError )
    {
        ImageType::Pointer imageData;
        imageData = m_reader->GetOutput();
        m_volumeData->setData( imageData );
        imageData->Delete();

        m_volumeData->getVolumeSourceInformation()->setFilenames( fileName );
        this->setVolumeInformation();
        emit progress( 100 );
    }
    return errorCode;
}

int Input::readFiles( QStringList filenames )
{
    int errorCode = NoError;
    if( filenames.isEmpty() )
    {
        WARN_LOG( "La llista de noms de fitxer per carregar és buida" );
        errorCode = InvalidFileName;
        return errorCode;
    }

    if( filenames.size() > 1 )
    {
        // això és necessari per després poder demanar-li el diccionari de meta-dades i obtenir els tags del DICOM
        m_seriesReader->SetImageIO( m_gdcmIO );

        // convertim la QStringList al format std::vector< std::string > que s'esperen les itk
        std::vector< std::string > stlFilenames = qstringListToStdVectorOfStdString( filenames );

        m_seriesReader->SetFileNames( stlFilenames );

        emit progress( 0 );

        try
        {
            m_seriesReader->Update();
        }
        catch ( itk::ExceptionObject & e )
        {
            ERROR_LOG( QString("Excepció llegint els arxius del directori [%1]\nDescripció: [%2]")
                .arg( QFileInfo( filenames.at(0) ).dir().path() )
                .arg( e.GetDescription() )
                );
            errorCode = SizeMismatch;
            emit progress( -1 ); // això podria indicar excepció
        }
        if ( errorCode == NoError )
        {
            ImageType::Pointer imageData;
            imageData = m_seriesReader->GetOutput();
            imageData->SetMetaDataDictionary( m_gdcmIO->GetMetaDataDictionary() );
            m_volumeData->setData( imageData );
            imageData->Delete();

            m_volumeData->getVolumeSourceInformation()->setFilenames( filenames );
            this->setVolumeInformation();
            emit progress( 100 );
        }
    }
    else
    {
        // això ho posem perquè quan es llegeix una sèrie d'una sola imatge pugui llegir la informació dicom. TODO potser caldria comprovar d'alguna manera si la imatge és dicom abans de res...
        m_reader->SetImageIO( m_gdcmIO );
        this->openFile( filenames.at(0) );
    }
    return errorCode;
}

int Input::readSeries( QString dirPath )
{
//     SeriesProgressCommand::Pointer observer = SeriesProgressCommand::New();
//     m_seriesReader->AddObserver( itk::ProgressEvent(), observer );

    m_namesGenerator->SetInputDirectory( qPrintable(dirPath) );

    const SeriesReaderType::FileNamesContainer &filenames = m_namesGenerator->GetInputFileNames();
    return readFiles( stdVectorOfStdStringToQStringList( filenames ) );
}

bool Input::queryTagAsString( QString tag , QString &result )
{
    bool ok = true;
    typedef itk::MetaDataDictionary   DictionaryType;
    const  DictionaryType & dictionary = m_gdcmIO->GetMetaDataDictionary();

    typedef itk::MetaDataObject< std::string > MetaDataStringType;

    DictionaryType::ConstIterator itr = dictionary.Begin();
    DictionaryType::ConstIterator end = dictionary.End();

    DictionaryType::ConstIterator tagItr = dictionary.Find( tag.toStdString() );

    if( tagItr == end )
    {
        ok = false;
    }
    else
    {
        MetaDataStringType::ConstPointer entryvalue = dynamic_cast<const MetaDataStringType *>( tagItr->second.GetPointer() );

        if( entryvalue )
        {
            result = entryvalue->GetMetaDataObjectValue().c_str();
        }
    }
    return ok;
}

QString Input::getOrientation( double vector[3] )
{
        char *orientation = new char[4];
        char *optr = orientation;
        *optr='\0';

        char orientationX = vector[0] < 0 ? 'R' : 'L';
        char orientationY = vector[1] < 0 ? 'A' : 'P';
        char orientationZ = vector[2] < 0 ? 'I' : 'S';

        double absX = fabs( vector[0] );
        double absY = fabs( vector[1] );
        double absZ = fabs( vector[2] );

        int i;
        for ( i = 0; i < 3; ++i )
        {
            if ( absX > .0001 && absX > absY && absX > absZ )
            {
                *optr++= orientationX;
                absX = 0;
            }
            else if ( absY > .0001 && absY > absX && absY > absZ )
            {
                *optr++= orientationY;
                absY = 0;
            }
            else if ( absZ > .0001 && absZ > absX && absZ > absY )
            {
                *optr++= orientationZ;
                absZ = 0;
            }
            else break;
            *optr='\0';
        }
        return QString( orientation );
}

void Input::setVolumeInformation()
{
    QString value;
    //obtenim l'string amb la posició del pacient relativa a la màquina(series level). Obligatori per MR i CT. No ha d'estar present si Patient Orientation Code Sequence (0054,0410) hi és, altrament és camp obligatori.
    if( queryTagAsString("0018|5100", value) )
    {
        m_volumeData->getVolumeSourceInformation()->setPatientPosition( value.trimmed() );
    }
    //obtenim la orientació de la imatge(image level). Això ens designarà la direcció anatòmica (R,L,P,A,H,F) dels pixels que van d'esquerra-dreta/dalt-abaix. És un camp requerit si l'imatge no requereix Image Orientation(0020,0037) i Image Position(0020,0032)
    if( queryTagAsString( "0020|0020" , value ) )
    {
        value.replace( QString( "\\" ) , QString( "," ) );
        m_volumeData->getVolumeSourceInformation()->setPatientOrientationString( value );
    }
    else
    {
        // si no tenim la informació directament l'haurem de deduir a partir dels direction cosines
        // l'Image Orientation
        if( queryTagAsString( "0020|0037", value ) )
        {
            // passem de l'string als valors double
            double dirCosinesValuesX[3] , dirCosinesValuesY[3] , dirCosinesValuesZ[3];
            QStringList list = value.split( "\\" );
            if( list.size() == 6 )
            {
                for ( int i = 0; i < 3; i++ )
                {
                    dirCosinesValuesX[ i ] = list.at( i ).toDouble();
                    dirCosinesValuesY[ i ] = list.at( i+3 ).toDouble();
                }

                vtkMath::Cross( dirCosinesValuesX , dirCosinesValuesY , dirCosinesValuesZ );
                // I ara ens disposem a crear l'string amb l'orientació del pacient
                QString patientOrientationString;

                patientOrientationString = this->getOrientation( dirCosinesValuesX );
                patientOrientationString += ",";
                patientOrientationString += this->getOrientation( dirCosinesValuesY );
                patientOrientationString += ",";
                patientOrientationString += this->getOrientation( dirCosinesValuesZ );
                m_volumeData->getVolumeSourceInformation()->setPatientOrientationString( patientOrientationString );
                m_volumeData->getVolumeSourceInformation()->setXDirectionCosines( dirCosinesValuesX );
                m_volumeData->getVolumeSourceInformation()->setYDirectionCosines( dirCosinesValuesY );
                m_volumeData->getVolumeSourceInformation()->setZDirectionCosines( dirCosinesValuesZ );

                DEBUG_LOG("Patient orientation string: " + patientOrientationString );
            }
            else
            {
                // hi ha algun error en l'string ja que han de ser 2 parells de 3 valors
                DEBUG_LOG( "No s'ha pogut determinar l'orientació del pacient (Tags 0020|0020 , 0020|0037) : " + value );
            }
        }
        else
        {
            // no podem obtenir l'string d'orientació del pacient
        }
    }
    // nom de la institució on s'ha fet l'estudi
    if( queryTagAsString( "0008|0080" , value ) )
    {
        m_volumeData->getVolumeSourceInformation()->setInstitutionName( value );
    }
    else
    {
        // no tenim aquesta informació \TODO cal posar res?
        m_volumeData->getVolumeSourceInformation()->setInstitutionName( tr( "Unknown" ) );
    }

    // nom del pacient
    if( queryTagAsString( "0010|0010" , value ) )
    {
        // pre-tractament per treure caràcters estranys com ^ que en alguns casos fan de separadors en comptes dels espais
        while( value.indexOf("^") >= 0 )
            value.replace( value.indexOf("^") , 1 , QString(" ") );
        m_volumeData->getVolumeSourceInformation()->setPatientName( value );
    }
    // ID del pacient
    if( queryTagAsString( "0010|0020" , value ) )
    {
        m_volumeData->getVolumeSourceInformation()->setPatientID( value );
    }

    // data de l'estudi
    if( queryTagAsString( "0008|0020" , value ) )
    {
        // la data està en format YYYYMMDD
        m_volumeData->getVolumeSourceInformation()->setStudyDate( value );
    }

    // hora de l'estudi
    if( queryTagAsString( "0008|0030" , value ) )
    {
        // l'hora està en format HHMMSS
        m_volumeData->getVolumeSourceInformation()->setStudyTime( value );
    }

    // accession number
    if( queryTagAsString( "0008|0050" , value ) )
    {
        m_volumeData->getVolumeSourceInformation()->setAccessionNumber( value );
    }

    // Protocol name
    if( queryTagAsString( "0018|1030" , value ) )
    {
        m_volumeData->getVolumeSourceInformation()->setProtocolName( value );
    }

    //Number of Phases, TAG PRIVAT PHILIPS
    if( queryTagAsString( "2001|1017" , value ) )
    {
        m_volumeData->getVolumeSourceInformation()->setNumberOfPhases( value.toInt() );
    }

    //Number of Slices Per Phase, nombre de llesques per cada fase, TAG PRIVAT PHILIPS
    if( queryTagAsString( "2001|1018" , value ) )
    {
        m_volumeData->getVolumeSourceInformation()->setNumberOfSlices( value.toInt() );
    }
}


QStringList Input::stdVectorOfStdStringToQStringList( std::vector< std::string > vector )
{
    QStringList list;
    for( unsigned int i = 0; i < vector.size(); i++ )
    {
        list += vector[i].c_str();
    }
    return list;
}

std::vector< std::string > Input::qstringListToStdVectorOfStdString( QStringList list )
{
    std::vector< std::string > stlVector;
    for( int i = 0; i < list.size(); i++ )
    {
        stlVector.push_back( list.at(i).toStdString() );
    }
    return stlVector;
}

}; // end namespace udg

#endif
