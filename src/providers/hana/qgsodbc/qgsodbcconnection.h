/***************************************************************************
   qgsodbcconnection.h
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
#ifndef QGSODBCCONNECTION_H
#define QGSODBCCONNECTION_H

#include <qglobal.h>
#include <memory>
#include "odbc/Connection.h"

class QString;
class QgsOdbcDatabaseMetadata;
class QgsOdbcPreparedStatement;
class QgsOdbcStatement;

class QgsOdbcConnection
{
  private:
    friend class QgsOdbcEnvironment;

  public:
    QgsOdbcConnection( QgsOdbcConnection &&conn );
    QgsOdbcConnection( std::unique_ptr<QgsOdbcConnection> &&conn );

  private:
    QgsOdbcConnection( NS_ODBC::ConnectionRef &&conn );

  protected:
    Q_DISABLE_COPY( QgsOdbcConnection )

  public:
    void connect( const QString &dsn, const QString &user, const QString &password );
    void connect( const QString &connString );
    void disconnect();
    bool connected() const;

    void commit();
    void rollback();

    void setAutoCommit( bool autoCommit );

    QgsOdbcStatement createStatement();
    QgsOdbcPreparedStatement prepareStatement( const QString &sql );

    QgsOdbcDatabaseMetadata getDatabaseMetadata();

  private:
    NS_ODBC::ConnectionRef mConnection;
};

#endif // QGSODBCCONNECTION_H
