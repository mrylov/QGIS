/***************************************************************************
   qgsodbcenvironment.h
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
#ifndef QGSODBCENVIRONMENT_H
#define QGSODBCENVIRONMENT_H

#include <qglobal.h>
#include "odbc/Environment.h"
#include <functional>

class QString;
class QStringList;
class QgsOdbcConnection;

class QgsOdbcEnvironment
{
  public:
    QgsOdbcEnvironment();

  protected:
    Q_DISABLE_COPY( QgsOdbcEnvironment )

  public:
    QgsOdbcConnection createConnection();
    QStringList getDataSources();
    bool isDriverInstalled( const QString &name );
    void iterateDrivers( const std::function<void( const QString &name, const QString &path )> &callback );

  private:
    NS_ODBC::EnvironmentRef mEnv;
};

#endif // QGSODBCENVIRONMENT_H
