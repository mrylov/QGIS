/***************************************************************************
   qgsodbcpreparedstatement.h
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
#ifndef QGSODBCPREPAREDSTATEMENT_H
#define QGSODBCPREPAREDSTATEMENT_H

#include <qglobal.h>
#include "qgsodbccommon.h"
#include "odbc/ParameterMetaData.h"
#include "odbc/PreparedStatement.h"
#include "odbc/Types.h"

class QByteArray;
class QDate;
class QDateTime;
class QgsGeometry;
class QString;
class QTime;
class QVariant;
class QgsOdbcResultSet;
class QgsOdbcResultSetMetadata;

class QgsOdbcPreparedStatement
{
  private:
    friend class QgsOdbcConnection;

  private:
    QgsOdbcPreparedStatement( NS_ODBC::PreparedStatementRef &&stmt );

  protected:
    Q_DISABLE_COPY( QgsOdbcPreparedStatement )

  public:
    QgsOdbcPreparedStatement( QgsOdbcPreparedStatement &&stmt );

  public:
    void addBatch();
    void clearBatch();
    void clearParameters();
    void executeBatch();
    size_t executeUpdate();
    QgsOdbcResultSet executeQuery();
    std::size_t getBatchDataSize() const;
    QgsOdbcResultSetMetadata getMetadata();
    void setBoolean( unsigned short paramIndex, const QgsOdbcNullable<bool> &value );
    void setByte( unsigned short paramIndex, const QgsOdbcNullable<std::int8_t> &value );
    void setUByte( unsigned short paramIndex, const QgsOdbcNullable<std::uint8_t> &value );
    void setShort( unsigned short paramIndex, const QgsOdbcNullable<std::int16_t> &value );
    void setUShort( unsigned short paramIndex, const QgsOdbcNullable<std::uint16_t> &value );
    void setInt( unsigned short paramIndex, const QgsOdbcNullable<std::int32_t> &value );
    void setUInt( unsigned short paramIndex, const QgsOdbcNullable<std::uint32_t> &value );
    void setLong( unsigned short paramIndex, const QgsOdbcNullable<std::int64_t> &value );
    void setULong( unsigned short paramIndex, const QgsOdbcNullable<std::uint64_t> &value );
    void setFloat( unsigned short paramIndex, const QgsOdbcNullable<float> &value );
    void setDouble( unsigned short paramIndex, const QgsOdbcNullable<double> &value );
    void setString( unsigned short paramIndex, const QString &value );
    void setNString( unsigned short paramIndex, const QString &value );
    void setBinary( unsigned short paramIndex, const QByteArray &value );
    void setDate( unsigned short paramIndex, const QDate &value );
    void setDateTime( unsigned short paramIndex, const QDateTime &value );
    void setTime( unsigned short paramIndex, const QTime &value );
    void setGeometry( unsigned short paramIndex, const QgsGeometry &value );
    void setVariant( unsigned short paramIndex, const QVariant &value );

  private:
    NS_ODBC::PreparedStatementRef mStatement;
    NS_ODBC::ParameterMetaDataRef mParameterMetadata;
};

#endif // QGSODBCPREPAREDSTATEMENT_H
