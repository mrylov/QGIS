/***************************************************************************
   qgshanadriver.cpp
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
#include "qgshanadriver.h"
#include "qgslogger.h"
#include "qgsodbc/qgsodbcconnection.h"
#include <QDir>
#include <QFileInfo>
#include <QLibrary>
#include <QString>
#include <QStringList>

static QString detectDriverPath( QgsOdbcEnvironment &env, const QString &libName, const QString &defaultPath )
{
  QString ret = defaultPath + QDir::separator() + libName;
  if ( QFileInfo::exists( ret ) )
    return ret;

  auto processDriver = [&ret, &libName]( const QString & name,  const QString & path )
  {
    if ( QString::compare( name, QLatin1String( "DRIVER" ), Qt::CaseInsensitive ) != 0 )
      return;

    if ( path.endsWith( libName ) && QFileInfo::exists( path ) )
      ret = path;
  };

  env.iterateDrivers( processDriver );
  return ret;
}

QgsHanaDriver::QgsHanaDriver()
{
  QgsDebugCall;
#if defined(Q_OS_WIN)
#if defined(Q_OS_WIN64)
  mDriver = mEnv.isDriverInstalled( QStringLiteral( "HDBODBC" ) ) ? QStringLiteral( "HDBODBC" ) : QString();
#else
  mDriver = mEnv.isDriverInstalled( QStringLiteral( "HDBODBC32" ) ) ? QStringLiteral( "HDBODBC32" ) : QString();
#endif
#elif defined(Q_OS_MAC)
  mDriver = detectDriverPath( mEnv, QStringLiteral( "libodbcHDB.dylib" ), QStringLiteral( "/Applications/sap/hdbclient" ) );
#else
  mDriver = detectDriverPath( mEnv, QStringLiteral( "libodbcHDB.so" ), QStringLiteral( "/usr/sap/hdbclient" ) );
#endif
}

QgsHanaDriver::~QgsHanaDriver()
{
  QgsDebugCall;
}

QgsOdbcConnection QgsHanaDriver::createConnection()
{
  return mEnv.createConnection();
}

QStringList QgsHanaDriver::dataSources()
{
  return mEnv.getDataSources();
}

const QString &QgsHanaDriver::driver() const
{
  return mDriver;
}

QgsHanaDriver *QgsHanaDriver::instance()
{
  static QgsHanaDriver instance;
  return &instance;
}

bool QgsHanaDriver::isInstalled( const QString &name )
{
  QgsOdbcEnvironment env;
  return env.isDriverInstalled( name.toStdString().c_str() );
}

bool QgsHanaDriver::isValidPath( const QString &path )
{
  if ( !QLibrary::isLibrary( path ) )
    return false;

  QLibrary lib( path );
  if ( !lib.load() )
    return false;
  const bool ret = lib.resolve( "SQLConnect" ) != nullptr;
  lib.unload();
  return ret;
}
