/***************************************************************************
   qgsodbcresultsetmetadata.h
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
#ifndef QGSODBCRESULTSETMETADATA_H
#define QGSODBCRESULTSETMETADATA_H

#include <qglobal.h>
#include "odbc/ResultSetMetaDataUnicode.h"

class QString;

class QgsOdbcResultSetMetadata
{
  private:
    friend class QgsOdbcPreparedStatement;
    friend class QgsOdbcResultSet;

  private:
    QgsOdbcResultSetMetadata( NS_ODBC::ResultSetMetaDataUnicodeRef &&metadata );

  protected:
    Q_DISABLE_COPY( QgsOdbcResultSetMetadata )

  public:
    QString getCatalogName( unsigned short columnIndex );
    QString getSchemaName( unsigned short columnIndex );
    QString getTableName( unsigned short columnIndex );
    QString getBaseTableName( unsigned short columnIndex );
    QString getBaseColumnName( unsigned short columnIndex );
    unsigned short getColumnCount();
    short getColumnType( unsigned short columnIndex );
    std::size_t getColumnLength( unsigned short columnIndex );
    std::size_t getColumnOctetLength( unsigned short columnIndex );
    std::size_t getColumnDisplaySize( unsigned short columnIndex );
    QString getColumnLabel( unsigned short columnIndex );
    QString getColumnName( unsigned short columnIndex );
    QString getColumnTypeName( unsigned short columnIndex );
    unsigned short getPrecision( unsigned short columnIndex );
    unsigned short getScale( unsigned short columnIndex );
    bool isAutoIncrement( unsigned short columnIndex );
    bool isCaseSensitive( unsigned short columnIndex );
    bool isNamed( unsigned short columnIndex );
    bool isNullable( unsigned short columnIndex );
    bool isReadOnly( unsigned short columnIndex );
    bool isSearchable( unsigned short columnIndex );
    bool isSigned( unsigned short columnIndex );

  private:
    NS_ODBC::ResultSetMetaDataUnicodeRef mMetadata;
};

#endif // QGSODBCRESULTSETMETADATA_H
