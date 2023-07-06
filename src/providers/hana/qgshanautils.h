/***************************************************************************
   qgshanautils.h
   --------------------------------------
   Date      : 31-05-2019
   Copyright : (C) SAP SE
   Author    : Maxim Rylov
 ***************************************************************************/

/***************************************************************************
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 ***************************************************************************/
#ifndef QGSHANAUTILS_H
#define QGSHANAUTILS_H

#include "qgsdatasourceuri.h"
#include "qgsfields.h"
#include "qgswkbtypes.h"

#include <QVariant>
#include <QVersionNumber>

class QgsHanaUtils
{
  public:
    QgsHanaUtils() = delete;

    static QString connectionInfo( const QgsDataSourceUri &uri );

    static QString quotedIdentifier( const QString &str );
    static QString quotedString( const QString &str );
    static QString quotedValue( const QVariant &value );

    static QString toConstant( const QVariant &value, QVariant::Type type );

    static QString toString( Qgis::DistanceUnit unit );

    static bool isGeometryTypeSupported( Qgis::WkbType wkbType );
    static Qgis::WkbType toWkbType( const QString &type, bool hasZ, bool hasM );
    static QVersionNumber toHANAVersion( const QString &dbVersion );
    static int toPlanarSRID( int srid );
    static bool convertField( QgsField &field );
    static int countFieldsWithFirstLetterInUppercase( const QgsFields &fields );
    static QString formatErrorMessage( const char *message, bool withPrefix = false );
};

#endif // QGSHANAUTILS_H
