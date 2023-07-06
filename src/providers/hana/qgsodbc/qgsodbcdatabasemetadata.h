/***************************************************************************
   qgsodbcdatabasemetadata.h
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
#ifndef QGSODBCDATABASEMETADATA_H
#define QGSODBCDATABASEMETADATA_H

#include <qglobal.h>
#include "qgsodbccommon.h"
#include "odbc/DatabaseMetaDataUnicode.h"
#include "odbc/Types.h"

class QString;
class QgsOdbcResultSet;

class QgsOdbcDatabaseMetadata
{
  private:
    friend class QgsOdbcConnection;

  private:
    QgsOdbcDatabaseMetadata( NS_ODBC::DatabaseMetaDataUnicodeRef &&metadata );

  protected:
    Q_DISABLE_COPY( QgsOdbcDatabaseMetadata )

  public:
    QgsOdbcResultSet getColumns(
      const QString &catalogName,
      const QString &schemaName,
      const QString &tableName,
      const QString &columnName );

    QString getDBMSVersion();

    QgsOdbcResultSet getPrimaryKeys(
      const QString &catalogName,
      const QString &schemaName,
      const QString &tableName );

    QgsOdbcResultSet getStatistics(
      const QString &catalogName,
      const QString &schemaName,
      const QString &tableName,
      QgsOdbcIndexType indexType,
      QgsOdbcStatisticsAccuracy accuracy );

  private:
    NS_ODBC::DatabaseMetaDataUnicodeRef mMetadata;
};

#endif // QGSODBCDATABASEMETADATA_H
