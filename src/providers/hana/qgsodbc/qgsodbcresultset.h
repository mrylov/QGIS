/***************************************************************************
   qgsodbcresultset.h
   --------------------------------------
   Date      : 05-07-2023
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
#ifndef QGSODBCRESULTSET_H
#define QGSODBCRESULTSET_H

#include <qglobal.h>
#include "qgsodbccommon.h"
#include "odbc/ResultSet.h"
#include "odbc/ResultSetMetaDataUnicode.h"
#include "odbc/Types.h"

class QDate;
class QDateTime;
class QString;
class QTime;
class QVariant;
class QgsGeometry;
class QgsOdbcResultSetMetadata;

class QgsOdbcResultSet
{
  private:
    friend class QgsOdbcDatabaseMetadata;
    friend class QgsOdbcPreparedStatement;
    friend class QgsOdbcStatement;

  private:
    QgsOdbcResultSet( NS_ODBC::ResultSetRef &&resultSet );

  protected:
    Q_DISABLE_COPY( QgsOdbcResultSet )

  public:
    QgsOdbcResultSet( QgsOdbcResultSet &&resultSet );
    virtual ~QgsOdbcResultSet() {}

    void close();
    bool next();

    QgsOdbcNullable<bool> getBoolean( unsigned short columnIndex );
    QgsOdbcNullable<std::int8_t> getByte( unsigned short columnIndex );
    QgsOdbcNullable<std::uint8_t> getUByte( unsigned short columnIndex );
    QgsOdbcNullable<std::int16_t> getShort( unsigned short columnIndex );
    QgsOdbcNullable<std::uint16_t> getUShort( unsigned short columnIndex );
    QgsOdbcNullable<std::int32_t> getInt( unsigned short columnIndex );
    QgsOdbcNullable<std::uint32_t> getUInt( unsigned short columnIndex );
    QgsOdbcNullable<std::int64_t> getLong( unsigned short columnIndex );
    QgsOdbcNullable<std::uint64_t> getULong( unsigned short columnIndex );
    QgsOdbcNullable<double> getDouble( unsigned short columnIndex );
    QgsOdbcNullable<float> getFloat( unsigned short columnIndex );
    QDate getDate( unsigned short columnIndex );
    QDateTime getDateTime( unsigned short columnIndex );
    QTime getTime( unsigned short columnIndex );
    QString getString( unsigned short columnIndex );
    QVariant getVariant( unsigned short columnIndex );
    QgsGeometry getGeometry( unsigned short columnIndex );

    QgsOdbcResultSetMetadata getMetadata();

  private:
    NS_ODBC::ResultSetRef mResultSet;
    NS_ODBC::ResultSetMetaDataUnicodeRef mMetadata;
};

#endif // QGSODBCRESULTSET_H
